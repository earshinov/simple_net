#include <cassert>
#include <cctype>
#include <cstring>
#include <iostream>
#include <string>
using namespace std;

#ifdef WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <netdb.h> // getaddrinfo etc.
  #include <sys/socket.h>
#endif

#include "../common/common.h"

namespace{

  /* Use small buffer to be able to test our client in interactive mode
   *
   */
const unsigned int BUFSIZE = 10; // bytes

int socket_type = SOCK_STREAM;
string addr("localhost");
string port("28635");

void usage(ostream & o){
  o << "Usage: client [-h] [-m MODE] [-a ADDRESS] [-p PORT]\n"
    << '\n'
    << "Options:\n"
    << '\n'
    << "-h       -  print this message and exit\n"
    << '\n'
    << "Arguments:\n"
    << '\n'
    << "MODE     -  \"UDP\" or \"TCP\" (case insensitive).\n"
    << "            Default: TCP.\n"
    << "ADDRESS  -  IPv4 or IPv6 address or host name.\n"
    << "            Default: localhost.\n"
    << "PORT     -  port number or service name (e.g., \"http\").\n"
    << "            Default: 28635.\n";
}

enum parseCommandLine_Result{
  parseCommandLine_ok,
  parseCommandLine_error,
  parseCommandLine_exit,
};
parseCommandLine_Result parseCommandLine(int argc, char ** argv){
  for (;;){
    int c = getopt(argc, argv, "hm:a:p:");
    if (c == -1)
      break;
    switch (c){
      case 'h':
        usage(cout);
        return parseCommandLine_exit;
      case 'm':
        for (char * p = optarg; *p != '\0'; ++p)
          *p = toupper(*p);
        if (strcmp(optarg, "TCP") == 0)
          socket_type = SOCK_STREAM;
        else if (strcmp(optarg, "UDP") == 0)
          socket_type = SOCK_DGRAM;
        else{
          cerr << "ERROR: Unknown mode: " << optarg << '\n';
          usage(cerr);
          return parseCommandLine_error;
        }
        break;
      case 'a':
        addr = optarg;
        break;
      case 'p':
        port = optarg;
        break;
      case '?':
      default:
        cerr << "ERROR: Bad arguments\n";
        usage(cerr);
        return parseCommandLine_error;
    }
  }
  if (optind != argc){
    cerr << "ERROR: Bad arguments\n";
    usage(cerr);
    return parseCommandLine_error;
  }
  return parseCommandLine_ok;
}

}

int main(int argc, char ** argv){

  switch (parseCommandLine(argc, argv)){
    case parseCommandLine_ok:
      break;
    case parseCommandLine_error:
      return 1;
    case parseCommandLine_exit:
      return 0;
    default:
      assert(false);
  }

  #ifdef WIN32
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata)){
      cerr << "ERROR: Could not initialize socket library\n";
      return 1;
    }
  #endif
  int ret = 1;

  addrinfo * res = 0;
  addrinfo * p = 0;
  int s = -1; // Our socket

  addrinfo hints = {0};
  hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
  hints.ai_socktype = socket_type;
  if (getaddrinfo(addr.c_str(), port.c_str(), &hints, &res) != 0){
    cerr << "ERROR: Could not resolve IP address and/or port\n";
    goto main_return;
  }  

  for(p = res; p != NULL; p = p->ai_next){
    s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (s == -1)
      continue;
    if (connect(s, p->ai_addr, p->ai_addrlen) == -1){
      close(s), s = -1;
      continue;
    }
    break;
  }
  if (s == -1){
    cerr << "ERROR: Could not connect to server\n";
    goto main_return;
  }

  __int8_t buf[BUFSIZE];
  while (!cin.eof()){

    cin.read(reinterpret_cast<char *>(buf), BUFSIZE / sizeof(char));
    int count = cin.gcount() * sizeof(char);
    if (count == 0)
      continue;
    cerr << "TRACE: " << count << " bytes to send\n";

    int sent_total = 0;
    while (sent_total < count){
      int sent_current = send(s, buf + sent_total, count - sent_total, 0);
      if (sent_current == -1){
        cerr << "ERROR: Encountered an error while sending data\n";
        goto main_return;
      }
      cerr << "TRACE: " << sent_current << " bytes sent\n";
      sent_total += sent_current;
    }
    cerr << "TRACE: Start receiving\n";

    int recv_total = 0;
    while (recv_total < count){
      int recv_current = recv(s, buf + recv_total, BUFSIZE - recv_total, 0);
      if (recv_current == -1){
        cerr << "ERROR: Encountered an error while reading data\n";
        goto main_return;
      }
      if (recv_current == 0){
        cerr << "ERROR: Server shutdowned unexpectedly\n";
        goto main_return;
      }
      cerr << "TRACE: Received " << recv_current << " bytes\n";
      recv_total += recv_current;
    }
    cout.write(reinterpret_cast<char *>(buf), recv_total * sizeof(char));
  }

  ret = 0;
main_return:
  if (res != 0)
    freeaddrinfo(res);
  if (s != -1)
    close(s);
  return ret;
}
