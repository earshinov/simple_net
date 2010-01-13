#include "settings.hpp"

#include <cstring>
using namespace std;

namespace cmdfw {
namespace settings {
namespace mixins {

int SocketTypeMixin::handle(char * optarg){
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
    std::cerr << "ERROR: Unknown mode: " << optarg << '\n';
    return 2;
  }
}


int PortNumberMixin::handle(char * optarg){
  try{
    port = dirty_lexical_cast::lexical_cast<uint16_t>(optarg);
  }
  catch(dirty_lexical_cast::bad_lexical_cast &){
    std::cerr << "ERROR: Port must be integer: " << optarg << '\n';
    return 1;
  }
  return 0;
}


int BufferSizeMixin::handle(char * optarg){
  try{
    buffer_size = dirty_lexical_cast::lexical_cast<unsigned int>(optarg);
  }
  catch(dirty_lexical_cast::bad_lexical_cast &){
    std::cerr << "ERROR: Buffer size must be integer: " << optarg << '\n';
    return 1;
  }
  return 0;
}


int CxxIoInputMixin::handle(char * optarg){
  if (!set_input(optarg)){
    std::cerr << "ERROR: Could not open input file: " << optarg << '\n';
    return 1;
  }
  return 0;
}

bool CxxIoInputMixin::set_input(const char * filename) {
  input.open(filename, std::ifstream::in | std::ifstream::binary);
  return !input.fail();
}

std::istream & CxxIoInputMixin::get_input() const {
  if (input.is_open())
    return input;
  else
    return std::cin;
}


int CxxIoOutputMixin::handle(char * optarg){
  if (!set_output(optarg)){
    std::cerr << "ERROR: Could not open output file: " << optarg << '\n';
    return 1;
  }
  return 0;
}

bool CxxIoOutputMixin::set_output(const char * filename) {
  output.open(filename, std::ofstream::out | std::ifstream::binary);
  return !output.fail();
}

std::ostream & CxxIoOutputMixin::get_output() const {
  if (output.is_open())
    return output;
  else
    return std::cout;
}


int CIoInputMixin::handle(char * optarg){
  if (!set_input(optarg)){
    std::cerr << "ERROR: Could not open input file: " << optarg << '\n';
    return 1;
  }
  return 0;
}

bool CIoInputMixin::set_input(const char * filename) {
  if (input != NULL)
    verify_e(fclose(input), 0);

  input = fopen(filename, "rb");
  return input != NULL;
}

FILE * CIoInputMixin::get_input() const {
  if (input != NULL)
    return input;
  else
    return stdin;
}


int CIoOutputMixin::handle(char * optarg){
  if (!set_output(optarg)){
    std::cerr << "ERROR: Could not open output file: " << optarg << '\n';
    return 1;
  }
  return 0;
}

bool CIoOutputMixin::set_output(const char * filename) {
  if (output != NULL)
    verify_e(fclose(output), 0);

  output = fopen(filename, "wb");
  return output != NULL;
}

FILE * CIoOutputMixin::get_output() const {
  if (output != NULL)
    return output;
  else
    return stdout;
}


} // namespace mixins
} // namespace settings
} // namespace cmdfw
