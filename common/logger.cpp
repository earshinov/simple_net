#include "logger.h"

using namespace std;

static const char * const strings[] = {
  "TRACE",
  "DEBUG",
  "INFO",
  "WARN",
  "ERROR",
  "FATAL",
};
static const int N_STRINGS = 6;

namespace logger {

Writer::Writer(bool enabled_): enabled(enabled_) {
}

Logger::Logger(Level level_): level(level_),
  enabled_writer(true), disabled_writer(false) {
}

void Logger::setLevel(Level level_) {
  this->level = level_;
}

Writer & Logger::writer(Level level_) {
  if (level_ < this->level)
    return disabled_writer;
  else{
    if (level_ < 0 || level_ >= N_STRINGS)
      cerr << "(level " << level_ << "): ";
    else
      cerr << strings[level_] << ": ";
    return enabled_writer;
  }
}

} // namespace logger
