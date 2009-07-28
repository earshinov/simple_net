#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace cmdfw {

namespace settings {
  class Base;
}

namespace mode {

class Mode{
public:

  virtual ~Mode(){
  }

public:

  Mode(
    const std::string & name_,
    const std::string & short_description_):

    names(),
    short_description(short_description_){

    names.push_back(name_);
  }

  Mode(
      /*
       * Do not use map for names, because sort order may be important
       * (this is the order how mode names are presented to user in help).
       */
    const std::vector<std::string> & names_,
    const std::string & short_description_):

      /* FIXME: copying of vector */
    names(names_),
    short_description(short_description_){
  }

public:

  void usage(
    std::ostream & o,
    const std::string & executable_name,
    const settings::Base & settings) const;

  virtual settings::Base * settings() const = 0;

  virtual bool handle(settings::Base & settings) const = 0;

public:

  std::vector<std::string> names;
  std::string short_description;
};

typedef std::vector<Mode *> Modes;

} // namespace mode
} // namespace cmdfw
