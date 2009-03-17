#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
using namespace std;

#ifdef WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <fcntl.h>
	#include <getopt.h>
	#include <netdb.h> // getaddrinfo etc.
	#include <semaphore.h>
	#include <signal.h>
	#include <sys/socket.h>
	#include <sys/wait.h>
	#include <unistd.h>
#endif

#include "../common/common.h"
using namespace dirty_lexical_cast;

namespace{

  /* Must be less or equal to SO_SNDLOWAT socket option. */
const unsigned int BUFSIZE = 1024;
  /* Must be less or equal to FD_SETSIZE. */
const int CLIENTS_SIZE = FD_SETSIZE;

__uint16_t port = 28635;

void usage(ostream & o){
  o << "Usage: server [-h] [-p PORT]\n"
    << '\n'
    << "  Creates an echo server accepting connections from all IPs on all\n"
    << "  available network interfaces.\n"
    << '\n'
    << "Options:\n"
    << '\n'
    << "-h       -  print this message and exit\n"
    << '\n'
    << "Arguments:\n"
    << '\n'
    << "PORT     -  Port number.\n"
    << "            Default: 28635.\n";
}

enum parseCommandLine_Result{
  parseCommandLine_ok,
  parseCommandLine_error,
  parseCommandLine_exit,
};
parseCommandLine_Result parseCommandLine(int argc, char ** argv){
  for (;;){
    int c = getopt(argc, argv, "hp:");
    if (c == -1)
      break;
    switch (c){
      case 'h':
        usage(cout);
        return parseCommandLine_exit;
      case 'p':{
        int t;
        if (!lexical_cast_ref(optarg, t) || t <= 0 || t >= numeric_limits<__uint16_t>::max()){
          cerr << "ERROR: Port is not correct\n";
          usage(cerr);
          return parseCommandLine_error;
        }
        port = static_cast<__uint16_t>(t);
        break;
      }
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

} // namespace

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

  int s = socket(PF_INET6, SOCK_STREAM, 0);
  if (s == -1){
    s = socket(PF_INET, SOCK_STREAM, 0);
    if (s == -1){
      cerr << "ERROR: Could not create socket\n";
      goto main_return;
    }
    else{
      cerr << "TRACE: IPv6 is unavailable. Using IPv4.\n";

      sockaddr_in addr = {0};
      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);
      addr.sin_addr.s_addr = INADDR_ANY;
      if (bind(s, (sockaddr *)&addr, sizeof(addr)) == -1){
        cerr << "ERROR: Could not bind socket\n";
        goto main_return;
      } 
    }
  }
  else{
    cerr << "TRACE: Using IPv6.\n";

    sockaddr_in6 addr = {0};
    addr.sin6_family = AF_INET6;
    addr.sin6_flowinfo = 0;
    addr.sin6_port = htons(port);
    addr.sin6_addr = in6addr_any;
    if (bind(s, (sockaddr *)&addr, sizeof(addr)) == -1){
      cerr << "ERROR: Could not bind socket\n";
      goto main_return;
    }
  }

  if (listen(s, 5) == -1){
    cerr << "ERROR: Could not start listening\n";
    goto main_return;
  }

  /* See Stevens W.R. - "Unix Network Programming", chapter 15.6 */
  #ifdef UNIX
    {
      int flags = fcntl(s, F_GETFL, 0);
      assert(flags != -1);
      verify_ne(fcntl(s, F_SETFL, flags | O_NONBLOCK), -1);
    }
  #endif
  #ifdef WIN32
    {
      unsigned long val = 1;
      verify_ne(ioctlsocket(s, FIONBIO, &val), -1);
    }
  #endif

  {
    __int8_t buf[BUFSIZE];

    int clients[CLIENTS_SIZE];
    for (int i = 0; i < CLIENTS_SIZE; ++i)
      clients[i] = -1;

    fd_set set;
    FD_ZERO(&set);
    FD_SET(s, &set);
    int maxfd = s;

    for (;;){
      fd_set rset = set;
      int num_ready = select(maxfd + 1, &rset, 0, 0, 0);

      if (FD_ISSET(s, &rset)){
        --num_ready;

        int i = 0;
        for (; i < CLIENTS_SIZE && clients[i] != -1; ++i){
        }
        if (i == CLIENTS_SIZE){
          cerr << "TRACE: Do not accept new connection - too many clients\n";
        }
        else{

          int c = accept(s, 0, 0);
          if (c == -1){
            /* See Stevens W.R. - "Unix Network Programming", chapter 15.6 */
            #ifdef UNIX
              if (errno != EWOULDBLOCK && errno != ECONNABORTED && errno != EPROTO && errno != EINTR){
                cerr << "ERROR: Could not accept a connection\n";
                goto main_return;
              }
            #endif
            #ifdef WIN32
              if (errno != WSAECONNRESET){
                cerr << "ERROR: Could not accept a connection\n";
                goto main_return;
              }
            #endif
          }
          
          clients[i] = c;
              
          FD_SET(c, &set);
          if (c > maxfd)
            maxfd = c;
        }
      }

      for (int i = 0; num_ready > 0; ++i){
        int c = clients[i];
        if (c != -1 && FD_ISSET(c, &rset)) {
          --num_ready;

          int count = recv(c, buf, BUFSIZE, 0);
          if (count == -1){
            cerr << "WARNING: Encountered an error while reading data\n";
          }
          else if (count == 0){
            cerr << "TRACE: Client shutdowned\n";
          }
          
          if (count <= 0){
            goto client_fail;
          }
          else{

            int sent_total = 0;
            while (sent_total < count){
              int sent_current = send(c, buf + sent_total, count - sent_total, 0);
              if (sent_current == -1){
                cerr << "WARNING: Encountered an error while sending data\n";
                goto client_fail;
              }
              sent_total += sent_current;
            }
          }

          continue;
  client_fail:
          close(c);          
          clients[i] = -1;
          FD_CLR(c, &set);
        }
      }
    }
  }
  
main_return:
  if (s != -1)
    verify_ne(close(s), -1);
  return 1;
}
