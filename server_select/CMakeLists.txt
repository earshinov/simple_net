add_executable(server_select server_select.cpp)

set(LIBS)
if(WIN32)
  set(LIBS common WS2_32)
endif(WIN32)
target_link_libraries(server_select ${LIBS})

if(WIN32)
  install(TARGETS server_select RUNTIME DESTINATION bin)
endif(WIN32)
