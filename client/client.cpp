#include "../common/common.h"
#include "../cmdfw/cmdfw.hpp"

#include "../common/logger.h"
using namespace logger;

#include <cstdlib>
using namespace std;

#ifdef UNIX
  #include <sys/wait.h>
#endif

namespace{

struct AllOptentries {

  /* GeneralOptentries */
  cmdfw::optentries::SocketType socket_type;
  cmdfw::optentries::Hostname addr;
  cmdfw::optentries::Port port;
  cmdfw::optentries::BufferSize buffer_size;
  cmdfw::optentries::Verbosity verbosity;
  cmdfw::optentries::Quiet quiet;

  /* CxxIoOptentries */
  cmdfw::optentries::CxxIoInput cxxio_input;
  cmdfw::optentries::CxxIoOutput cxxio_output;

  /* CIoOptentries */
  cmdfw::optentries::CIoInput cio_input;
  cmdfw::optentries::CIoOutput cio_output;

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
  cmdfw::optentries::Hostname::value_t & addr(){ return OPTENTRIES.addr.addr; }
  cmdfw::optentries::Port::value_t & port(){ return OPTENTRIES.port.port; }
  cmdfw::optentries::BufferSize::value_t & buffer_size(){ return OPTENTRIES.buffer_size.buffer_size; }
  cmdfw::optentries::Verbosity::value_t & verbosity(){ return OPTENTRIES.verbosity.verbosity; }
  cmdfw::optentries::Quiet::value_t & quiet(){ return OPTENTRIES.quiet.quiet; }

protected:

  static cmdfw::optentries::BasicOptentry * entries[];
};

/* static */ cmdfw::optentries::BasicOptentry * GeneralOptentries::entries[] = {
  //&OPTENTRIES.socket_type, /* hidden */
  &OPTENTRIES.addr,
  &OPTENTRIES.port,
  &OPTENTRIES.buffer_size,
  &OPTENTRIES.verbosity,
  &OPTENTRIES.quiet,
  0
};

struct CxxIoOptentries: public GeneralOptentries {
public:
  static auto_ptr<CxxIoOptentries> create(){
    auto_ptr<CxxIoOptentries> ret(new CxxIoOptentries());
    ret->append_optentries();
    return ret;
  }
protected:
  CxxIoOptentries() {}
  void append_optentries(){
    cmdfw::optentries::Optentries::append_optentries(entries);
    GeneralOptentries::append_optentries();
  }

public:

  istream & get_input(){ return OPTENTRIES.cxxio_input.get_input(); }
  ostream & get_output(){ return OPTENTRIES.cxxio_output.get_output(); }

protected:

  static cmdfw::optentries::BasicOptentry * entries[];
};

/* static */ cmdfw::optentries::BasicOptentry * CxxIoOptentries::entries[] = {
  &OPTENTRIES.cxxio_input,
  &OPTENTRIES.cxxio_output,
  0
};

struct CIoOptentries: public GeneralOptentries {
public:
  static auto_ptr<CIoOptentries> create(){
    auto_ptr<CIoOptentries> ret(new CIoOptentries());
    ret->append_optentries();
    return ret;
  }
protected:
  CIoOptentries() {}
  void append_optentries(){
    cmdfw::optentries::Optentries::append_optentries(entries);
    GeneralOptentries::append_optentries();
  }

public:

  FILE * get_input(){ return OPTENTRIES.cio_input.get_input(); }
  FILE * get_output(){ return OPTENTRIES.cio_output.get_output(); }

protected:

  static cmdfw::optentries::BasicOptentry * entries[];
};

/* static */ cmdfw::optentries::BasicOptentry * CIoOptentries::entries[] = {
  &OPTENTRIES.cio_input,
  &OPTENTRIES.cio_output,
  0
};

typedef CxxIoOptentries SelectWindowsOptentries;
typedef CIoOptentries SelectUnixOptentries;

struct MultiprotoOptentries: public CxxIoOptentries{
public:
  static auto_ptr<MultiprotoOptentries> create(){
    auto_ptr<MultiprotoOptentries> ret(new MultiprotoOptentries());
    ret->append_optentries();
    return ret;
  }
protected:
  MultiprotoOptentries() {}
  void append_optentries() {
    cmdfw::optentries::Optentries::append_optentries(entries);
    CxxIoOptentries::append_optentries();
  }

protected:

  static cmdfw::optentries::BasicOptentry * entries[];
};

/* static */ cmdfw::optentries::BasicOptentry * MultiprotoOptentries::entries[] = {
  &OPTENTRIES.socket_type,
  0
};

typedef MultiprotoOptentries SimpleOptentries, MtOptentries;


typedef bool (*Handler)(GeneralOptentries & optentries, int s);
bool invoke_handler(Handler handler, GeneralOptentries & optentries);

bool simple_handler(GeneralOptentries & optentries_, int s);
bool select_windows_handler(GeneralOptentries & optentries_, int s);
bool select_unix_handler(GeneralOptentries & optentries_, int s);
bool mt_handler(GeneralOptentries & optentries_, int s);

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
Logger logger;


bool invoke_handler(Handler handler, GeneralOptentries & optentries){
  logger.setLevel(
    (optentries.verbosity() > 0) ? TRACE :
      (optentries.quiet() ? WARN : INFO));

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
  hints.ai_socktype = optentries.socket_type();
  if (getaddrinfo(
        optentries.addr().c_str(),
        optentries.port().c_str(),
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

  ret = handler(optentries, s);
run_return:
  if (res != 0)
    freeaddrinfo(res);
  if (s != -1)
    verify_ne(close(s), -1);
  return ret;
}


bool simple_handler(GeneralOptentries & optentries_, int s){
  SimpleOptentries & optentries = static_cast<SimpleOptentries &>(optentries_);

  istream & input = optentries.get_input();
  ostream & output = optentries.get_output();

  Buffer buffer(optentries.buffer_size());
  int8_t * const buf = &buffer[0];

  while (!input.eof()){

    input.read(reinterpret_cast<char *>(buf), optentries.buffer_size() / sizeof(char));
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
      int recv_current = recv(s, buf + recv_total, optentries.buffer_size() - recv_total, 0);
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

bool select_windows_handler(GeneralOptentries & optentries_, int s){
  SelectWindowsOptentries & optentries = static_cast<SelectWindowsOptentries &>(optentries_);

  istream & input = optentries.get_input();
  ostream & output = optentries.get_output();

  Buffer buffer(optentries.buffer_size());
  int8_t * const buf = &buffer[0];

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
      int received = recv(s, buf, optentries.buffer_size(), 0);
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
      input.read(reinterpret_cast<char *>(buf), optentries.buffer_size() / sizeof(char));
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
bool select_unix_handler(GeneralOptentries & optentries_, int s){
  SelectUnixOptentries & optentries = static_cast<SelectUnixOptentries &>(optentries_);

  int input = fileno(optentries.get_input());
  int output = fileno(optentries.get_output());

  ReadWriteBuffer to(optentries.buffer_size());
  ReadWriteBuffer from(optentries.buffer_size());

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
  int8_t * const buf = &buffer[0];

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
  int8_t * const buf = &buffer[0];

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

bool mt_handler(GeneralOptentries & optentries_, int s){
  MtOptentries & optentries = static_cast<MtOptentries &>(optentries_);

  istream & input = optentries.get_input();
  ostream & output = optentries.get_output();

  #ifdef UNIX
    pid_t childpid = fork();
    if (childpid)
      return mt_mainprocess(s, optentries.socket_type(), optentries.buffer_size(), input, childpid);
    else{
      mt_subprocess(s, optentries.buffer_size(), output);
      exit(0);
    }
  #endif
  #ifdef WIN32
    mt_subprocess_data * data = new mt_subprocess_data;
    data->s = s;
    data->buffer_size = optentries.buffer_size();
    data->output = &output;
    HANDLE childthread = CreateThread(0, 0, mt_subprocess, data, 0, 0);
    return mt_mainprocess(s, optentries.socket_type(), optentries.buffer_size(), input, childthread);
  #endif
}

} // namespace

int main(int argc, char ** argv){
  cmdfw::mode::Modes modes;
  vector<string> names;

  modes.push_back(new Mode<SimpleOptentries>("simple", "Simple client.", simple_handler));

  names.clear();
  #ifdef WIN32
    names.push_back("select");
  #endif
  names.push_back("select-windows");
  modes.push_back(new Mode<SelectWindowsOptentries>(names,
    "Client implemented using select() function.", select_windows_handler));

  #ifdef UNIX
    modes.push_back(new Mode<SelectUnixOptentries>("select",
      "Client implemented using select() with Unix-specific optimizations.",
      select_unix_handler));
  #endif

  modes.push_back(new Mode<MtOptentries>("mt",
    "Multiprocess (on Unix) / multithreaded (on Windows) client.", mt_handler));

  return cmdfw::framework::run("client", argc, argv, cmdfw::factory::Factory(modes)) ? 0 : 1;
}
