#pragma once

#include <iostream>

namespace logger {

enum Level {
    TRACE = 0,
    DEBUG_, /* use trailing underscore because MSVC #define's 'DEBUG' sometimes */
    INFO,
    WARN,
    ERROR_, /* use trailing underscore as 'ERROR' is #define'd within Windows system headers */
    FATAL,
  };


class Writer {
  friend class Logger;

public:

  template <typename X> Writer & operator <<(const X & x) {
    if (this->enabled) std::cerr << x;
    return *this;
  }

private:
  explicit Writer(bool enabled);

private:
  bool enabled;
};


class Logger {
public:
  Logger(Level level=TRACE);

  void setLevel(Level level);

  Writer & writer(Level level);

private:
  Level level;
  Writer enabled_writer;
  Writer disabled_writer;
};

} // namespace logger

