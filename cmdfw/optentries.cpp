#include "optentries.hpp"

#include <cstring>
#include <iostream>
#include <sstream>
using namespace std;

namespace cmdfw {
namespace optentries {

#define FOREACH_OPTENTRY_BEGIN { \
  list<BasicOptentry **>::iterator it = optentries_.begin(); \
  const list<BasicOptentry **>::iterator end = optentries_.end(); \
  for (; it != end; ++it){ \
    for (BasicOptentry ** i = *it; *i; ++i){ \
      BasicOptentry & optentry = **i;

#define FOREACH_CONST_OPTENTRY_BEGIN { \
  list<BasicOptentry **>::const_iterator it = optentries_.begin(); \
  const list<BasicOptentry **>::const_iterator end = optentries_.end(); \
  for (; it != end; ++it){ \
    for (BasicOptentry ** i = *it; *i; ++i){ \
      const BasicOptentry & optentry = **i;

#define FOREACH_OPTENTRY_END }}}
#define FOREACH_CONST_OPTENTRY_END FOREACH_OPTENTRY_END

std::string Optentries::options() const{
  stringstream ss;
  FOREACH_CONST_OPTENTRY_BEGIN
    ss << optentry.option();
  FOREACH_CONST_OPTENTRY_END
  return ss.str();
}

std::string Optentries::options_help() const{
  stringstream ss;
  FOREACH_CONST_OPTENTRY_BEGIN
    ss << '-' << optentry.letter() << '\n' << "  " << optentry.description() << '\n';
  FOREACH_CONST_OPTENTRY_END
  return ss.str();
}

int Optentries::handle(char letter, char * optarg){
  FOREACH_OPTENTRY_BEGIN
    if (letter == optentry.letter()){
      switch(optentry.handle(optarg)){
        case 0: return 0;
        case 1: return 2;
        case 2: return 3;
        default:
          assert(false);
          return 3;
      }
    }
  FOREACH_OPTENTRY_END
  return 1;
}

bool Optentries::validate() const{
  bool success = true;
  FOREACH_CONST_OPTENTRY_BEGIN
    if (!optentry.is_set()){
      cerr << "ERROR: Option '-" << optentry.letter() << "' is required.\n";
      success = false;
    }
  FOREACH_CONST_OPTENTRY_END
  return success;
}

#undef FOREACH_OPTENTRY_BEGIN
#undef FOREACH_CONST_OPTENTRY_BEGIN
#undef FOREACH_OPTENTRY_END
#undef FOREACH_CONST_OPTENTRY_END

/* ---------------------------------------------------------------------------------------------- */

int SocketType::handle(char * optarg){
  for (char * p = optarg; *p != '\0'; ++p)
    *p = toupper(*p);
  if (strcmp(optarg, "TCP") == 0){
    socket_type = SOCK_STREAM;
    return 0;
  }
  else if (strcmp(optarg, "UDP") == 0){
    socket_type = SOCK_DGRAM;
    return 0;
  }
  else{
    cerr << "ERROR: Unknown mode: " << optarg << '\n';
    return 2;
  }
}


int PortNumber::handle(char * optarg){
  try{
    port = dirty_lexical_cast::lexical_cast<uint16_t>(optarg);
  }
  catch(dirty_lexical_cast::bad_lexical_cast &){
    cerr << "ERROR: Port must be integer: " << optarg << '\n';
    return 1;
  }
  return 0;
}


int BufferSize::handle(char * optarg){
  try{
    buffer_size = dirty_lexical_cast::lexical_cast<unsigned int>(optarg);
  }
  catch(dirty_lexical_cast::bad_lexical_cast &){
    cerr << "ERROR: Buffer size must be integer: " << optarg << '\n';
    return 1;
  }
  return 0;
}


int CxxIoInput::handle(char * optarg){
  if (!set_input(optarg)){
    cerr << "ERROR: Could not open input file: " << optarg << '\n';
    return 1;
  }
  return 0;
}

bool CxxIoInput::set_input(const char * filename) {
  input.open(filename, ifstream::in | ifstream::binary);
  return !input.fail();
}

std::istream & CxxIoInput::get_input() const {
  if (input.is_open())
    return input;
  else
    return cin;
}


int CxxIoOutput::handle(char * optarg){
  if (!set_output(optarg)){
    cerr << "ERROR: Could not open output file: " << optarg << '\n';
    return 1;
  }
  return 0;
}

bool CxxIoOutput::set_output(const char * filename) {
  output.open(filename, ofstream::out | ifstream::binary);
  return !output.fail();
}

std::ostream & CxxIoOutput::get_output() const {
  if (output.is_open())
    return output;
  else
    return cout;
}


int CIoInput::handle(char * optarg){
  if (!set_input(optarg)){
    cerr << "ERROR: Could not open input file: " << optarg << '\n';
    return 1;
  }
  return 0;
}

bool CIoInput::set_input(const char * filename) {
  if (input != NULL)
    verify_e(fclose(input), 0);

  input = fopen(filename, "rb");
  return input != NULL;
}

FILE * CIoInput::get_input() const {
  if (input != NULL)
    return input;
  else
    return stdin;
}


int CIoOutput::handle(char * optarg){
  if (!set_output(optarg)){
    cerr << "ERROR: Could not open output file: " << optarg << '\n';
    return 1;
  }
  return 0;
}

bool CIoOutput::set_output(const char * filename) {
  if (output != NULL)
    verify_e(fclose(output), 0);

  output = fopen(filename, "wb");
  return output != NULL;
}

FILE * CIoOutput::get_output() const {
  if (output != NULL)
    return output;
  else
    return stdout;
}

} // namespace optentries
} // namespace cmdfw
