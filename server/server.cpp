#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
using namespace std;

#ifdef WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
	#include <getopt.h>
	#include <netdb.h> // getaddrinfo etc.
	#include <semaphore.h>
	#include <signal.h>
	#include <sys/socket.h>
	#include <sys/wait.h>
#endif

#include "../common/common.h"
using namespace dirty_lexical_cast;

namespace{

const unsigned int BUFSIZE = 1024;

int socket_type = SOCK_STREAM; // TCP
__uint16_t port = 28635;
int limit = 0; // 0 threades are useless, so it will mean "not limited"

void usage(ostream & o){
  o << "Usage: server [-h] [-m MODE] [-p PORT] [-l LIMIT]\n"
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
    << "MODE     -  \"TCP\" or \"UDP\" (case insensitive).\n"
    << "            Default: TCP.\n"
    << "PORT     -  Port number.\n"
    << "            Default: 28635.\n"
    << "LIMIT    -  Maximum number of simultaneous connections, must be\n"
    << "            a positive integer value.\n"
    << "            Default: number of connections is unlimited\n";
}

enum parseCommandLine_Result{
  parseCommandLine_ok,
  parseCommandLine_error,
  parseCommandLine_exit,
};
parseCommandLine_Result parseCommandLine(int argc, char ** argv){
  for (;;){
    int c = getopt(argc, argv, "hm:p:l:");
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
      case 'l':{
        int t;
        if (!lexical_cast_ref(optarg, t) || t <= 0){
          cerr << "ERROR: Limit is not correct\n";
          usage(cerr);
          return parseCommandLine_error;
        }
        limit = t;
        break;
      }
      case '?':
      default:
        cerr << "ERROR: Bad arguments\n";
        usage(cerr);
        return parseCommandLine_error;
    }
  }
  if (socket_type != SOCK_STREAM && limit != 0)
    cerr << "WARNING: Limit options is applicable for TCP only\n";
  if (optind != argc){
    cerr << "ERROR: Bad arguments\n";
    usage(cerr);
    return parseCommandLine_error;
  }
  return parseCommandLine_ok;
}

#ifdef UNIX
sem_t sem;
void sig_chld(int signal){
  while (waitpid(-1, 0, WNOHANG) > 0)
    if (limit != 0)
      verify_ne(sem_post(&sem), -1);
}
#endif

#ifdef UNIX
  void tcp_subprocess(int c){
#endif
#ifdef WIN32
  struct tcp_subprocess_data{
    HANDLE sem;
    int c;
  };
  DWORD WINAPI tcp_subprocess(void * param){
     tcp_subprocess_data * data = reinterpret_cast<tcp_subprocess_data *>(param);
     assert(data);

     HANDLE sem = data->sem;
     int c = data->c;

     delete data;
     data = 0;
#endif
  __int8_t buf[BUFSIZE];
  for (;;){
    int count = recv(c, buf, BUFSIZE, 0);
    if (count == -1){
      cerr << "WARNING: Encountered an error while reading data\n";
      goto subprocess_return;
    }
    if (count == 0){
      cerr << "NOTE: Client shutdowned\n";
      goto subprocess_return;
    }

    int sent_total = 0;
    while (sent_total < count){
      int sent_current = send(c, buf + sent_total, count - sent_total, 0);
      if (sent_current == -1){
        cerr << "WARNING: Encountered an error while sending data\n";
        goto subprocess_return;
      }
      sent_total += sent_current;
    }
  }
subprocess_return:
  verify_ne(close(c), -1);
  #ifdef WIN32
    verify(ReleaseSemaphore(sem, 1, 0));
    return 0;
  #endif
}

bool tcp(int s){
  #ifdef UNIX
    if (signal(SIGCHLD, sig_chld) == SIG_ERR){
      cerr << "ERROR: Could not bind SIGCHLD handler\n";
      return false;
    }
  #endif

  #ifdef UNIX
    if (limit != 0){
      if (sem_init(&sem, 0 /* not shared */, limit) == -1){
        cerr << "ERROR: Could not apply limit\n";
        return false;
      }
    }
  #endif
  #ifdef WIN32
    HANDLE sem = 0;
    if (limit != 0){
      sem = CreateSemaphore(0, limit, limit, 0);
      if (!sem){
        cerr << "ERROR: Could not apply limit\n";
        return false;
      }
    }
  #endif
  
  if (listen(s, 5) == -1){
    cerr << "ERROR: Could not start listening\n";
    goto tcp_return;
  }

  for (;;){

    if (limit != 0){
      #ifdef UNIX
        verify_ne(sem_wait(&sem), -1);
      #endif
      #ifdef WIN32
        WaitForSingleObject(sem, INFINITE);
      #endif
    }
      
    int c = accept(s, 0, 0); /* Socket bound to this connection */
    if (c == -1){
      cerr << "ERROR: Could not accept a connection\n";
      goto tcp_return;
    }
    
    #ifdef UNIX
      if (fork()){
        // This process
        verify_ne(close(c), -1);
      }
      else{
        // Child process
        verify_ne(close(s), -1);
        tcp_subprocess(c);
        exit(0); 
      }
    #endif
    #ifdef WIN32
      tcp_subprocess_data * data = new tcp_subprocess_data;
      data->sem = sem;
      data->c = c;
      CreateThread(0, 0, tcp_subprocess, data, 0, 0);
    #endif
  }

tcp_return:
  if (limit != 0){
    #ifdef UNIX
      verify_ne(sem_destroy(&sem), -1);
    #endif
    #ifdef WIN32
      verify(CloseHandle(sem));
    #endif
  }
  return false;
}


bool udp(int s){
  __int8_t buf[BUFSIZE];
  for (;;){
      /* sockaddr_storage matches the size of the largest struct that can be returned
       *
       */
    sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    
    int count = recvfrom(s, buf, BUFSIZE, 0, reinterpret_cast<sockaddr *>(&addr), &addr_len);
    if (count == -1){
      cerr << "WARNING: Encountered an error while reading data\n";
      continue;
    }
    if (count == 0){
      cerr << "NOTE: Client shutdowned\n";
      continue;
    }

    int sent_total = 0;
    while (sent_total < count){
      int sent_current = sendto(s, buf + sent_total, count - sent_total, 0, reinterpret_cast<sockaddr *>(&addr), addr_len);
      if (sent_current == -1){
        cerr << "WARNING: Encountered an error while sending data\n";
        break;
      }
      sent_total += sent_current;
    }
  }
  return true;
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

  int s = socket(PF_INET6, socket_type, 0);
  if (s == -1){
    s = socket(PF_INET, socket_type, 0);
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

  if (socket_type == SOCK_STREAM){ // TCP
    if (!tcp(s))
      goto main_return;
  }
  else if (socket_type == SOCK_DGRAM){ // UDP
    if (!udp(s))
      goto main_return;
  }
  else
    assert(false);

main_return:
  if (s != -1)
    verify_ne(close(s), -1);
  return 1;
}
