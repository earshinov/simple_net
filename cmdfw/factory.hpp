#pragma once

#include "mode.hpp"

#include <iosfwd>
#include <string>

namespace cmdfw {
namespace factory {

class Factory{
public:

  Factory(const mode::Modes & modes): modes_(modes) { }

public:

  void usage(std::ostream & o, const std::string & executable_name) const;

  const mode::BasicMode * mode(const std::string & name) const;

private:

  mode::Modes modes_;
};

} // namespace factory
} // namespace cmdfw
