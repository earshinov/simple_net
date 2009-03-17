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

  /* Must be less or equal than SO_SNDLOWAT socket option.
   * Use small buffer to be able to test our client in interactive mode.
   *
   */
const unsigned int BUFSIZE = 10; // bytes

string addr("localhost");
string port("28635");

void usage(ostream & o){
  o << "Usage: client_select [-h] [-a ADDRESS] [-p PORT]\n"
    << '\n'
    << "Options:\n"
    << '\n'
    << "-h       -  print this message and exit\n"
    << '\n'
    << "Arguments:\n"
    << '\n'
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
    int c = getopt(argc, argv, "h:a:p:");
    if (c == -1)
      break;
    switch (c){
      case 'h':
        usage(cout);
        return parseCommandLine_exit;
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
  hints.ai_socktype = SOCK_STREAM;
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

  {
    __int8_t buf[BUFSIZE];
    bool stdin_eof = false;

    fd_set rset, wset;
    FD_ZERO(&rset);
    FD_ZERO(&wset);

    for (;;){
      FD_SET(s, &rset);
      if (!stdin_eof)
        FD_SET(s, &wset);

      cerr << "TRACE: calling select()\n";
      int num_ready = select(s + 1, &rset, &wset, 0, 0);
      if (num_ready == -1){
        cerr << "ERROR: select() reported an error\n";
      }
      if (FD_ISSET(s, &rset)){
        int received = recv(s, buf, BUFSIZE, 0);
        if (received == -1){
          cerr << "ERROR: error when receiving information from the server\n";
          break;
        }
        else if (received == 0){
          if (stdin_eof)
            cerr << "TRACE: server closed the connection\n";
          else
            cerr << "ERROR: server closed the connection\n";
          break;
        }
        else {
          cerr << "TRACE: " << received << " bytes to print\n";

          cout.write(reinterpret_cast<char *>(buf), received * sizeof(char));
          int count = cin.gcount() * sizeof(char);
          assert(count == received);
          cerr << "TRACE: " << count << " bytes printed\n";
        }
      }
      if (FD_ISSET(s, &wset)){
        cin.read(reinterpret_cast<char *>(buf), BUFSIZE / sizeof(char));
        int count = cin.gcount() * sizeof(char);
        if (count == 0){
          cerr << "TRACE: EOF at standard input\n";
          stdin_eof = true;
          verify_ne(shutdown(s, SHUT_WR), -1);
          FD_CLR(s, &wset);
        }
        else{
          cerr << "TRACE: " << count << " bytes to send\n";
        
          int sent = send(s, buf, count, 0);
          if (sent == -1){
            cerr << "ERROR: error when sending information to the server\n";
            break;
          }
          else{
            assert(sent == count);
            cerr << "TRACE: " << sent << " bytes sent\n";
          }
        }
      }
    }
  }

  ret = 0;
main_return:
  if (res != 0)
    freeaddrinfo(res);
  if (s != -1)
    close(s);
  return ret;
}
