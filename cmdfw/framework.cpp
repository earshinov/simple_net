#include "factory.hpp"
#include "framework.hpp"
#include "mode.hpp"
#include "settings.hpp"

#include "../common/common.h"

#include <iostream>
#include <string>
using namespace std;

namespace {

using namespace cmdfw;

int parse_argv(int argc, char ** argv, settings::Base & settings){
  /*
   * Return values:
   * 0 - everything is successful, continue;
   * 1 - an error occured, print usage and exit (exit code = 1);
   * 2 - an error occured, exit (exit code = 1);
   * 3 - everything is successful, exit (exit code = 0).
   */

  const std::string getopt_options = settings.options() + "h";
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

        switch (settings.handle(c, optarg)){
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

  return settings.validate() ? 0 : 1;
}

} // namespace

namespace cmdfw {
namespace framework {

bool run(
  const std::string & executable_name,
  int argc, char ** argv,
  const factory::Factory & factory){

  if (argc < 2){
    factory.usage(cerr, executable_name);
    return false;
  }

  std::string mode_name = argv[1];
  if (mode_name == "help" || mode_name == "-h"){
    if (argc > 2){
      cerr << "ERROR: Mode \"help\" does not take command line options.\n";
      factory.usage(cerr, executable_name);
      return false;
    }
    factory.usage(cout, executable_name);
    return true;
  }
  --argc, ++argv;

  const mode::Mode * mode = factory.mode(mode_name);
  if (!mode){
    cerr << "ERROR: Unknown mode: " << mode_name << '\n';
    factory.usage(cerr, executable_name);
    return false;
  }

  bool ret = false;
  settings::Base * settings = mode->settings();
  switch (parse_argv(argc, argv, *settings)){
    case 0:
      ret = mode->handle(*settings);
      break;
    case 1:
      mode->usage(cerr, executable_name, *settings);
      ret = false;
      break;
    case 2:
      ret = false;
      break;
    case 3:
      mode->usage(cout, executable_name, *settings);
      ret = true;
      break;
    default:
      assert(false);
      ret = false;
      break;
  }
  delete settings;
  return ret;
}

} // namespace framework
} // namespace cmdfw
