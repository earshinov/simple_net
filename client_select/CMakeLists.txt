add_executable(client_select client_select.cpp)

set(LIBS)
if(WIN32)
  set(LIBS common WS2_32)
endif(WIN32)
target_link_libraries(client_select ${LIBS})

if(WIN32)
  install(TARGETS client_select RUNTIME DESTINATION bin)
endif(WIN32)
