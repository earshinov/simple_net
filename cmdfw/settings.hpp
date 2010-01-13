#pragma once

#include "../common/common.h"

#include <fstream>
#include <string>

namespace cmdfw {
namespace settings {

  /*
   * Please see existing derived class implementations for reference.
   * Some template metaprogramming would be necessary here.
   */
class Base{
public:

  virtual ~Base(){
  }

public:

  virtual std::string options() const{
    return std::string();
  }

  virtual std::string options_help() const{
    return std::string();
  }

  virtual int handle(char letter, char * optarg){
    /*
     * Return values:
     * 0 - everything successful, proceed;
     * 1 - no handler found for given option;
     * 2 - handler returned an error, print usage to stderr and exit (exit code = 1);
     * 3 - handler returned an error, exit (exit code = 1).
     */
    return 1;
  }

  virtual bool validate(){
    return true;
  }
};

namespace mixins{

  struct Base{
      /*
       * Currently we do not use polymorphic behaviour,
       * so there is no need to make this function virtual.
       */
    bool is_set() const {
      return true;
    }

    /*
     * Mixin should have a data member storing the value corresponding
     * to the value passed with command line option. Mixin is meaningless
     * without this. Consider adding the default constructor that sets
     * default value of that data member properly.
     *
     * Mixin must also have the following methods:
     *
     *   char letter() const
     *     Return getopt letter.
     *
     *   std::string option() const
     *     Return getopt option.
     *
     *   std::string description() const
     *     Return user-friendly description of the option.
     *
     *   int handle(char * optarg)
     *     Process command line option. Caller must be sure that
     *     it is processing command line option equal to the value
     *     returned by option() method of this mixin.
     *
     *     Return values:
     *     0 - everything successful;
     *     1 - an error occured, caller should print usage information;
     *     2 - an error occured, caller shouldn't print usage information.
     *
     *   bool is_set() const
     *     Return whether the data member is set. If you set it to
     *     some default value in default constructor, consider omitting
     *     the method in your mixin so that the base class implementation
     *     (returning 'true') is used.
     *
     * Use code of existing mixins for reference.
     */
  };

  /*
   * Currently there are not so many mixins, so we can assign a command line
   * option to each of them safely.
   *
   * NOTE: Do not use 'h' letter because it is reserved for help.
   */

  struct SocketTypeMixin: public Base {
    char letter() const { return 't'; }
    std::string option() const { return "t:"; }
    std::string description() const { return "TCP or UDP. Default: TCP."; }

    int socket_type;
    SocketTypeMixin(): socket_type(SOCK_STREAM) {}
    int handle(char * optarg);
  };

  struct AddressMixin: public Base {
    char letter() const { return 'a'; }
    std::string option() const { return "a:"; }
    std::string description() const { return "Server IP address or host name. Default: localhost."; }

    std::string addr;
    AddressMixin(): addr("localhost") {}

    int handle(char * optarg){
      addr = optarg;
      return 0;
    }
  };

    /* Conflicts with PortNumberMixin. */
  struct PortMixin: public Base {
    char letter() const { return 'p'; }
    std::string option() const { return "p:"; }
    std::string description() const { return "Port. Default: 28635."; }

    std::string port;
    PortMixin(): port("28635") {}

    int handle(char * optarg){
      port = optarg;
      return 0;
    }
  };

    /* Conflicts with PortMixin. */
  struct PortNumberMixin: public Base {
    char letter() const { return 'p'; }
    std::string option() const { return "p:"; }
    std::string description() const { return "Port. Default: 28635."; }

    uint16_t port;
    PortNumberMixin(): port(28635) {}

    int handle(char * optarg){
      try{
        port = dirty_lexical_cast::lexical_cast<uint16_t>(optarg);
      }
      catch(dirty_lexical_cast::bad_lexical_cast &){
        std::cerr << "ERROR: Port must be integer: " << optarg << '\n';
        return 1;
      }
      return 0;
    }
  };

  struct BufferSizeMixin: public Base {
    char letter() const { return 'b'; }
    std::string option() const { return "b:"; }
    std::string description() const { return "Buffer size in bytes. Default: 1024."; }

    int buffer_size;
    BufferSizeMixin(): buffer_size(1024) {}

    int handle(char * optarg){
      try{
        buffer_size = dirty_lexical_cast::lexical_cast<unsigned int>(optarg);
      }
      catch(dirty_lexical_cast::bad_lexical_cast &){
        std::cerr << "ERROR: Buffer size must be integer: " << optarg << '\n';
        return 1;
      }
      return 0;
    }
  };

    /* Conflicts with CIoInputMixin. */
  struct CxxIoInputMixin: public Base {
    char letter() const { return 'i'; }
    std::string option() const { return "i:"; }
    std::string description() const { return "Input file name. Default: use standard input."; }

    mutable std::ifstream input;
    private:
      bool set(const char * filename) {
        input.open(filename, std::ifstream::in | std::ifstream::binary);
        return !input.fail();
      }
    public:

    std::istream & get_input() const {
      if (input.is_open())
        return input;
      else
        return std::cin;
    }

    int handle(char * optarg){
      if (!set(optarg)){
        std::cerr << "ERROR: Could not open input file: " << optarg << '\n';
        return 1;
      }
      return 0;
    }
  };

    /* Conflicts with CIoOutputMixin. */
  struct CxxIoOutputMixin: public Base {
    char letter() const { return 'o'; }
    std::string option() const { return "o:"; }
    std::string description() const { return "Output file name. Default: use standard output."; }

    mutable std::ofstream output;
    private:
    bool set(const char * filename) {
      output.open(filename, std::ofstream::out | std::ifstream::binary);
      return !output.fail();
    }
    public:

    std::ostream & get_output() const {
      if(output.is_open())
        return output;
      else
        return std::cout;
    }

    int handle(char * optarg){
      if (!set(optarg)){
        std::cerr << "ERROR: Could not open output file: " << optarg << '\n';
        return 1;
      }
      return 0;
    }
  };

    /* Conflicts with CxxIoInputMixin. */
  struct CIoInputMixin: public Base {
    CIoInputMixin(): input(NULL){
    }
    ~CIoInputMixin(){
      if (input != NULL)
        verify_e(fclose(input), 0);
    }

    char letter() const { return 'i'; }
    std::string option() const { return "i:"; }
    std::string description() const { return "Input file name. Default: use standard input."; }

    FILE * input;
    private:
      bool set(const char * filename) {
        if (input != NULL)
          verify_e(fclose(input), 0);

        input = fopen(filename, "rb");
        return input != NULL;
      }
    public:

    FILE * get_input() const {
      if (input != NULL)
        return input;
      else
        return stdin;
    }

    int handle(char * optarg){
      if (!set(optarg)){
        std::cerr << "ERROR: Could not open input file: " << optarg << '\n';
        return 1;
      }
      return 0;
    }
  };

    /* Conflicts with CxxIoOutputMixin. */
  struct CIoOutputMixin: public Base {
    CIoOutputMixin(): output(NULL){
    }
    ~CIoOutputMixin(){
      if (output != NULL)
        verify_e(fclose(output), 0);
    }

    char letter() const { return 'o'; }
    std::string option() const { return "o:"; }
    std::string description() const { return "Output file name. Default: use standard output."; }

    FILE * output;
    private:
    bool set(const char * filename) {
      if (output != NULL)
        verify_e(fclose(output), 0);

      output = fopen(filename, "wb");
      return output != NULL;
    }
    public:

    FILE * get_output() const {
      if (output != NULL)
        return output;
      else
        return stdout;
    }

    int handle(char * optarg){
      if (!set(optarg)){
        std::cerr << "ERROR: Could not open output file: " << optarg << '\n';
        return 1;
      }
      return 0;
    }
  };

  struct VerbosityMixin: public Base {
    VerbosityMixin(): verbosity(0){
    }
    ~VerbosityMixin(){
    }

    char letter() const { return 'v'; }
    std::string option() const { return "v"; }
    std::string description() const { return "Make output more verbose."; }

    int verbosity;

    int handle(char * optarg){
      ++verbosity;
      return 0;
    }
  };

  struct QuietMixin: public Base {
    QuietMixin(): quiet(false){
    }
    ~QuietMixin(){
    }

    char letter() const { return 'q'; }
    std::string option() const { return "q"; }
    std::string description() const { return "Show only errors and warnings."; }

    bool quiet;

    int handle(char * optarg){
      quiet = true;
      return 0;
    }
  };

} // namespace mixins

} // namespace settings
} // namespace cmdfw
