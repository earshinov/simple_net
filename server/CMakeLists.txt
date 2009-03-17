if(UNIX)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lrt") # POSIX run-time library
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lrt")
endif(UNIX)

add_executable(server server.cpp)

set(LIBS)
if(WIN32)
  set(LIBS common WS2_32)
endif(WIN32)
target_link_libraries(server ${LIBS})

if(WIN32)
  install(TARGETS server RUNTIME DESTINATION bin)
endif(WIN32)
