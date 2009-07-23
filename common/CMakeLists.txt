set(SOURCES common.h framework.h logger.h logger.cpp)

if(WIN32)
  set(SOURCES ${SOURCES} XGetOpt.h XGetOpt.cpp)
endif(WIN32)
add_library(common STATIC ${SOURCES})
