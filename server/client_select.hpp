#pragma once

#include "client.hpp"

#include <list>


template <typename ClientT>
struct ListStorageMixin{
  typedef std::list<ClientT> Clients;
  typedef typename Clients::iterator iterator;

  iterator begin(){ return clients_.begin(); }
  iterator end(){ return clients_.end(); }

protected:
  /* methods accessed from factory */
  iterator create(int buffer_size, int s){
    clients_.push_back(ClientT(buffer_size, s));
    iterator ret = clients_.end();
    return --ret;
  }
  iterator erase(iterator where){
    return clients_.erase(where);
  }

private:
  Clients clients_;
};


struct SelectClient: public Client<SelectClient>{
  typedef Client<SelectClient> base;

  typedef ListStorageMixin<SelectClient> SelectStorageMixin;

  struct SelectNetworkMixin{
    friend struct SelectClient;

    SelectNetworkMixin(fd_set * rset_, fd_set * wset_, int * maxfd_):
      rset(rset_), wset(wset_), maxfd(maxfd_), maxfd_min(*maxfd_){
    }

  private:
    /* data members accessed from clients */

    fd_set * rset;
    fd_set * wset;
    int * maxfd;
    int maxfd_min;
  };

  struct SelectClientFactory:
    public ClientFactory<SelectClientFactory, SelectStorageMixin, SelectNetworkMixin, SelectClient>{
    typedef ClientFactory<SelectClientFactory, SelectStorageMixin, SelectNetworkMixin, SelectClient> base;

    /* using base class ctor */
    SelectClientFactory(int buffer_size, storage_mixin_t storage_mixin, network_mixin_t network_mixin):
      base(buffer_size, storage_mixin, network_mixin){
    }
  };

  friend struct Client<SelectClient>;
  friend struct ClientFactory<SelectClientFactory, SelectStorageMixin, SelectNetworkMixin, SelectClient>;

  /* using base class ctor */
  SelectClient(int s, int buffer_size): base(s, buffer_size){
  }

private:
  /* private methods, accessed from base class and from factory */

  void created(SelectClientFactory & f){
    if (*(f.maxfd) < s)
      *(f.maxfd) = s;
  }
  void register_read(SelectClientFactory & f){
    FD_SET(s, f.rset);
  }
  void register_write(SelectClientFactory & f){
    FD_SET(s, f.wset);
  }
  void unregister_read(SelectClientFactory & f){
    FD_CLR(s, f.rset);
  }
  void unregister_write(SelectClientFactory & f){
    FD_CLR(s, f.wset);
  }
  void prepare_close(SelectClientFactory & f){
    *(f.maxfd) = f.maxfd_min;
    SelectClientFactory::iterator begin(f.begin());
    const SelectClientFactory::iterator end(f.end());
    for(; begin != end; ++begin)
      if (begin->s > *(f.maxfd))
        *(f.maxfd) = begin->s;
  }
};


typedef SelectClient::SelectClientFactory SelectClientFactory;
