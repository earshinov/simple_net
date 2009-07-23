#pragma once

#include "config.h"
#ifdef HAVE_LIBEV

#include "../common/common.h"
#include <ev.h>

/*
 * Warning: this will case a memory leak if some created elements are not erased.
 * It is not important in our case as the only object of this class is
 * destroyed when application exits, so all used memory will be freed by OS.
 */
template <typename ClientT>
struct SimpleStorageMixin{
  typedef ClientT * iterator;
protected:
  /* methods accessed from factory */
  iterator create(int s, int buffer_size){
    return new ClientT(s, buffer_size);
  }
  iterator erase(iterator where){
    delete where;
    return 0;
  }
};


struct LibevClient: public Client<LibevClient>{
  typedef Client<LibevClient> base;

  typedef SimpleStorageMixin<LibevClient> LibevStorageMixin;

  struct LibevNetworkMixin{
    friend struct LibevClient;
    LibevNetworkMixin(struct ev_loop * loop_): loop(loop_){ }
  private:
    /* data members accessed from clients */
    struct ev_loop * loop;
  };

  struct LibevClientFactory:
    public ClientFactory<LibevClientFactory, LibevStorageMixin, LibevNetworkMixin, LibevClient>{
    typedef ClientFactory<LibevClientFactory, LibevStorageMixin, LibevNetworkMixin, LibevClient> base;

    LibevClientFactory(
        int s, int buffer_size, int limit_,
        storage_mixin_t storage_mixin, network_mixin_t network_mixin);
    ~LibevClientFactory();

    void delete_client(iterator iter){
      base::delete_client(iter);

      pthread_mutex_lock(&m);
      --n_clients;
      if (!ev_is_active(&accept_watcher))
        ev_io_start(EV_A_ &accept_watcher);
      assert(n_clients >= 0);
      pthread_mutex_unlock(&m);
    }

  protected:

      /*
       * Make this method protected. We will call it ourselves and
       * forbid calling it from outside.
       */
    void new_client(int s) {
      base::new_client(s);

      pthread_mutex_lock(&m);
      ++n_clients;
      if (limit && n_clients == limit)
        ev_io_stop(EV_A_ &accept_watcher);
      pthread_mutex_unlock(&m);
    }

    static void accept_cb(EV_P_ ev_io * watcher, int revents){
      int c = accept(watcher->fd, 0, 0);
      if (c == -1){
        SOCKETS_PERROR("ERROR: accept");
        ev_unloop(EV_A_ EVUNLOOP_ONE);
      }
      else{
        static_cast<LibevClientFactory *>(watcher->data)->new_client(c);
      }
    }

  private:
    int limit;
    int n_clients;
    pthread_mutex_t m;
    ev_io accept_watcher;
  };

  friend struct Client<LibevClient>;
  friend struct ClientFactory<LibevClientFactory, LibevStorageMixin, LibevNetworkMixin, LibevClient>;

  /* using base class ctor */
  LibevClient(int s, int buffer_size): base(s, buffer_size){ }

private:
  /* private methods, accessed from base class and from factory */

  void created(LibevClientFactory & f);

  void register_read(LibevClientFactory & f){
    struct ev_loop * loop = f.loop;
    ev_io_start(EV_A_ &read_watcher);
  }
  void register_write(LibevClientFactory & f){
    struct ev_loop * loop = f.loop;
    ev_io_start(EV_A_ &write_watcher);
  }
  void unregister_read(LibevClientFactory & f){
    struct ev_loop * loop = f.loop;
    ev_io_stop(EV_A_ &read_watcher);
  }
  void unregister_write(LibevClientFactory & f){
    struct ev_loop * loop = f.loop;
    ev_io_stop(EV_A_ &write_watcher);
  }
  void prepare_close(LibevClientFactory & f){
  }

private:
  /* private methods */

  static void read_cb_static(EV_P_ ev_io * watcher, int revents){
    static_cast<LibevClient *>(watcher->data)->read_cb(EV_A_ watcher, revents);
  }

  static void write_cb_static(EV_P_ ev_io * watcher, int revents){
    static_cast<LibevClient *>(watcher->data)->write_cb(EV_A_ watcher, revents);
  }

  void read_cb(EV_P_ ev_io * watcher, int revents);
  void write_cb(EV_P_ ev_io * watcher, int revents);

private:
  /* private data members */

  ev_io read_watcher;
  ev_io write_watcher;
};

/*
 * Warning: do not forget to define this object in one of your source files.
 *
 * This global variable is used in libev callbacks. Another way is to associate
 * custom data with libev watchers, but it would consume O(n) amount of memory,
 * where n is the number of clients. That costs too much for eliminating a
 * global variable.
 *
 * A better way is using ev_userdata/ev_set_userdata functions, but they are
 * available in libev >= 3.7 only. It's a rather recent version, so it may be
 * unavailable in official repos of user's distro, if any).
 */
typedef LibevClient::LibevClientFactory LibevClientFactory;
extern LibevClientFactory * libev_factory;

LibevClient::LibevClientFactory::LibevClientFactory(
    int s, int buffer_size, int limit_,
    storage_mixin_t storage_mixin, network_mixin_t network_mixin):
  base(buffer_size, storage_mixin, network_mixin),
  limit(limit_), n_clients(0){

  libev_factory = this;
  pthread_mutex_init(&m, 0);

  ev_io_init(&accept_watcher, &accept_cb, s, EV_READ);
  accept_watcher.data = this;
  ev_io_start(EV_A_ &accept_watcher);
}

LibevClient::LibevClientFactory::~LibevClientFactory(){
  libev_factory = 0;
  assert("mutex unlocked" && pthread_mutex_trylock(&m));
  pthread_mutex_destroy(&m);
}

void LibevClient::created(LibevClientFactory & f){
  ev_io_init(&read_watcher, &read_cb_static, s, EV_READ);
  read_watcher.data = this;
  ev_io_init(&write_watcher, &write_cb_static, s, EV_WRITE);
  write_watcher.data = this;
}

void LibevClient::read_cb(EV_P_ ev_io * watcher, int revents){
  int count = recv(s, &*(buffer.rd_begin()), buffer.rd_size(), 0);
  if (count == -1){
    SOCKETS_PERROR("WARNING: recv");
    libev_factory->delete_client(this);
  }
  else if (count == 0){
    cerr << "TRACE: Client stopped sending data.\n";

    if (buffer.snd_size() == 0){
      cerr << "TRACE: Nothing to send to client. Close connection to it.\n";
      libev_factory->delete_client(this);
    }
    else{
      /* Do not close socket here as there are something to send. */
      libev_factory->rd_disable(this);
    }
  }
  else
    libev_factory->rd_advance(this, count);
}

void LibevClient::write_cb(EV_P_ ev_io * watcher, int revents){
  int count = send(s, &*(buffer.snd_begin()), buffer.snd_size(), 0);
  if (count == -1){
    SOCKETS_PERROR("WARNING: send");
    libev_factory->delete_client(this);
  }
  else{
    libev_factory->snd_advance(this, count);
    if (rd_disabled() && buffer.empty()){
      cerr << "TRACE: Nothing to send to client. Close connection to it.\n";
      libev_factory->delete_client(this);
    }
  }
}

#endif // HAVE_LIBEV
