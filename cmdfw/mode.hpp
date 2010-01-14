#pragma once

#include "optentries.hpp"

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace cmdfw {
namespace mode {

class BasicMode{
public:

  virtual ~BasicMode(){
  }

public:

  BasicMode(
    const std::string & name_,
    const std::string & short_description_):

    names(),
    short_description(short_description_){

    names.push_back(name_);
  }

  BasicMode(
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
    const optentries::BasicOptentries & optentries) const;

  virtual std::auto_ptr<optentries::BasicOptentries> optentries() const = 0;

  virtual bool handle(optentries::BasicOptentries & optentries) const = 0;

public:

  std::vector<std::string> names;
  std::string short_description;
};

typedef std::vector<BasicMode *> Modes;

} // namespace mode
} // namespace cmdfw
