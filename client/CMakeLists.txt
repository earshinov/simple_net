add_executable(client client.cpp)

set(LIBS common)
if(WIN32)
  set(LIBS ${LIBS} WS2_32)
endif(WIN32)
target_link_libraries(client ${LIBS})

if(WIN32)
  install(TARGETS client RUNTIME DESTINATION bin)
endif(WIN32)
