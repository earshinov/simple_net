#include "config.h"
#include "../common/common.h"
#include "../cmdfw/cmdfw.hpp"
#include "client_select.hpp"

#include <cstdlib>
using namespace std;

#ifdef UNIX
  #include <getopt.h>
  #include <semaphore.h>
  #include <signal.h>
  #include <sys/wait.h>
#endif

#ifdef HAVE_LIBEV
  #include "client_libev.hpp"
  LibevClientFactory * libev_factory = 0;

  #include <ev.h>
#endif

namespace{

struct LimitOptentry: public cmdfw::optentries::BasicOptentry {
  typedef int value_t;

  /* override */ virtual char letter() const { return 'l'; }
  /* override */ virtual std::string option() const { return "l:"; }
  /* override */ virtual std::string description() const { return
    "Maximum number of simultaneous connections, must be a positive integer value.\n"
    "  Default: number of connections is unlimited."; }

  value_t limit; /* 0 means unlimited */
  LimitOptentry(): limit(0) {}

  /* override */ virtual int handle(char * optarg){
    int t;
    if (!dirty_lexical_cast::lexical_cast_ref(optarg, t) || t <= 0){
      cerr << "ERROR: Limit is not correct.\n";
      return 1;
    }
    limit = t;
    return 0;
  }
};

struct AllOptentries {

  /* GeneralOptentries */
  cmdfw::optentries::SocketType socket_type;
  cmdfw::optentries::PortNumber port;
  cmdfw::optentries::BufferSize buffer_size;

  /* LimitOptentries */
  LimitOptentry limit;

} OPTENTRIES;

class GeneralOptentries: public cmdfw::optentries::Optentries {
public:
  static auto_ptr<GeneralOptentries> create(){
    auto_ptr<GeneralOptentries> ret(new GeneralOptentries());
    ret->append_optentries();
    return ret;
  }
protected:
  GeneralOptentries() {}
  void append_optentries(){
    cmdfw::optentries::Optentries::append_optentries(entries);
  }

public:

  cmdfw::optentries::SocketType::value_t & socket_type(){ return OPTENTRIES.socket_type.socket_type; }
  cmdfw::optentries::PortNumber::value_t & port(){ return OPTENTRIES.port.port; }
  cmdfw::optentries::BufferSize::value_t & buffer_size(){ return OPTENTRIES.buffer_size.buffer_size; }

protected:

  static cmdfw::optentries::BasicOptentry * entries[];
};

/* static */ cmdfw::optentries::BasicOptentry * GeneralOptentries::entries[] = {
  //&OPTENTRIES.socket_type, /* hidden */
  &OPTENTRIES.port,
  &OPTENTRIES.buffer_size,
  0
};

struct UdpOptentries: public GeneralOptentries {
public:
  static auto_ptr<UdpOptentries> create(){
    auto_ptr<UdpOptentries> ret(new UdpOptentries());
    ret->append_optentries();

    /* This function is called when the corresponding mode is known to be chosen by user,
     * so it is safe to tamper global OPTENTRIES structure here */
    OPTENTRIES.socket_type.socket_type = SOCK_DGRAM;

    return ret;
  }
protected:
  UdpOptentries() {}
};

class LimitOptentries: public GeneralOptentries {
public:
  static auto_ptr<LimitOptentries> create(){
    auto_ptr<LimitOptentries> ret(new LimitOptentries());
    ret->append_optentries();
    return ret;
  }
protected:
  LimitOptentries() {}
  void append_optentries(){
    GeneralOptentries::append_optentries();
    cmdfw::optentries::Optentries::append_optentries(entries);
  }

public:

  LimitOptentry::value_t limit(){ return OPTENTRIES.limit.limit; }

protected:

  static cmdfw::optentries::BasicOptentry * entries[];
};

/* static */ cmdfw::optentries::BasicOptentry * LimitOptentries::entries[] = {
  &OPTENTRIES.limit,
  0
};

typedef GeneralOptentries SelectOptentries;
typedef LimitOptentries TcpOptentries, LibevOptentries;


typedef bool (*Handler)(GeneralOptentries & settings, int s);
bool invoke_handler(Handler handler, GeneralOptentries & settings);

bool udp_handler(GeneralOptentries & settings, int s);
bool tcp_handler(GeneralOptentries & settings_, int s);
bool select_handler(GeneralOptentries & settings_, int s);
#ifdef HAVE_LIBEV
  bool libev_handler(GeneralOptentries & settings_, int s);
#endif


template <typename Optentries> class Mode: public cmdfw::mode::BasicMode {
public:

  Mode(const string & name, const string & description, Handler handler):
    cmdfw::mode::BasicMode(name, description), handler_(handler) { }
  Mode(const vector<string> & names, const string & description, Handler handler):
    cmdfw::mode::BasicMode(names, description), handler_(handler) { }

  /* override */ virtual auto_ptr<cmdfw::optentries::BasicOptentries> optentries() const {
    return auto_ptr<cmdfw::optentries::BasicOptentries>(Optentries::create());
  }

  /* override */ virtual bool handle(cmdfw::optentries::BasicOptentries & optentries) const {
    return invoke_handler(handler_, static_cast<GeneralOptentries &>(optentries));
  }

private:
  Handler handler_;
};


typedef vector<int8_t> Buffer;

/* ------------------------------------------------------------------------- */

bool udp_handler(GeneralOptentries & optentries_, int s){
  UdpOptentries & optentries = static_cast<UdpOptentries &>(optentries_);

  Buffer buffer(optentries.buffer_size());
  int8_t * buf = &buffer[0];

  for (;;){
      /*
       * sockaddr_storage matches the size of the largest struct that can be returned
       */
    sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);

    int count = recvfrom(s, buf, optentries.buffer_size(), 0,
      reinterpret_cast<sockaddr *>(&addr), &addr_len);
    if (count == -1){
      SOCKETS_PERROR("WARNING: recvfrom");
      continue;
    }

    int sent_total = 0;
    while (sent_total < count){
      int sent_current = sendto(s, buf + sent_total, count - sent_total, 0,
        reinterpret_cast<sockaddr *>(&addr), addr_len);
      if (sent_current == -1){
        SOCKETS_PERROR("WARNING: sendto");
        break;
      }
      sent_total += sent_current;
    }
  }
  return true;
}

/* ------------------------------------------------------------------------- */

#ifdef UNIX
sem_t sem;
int limit;
void sig_chld(int signal){
  while (waitpid(-1, 0, WNOHANG) > 0)
    if (limit != 0)
      verify_ne(sem_post(&sem), -1);
}
#endif

#ifdef UNIX
  void tcp_subprocess(int c, int buffer_size){
#endif
#ifdef WIN32
  struct tcp_subprocess_data{
    HANDLE sem;
    int c;
    int buffer_size;
  };
  DWORD WINAPI tcp_subprocess(void * param){
     tcp_subprocess_data * data = reinterpret_cast<tcp_subprocess_data *>(param);
     assert(data);

     HANDLE sem = data->sem;
     int c = data->c;
     int buffer_size = data->buffer_size;

     delete data;
     data = 0;
#endif

  Buffer buffer(buffer_size);
  int8_t * buf = &buffer[0];

  for (;;){
    int count = recv(c, buf, buffer_size, 0);
    if (count == -1){
      SOCKETS_PERROR("WARNING: recv");
      goto subprocess_return;
    }
    if (count == 0){
      cerr << "TRACE: Client shutdowned.\n";
      goto subprocess_return;
    }

    int sent_total = 0;
    while (sent_total < count){
      int sent_current = send(c, buf + sent_total, count - sent_total, 0);
      if (sent_current == -1){
        SOCKETS_PERROR("WARNING: send");
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

bool tcp_handler(GeneralOptentries & optentries_, int s){
  TcpOptentries & optentries = static_cast<TcpOptentries &>(optentries_);

  #ifdef UNIX
    limit = optentries.limit();
    if (signal(SIGCHLD, sig_chld) == SIG_ERR){
      cerr << "ERROR: Could not bind SIGCHLD handler.\n";
      return false;
    }
  #endif

  #ifdef UNIX
    if (optentries.limit() != 0){
      if (sem_init(&sem, 0 /* not shared */, optentries.limit()) == -1){
        cerr << "ERROR: Could not apply limit.\n";
        return false;
      }
    }
  #endif
  #ifdef WIN32
    HANDLE sem = 0;
    if (optentries.limit() != 0){
      sem = CreateSemaphore(0, optentries.limit(), optentries.limit(), 0);
      if (!sem){
        cerr << "ERROR: Could not apply limit.\n";
        return false;
      }
    }
  #endif

  if (listen(s, 5) == -1){
    SOCKETS_PERROR("ERROR: listen");
    goto tcp_return;
  }

  for (;;){

    if (optentries.limit() != 0){
      #ifdef UNIX
        verify_ne(sem_wait(&sem), -1);
      #endif
      #ifdef WIN32
        WaitForSingleObject(sem, INFINITE);
      #endif
    }

    int c = accept(s, 0, 0); /* Socket bound to this connection */
    if (c == -1){
      SOCKETS_PERROR("ERROR: accept");
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
        tcp_subprocess(c, optentries.buffer_size());
        exit(0);
      }
    #endif
    #ifdef WIN32
      tcp_subprocess_data * data = new tcp_subprocess_data;
      data->sem = sem;
      data->c = c;
      data->buffer_size = optentries.buffer_size();
      CloseHandle(CreateThread(0, 0, tcp_subprocess, data, 0, 0));
    #endif
  }

tcp_return:
  if (optentries.limit() != 0){
    #ifdef UNIX
      verify_ne(sem_destroy(&sem), -1);
    #endif
    #ifdef WIN32
      verify(CloseHandle(sem));
    #endif
  }
  return false;
}

bool select_handler(GeneralOptentries & optentries_, int s){
  SelectOptentries & optentries = static_cast<SelectOptentries &>(optentries_);

  if (listen(s, 5) == -1){
    SOCKETS_PERROR("ERROR: listen");
    return false;
  }

  /* See Stevens W.R. - "Unix Network Programming", chapter 15.6 */
  verify_ne(setnonblocking(s), -1);

  fd_set xrset, xwset;
  FD_ZERO(&xrset);
  FD_ZERO(&xwset);
  FD_SET(s, &xrset);
  int maxfd = s;

  SelectClientFactory client_factory(optentries.buffer_size(),
    SelectClientFactory::storage_mixin_t(),
    SelectClientFactory::network_mixin_t(&xrset, &xwset, &maxfd));

  for (;;){
    fd_set rset = xrset;
    fd_set wset = xwset;
    int num_ready = select(maxfd + 1, &rset, &wset, 0, 0);

    if (FD_ISSET(s, &rset)){
      --num_ready;

      int c = accept(s, 0, 0);
      if (c == -1){
        /* See Stevens W.R. - "Unix Network Programming", chapter 15.6 */
        #ifdef UNIX
          if (errno != EWOULDBLOCK && errno != ECONNABORTED && errno != EPROTO && errno != EINTR)
        #endif
        #ifdef WIN32
          if (WSAGetLastError() != WSAECONNRESET)
        #endif
          {
            SOCKETS_PERROR("ERROR: accept");
            return false;
          }
      }

        /*
         * NONBLOCK option is not always inherited.
         * E.g., it is not inherited in Linux.
         * We want this socket to be non-blocking.
         */
      verify_ne(setnonblocking(c), -1);

      client_factory.new_client(c);
    }

    SelectClientFactory::iterator iter = client_factory.begin();
    const SelectClientFactory::iterator end = client_factory.end();
    while (num_ready > 0){
      assert(iter != end);
      int c = iter->s;

      bool isrset = !!FD_ISSET(c, &rset);
      bool iswset = !!FD_ISSET(c, &wset);

        /*
         * Decrease num_ready here in order not to code it each time we finish
         * processing the socket prematurely. Note that one socket can reside
         * in rset and wset simultaneously.
         */
      if (isrset)
        --num_ready;
      if (iswset)
        --num_ready;

      if (isrset) {
        int count = recv(c, &*(iter->buffer.rd_begin()), iter->buffer.rd_size(), 0);
        if (count == -1){
          SOCKETS_PERROR("WARNING: recv");
          iter = client_factory.delete_client(iter);
          continue;
        }
        else if (count == 0){
          cerr << "TRACE: Client stopped sending data.\n";

          if (iter->buffer.snd_size() == 0){
            cerr << "TRACE: Nothing to send to client. Close connection to it.\n";
            iter = client_factory.delete_client(iter);
            continue;
          }
          else{
            /* Do not close socket here as there are something to send. */
            client_factory.rd_disable(iter);
          }
        }
        else
          client_factory.rd_advance(iter, count);
      }

      if (iswset) {
        int count = send(c, &*(iter->buffer.snd_begin()), iter->buffer.snd_size(), 0);
        if (count == -1){
          SOCKETS_PERROR("WARNING: send");
          iter = client_factory.delete_client(iter);
          continue;
        }
        else{
          client_factory.snd_advance(iter, count);
          if (iter->rd_disabled() && iter->buffer.empty()){
            cerr << "TRACE: Nothing to send to client. Close connection to it.\n";
            iter = client_factory.delete_client(iter);
            continue;
          }
        }
      }

      ++iter;
    }
  }
}

/* ------------------------------------------------------------------------- */
#ifdef HAVE_LIBEV

bool libev_handler(GeneralOptentries & optentries_, int s){
  LibevOptentries & optentries = static_cast<LibevOptentries &>(optentries_);

  if (listen(s, 5) == -1){
    SOCKETS_PERROR("ERROR: listen");
    return false;
  }

  struct ev_loop * loop = ev_default_loop(0);
  if (!loop){
    cerr << "ERROR: ev_default_loop\n";
    return false;
  }

  LibevClientFactory client_factory(
    s, optentries.buffer_size(), optentries.limit(),
    LibevClientFactory::storage_mixin_t(),
    LibevClientFactory::network_mixin_t(loop));

  ev_loop(loop, 0);

  return false;
}

#endif // HAVE_LIBEV
/* ------------------------------------------------------------------------- */

bool invoke_handler(Handler handler, GeneralOptentries & optentries){

  #ifdef WIN32
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata)){
      cerr << "ERROR: Could not initialize socket library.\n";
      return false;
    }
  #endif

  int s = socket(PF_INET6, optentries.socket_type(), 0);
  if (s == -1){
    s = socket(PF_INET, optentries.socket_type(), 0);
    if (s == -1){
      SOCKETS_PERROR("ERROR: socket");
      goto run_return;
    }
    else{
      cerr << "TRACE: IPv6 is unavailable. Using IPv4.\n";

      sockaddr_in addr = {0};
      addr.sin_family = AF_INET;
      addr.sin_port = htons(optentries.port());
      addr.sin_addr.s_addr = INADDR_ANY;
      if (bind(s, (sockaddr *)&addr, sizeof(addr)) == -1){
        SOCKETS_PERROR("ERROR: bind");
        goto run_return;
      }
    }
  }
  else{
    cerr << "TRACE: Using IPv6.\n";

    sockaddr_in6 addr = {0};
    addr.sin6_family = AF_INET6;
    addr.sin6_flowinfo = 0;
    addr.sin6_port = htons(optentries.port());
    addr.sin6_addr = in6addr_any;
    if (bind(s, (sockaddr *)&addr, sizeof(addr)) == -1){
      SOCKETS_PERROR("ERROR: bind");
      goto run_return;
    }
  }

  handler(optentries, s);

run_return:
  if (s != -1)
    verify_ne(close(s), -1);
  return false;
}

} // namespace

int main(int argc, char ** argv){

  string tcp_description =
    "Simple TCP server. WARNING: Server is vulnerable to DoS attack if -l option\n"
    "  is specified. I did not consider timeouts useful in a test program.";

  cmdfw::mode::Modes modes;
  modes.push_back(new Mode<UdpOptentries>("udp", "UDP server.", udp_handler));
  modes.push_back(new Mode<TcpOptentries>("tcp", tcp_description, tcp_handler));
  modes.push_back(new Mode<SelectOptentries>("select",
    "TCP server written using select() function.", select_handler));
  #ifdef HAVE_LIBEV
    modes.push_back(new Mode<LibevOptentries>("libev",
      "TCP server written using libev library.", &libev_handler));
  #endif
  return cmdfw::framework::run("server", argc, argv, cmdfw::factory::Factory(modes)) ? 0 : 1;
}
