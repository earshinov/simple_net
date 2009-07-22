/*
 * To understand some aspects of the code below, please see
 * http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
 */

#pragma once

#include "../common/common.h" /* ReadWriteBuffer */

/* ------------------------------------------------------------------------ */

#if 0 /* examples */

template <typename ClientT>
struct ExampleStorageMixin{
protected:
  /* methods accessed from factory */

  void create(int s, int buffer_size){
    collection.push_back(ClientT(s, buffer_size));
    return collection.end() - 1;
  }
  iterator erase(iterator where){
    return collection.erase(where);
  }
private:
  /* ... any collection class ... */<ClientT> collection;
};


struct ExampleNetworkMixin{
  friend struct ExampleClient;

private:
  /* data members accessed from clients */

  /* ... necessary data members ... */
};


struct ExampleClient: public Client<SelectClient>{
  typedef Client<SelectClient> base;

  friend struct Client<SelectClient>;
  template <typename Derived, typename StorageMixin, typename NetworkMixin, typename ClientT>
    friend struct ClientFactory;

  /* using base class ctor */
  ExampleClient(int s, int buffer_size): base(s, buffer_size){
  }

private:
  /* private methods, accessed from base class and from factory */

  struct ExampleClientFactory;
  void created(ExampleClientFactory & f){ /* ... */ }
  void register_read(SelectClientFactory & f){ /* ... */ }
  void register_write(SelectClientFactory & f){ /* ... */ }
  void unregister_read(SelectClientFactory & f){ /* ... */ }
  void unregister_write(SelectClientFactory & f){ /* ... */ }
  void prepare_close(SelectClientFactory & f){ /* ... */ }
};


struct ExampleClientFactory:
  public ClientFactory<ExampleClientFactory, ExampleStorageMixin, ExampleNetworkMixin, ExampleClient>{
  typedef ClientFactory<ExampleClientFactory, ExampleStorageMixin, ExampleNetworkMixin, ExampleClient> base;

  /* using base class ctor */
  ExampleClientFactory(int buffer_size, storage_mixin_t storage_mixin, network_mixin_t network_mixin):
    base(buffer_size, storage_mixin, network_mixin){
  }
};

#endif

#if 0 /* alternative for client and factory */

struct ExampleClient: public Client<SelectClient>{
  typedef Client<SelectClient> base;

  struct Factory:
    public ClientFactory<Factory, ExampleStorageMixin, ExampleNetworkMixin, ExampleClient>{
    typedef ClientFactory<Factory, ExampleStorageMixin, ExampleNetworkMixin, ExampleClient> base;

    /* using base class ctor */
    Factory(int buffer_size, storage_mixin_t storage_mixin, network_mixin_t network_mixin):
      base(buffer_size, storage_mixin, network_mixin){
    }
  };

  friend struct Client<SelectClient>;
  friend struct ClientFactory<Factory, ExampleStorageMixin, ExampleNetworkMixin, ExampleClient>;

  /* using base class ctor */
  ExampleClient(int s, int buffer_size): base(s, buffer_size){
  }

private:
  /* private methods, accessed from base class and from factory */

  void created(Factory & f){ /* ... */ }
  void register_read(Factory & f){ /* ... */ }
  void register_write(Factory & f){ /* ... */ }
  void unregister_read(Factory & f){ /* ... */ }
  void unregister_write(Factory & f){ /* ... */ }
  void prepare_close(Factory & f){ /* ... */ }
};

typedef ExampleClient::Factory ExampleClientFactory;

#endif

/* ------------------------------------------------------------------------ */

template <typename Derived, typename StorageMixin, typename NetworkMixin, typename ClientT>
struct ClientFactory: public StorageMixin, public NetworkMixin{

  /* typedefs that may be necessary for derived classes */
  typedef StorageMixin storage_mixin_t;
  typedef NetworkMixin network_mixin_t;

  typedef typename StorageMixin::iterator iterator;

  ClientFactory(int buffer_size_, const StorageMixin & storage_mixin, const NetworkMixin & network_mixin):
    StorageMixin(storage_mixin), NetworkMixin(network_mixin), buffer_size(buffer_size_){
  }

  void new_client(int s){
    iterator iter = this->create(s, buffer_size);
    iter->created(derived());
    iter->register_read(derived());
  }

  void rd_advance(iterator iter, int count){
    return client(iter).rd_advance(count, derived());
  }
  void snd_advance(iterator iter, int count){
    return client(iter).snd_advance(count, derived());
  }
  void rd_disable(iterator iter){
    return client(iter).rd_disable(derived());
  }

  iterator delete_client(iterator iter){
    ClientT & c = client(iter);
    c.unregister_read(derived());
    c.unregister_write(derived());
    c.prepare_close(derived());
    verify_ne(close(c.s), -1);
    iter = this->erase(iter);
    return iter;
  }

private:
  Derived & derived(){ return static_cast<Derived &>(*this); }
  ClientT & client(iterator iter){ return static_cast<ClientT &>(*iter); }

private:
  int buffer_size;
};


template <typename Derived>
struct Client{

  Client(int s_, int buffer_size): s(s_), buffer(buffer_size), rd_disabled_(false){
  }

  template <typename Factory>
  void rd_advance(int count, Factory & f){
    bool not_in_w = buffer.snd_empty();

    buffer.rd_advance(count);
    if (buffer.rd_empty())
      derived().unregister_read(f);
    if (not_in_w)
      derived().register_write(f);
  }

  template <typename Factory>
  void snd_advance(int count, Factory & f){
    bool not_in_r = buffer.rd_empty();

    buffer.snd_advance(count);
    if (buffer.snd_empty())
      derived().unregister_write(f);
    if (!rd_disabled_ && not_in_r && !buffer.rd_empty())
      derived().register_read(f);
  }

  template <typename Factory>
  void rd_disable(Factory & f){
    rd_disabled_ = true;
    derived().unregister_read(f);
  }

  bool rd_disabled() const{
    return rd_disabled_;
  }

public:
  int s;
  ReadWriteBuffer buffer;

protected:
  bool rd_disabled_;

private:
  /*
   * Private methods, accessed from other methods and from factory.
   * Define them in a derived class, replacing "Factory" with your factory class.
   */

  /*
  void created(Factory & f);
  void register_read(Factory & f);
  void register_write(Factory & f);
  void unregister_read(Factory & f);
  void unregister_write(Factory & f);
  void prepare_close(Factory & f);
  */

private:
  /* utility private methods, not actually needed in factory */

  Derived & derived(){ return static_cast<Derived &>(*this); }
};
