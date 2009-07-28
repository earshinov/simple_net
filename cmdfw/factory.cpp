#include "factory.hpp"
#include "mode.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
using namespace std;

namespace cmdfw {
namespace factory {

const mode::Mode * Factory::mode(const std::string & name) const{
  mode::Modes::const_iterator it = modes_.begin();
  const mode::Modes::const_iterator end = modes_.end();
  for (; it != end; ++it)
    if (find((*it)->names.begin(), (*it)->names.end(), name) != (*it)->names.end())
      break;
  if (it == end)
    return 0;
  else
    return (*it);
}

void Factory::usage(std::ostream & o, const std::string & executable_name) const{

  o << "Usage: " << executable_name << " MODE OPTIONS\n"
       "\n"
       "Modes:\n"
       "\n"
       "help, -h\n"
       "  Print this message and exit. This mode does not take options.\n";

  mode::Modes::const_iterator it = modes_.begin();
  const mode::Modes::const_iterator end = modes_.end();
  for(; it != end; ++it){
    std::vector<std::string>::const_iterator it2 = (*it)->names.begin();
    const std::vector<std::string>::const_iterator end2 = (*it)->names.end();

    assert(it2 != end2);
    o << *it2++;

    for(; it2 != end2; ++it2)
      o << ", " << *it2;

    o << "\n  " << (*it)->short_description << '\n';
  }

  o << "\n"
       "In order to get more information about particular mode, type\n"
       "  " << executable_name << " MODE -h\n";
}

} // namespace factory
} // namespace cmdfw
