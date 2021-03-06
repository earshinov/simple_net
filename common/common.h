#pragma once

#include <cassert>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#ifdef WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <cerrno>
  #include <cstdio> /* for `perror()' */
  #include <fcntl.h>
  #include <netdb.h>
  #include <sys/socket.h>
  #include <unistd.h>
#endif

/* Miscellaneous defines */

#ifdef _MSC_VER
  typedef __int8 int8_t;
  typedef unsigned __int8 uint8_t;
  typedef __int16 int16_t;
  typedef unsigned __int16 uint16_t;
  typedef __int32 int32_t;
  typedef unsigned __int32 uint32_t;
#endif

/* Socket function defines */

#ifdef UNIX
  #define SOCKETS_ERRNO errno
  #define SOCKETS_ERROR(error) error
  inline void SOCKETS_PERROR(const char * s){ return perror(s); }

  inline int getsockopt_(int s, int level, int optname, void * optval, socklen_t * optlen){
    return getsockopt(s, level, optname, optval, optlen);
  }
  inline int setsockopt_(int s, int level, int optname, const void *optval, socklen_t optlen){
    return setsockopt(s, level, optname, optval, optlen);
  }

  inline int setnonblocking(int s){
    int flags = fcntl(s, F_GETFL, 0);
    assert(flags != -1);
    return fcntl(s, F_SETFL, flags | O_NONBLOCK);
  }
  inline int setblocking(int s){
    int flags = fcntl(s, F_GETFL, 0);
    assert(flags != -1);
    return fcntl(s, F_SETFL, flags & ~O_NONBLOCK);
  }

  inline void sleep_(uint8_t seconds){
    sleep(seconds);
  }
#endif
#ifdef WIN32
  #define SOCKETS_ERRNO WSAGetLastError()
  #define SOCKETS_ERROR(error) WSA##error
  inline void SOCKETS_PERROR(const char * s){
    std::cerr << s << ": " << SOCKETS_ERRNO << "\n"
      "  Please see http://msdn.microsoft.com/en-us/library/ms740668(VS.85).aspx\n"
      "  for description of this error code.\n";
  }

  inline int getsockopt_(int s, int level, int optname, void * optval, socklen_t * optlen){
    return getsockopt(s, level, optname, static_cast<char *>(optval), optlen);
  }
  inline int setsockopt_(int s, int level, int optname, const void *optval, socklen_t optlen){
    return setsockopt(s, level, optname, static_cast<const char *>(optval), optlen);
  }

  inline int setnonblocking(int s){
    unsigned long val = 1;
    return ioctlsocket(s, FIONBIO, &val);
  }
  inline int setblocking(int s){
    unsigned long val = 0;
    return ioctlsocket(s, FIONBIO, &val);
  }

  inline int close(int s){
    return closesocket(s);
  }

  inline void sleep_(uint8_t seconds){
    return Sleep(seconds * 1000);
  }

  #define SHUT_WR SD_SEND
#endif

/* Getopt defines */

#if defined(WIN32) && !defined(MINGW)
  #include "XGetOpt.h"
#else
  #include <getopt.h>
#endif

/* Useful assert() and verify() macros */

#ifdef _DEBUG
#define verify(e) assert(e)
#define verify_e(e, constant_) assert(e == constant_)
#define verify_ne(e, constant_) assert(e != constant_)
#else
#define verify(e) { e; }
#define verify_e(e, constant_) { e; }
#define verify_ne(e, constant_) { e; }
#endif

/* Quick and dirty implementation of something similar to boost::lexical_cast */

namespace dirty_lexical_cast{

struct bad_lexical_cast{
};

template<class Target, class Source>
bool lexical_cast_ref(Source const & arg, Target & ret){
  std::stringstream ss;
  ss << arg;
  return (ss >> ret) && ss.get() == std::char_traits<char>::eof();
}

template<class Target, class Source>
Target lexical_cast(Source const & arg){
  Target ret;
  if (lexical_cast_ref<Target, Source>(arg, ret))
    return ret;
  else
    throw bad_lexical_cast();
}

  /* TODO: Add optimized specialized versions if needed */
}

/* Read-write buffer */

class ReadWriteBuffer{
public:
  typedef std::vector<int8_t>::iterator iterator;

  ReadWriteBuffer(int buffer_size):
    buffer_(buffer_size),
    begin_(buffer_.begin()),
    end_(buffer_.end()),
    snd_(begin_),
    rd_(begin_),
    overwrite(false){
  }

  ReadWriteBuffer(const ReadWriteBuffer & other):
    buffer_(other.buffer_),
    begin_(buffer_.begin()),
    end_(buffer_.end()),
    snd_(begin_ + (other.snd_ - other.begin_)),
    rd_(begin_ + (other.rd_ - other.begin_)),
    overwrite(other.overwrite){
  }

  bool empty() const{
    return !overwrite && snd_ == begin_ && rd_ == begin_;
  }

  iterator rd_begin(){
    return overwrite ? begin_ : rd_;
  }
  iterator rd_end(){
    return overwrite ? snd_ : end_;
  }
  size_t rd_size(){
    return rd_end() - rd_begin();
  }
  bool rd_empty(){
    return rd_end() == rd_begin();
  }
  void rd_advance(int distance){
    assert(distance >= 0);
    rd_ += distance;
    if (!overwrite){
      assert(rd_ <= end_);
      if (rd_ == end_){
        rd_ = begin_;
        overwrite = true;
      }
    }
    else
      assert(rd_ <= snd_);
  }

  iterator snd_begin(){
    return snd_;
  }
  iterator snd_end(){
    return rd_;
  }
  size_t snd_size(){
    return (overwrite ? end_ : rd_) - snd_;
  }
  bool snd_empty(){
    return snd_size() == 0;
  }
  void snd_advance(int distance){
    assert(distance >= 0);
    snd_ += distance;

    if (!overwrite){
      assert(snd_ <= rd_);
      if (snd_ == rd_)
        snd_ = rd_ = begin_;
    }
    else{
      assert(snd_ <= end_);
      if (snd_ == end_){
        snd_ = begin_;
        overwrite = false;
      }
    }
  }

private:
  std::vector<int8_t> buffer_;
  const iterator begin_;
  const iterator end_;
  iterator snd_;
  iterator rd_;
  bool overwrite;
};
