#include "../common/common.h"
#include "../common/framework.h"
using namespace std;

#ifdef UNIX
  #include <fcntl.h>
	#include <getopt.h>
	#include <semaphore.h>
	#include <signal.h>
	#include <sys/wait.h>
#endif

const int CLIENTS_SIZE = FD_SETSIZE;

namespace{

struct Settings:
  public framework::settings::Base,
  public framework::settings::mixins::AddressMixin,
  public framework::settings::mixins::PortNumberMixin,
  public framework::settings::mixins::BufferSizeMixin {

    /* We could also derive from framework::settings::mixins::SocketTypeMixin. */
  int socket_type;
  Settings(): socket_type(SOCK_STREAM) {}

  /* override */ virtual std::string options() const{
    return
      framework::settings::mixins::AddressMixin::option() +
      framework::settings::mixins::PortNumberMixin::option() +
      framework::settings::mixins::BufferSizeMixin::option();
  }

  /* override */ virtual std::string options_help() const{
    return string() +
      "-" + framework::settings::mixins::AddressMixin::letter()            + "\n"
        "  " + framework::settings::mixins::AddressMixin::description()    + "\n"
      "-" + framework::settings::mixins::PortNumberMixin::letter()         + "\n"
        "  " + framework::settings::mixins::PortNumberMixin::description() + "\n"
      "-" + framework::settings::mixins::BufferSizeMixin::letter()         + "\n"
        "  " + framework::settings::mixins::BufferSizeMixin::description() + "\n";
  }

  /* override */ virtual int handle(char letter, char * optarg){
    int result;
    if (letter == framework::settings::mixins::AddressMixin::letter())
      result = framework::settings::mixins::AddressMixin::handle(optarg);
    else if (letter == framework::settings::mixins::PortNumberMixin::letter())
      result = framework::settings::mixins::PortNumberMixin::handle(optarg);
    else if (letter == framework::settings::mixins::BufferSizeMixin::letter())
      result = framework::settings::mixins::BufferSizeMixin::handle(optarg);
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
    if (!framework::settings::mixins::AddressMixin::is_set()){
      success = false;
      cerr << "ERROR: Option '-" << framework::settings::mixins::AddressMixin::letter() << "' is required.\n";
    }
    if (!framework::settings::mixins::PortNumberMixin::is_set()){
      success = false;
      cerr << "ERROR: Option '-" << framework::settings::mixins::PortNumberMixin::letter() << "' is required.\n";
    }
    if (!framework::settings::mixins::BufferSizeMixin::is_set()){
      success = false;
      cerr << "ERROR: Option '-" << framework::settings::mixins::BufferSizeMixin::letter() << "' is required.\n";
    }
    return success;
  }
};

struct UdpSettings: public Settings{
  UdpSettings() { socket_type = SOCK_DGRAM; }
};

struct LimitMixin: public framework::settings::mixins::Base {
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
      cerr << "ERROR: Limit is not correct\n";
      return 1;
    }
    limit = t;
    return 0;
  }
};

struct TcpSettings:
  public Settings,
  public LimitMixin{

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
    }
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

typedef Settings UdpSetting, SelectSettings;


typedef vector<__int8_t> Buffer;
typedef bool (*Handler)(Settings * settings, int s);
typedef framework::Framework<Handler> ThisFramework;


bool udp_handler(Settings * settings, int s){
  Buffer buffer(settings->buffer_size);
  __int8_t * buf = &buffer[0];

  for (;;){
      /* sockaddr_storage matches the size of the largest struct that can be returned
       *
       */
    sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);

    int count = recvfrom(s, buf, settings->buffer_size, 0,
      reinterpret_cast<sockaddr *>(&addr), &addr_len);
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
      int sent_current = sendto(s, buf + sent_total, count - sent_total, 0,
        reinterpret_cast<sockaddr *>(&addr), addr_len);
      if (sent_current == -1){
        cerr << "WARNING: Encountered an error while sending data\n";
        break;
      }
      sent_total += sent_current;
    }
  }
  return true;
}

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
  __int8_t * buf = &buffer[0];

  for (;;){
    int count = recv(c, buf, buffer_size, 0);
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

bool tcp_handler(Settings * settings_, int s){
  TcpSettings * settings = static_cast<TcpSettings *>(settings_);

  #ifdef UNIX
    limit = settings->limit;
    if (signal(SIGCHLD, sig_chld) == SIG_ERR){
      cerr << "ERROR: Could not bind SIGCHLD handler\n";
      return false;
    }
  #endif

  #ifdef UNIX
    if (settings->limit != 0){
      if (sem_init(&sem, 0 /* not shared */, settings->limit) == -1){
        cerr << "ERROR: Could not apply limit\n";
        return false;
      }
    }
  #endif
  #ifdef WIN32
    HANDLE sem = 0;
    if (settings->limit != 0){
      sem = CreateSemaphore(0, settings->limit, settings->limit, 0);
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

    if (settings->limit != 0){
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
        tcp_subprocess(c, settings->buffer_size);
        exit(0);
      }
    #endif
    #ifdef WIN32
      tcp_subprocess_data * data = new tcp_subprocess_data;
      data->sem = sem;
      data->c = c;
      data->buffer_size = settings->buffer_size;
      CreateThread(0, 0, tcp_subprocess, data, 0, 0);
    #endif
  }

tcp_return:
  if (settings->limit != 0){
    #ifdef UNIX
      verify_ne(sem_destroy(&sem), -1);
    #endif
    #ifdef WIN32
      verify(CloseHandle(sem));
    #endif
  }
  return false;
}

bool select_handler(Settings * settings, int s){
  if (listen(s, 5) == -1){
    cerr << "ERROR: Could not start listening\n";
    return false;
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

  Buffer buffer(settings->buffer_size);
  __int8_t * buf = &buffer[0];

  int clients[CLIENTS_SIZE];
  for (int i = 0; i < CLIENTS_SIZE; ++i)
    clients[i] = -1;

  if (!try_setsockopt_sndlowat(s, settings->buffer_size))
    return false;

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
              return false;
            }
          #endif
          #ifdef WIN32
            if (WSAGetLastError() != WSAECONNRESET){
              cerr << "ERROR: Could not accept a connection\n";
              return false;
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

        int count = recv(c, buf, settings->buffer_size, 0);
        if (count == -1){
          SOCKETS_PERROR("WARNING: Encountered an error while reading data");
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
              SOCKETS_PERROR("WARNING: Encountered an error while sending data");
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

bool invoke_handler(Handler handler, framework::settings::Base * settings_){
  Settings * settings = static_cast<Settings *>(settings_);

  #ifdef WIN32
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata)){
      cerr << "ERROR: Could not initialize socket library\n";
      return false;
    }
  #endif

  int s = socket(PF_INET6, settings->socket_type, 0);
  if (s == -1){
    s = socket(PF_INET, settings->socket_type, 0);
    if (s == -1){
      cerr << "ERROR: Could not create socket\n";
      goto run_return;
    }
    else{
      cerr << "TRACE: IPv6 is unavailable. Using IPv4.\n";

      sockaddr_in addr = {0};
      addr.sin_family = AF_INET;
      addr.sin_port = htons(settings->port);
      addr.sin_addr.s_addr = INADDR_ANY;
      if (bind(s, (sockaddr *)&addr, sizeof(addr)) == -1){
        cerr << "ERROR: Could not bind socket\n";
        goto run_return;
      }
    }
  }
  else{
    cerr << "TRACE: Using IPv6.\n";

    sockaddr_in6 addr = {0};
    addr.sin6_family = AF_INET6;
    addr.sin6_flowinfo = 0;
    addr.sin6_port = htons(settings->port);
    addr.sin6_addr = in6addr_any;
    if (bind(s, (sockaddr *)&addr, sizeof(addr)) == -1){
      cerr << "ERROR: Could not bind socket\n";
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
  UdpSettings udp_settings;
  TcpSettings tcp_settings;
  SelectSettings select_settings;

  string tcp_description =
    "Simple TCP server. WARNING: Server is vulnerable to DoS attack if -l option\n"
    "  is specified. I did not consider timeouts useful in a test program.";

  ThisFramework::Modes modes;
  modes.push_back(ThisFramework::ModeDescription(
    "udp", "UDP server.", &udp_settings, &udp_handler));
  modes.push_back(ThisFramework::ModeDescription(
    "tcp", tcp_description, &tcp_settings, &tcp_handler));
  modes.push_back(ThisFramework::ModeDescription(
    "select", "A TCP server written using select() function.", &select_settings, &select_handler));
  return ThisFramework::run("server", argc, argv, modes, invoke_handler) ? 0 : 1;
}
