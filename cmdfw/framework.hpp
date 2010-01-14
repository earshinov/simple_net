#pragma once

#include "factory.hpp"

#include <string>

namespace cmdfw {
namespace framework {

bool run(
  const std::string & executable_name,
  int argc, char ** argv,
  const factory::Factory & factory);

} // namespace framework
} // namespace cmdfw
