#pragma once

#include <string>

namespace cmdfw {

namespace mode {
  class Mode;
}
namespace settings {
  class Base;
}
namespace factory {
  class Factory;
}

namespace framework {

bool run(
  const std::string & executable_name,
  int argc, char ** argv,
  const factory::Factory & factory);

} // namespace framework
} // namespace cmdfw
