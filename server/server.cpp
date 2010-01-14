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

class Settings:
  public cmdfw::settings::Base,
  public cmdfw::settings::mixins::PortNumberMixin,
  public cmdfw::settings::mixins::BufferSizeMixin {
public:

    /* We could also derive from cmdfw::settings::mixins::SocketTypeMixin. */
  int socket_type;
  Settings(): socket_type(SOCK_STREAM) {}

  /* override */ virtual std::string options() const{
    return
      cmdfw::settings::mixins::PortNumberMixin::option() +
      cmdfw::settings::mixins::BufferSizeMixin::option();
  }

  /* override */ virtual std::string options_help() const{
    return string() +
      "-" + cmdfw::settings::mixins::PortNumberMixin::letter()         + "\n"
        "  " + cmdfw::settings::mixins::PortNumberMixin::description() + "\n"
      "-" + cmdfw::settings::mixins::BufferSizeMixin::letter()         + "\n"
        "  " + cmdfw::settings::mixins::BufferSizeMixin::description() + "\n";
  }

  /* override */ virtual int handle(char letter, char * optarg){
    int result;
    if (letter == cmdfw::settings::mixins::PortNumberMixin::letter())
      result = cmdfw::settings::mixins::PortNumberMixin::handle(optarg);
    else if (letter == cmdfw::settings::mixins::BufferSizeMixin::letter())
      result = cmdfw::settings::mixins::BufferSizeMixin::handle(optarg);
    else
      return 1;

    switch(result){
      case 0:
        return 0;
      case 1:
        return 2;
      case 2:
        return 3;
      default:
        assert(false);
        return 3;
    }
  }

  /* override */ virtual bool validate(){
    bool success = true;
    if (!cmdfw::settings::mixins::PortNumberMixin::is_set()){
      success = false;
      cerr << "ERROR: Option '-" << cmdfw::settings::mixins::PortNumberMixin::letter() << "' is required.\n";
    }
    if (!cmdfw::settings::mixins::BufferSizeMixin::is_set()){
      success = false;
      cerr << "ERROR: Option '-" << cmdfw::settings::mixins::BufferSizeMixin::letter() << "' is required.\n";
    }
    return success;
  }
};

class UdpSettings: public Settings{
public:
  UdpSettings() { socket_type = SOCK_DGRAM; }
};

struct LimitMixin: public cmdfw::settings::mixins::Base {
  char letter() const { return 'l'; }
  std::string option() const { return "l:"; }
  std::string description() const { return
    "Maximum number of simultaneous connections, must be a positive integer value.\n"
    "  Default: number of connections is unlimited."; }

  /*
   * "0" means "unlimited".
   */
  int limit;
  LimitMixin(): limit(0) {}

  int handle(char * optarg){
    int t;
    if (!dirty_lexical_cast::lexical_cast_ref(optarg, t) || t <= 0){
      cerr << "ERROR: Limit is not correct.\n";
      return 1;
    }
    limit = t;
    return 0;
  }
};

class LimitSettings:
  public Settings,
  public LimitMixin{
public:

  /* override */ virtual std::string options() const{
    return Settings::options() +
      LimitMixin::option();
  }

  /* override */ virtual std::string options_help() const{
    return Settings::options_help() +
      "-" + LimitMixin::letter()         + "\n"
        "  " + LimitMixin::description() + "\n";
  }

  /* override */ virtual int handle(char letter, char * optarg){
    int result = Settings::handle(letter, optarg);
    if (result == 1){
      if (letter == LimitMixin::letter())
        result = LimitMixin::handle(optarg);
      else
        return result;

      switch(result){
        case 0:
          return 0;
        case 1:
          return 2;
        case 2:
          return 3;
        default:
          assert(false);
          return 3;
      }
    }
    else
      return result;
  }

  /* override */ virtual bool validate(){
    bool success = true;
    if (!LimitMixin::is_set()){
      success = false;
      cerr << "ERROR: Option '-" << LimitMixin::letter() << "' is required.\n";
    }
      /*
       * Careful, Settings::validate() must fire even if success is already false.
       */
    success = Settings::validate() && success;
    return success;
  }
};

typedef Settings SelectSettings;
typedef LimitSettings TcpSettings, LibevSettings;


typedef vector<int8_t> Buffer;
typedef bool (*Handler)(Settings & settings, int s);

bool invoke_handler(Handler handler, Settings & settings);

bool udp_handler(Settings & settings, int s);
bool tcp_handler(Settings & settings_, int s);
bool select_handler(Settings & settings_, int s);
#ifdef HAVE_LIBEV
  bool libev_handler(Settings & settings_, int s);
#endif


template <typename TSettings> class ModeBase: public cmdfw::mode::Mode {
public:

  ModeBase(const string & name, const string & description, Handler handler):
    cmdfw::mode::Mode(name, description), handler_(handler) { }
  ModeBase(const vector<string> & names, const string & description, Handler handler):
    cmdfw::mode::Mode(names, description), handler_(handler) { }

  /* override */ virtual cmdfw::settings::Base * settings() const {
    return new TSettings();
  }

  /* override */ virtual bool handle(cmdfw::settings::Base & settings) const {
    return invoke_handler(handler_, static_cast<Settings &>(settings));
  }

private:
  Handler handler_;
};

/* ------------------------------------------------------------------------- */

bool udp_handler(Settings & settings, int s){
  Buffer buffer(settings.buffer_size);
  int8_t * buf = &buffer[0];

  for (;;){
      /*
       * sockaddr_storage matches the size of the largest struct that can be returned
       */
    sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);

    int count = recvfrom(s, buf, settings.buffer_size, 0,
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

bool tcp_handler(Settings & settings_, int s){
  TcpSettings & settings = static_cast<TcpSettings &>(settings_);

  #ifdef UNIX
    limit = settings.limit;
    if (signal(SIGCHLD, sig_chld) == SIG_ERR){
      cerr << "ERROR: Could not bind SIGCHLD handler.\n";
      return false;
    }
  #endif

  #ifdef UNIX
    if (settings.limit != 0){
      if (sem_init(&sem, 0 /* not shared */, settings.limit) == -1){
        cerr << "ERROR: Could not apply limit.\n";
        return false;
      }
    }
  #endif
  #ifdef WIN32
    HANDLE sem = 0;
    if (settings.limit != 0){
      sem = CreateSemaphore(0, settings.limit, settings.limit, 0);
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

    if (settings.limit != 0){
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
        tcp_subprocess(c, settings.buffer_size);
        exit(0);
      }
    #endif
    #ifdef WIN32
      tcp_subprocess_data * data = new tcp_subprocess_data;
      data->sem = sem;
      data->c = c;
      data->buffer_size = settings.buffer_size;
      CloseHandle(CreateThread(0, 0, tcp_subprocess, data, 0, 0));
    #endif
  }

tcp_return:
  if (settings.limit != 0){
    #ifdef UNIX
      verify_ne(sem_destroy(&sem), -1);
    #endif
    #ifdef WIN32
      verify(CloseHandle(sem));
    #endif
  }
  return false;
}

bool select_handler(Settings & settings, int s){
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

  SelectClientFactory client_factory(settings.buffer_size,
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

bool libev_handler(Settings & settings_, int s){
  LibevSettings & settings = static_cast<LibevSettings &>(settings_);

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
    s, settings.buffer_size, settings.limit,
    LibevClientFactory::storage_mixin_t(),
    LibevClientFactory::network_mixin_t(loop));

  ev_loop(loop, 0);

  return false;
}

#endif // HAVE_LIBEV
/* ------------------------------------------------------------------------- */

bool invoke_handler(Handler handler, Settings & settings){

  #ifdef WIN32
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata)){
      cerr << "ERROR: Could not initialize socket library.\n";
      return false;
    }
  #endif

  int s = socket(PF_INET6, settings.socket_type, 0);
  if (s == -1){
    s = socket(PF_INET, settings.socket_type, 0);
    if (s == -1){
      SOCKETS_PERROR("ERROR: socket");
      goto run_return;
    }
    else{
      cerr << "TRACE: IPv6 is unavailable. Using IPv4.\n";

      sockaddr_in addr = {0};
      addr.sin_family = AF_INET;
      addr.sin_port = htons(settings.port);
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
    addr.sin6_port = htons(settings.port);
    addr.sin6_addr = in6addr_any;
    if (bind(s, (sockaddr *)&addr, sizeof(addr)) == -1){
      SOCKETS_PERROR("ERROR: bind");
      goto run_return;
    }
  }

  handler(settings, s);

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
  modes.push_back(new ModeBase<UdpSettings>("udp", "UDP server.", udp_handler));
  modes.push_back(new ModeBase<TcpSettings>("tcp", tcp_description, tcp_handler));
  modes.push_back(new ModeBase<SelectSettings>("select",
    "TCP server written using select() function.", select_handler));
  #ifdef HAVE_LIBEV
    modes.push_back(new ModeBase<LibevSettings>("libev",
      "TCP server written using libev library.", &libev_handler));
  #endif
  return cmdfw::framework::run("server", argc, argv, cmdfw::factory::Factory(modes)) ? 0 : 1;
}
