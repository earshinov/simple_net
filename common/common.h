#pragma once


#ifdef _MSC_VER
  #define __int8_t __int8
  #define __uint8_t unsigned __int8
  #define __int16_t __int16
  #define __uint16_t unsigned __int16
  #define __int32_t __int32
  #define __uint32_t unsigned __int32
#endif

#ifdef WIN32
  #define close(socket) closesocket(socket)
  #define SHUT_WR SD_SEND
#endif


#include <cassert>
#include <string>
#include <sstream>


#if defined(WIN32) && !defined(MINGW)
  #include "XGetOpt.h"
#else
  #include <getopt.h>
#endif


#ifdef _DEBUG
#define verify(e) assert(e)
#define verify_ne(e, constant_) assert(e != constant_)
#else
#define verify(e) { e; }
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
