set(LIBS common)
if(UNIX)
  set(LIBS ${LIBS} rt) # POSIX run-time library
endif(UNIX)
if(WIN32)
  set(LIBS ${LIBS} WS2_32)
endif(WIN32)

find_library(LIBEV ev)
if(LIBEV)
  set(HAVE_LIBEV 1)
  set(LIBS ${LIBEV})
else(LIBEV)
  message(STATUS "libev library not found. Server will not have libev mode.")
endif(LIBEV)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h ESCAPE_QUOTES)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_executable(server server.cpp client.hpp client_select.hpp client_libev.hpp)
target_link_libraries(server ${LIBS})

if(WIN32)
  install(TARGETS server RUNTIME DESTINATION bin)
endif(WIN32)
