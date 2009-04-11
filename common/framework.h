#pragma once

#include "common.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>

namespace framework {

  namespace settings{

      /*
       * Please see existing derived class implementations for reference.
       * Some template metaprogramming would be necessary here.
       */
    struct Base{
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
           * Currently we do not polymorphic behaviour,
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

        int handle(char * optarg){
          for (char * p = optarg; *p != '\0'; ++p)
            *p = toupper(*p);
          if (strcmp(optarg, "TCP") == 0){
            socket_type = SOCK_STREAM;
            return 0;
          }
          else if (strcmp(optarg, "UDP") == 0){
            socket_type = SOCK_DGRAM;
            return 0;
          }
          else{
            std::cerr << "ERROR: Unknown mode: " << optarg << '\n';
            return 2;
          }
        }
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

        __uint16_t port;
        PortNumberMixin(): port(28635) {}

        int handle(char * optarg){
          try{
            port = dirty_lexical_cast::lexical_cast<__uint16_t>(optarg);
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
          int i;
          try{
            i = dirty_lexical_cast::lexical_cast<int>(optarg);
          }
          catch(dirty_lexical_cast::bad_lexical_cast &){
            std::cerr << "ERROR: Buffer size must be integer: " << optarg << '\n';
            return 1;
          }
          if (i <= 0){
            std::cerr << "ERROR: Buffer size must be positive: " << i << '\n';
            return 1;
          }
          buffer_size = static_cast<unsigned int>(i);
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

    } // namespace mixins

  } // namespace settings

    /*
     * Use a struct instead of plain template functions
     * so that it would be more convenient to pass the template argument.
     */
  template <typename TModeData> struct Framework {

    struct ModeDescription{

      ModeDescription(
        const std::string & name_,
        const std::string & short_description_,
        settings::Base * stgs_,
        TModeData data_):

        names(),
        short_description(short_description_),
        stgs(stgs_),
        data(data_){

        names.push_back(name_);
      }

      ModeDescription(
          /*
           * Do not use, because sort order may be important
           * (this is the order how mode names are presented to user in help).
           */
        const std::vector<std::string> & names_,

        const std::string & short_description_,
        settings::Base * stgs_,
        TModeData data_):

          /* FIXME: copying of vector */
        names(names_),
        short_description(short_description_),
        stgs(stgs_),
        data(data_){
      }

      std::vector<std::string> names;
      std::string short_description;
      settings::Base * stgs;
      TModeData data;
    };
    typedef std::vector<ModeDescription> Modes;
      /* Do not use a map as the entries must be sorted as we wish. */

    typedef bool (*Handler)(TModeData data, settings::Base * stgs);

  private:

    static int parse_argv(int argc, char ** argv, settings::Base * stgs){
      /*
       * Return values:
       * 0 - everything is successful, continue;
       * 1 - an error occured, print usage and exit (exit code = 1);
       * 2 - an error occured, exit (exit code = 1);
       * 3 - everything is successful, exit (exit code = 0).
       */

      const std::string getopt_options = stgs->options() + "h";
      const char * const getopt_options_cstr = getopt_options.c_str();

      for (;;){
        int c = getopt(argc, argv, getopt_options_cstr);
        if (c == -1)
          break;

        switch (c){
          case '?':
            /* Error message is printed by getopt itself. */
            return 1;
          case 'h':
            return 3;
          default:

            switch (stgs->handle(c, optarg)){
              case 0:
                break;
              case 1:
                std::cerr << "ASSERTION: Unknown command line option: " << static_cast<char>(c) << '\n';
                assert(false);
                return 1;
              case 2:
                return 1;
              case 3:
                return 2;
              default:
                assert(false);
            }
        }
      }

      if (optind != argc){
        std::cerr << "ERROR: Bad arguments\n";
        return 1;
      }

      return stgs->validate() ? 0 : 1;
    }

    static void generic_usage(
      std::ostream & o,
      const std::string & executable_name,
      const Modes & modes){

      o << "Usage: " << executable_name << " MODE OPTIONS\n"
           "\n"
           "Modes:\n"
           "\n"
           "help, -h\n"
           "  Print this message and exit. This mode does not take options.\n";

      typename Modes::const_iterator it = modes.begin();
      const typename Modes::const_iterator end = modes.end();
      for(; it != end; ++it){
        std::vector<std::string>::const_iterator it2 = it->names.begin();
        const std::vector<std::string>::const_iterator end2 = it->names.end();

        assert(it2 != end2);
        o << *it2++;

        for(; it2 != end2; ++it2)
          o << ", " << *it2;

        o << "\n  " << it->short_description << '\n';
      }

      o << "\n"
           "In order to get more information about particular mode, type\n"
           "  " << executable_name << " MODE -h\n";
    }

    static void usage(
      std::ostream & o,
      const std::string & executable_name,
      const ModeDescription & mode){

      o << "Usage: " << executable_name << ' ';

      if (mode.names.size() == 1)
        o << mode.names[0];
      else{
        o << '{';

        std::vector<std::string>::const_iterator it = mode.names.begin();
        const std::vector<std::string>::const_iterator end = mode.names.end();

        assert(it != end);
        o << *it++;

        for(; it != end; ++it)
          o << ", " << *it;

        o << '}';
      }

      o << " OPTIONS\n"
           "\n"
           "Options:\n"
           "\n"
           "-h\n"
           "  Print this message and exit\n"
        << mode.stgs->options_help();
    }

  public:

      /*
       * Here is the function which framework's user will call.
       */
    static bool run(
      const std::string & executable_name,
      int argc, char ** argv,
      const Modes & modes, Handler handler){

      if (argc < 2){
        generic_usage(std::cerr, executable_name, modes);
        return false;
      }

      std::string mode_name = argv[1];
      if (mode_name == "help" || mode_name == "-h"){
        if (argc > 2){
          std::cerr << "ERROR: Mode \"help\" does not take command line options.\n";
          generic_usage(std::cerr, executable_name, modes);
          return false;
        }
        generic_usage(std::cout, executable_name, modes);
        return true;
      }
      --argc, ++argv;

      typename Modes::const_iterator it = modes.begin();
      {
        const typename Modes::const_iterator end = modes.end();
        for (; it != end; ++it)
          if (std::find(it->names.begin(), it->names.end(), mode_name) != it->names.end())
            break;
        if (it == end){
          std::cerr << "ERROR: Unknown mode: " << mode_name << '\n';
          generic_usage(std::cerr, executable_name, modes);
          return false;
        }
      }
      const ModeDescription & mode = *it;

      switch (parse_argv(argc, argv, mode.stgs)){
        case 0:
          return handler(mode.data, mode.stgs);
        case 1:
          usage(std::cerr, executable_name, mode);
          return false;
        case 2:
          return false;
        case 3:
          usage(std::cout, executable_name, mode);
          return true;
        default:
          assert(false);
          return false;
      }
    }
  };

} // namespace framework
