set(SOURCES
  cmdfw.hpp
  framework.cpp framework.hpp
  factory.cpp factory.hpp
  mode.cpp mode.hpp
  optentries.cpp optentries.hpp)

add_library(cmdfw STATIC ${SOURCES})
