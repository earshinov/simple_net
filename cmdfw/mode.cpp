#include "mode.hpp"
#include "settings.hpp"

#include <iostream>
using namespace std;

namespace cmdfw {
namespace mode {

void Mode::usage(
  std::ostream & o,
  const std::string & executable_name,
  const settings::Base & settings) const {

  o << "Usage: " << executable_name << ' ';

  if (names.size() == 1)
    o << names[0];
  else{
    o << '{';

    vector<string>::const_iterator it = names.begin();
    const vector<string>::const_iterator end = names.end();

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
    << settings.options_help();
}

} // namespace mode
} // namespace cmdfw
