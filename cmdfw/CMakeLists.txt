set(SOURCES
  cmdfw.hpp
  framework.cpp framework.hpp
  factory.cpp factory.hpp
  mode.cpp mode.hpp
  settings.cpp settings.hpp)

add_library(cmdfw STATIC ${SOURCES})
