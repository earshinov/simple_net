#include "../common/common.h"
#include "../common/framework.h"
#include "../common/logger.h"

using namespace std;
using namespace logger;

#ifdef UNIX
  #include <sys/wait.h>
#endif

namespace{

struct Settings:
  public framework::settings::Base,
  public framework::settings::mixins::SocketTypeMixin,
  public framework::settings::mixins::AddressMixin,
  public framework::settings::mixins::PortMixin,
  public framework::settings::mixins::BufferSizeMixin,
  public framework::settings::mixins::VerbosityMixin,
  public framework::settings::mixins::QuietMixin{

  /*
   * NOTE: SocketTypeMixin is not mentioned in methods as we do not want this
   * option to be available in all modes (to be precise, in "select" mode).
   */

  /* override */ virtual std::string options() const{
    return
      framework::settings::mixins::AddressMixin::option() +
      framework::settings::mixins::PortMixin::option() +
      framework::settings::mixins::BufferSizeMixin::option() +
      framework::settings::mixins::VerbosityMixin::option() +
      framework::settings::mixins::QuietMixin::option();
  }

  /* override */ virtual std::string options_help() const{
    return string() +
      "-" + framework::settings::mixins::AddressMixin::letter()            + "\n"
        "  " + framework::settings::mixins::AddressMixin::description()    + "\n"
      "-" + framework::settings::mixins::PortMixin::letter()               + "\n"
        "  " + framework::settings::mixins::PortMixin::description()       + "\n"
      "-" + framework::settings::mixins::BufferSizeMixin::letter()         + "\n"
        "  " + framework::settings::mixins::BufferSizeMixin::description() + "\n"
      "-" + framework::settings::mixins::VerbosityMixin::letter()          + "\n"
        "  " + framework::settings::mixins::VerbosityMixin::description()  + "\n"
      "-" + framework::settings::mixins::QuietMixin::letter()              + "\n"
        "  " + framework::settings::mixins::QuietMixin::description()      + "\n";
  }

  /* override */ virtual int handle(char letter, char * optarg){
    int result;
    if (letter == framework::settings::mixins::AddressMixin::letter())
      result = framework::settings::mixins::AddressMixin::handle(optarg);
    else if (letter == framework::settings::mixins::PortMixin::letter())
      result = framework::settings::mixins::PortMixin::handle(optarg);
    else if (letter == framework::settings::mixins::BufferSizeMixin::letter())
      result = framework::settings::mixins::BufferSizeMixin::handle(optarg);
    else if (letter == framework::settings::mixins::VerbosityMixin::letter())
      result = framework::settings::mixins::VerbosityMixin::handle(optarg);
    else if (letter == framework::settings::mixins::QuietMixin::letter())
      result = framework::settings::mixins::QuietMixin::handle(optarg);
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
    if (!framework::settings::mixins::PortMixin::is_set()){
      success = false;
      cerr << "ERROR: Option '-" << framework::settings::mixins::PortMixin::letter() << "' is required.\n";
    }
    if (!framework::settings::mixins::BufferSizeMixin::is_set()){
      success = false;
      cerr << "ERROR: Option '-" << framework::settings::mixins::BufferSizeMixin::letter() << "' is required.\n";
    }
    if (!framework::settings::mixins::VerbosityMixin::is_set()){
      success = false;
      cerr << "ERROR: Option '-" << framework::settings::mixins::VerbosityMixin::letter() << "' is required.\n";
    }
    return success;
  }
};

struct CxxIoSettings:
  public Settings,
  public framework::settings::mixins::CxxIoInputMixin,
  public framework::settings::mixins::CxxIoOutputMixin {

  /* override */ virtual std::string options() const{
    return Settings::options()
      + framework::settings::mixins::CxxIoInputMixin::option()
      + framework::settings::mixins::CxxIoOutputMixin::option();
  }

  /* override */ virtual std::string options_help() const{
    return string() +
      "-" + framework::settings::mixins::CxxIoInputMixin::letter()          + "\n"
        "  " + framework::settings::mixins::CxxIoInputMixin::description()  + "\n"
      "-" + framework::settings::mixins::CxxIoOutputMixin::letter()         + "\n"
        "  " + framework::settings::mixins::CxxIoOutputMixin::description() + "\n"
      + Settings::options_help();
  }

  /* override */ virtual int handle(char letter, char * optarg){
    int result = Settings::handle(letter, optarg);
    if (result == 1){
      if (letter == framework::settings::mixins::CxxIoInputMixin::letter())
        result = framework::settings::mixins::CxxIoInputMixin::handle(optarg);
      else if (letter == framework::settings::mixins::CxxIoOutputMixin::letter())
        result = framework::settings::mixins::CxxIoOutputMixin::handle(optarg);
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
    if (!framework::settings::mixins::CxxIoInputMixin::is_set()){
      success = false;
      cerr << "ERROR: Option '-" << framework::settings::mixins::CxxIoInputMixin::letter() << "' is required.\n";
    }
    if (!framework::settings::mixins::CxxIoOutputMixin::is_set()){
      success = false;
      cerr << "ERROR: Option '-" << framework::settings::mixins::CxxIoOutputMixin::letter() << "' is required.\n";
    }
      /*
       * Careful, Settings::validate() must fire even if success is already false.
       */
    success = Settings::validate() && success;
    return success;
  }
};

struct CIoSettings:
  public Settings,
  public framework::settings::mixins::CIoInputMixin,
  public framework::settings::mixins::CIoOutputMixin {

  /* override */ virtual std::string options() const{
    return Settings::options()
      + framework::settings::mixins::CIoInputMixin::option()
      + framework::settings::mixins::CIoOutputMixin::option();
  }

  /* override */ virtual std::string options_help() const{
    return string() +
      "-" + framework::settings::mixins::CIoInputMixin::letter()          + "\n"
        "  " + framework::settings::mixins::CIoInputMixin::description()  + "\n"
      "-" + framework::settings::mixins::CIoOutputMixin::letter()         + "\n"
        "  " + framework::settings::mixins::CIoOutputMixin::description() + "\n"
      + Settings::options_help();
  }

  /* override */ virtual int handle(char letter, char * optarg){
    int result = Settings::handle(letter, optarg);
    if (result == 1){
      if (letter == framework::settings::mixins::CIoInputMixin::letter())
        result = framework::settings::mixins::CIoInputMixin::handle(optarg);
      else if (letter == framework::settings::mixins::CIoOutputMixin::letter())
        result = framework::settings::mixins::CIoOutputMixin::handle(optarg);
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
    if (!framework::settings::mixins::CIoInputMixin::is_set()){
      success = false;
      cerr << "ERROR: Option '-" << framework::settings::mixins::CIoInputMixin::letter() << "' is required.\n";
    }
    if (!framework::settings::mixins::CIoOutputMixin::is_set()){
      success = false;
      cerr << "ERROR: Option '-" << framework::settings::mixins::CIoOutputMixin::letter() << "' is required.\n";
    }
      /*
       * Careful, Settings::validate() must fire even if success is already false.
       */
    success = Settings::validate() && success;
    return success;
  }
};

typedef CxxIoSettings SelectWindowsSettings;
typedef CIoSettings SelectUnixSettings;

struct MultiprotoSettings: public CxxIoSettings{

  /* override */ virtual std::string options() const{
    return CxxIoSettings::options() +
      framework::settings::mixins::SocketTypeMixin::option();
  }

  /* override */ virtual std::string options_help() const{
    return string() +
      "-" + framework::settings::mixins::SocketTypeMixin::letter()         + "\n"
        "  " + framework::settings::mixins::SocketTypeMixin::description() + "\n"
      + CxxIoSettings::options_help();
  }

  /* override */ virtual int handle(char letter, char * optarg){
    int result = CxxIoSettings::handle(letter, optarg);
    if (result == 1){
      if (letter == framework::settings::mixins::SocketTypeMixin::letter())
        result = framework::settings::mixins::SocketTypeMixin::handle(optarg);
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
    if (!framework::settings::mixins::SocketTypeMixin::is_set()){
      success = false;
      cerr << "ERROR: Option '-" << framework::settings::mixins::SocketTypeMixin::letter() << "' is required.\n";
    }
      /*
       * Careful, Settings::validate() must fire even if success is already false.
       */
    success = CxxIoSettings::validate() && success;
    return success;
  }
};

typedef MultiprotoSettings SimpleSettings, MtSettings;


typedef vector<__int8_t> Buffer;
typedef bool (*Handler)(Settings * settings, int s);
typedef framework::Framework<Handler> ThisFramework;


Logger logger;


bool invoke_handler(Handler handler, framework::settings::Base * settings_){
  Settings * settings = static_cast<Settings *>(settings_);
  logger.setLevel(
    (settings->verbosity > 0) ? TRACE :
      (settings->quiet ? WARN : INFO));

  bool ret = false;

  #ifdef WIN32
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata)){
      logger.writer(ERROR_) << "Could not initialize socket library.\n";
      return false;
    }
  #endif

  addrinfo * res = 0;
  addrinfo * p = 0;
  int s = -1; // Our socket

  addrinfo hints = {0};
  hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
  hints.ai_socktype = settings->socket_type;
  if (getaddrinfo(
        settings->addr.c_str(),
        settings->port.c_str(),
        &hints, &res) != 0){
    logger.writer(ERROR_) << "Could not resolve IP address and/or port.\n";
    goto run_return;
  }

  for(p = res; p != NULL; p = p->ai_next){
    s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (s == -1)
      continue;
    if (connect(s, p->ai_addr, p->ai_addrlen) == -1){
      verify_ne(close(s), -1);
      s = -1;
      continue;
    }
    break;
  }
  if (s == -1){
    logger.writer(ERROR_) << "Could not connect to server.\n";
    goto run_return;
  }

  ret = handler(settings, s);
run_return:
  if (res != 0)
    freeaddrinfo(res);
  if (s != -1)
    verify_ne(close(s), -1);
  return ret;
}

bool simple_handler(Settings * settings_, int s){
  SimpleSettings * settings = static_cast<SimpleSettings *>(settings_);

  istream & input = settings->get_input();
  ostream & output = settings->get_output();

  Buffer buffer(settings->buffer_size);
  __int8_t * const buf = &buffer[0];

  while (!input.eof()){

    input.read(reinterpret_cast<char *>(buf), settings->buffer_size / sizeof(char));
    int count = input.gcount() * sizeof(char);
    if (count == 0)
      continue;
    logger.writer(TRACE) << count << " bytes to send.\n";

    int sent_total = 0;
    while (sent_total < count){
      int sent_current = send(s, buf + sent_total, count - sent_total, 0);
      if (sent_current == -1){
        SOCKETS_PERROR("ERROR: send");
        return false;
      }
      logger.writer(TRACE) << sent_current << " bytes sent.\n";
      sent_total += sent_current;
    }
    logger.writer(TRACE) << "Start receiving.\n";

    int recv_total = 0;
    while (recv_total < count){
      int recv_current = recv(s, buf + recv_total, settings->buffer_size - recv_total, 0);
      if (recv_current == -1){
        SOCKETS_PERROR("ERROR: recv");
        return false;
      }
      if (recv_current == 0){
        logger.writer(ERROR_) << "Server shutdowned unexpectedly.\n";
        return false;
      }
      logger.writer(TRACE) << "Received " << recv_current << " bytes.\n";
      recv_total += recv_current;
    }
    output.write(reinterpret_cast<char *>(buf), recv_total);
  }
  return true;
}

bool select_windows_handler(Settings * settings_, int s){
  SelectWindowsSettings * settings = static_cast<SelectWindowsSettings *>(settings_);

  istream & input = settings->get_input();
  ostream & output = settings->get_output();

  Buffer buffer(settings->buffer_size);
  __int8_t * const buf = &buffer[0];

  bool stdin_eof = false;

  fd_set rset, wset;
  FD_ZERO(&rset);
  FD_ZERO(&wset);

  for (;;){
    FD_SET(s, &rset);
    if (!stdin_eof)
      FD_SET(s, &wset);

    int num_ready = select(s + 1, &rset, &wset, 0, 0);
    if (num_ready == -1){
      logger.writer(ERROR_) << "select() reported an error.\n";
      return false;
    }

    if (FD_ISSET(s, &rset)){
      int received = recv(s, buf, settings->buffer_size, 0);
      if (received == -1){
        SOCKETS_PERROR("ERROR: recv");
        return false;
      }
      else if (received == 0){
        if (stdin_eof){
          logger.writer(INFO) << "Server closed the connection.\n";
          return true;
        }
        else{
          logger.writer(ERROR_) << "Server closed the connection unexpectedly.\n";
          return false;
        }
      }
      else {
        logger.writer(TRACE) << received << " bytes to print.\n";
        output.write(reinterpret_cast<char *>(buf), received);
        logger.writer(TRACE) << "Bytes printed.\n";
      }
    }

    if (FD_ISSET(s, &wset)){
      input.read(reinterpret_cast<char *>(buf), settings->buffer_size / sizeof(char));
      int count = input.gcount() * sizeof(char);
      if (count == 0){
        logger.writer(INFO) << "EOF at input.\n";
        stdin_eof = true;
        verify_ne(shutdown(s, SHUT_WR), -1);
        FD_CLR(s, &wset);
      }
      else{
        logger.writer(TRACE) << count << " bytes to send.\n";

        int sent = send(s, buf, count, 0);
        if (sent == -1){
          SOCKETS_PERROR("ERROR: send");
          return false;
        }
        else{
          assert(sent == count);
          logger.writer(TRACE) << sent << " bytes sent.\n";
        }
      }
    }
  }
}

#ifdef UNIX
  /*
   * In Unix we can pass to select() file descriptors.
   */
bool select_unix_handler(Settings * settings_, int s){
  SelectUnixSettings * settings = static_cast<SelectUnixSettings *>(settings_);

  int input = fileno(settings->get_input());
  int output = fileno(settings->get_output());

  ReadWriteBuffer to(settings->buffer_size);
  ReadWriteBuffer from(settings->buffer_size);

  enum{
    STATE_NORMAL,
    STATE_NO_INPUT,
    STATE_ALL_SENT,
  } state = STATE_NORMAL;

  for (;;){

    fd_set rset, wset;
    FD_ZERO(&rset);
    FD_ZERO(&wset);
    int maxfd = 0;

    if (!to.rd_empty()){
      FD_SET(s, &rset);
      maxfd = max(maxfd, s);
    }
    if (!to.snd_empty()){
      FD_SET(output, &wset);
      maxfd = max(maxfd, output);
    }

    if (state < STATE_NO_INPUT && !from.rd_empty()){
      FD_SET(input, &rset);
      maxfd = max(maxfd, input);
    }
    if (state < STATE_ALL_SENT && !from.snd_empty()){
      FD_SET(s, &wset);
      maxfd = max(maxfd, s);
    }

    int num_ready = select(maxfd + 1, &rset, &wset, 0, 0);
    if (num_ready == -1){
      logger.writer(ERROR_) << "select() reported an error.\n";
      return false;
    }

      /*
       * Check this BEFORE FD_ISSET(s, &rset) in order not to return too early.
       */
    if (FD_ISSET(output, &wset)){
      int count = write(output, &*(to.snd_begin()), to.snd_size());
      if (count == -1){
        logger.writer(ERROR_) << "An error occured while writing to output.\n";
        return false;
      }
      else
        to.snd_advance(count);
    }

    if (FD_ISSET(s, &rset)){
      int count = recv(s, &*(to.rd_begin()), to.rd_size(), 0);
      if (count == -1){
        SOCKETS_PERROR("ERROR: Error when receiving");
        return false;
      }
      else if (count == 0){
        if (state == STATE_ALL_SENT){
          logger.writer(INFO) << "Server closed the connection.\n";
          return true;
        }
        else{
          logger.writer(ERROR_) << "Server closed the connection unexpectedly.\n";
          return false;
        }
      }
      else
        to.rd_advance(count);
    }

    if (FD_ISSET(input, &rset)){
      int count = read(input, &*(from.rd_begin()), from.rd_size());
      if (count == -1){
        logger.writer(ERROR_) << "An error occured while reading from input.\n";
        return false;
      }
      else if (count == 0){
        logger.writer(INFO) << "EOF at standard input.\n";
        state = STATE_NO_INPUT;

        if (from.snd_empty()){
          verify_ne(shutdown(s, SHUT_WR), -1);
          state = STATE_ALL_SENT;
        }
      }
      else
        from.rd_advance(count);
    }

    if (FD_ISSET(s, &wset)){
      int count = send(s, &*(from.snd_begin()), from.snd_size(), 0 /* flags */);
      if (count == -1){
        SOCKETS_PERROR("ERROR: send");
        return false;
      }
      else{
        from.snd_advance(count);
        if (state == STATE_NO_INPUT && from.snd_empty()){
          verify_ne(shutdown(s, SHUT_WR), -1);
          state = STATE_ALL_SENT;
        }
      }
    }
  }
}
#endif

#ifdef UNIX
bool mt_mainprocess(int s, int socket_type, int buffer_size, istream & input, pid_t childpid){
#endif
#ifdef WIN32
bool mt_mainprocess(int s, int socket_type, int buffer_size, istream & input, HANDLE childthread){
#endif
  bool ret = true;

  Buffer buffer(buffer_size);
  __int8_t * const buf = &buffer[0];

  while (true){
    input.read(reinterpret_cast<char *>(buf), buffer_size / sizeof(char));
    int count = input.gcount() * sizeof(char);
    if (count == 0){
      logger.writer(INFO) << "EOF at input.\n";

      if (socket_type == SOCK_STREAM)
        verify_ne(shutdown(s, SHUT_WR), -1);

      break;
    }

    int sent = 0;
    while (count != sent){
      int cur = send(s, buf + sent, count - sent, 0 /* flags */ );
      if (cur == -1){
        SOCKETS_PERROR("ERROR: send");
        ret = false;
        break;
      }
      sent += cur;
    }
    logger.writer(TRACE) << count << " bytes sent.\n";
  }

  if (socket_type == SOCK_STREAM){
    logger.writer(INFO) << "Waiting for child process/thread termination.\n";

    #ifdef UNIX
      if (waitpid(childpid, NULL, 0 /* options */ ) == -1){
        logger.writer(ERROR_) << "waitpid failed.\n";
        return false;
      }
    #endif
    #ifdef WIN32
      verify_ne(WaitForSingleObject(childthread, INFINITE), WAIT_FAILED);
    #endif
  }
  else if (socket_type == SOCK_DGRAM){
    logger.writer(INFO) << "Sleep 5 seconds to receive remained data from server if any.\n";
    sleep_(5);

    logger.writer(INFO) << "Terminate child process/thread.\n";
    #ifdef UNIX
      kill(childpid, SIGTERM);
    #endif
    #ifdef WIN32
      verify_ne(TerminateThread(childthread, 0), 0);
    #endif
  }
  else
    assert(false);

  logger.writer(INFO) << "Exit main process/thread.\n";
  return ret;
}

#ifdef UNIX
  void mt_subprocess(int s, int buffer_size, ostream & output){
#endif
#ifdef WIN32
  struct mt_subprocess_data{
    int s;
    int buffer_size;
    ostream * output;
  };
  DWORD WINAPI mt_subprocess(void * param){
  mt_subprocess_data * data = reinterpret_cast<mt_subprocess_data *>(param);
  assert(data);

  int s = data->s;
  int buffer_size = data->buffer_size;
  ostream & output = *data->output;

  delete data;
  data = 0;
#endif

  Buffer buffer(buffer_size);
  __int8_t * const buf = &buffer[0];

  while (true){
    int count = recv(s, buf, buffer_size, 0 /* flags */ );
    if (count == -1){
      SOCKETS_PERROR("ERROR: recv");
      break;
    }
    else if (count == 0){
      logger.writer(INFO) << "Server closed connection.\n";
      break;
    }
    else{
      logger.writer(TRACE) << count << " bytes read.\n";
      output.write(reinterpret_cast<char *>(buf), count);

      /*
       * This is necessary.
       *
       * For TCP, it can be places before exit(0)
       * (depends on how child process may be terminated).
       */
      output.flush();
    }
  }

  logger.writer(INFO) << "Exit child thread/process.\n";
  #ifdef WIN32
    return 0;
  #endif
}

bool mt_handler(Settings * settings_, int s){
  MtSettings * settings = static_cast<MtSettings *>(settings_);

  istream & input = settings->get_input();
  ostream & output = settings->get_output();

  #ifdef UNIX
    pid_t childpid = fork();
    if (childpid)
      return mt_mainprocess(s, settings->socket_type, settings->buffer_size, input, childpid);
    else{
      mt_subprocess(s, settings->buffer_size, output);
      exit(0);
    }
  #endif
  #ifdef WIN32
    mt_subprocess_data * data = new mt_subprocess_data;
    data->s = s;
    data->buffer_size = settings->buffer_size;
    data->output = &output;
    HANDLE childthread = CreateThread(0, 0, mt_subprocess, data, 0, 0);
    return mt_mainprocess(s, settings->socket_type, settings->buffer_size, input, childthread);
  #endif
}

} // namespace

int main(int argc, char ** argv){
  SimpleSettings        simple_settings;
  SelectWindowsSettings select_windows_settings;
  SelectUnixSettings    select_unix_settings;
  MtSettings            mt_settings;

  vector<string> select_windows_mode_names;
  #ifdef WIN32
    select_windows_mode_names.push_back("select");
  #endif
  select_windows_mode_names.push_back("select-windows");

  vector<string> mt_mode_names;
  mt_mode_names.push_back("multithreaded");
  mt_mode_names.push_back("mt");

  ThisFramework::Modes modes;
  modes.push_back(ThisFramework::ModeDescription(
    "simple",
    "Simple client.",
    &simple_settings,
    &simple_handler));
  modes.push_back(ThisFramework::ModeDescription(
    select_windows_mode_names,
    "Client implemented using select() function.",
    &select_windows_settings,
    &select_windows_handler));
  #ifdef UNIX
    modes.push_back(ThisFramework::ModeDescription(
      "select",
      "Client implemented using select() with Unix-specific optimizations.",
      &select_unix_settings,
      &select_unix_handler));
  #endif
  modes.push_back(ThisFramework::ModeDescription(
    mt_mode_names,
    "Multiprocess (on Unix) / multithreaded (on Windows) client.",
    &mt_settings,
    &mt_handler));
  return ThisFramework::run("client", argc, argv, modes, invoke_handler) ? 0 : 1;
}
