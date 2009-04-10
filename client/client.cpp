#include "../common/common.h"
#include "../common/framework.h"
using namespace std;

namespace{

struct Settings:
  public framework::settings::Base,
  public framework::settings::mixins::SocketTypeMixin,
  public framework::settings::mixins::InputMixin,
  public framework::settings::mixins::OutputMixin,
  public framework::settings::mixins::AddressMixin,
  public framework::settings::mixins::PortMixin,
  public framework::settings::mixins::BufferSizeMixin {

  /*
   * NOTE: SocketTypeMixin is not mentioned in methods as we do not want this
   * option to be available in all modes (to be precise, in "select" mode).
   */

  /* override */ virtual std::string options() const{
    return
      framework::settings::mixins::InputMixin::option() +
      framework::settings::mixins::OutputMixin::option() +
      framework::settings::mixins::AddressMixin::option() +
      framework::settings::mixins::PortMixin::option() +
      framework::settings::mixins::BufferSizeMixin::option();
  }

  /* override */ virtual std::string options_help() const{
    return string() +
      "-" + framework::settings::mixins::InputMixin::letter()              + "\n"
        "  " + framework::settings::mixins::InputMixin::description()      + "\n"
      "-" + framework::settings::mixins::OutputMixin::letter()             + "\n"
        "  " + framework::settings::mixins::OutputMixin::description()     + "\n"
      "-" + framework::settings::mixins::AddressMixin::letter()            + "\n"
        "  " + framework::settings::mixins::AddressMixin::description()    + "\n"
      "-" + framework::settings::mixins::PortMixin::letter()               + "\n"
        "  " + framework::settings::mixins::PortMixin::description()       + "\n"
      "-" + framework::settings::mixins::BufferSizeMixin::letter()         + "\n"
        "  " + framework::settings::mixins::BufferSizeMixin::description() + "\n";
  }

  /* override */ virtual int handle(char letter, char * optarg){
    int result;
    if (letter == framework::settings::mixins::InputMixin::letter())
      result = framework::settings::mixins::InputMixin::handle(optarg);
    else if (letter == framework::settings::mixins::OutputMixin::letter())
      result = framework::settings::mixins::OutputMixin::handle(optarg);
    else if (letter == framework::settings::mixins::AddressMixin::letter())
      result = framework::settings::mixins::AddressMixin::handle(optarg);
    else if (letter == framework::settings::mixins::PortMixin::letter())
      result = framework::settings::mixins::PortMixin::handle(optarg);
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
    if (!framework::settings::mixins::InputMixin::is_set()){
      success = false;
      cerr << "ERROR: Option '-" << framework::settings::mixins::InputMixin::letter() << "' is required.\n";
    }
    if (!framework::settings::mixins::OutputMixin::is_set()){
      success = false;
      cerr << "ERROR: Option '-" << framework::settings::mixins::OutputMixin::letter() << "' is required.\n";
    }
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
    return success;
  }
};

typedef Settings SelectSettings;

struct SimpleSettings: public Settings{

  /* override */ virtual std::string options() const{
    return Settings::options() +
      framework::settings::mixins::SocketTypeMixin::option();
  }

  /* override */ virtual std::string options_help() const{
    return string() +
      "-" + framework::settings::mixins::SocketTypeMixin::letter()         + "\n"
        "  " + framework::settings::mixins::SocketTypeMixin::description() + "\n"
      + Settings::options_help();
  }

  /* override */ virtual int handle(char letter, char * optarg){
    int result = Settings::handle(letter, optarg);
    if (result == 1){
      if (letter == framework::settings::mixins::SocketTypeMixin::letter())
        result = framework::settings::mixins::SocketTypeMixin::handle(optarg);
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
    if (!framework::settings::mixins::SocketTypeMixin::is_set()){
      success = false;
      cerr << "ERROR: Option '-" << framework::settings::mixins::SocketTypeMixin::letter() << "' is required.\n";
    }
      /*
       * Careful, Settings::validate() must fire even if success is already false.
       */
    success = Settings::validate() && success;
    return success;
  }
};

typedef vector<__int8_t> Buffer;
typedef bool (*Handler)(Settings * settings, int s, Buffer & buffer);
typedef framework::Framework<Handler> ThisFramework;

bool invoke_handler(Handler handler, framework::settings::Base * settings_){
  Settings * settings = static_cast<Settings *>(settings_);
  bool ret = false;

  #ifdef WIN32
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata)){
      cerr << "ERROR: Could not initialize socket library\n";
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
    cerr << "ERROR: Could not resolve IP address and/or port\n";
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
    cerr << "ERROR: Could not connect to server\n";
    goto run_return;
  }

  {
    Buffer buffer(settings->buffer_size);
    ret = handler(settings, s, buffer);
  }

run_return:
  if (res != 0)
    freeaddrinfo(res);
  if (s != -1)
    verify_ne(close(s), -1);
  return ret;
}

bool simple_handler(Settings * settings, int s, Buffer & buffer){
  istream & input = settings->get_input();
  ostream & output = settings->get_output();
  __int8_t * const buf = &buffer[0];

  while (!input.eof()){

    input.read(reinterpret_cast<char *>(buf), settings->buffer_size / sizeof(char));
    int count = input.gcount() * sizeof(char);
    if (count == 0)
      continue;
    cerr << "TRACE: " << count << " bytes to send\n";

    int sent_total = 0;
    while (sent_total < count){
      int sent_current = send(s, buf + sent_total, count - sent_total, 0);
      if (sent_current == -1){
        cerr << "ERROR: Encountered an error while sending data\n";
        return false;
      }
      cerr << "TRACE: " << sent_current << " bytes sent\n";
      sent_total += sent_current;
    }
    cerr << "TRACE: Start receiving\n";

    int recv_total = 0;
    while (recv_total < count){
      int recv_current = recv(s, buf + recv_total, settings->buffer_size - recv_total, 0);
      if (recv_current == -1){
        cerr << "ERROR: Encountered an error while reading data\n";
        return false;
      }
      if (recv_current == 0){
        cerr << "ERROR: Server shutdowned unexpectedly\n";
        return false;
      }
      cerr << "TRACE: Received " << recv_current << " bytes\n";
      recv_total += recv_current;
    }
    output.write(reinterpret_cast<char *>(buf), recv_total * sizeof(char));
  }
  return true;
}

bool select_handler(Settings * settings, int s, Buffer & buffer){
  istream & input = settings->get_input();
  ostream & output = settings->get_output();
  __int8_t * const buf = &buffer[0];

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
      return false;
    }

    if (FD_ISSET(s, &rset)){
      int received = recv(s, buf, settings->buffer_size, 0);
      if (received == -1){
        SOCKETS_PERROR("ERROR: Error when receiving");
        return false;
      }
      else if (received == 0){
        if (stdin_eof){
          cerr << "TRACE: Server closed the connection\n";
          return true;
        }
        else{
          cerr << "ERROR: Server closed the connection unexpectedly\n";
          return false;
        }
      }
      else {
        cerr << "TRACE: " << received << " bytes to print\n";
        output.write(reinterpret_cast<char *>(buf), received * sizeof(char));
        cerr << "TRACE: Bytes printed\n";
      }
    }

    if (FD_ISSET(s, &wset)){
      input.read(reinterpret_cast<char *>(buf), settings->buffer_size / sizeof(char));
      int count = input.gcount() * sizeof(char);
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
          SOCKETS_PERROR("ERROR: Error when sending");
          return false;
        }
        else{
          assert(sent == count);
          cerr << "TRACE: " << sent << " bytes sent\n";
        }
      }
    }
  }
}

} // namespace

int main(int argc, char ** argv){
  SimpleSettings simple_settings;
  SelectSettings select_settings;
  ThisFramework::Modes modes;
  modes.push_back(ThisFramework::ModeDescription(
    "simple", "Simple client.", &simple_settings, &simple_handler));
  modes.push_back(ThisFramework::ModeDescription(
    "select", "Client implemented using select() function.", &select_settings, &select_handler));
  return ThisFramework::run("client", argc, argv, modes, invoke_handler) ? 0 : 1;
}
