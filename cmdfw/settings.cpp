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

} // namespace mixins
} // namespace settings
} // namespace cmdfw
