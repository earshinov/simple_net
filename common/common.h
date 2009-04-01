#pragma once

#include <cassert>
#include <iostream>
#include <string>
#include <sstream>

#ifdef WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <cerrno>
  #include <sys/socket.h>
  #include <netdb.h>
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
  inline int close(int s){
    return closesocket(s);
  }

  #define SHUT_WR SD_SEND
#else
  #define SOCKETS_ERRNO errno
  #define SOCKETS_ERROR(error) error
  inline void SOCKETS_PERROR(const char * s){ return perror(s); }

  inline int getsockopt_(int s, int level, int optname, void * optval, socklen_t * optlen){
    return getsockopt(s, level, optname, optval, optlen);
  }
  inline int setsockopt_(int s, int level, int optname, const void *optval, socklen_t optlen){
    return setsockopt(s, level, optname, optval, optlen);
  }
#endif

/* Some socket functions common to clients and servers */

inline bool try_setsockopt_sndlowat(int s, int buffer_size){

  /*
   * TODO:
   *
   * Problem
   * -------
   *
   * On Linux SO_SNDLOWAT can not be set, but can be get, and
   * the value is 1. That is, if our buffer size is greater than 1 (and it
   * mostly is, because we have awful performance using such a small buffer)
   * we can not be sure that send() function won't block when we call it
   * after accept() returned the socket descriptor in write FD_SET.
   *
   * Same problem on Windows (we can not even get SO_SNDLOWAT value there).
   *
   * Solution
   * --------
   *
   *   1. make socket non-blocking;
   *   2. if the EWOULDBLOCK error occurs when calling send(),
   *      save the unsent part of the buffer;
   *   3. do not pass our read FDSET to accept() until the buffer is all sent.
   *
   * More efficient solution is described in the "UNIX Network Programming" book
   * (to be precise, in chapter 15.2 describing non-blocking IO).
   * But with that solution we won't need select() at all ^-)
   *
   * State of work
   * -------------
   *
   * I don't want to do it because implementation of the solution is tedious.
   */

  /*
  if (setsockopt_(s, SOL_SOCKET, SO_SNDLOWAT, &buffer_size, sizeof(buffer_size)) != -1)
    return true;
  assert(SOCKETS_ERRNO == SOCKETS_ERROR(ENOPROTOOPT));

  int max_buffer_size = 0;
  socklen_t len = sizeof(max_buffer_size);
  bool success = getsockopt_(s, SOL_SOCKET, SO_SNDLOWAT, &max_buffer_size, &len) != -1;
  assert(len == sizeof(max_buffer_size));

  if (success){
    if (buffer_size > max_buffer_size){
      std::cerr <<
        "ERROR: Can not set so large buffer size. It must not exceed SO_SNDLOWAT socket\n"
        "  option equal to " << max_buffer_size << " on this platform.\n";
      return false;
    }
  }
  else{
    assert(SOCKETS_ERRNO == SOCKETS_ERROR(ENOPROTOOPT));
    std::cerr <<
      "WARNING: Could not set SO_SNDLOWAT socket option. Program may work incorrectly\n"
      "  if you set large buffer size with -b command line option.\n";
  }
  */

  return true;
}

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
