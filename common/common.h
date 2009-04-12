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
  #include <fcntl.h>
  #include <netdb.h>
  #include <sys/socket.h>
  #include <unistd.h>
#endif

/* Miscellaneous defines */

#ifdef _MSC_VER
  #define __int8_t __int8
  #define __uint8_t unsigned __int8
  #define __int16_t __int16
  #define __uint16_t unsigned __int16
  #define __int32_t __int32
  #define __uint32_t unsigned __int32
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

  inline void sleep_(__uint8_t seconds){
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

  inline void sleep_(unsigned __int8_t seconds){
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
  typedef std::vector<__int8_t>::iterator iterator;

  ReadWriteBuffer(int buffer_size):
    buffer_(buffer_size),
	begin_(buffer_.begin()),
    end_(buffer_.end()),
    snd_(begin_),
	rd_(begin_){
  }

  ReadWriteBuffer(const ReadWriteBuffer & other):
    buffer_(other.buffer_),
	begin_(buffer_.begin()),
	end_(buffer_.end()),
	snd_(begin_ + (other.snd_ - other.begin_)),
	rd_(begin_ + (other.rd_ - other.begin_)){
  }

  bool empty() const{
    return snd_ == begin_ && rd_ == begin_;
  }

  iterator rd_begin(){
    return rd_;
  }
  iterator rd_end(){
    return end_;
  }
  size_t rd_size() const{
    return end_ - rd_;
  }
  bool rd_empty() const{
    return rd_ == end_;
  }
  void rd_advance(int distance){
    assert(distance >= 0);
    rd_ += distance;
    assert(rd_ <= end_);
  }

  iterator snd_begin(){
    return snd_;
  }
  iterator snd_end(){
    return rd_;
  }
  size_t snd_size() const{
    return rd_ - snd_;
  }
  bool snd_empty() const{
    return snd_ == rd_;
  }
  void snd_advance(int distance){
    assert(distance >= 0);
    snd_ += distance;
    assert(snd_ <= rd_);

    if (snd_ == rd_)
      rd_ = snd_ = begin_;
  }

private:
  std::vector<__int8_t> buffer_;
  const iterator begin_;
  const iterator end_;
  iterator snd_;
  iterator rd_;
};
