cmake_minimum_required(VERSION 2.6)

set(PROJECT_NAME simple_net)
project(${PROJECT_NAME} CXX)

if(WIN32)
  add_definitions(-DNOMINMAX)
endif(WIN32)
if(UNIX)
  add_definitions(-DUNIX)
endif(UNIX)

add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(client_select)
add_subdirectory(server_select)

if(WIN32)
  install(FILES
    ${simple_net_SOURCE_DIR}/Readme.rst
    ${simple_net_SOURCE_DIR}/Readme-Windows-binaries.rst
  DESTINATION doc)

  set(CPACK_PACKAGE_VERSION_MAJOR "0")
  set(CPACK_PACKAGE_VERSION_MINOR "1")
  set(CPACK_PACKAGE_VERSION_PATCH "0")
  set(CPACK_GENERATOR "ZIP")
  include (CPack)
endif(WIN32)
