#pragma once

#include "../common/common.h"

#include <fstream>
#include <list>
#include <string>

namespace cmdfw {
namespace optentries {

struct BasicOptentry {

  /*
   * Optentry should have a data member storing the value corresponding to the value passed
   * with command line option. Consider adding the default constructor that sets default value
   * of that data member properly.
   *
   * Use code of existing classes for reference.
   */

  /* Return getopt letter. */
  virtual char letter() const = 0;

  /* Return getopt option. */
  virtual std::string option() const = 0;

  /* Return user-friendly description of the option. */
  virtual std::string description() const = 0;

  /*
   * Process command line option. Caller must be sure that it is processing command line option
   * equal to the value returned by option() method.
   *
   * Return values:
   *   0 - success;
   *   1 - an error occured, caller should print usage information;
   *   2 - an error occured, caller shouldn't print usage information.
   */
  virtual int handle(char * optarg) = 0;

  /*
   * Return whether the data member is set. If you set it to some default value in default
   * constructor, consider omitting the method in your class so that the base class
   * implementation (returning 'true') is used.
   */
  virtual bool is_set() const{
    return true;
  }
};

/* ---------------------------------------------------------------------------------------------- */

class BasicOptentries{
public:

  virtual ~BasicOptentries(){
  }

  virtual std::string options() const{
    return std::string();
  }

  virtual std::string options_help() const{
    return std::string();
  }

    /*
     * Return values:
     * 0 - everything successful, proceed;
     * 1 - no handler found for given option;
     * 2 - handler returned an error, print usage to stderr and exit (exit code = 1);
     * 3 - handler returned an error, exit (exit code = 1).
     */
  virtual int handle(char letter, char * optarg){
    return 1;
  }

  virtual bool validate() const{
    return true;
  }
};

class Optentries: public BasicOptentries{
public:

  Optentries() {}

  /* override */ virtual std::string options() const;
  /* override */ virtual std::string options_help() const;
  /* override */ virtual int handle(char letter, char * optarg);
  /* override */ virtual bool validate() const;

protected:

  void append_optentries(BasicOptentry ** optentries){ optentries_.push_back(optentries); }

private:

  std::list<BasicOptentry **> optentries_;
};

/* ---------------------------------------------------------------------------------------------- */

/*
 * Currently there are not so many classes, so we can assign a command line
 * option to each of them safely.
 *
 * NOTE: Do not use 'h' letter because it is reserved for help.
 */

struct SocketType: public BasicOptentry {
  typedef int value_t;

  /* override */ virtual char letter() const { return 't'; }
  /* override */ virtual std::string option() const { return "t:"; }
  /* override */ virtual std::string description() const { return "TCP or UDP. Default: TCP."; }

  value_t socket_type;
  SocketType(): socket_type(SOCK_STREAM) {}

  /* override */ virtual int handle(char * optarg);
};

struct Hostname: public BasicOptentry {
  typedef std::string value_t;

  /* override */ virtual char letter() const { return 'a'; }
  /* override */ virtual std::string option() const { return "a:"; }
  /* override */ virtual std::string description() const { return "Server IP address or host name. Default: localhost."; }

  value_t addr;
  Hostname(): addr("localhost") {}

  /* override */ virtual int handle(char * optarg){ addr = optarg; return 0; }
};

  /* Conflicts with PortNumber. */
struct Port: public BasicOptentry {
  typedef std::string value_t;

  /* override */ virtual char letter() const { return 'p'; }
  /* override */ virtual std::string option() const { return "p:"; }
  /* override */ virtual std::string description() const { return "Port. Default: 28635."; }

  value_t port;
  Port(): port("28635") {}

  /* override */ virtual int handle(char * optarg){ port = optarg; return 0; }
};

  /* Conflicts with Port. */
struct PortNumber: public BasicOptentry {
  typedef uint16_t value_t;

  /* override */ virtual char letter() const { return 'p'; }
  /* override */ virtual std::string option() const { return "p:"; }
  /* override */ virtual std::string description() const { return "Port. Default: 28635."; }

  value_t port;
  PortNumber(): port(28635) {}

  /* override */ virtual int handle(char * optarg);
};

struct BufferSize: public BasicOptentry {
  typedef int value_t;

  /* override */ virtual char letter() const { return 'b'; }
  /* override */ virtual std::string option() const { return "b:"; }
  /* override */ virtual std::string description() const { return "Buffer size in bytes. Default: 1024."; }

  value_t buffer_size;
  BufferSize(): buffer_size(1024) {}

  /* override */ virtual int handle(char * optarg);
};

  /* Conflicts with CIoInput. */
struct CxxIoInput: public BasicOptentry {
  /* override */ virtual char letter() const { return 'i'; }
  /* override */ virtual std::string option() const { return "i:"; }
  /* override */ virtual std::string description() const { return "Input file name. Default: use standard input."; }

  /* override */ virtual int handle(char * optarg);

  std::istream & get_input() const;

private:
  mutable std::ifstream input;
  bool set_input(const char * filename);
};

  /* Conflicts with CIoOutput. */
struct CxxIoOutput: public BasicOptentry {
  /* override */ virtual char letter() const { return 'o'; }
  /* override */ virtual std::string option() const { return "o:"; }
  /* override */ virtual std::string description() const { return "Output file name. Default: use standard output."; }

  /* override */ virtual int handle(char * optarg);

  std::ostream & get_output() const;

private:
  mutable std::ofstream output;
  bool set_output(const char * filename);
};

  /* Conflicts with CxxIoInput. */
struct CIoInput: public BasicOptentry {
  /* override */ virtual char letter() const { return 'i'; }
  /* override */ virtual std::string option() const { return "i:"; }
  /* override */ virtual std::string description() const { return "Input file name. Default: use standard input."; }

  CIoInput(): input(NULL) {}
  ~CIoInput(){ if (input != NULL) verify_e(fclose(input), 0); }

  /* override */ virtual int handle(char * optarg);

  FILE * get_input() const;

private:
  FILE * input;
  bool set_input(const char * filename);
};

  /* Conflicts with CxxIoOutput. */
struct CIoOutput: public BasicOptentry {
  /* override */ virtual char letter() const { return 'o'; }
  /* override */ virtual std::string option() const { return "o:"; }
  /* override */ virtual std::string description() const { return "Output file name. Default: use standard output."; }

  CIoOutput(): output(NULL) {}
  ~CIoOutput(){ if (output != NULL) verify_e(fclose(output), 0); }

  /* override */ virtual int handle(char * optarg);
  FILE * get_output() const;

private:
  FILE * output;
  bool set_output(const char * filename);
};

struct Verbosity: public BasicOptentry {
  typedef int value_t;

  /* override */ virtual char letter() const { return 'v'; }
  /* override */ virtual std::string option() const { return "v"; }
  /* override */ virtual std::string description() const { return "Make output more verbose."; }

  value_t verbosity;
  Verbosity(): verbosity(0) {}

  /* override */ virtual int handle(char * optarg){ ++verbosity; return 0; }
};

struct Quiet: public BasicOptentry {
  typedef bool value_t;

  /* override */ virtual char letter() const { return 'q'; }
  /* override */ virtual std::string option() const { return "q"; }
  /* override */ virtual std::string description() const { return "Show only errors and warnings."; }

  value_t quiet;
  Quiet(): quiet(false) {}

  /* override */ virtual int handle(char * optarg){ quiet = true; return 0; }
};

} // namespace optentries
} // namespace cmdfw
