cmake_minimum_required(VERSION 2.6)

set(PROJECT_NAME simple_net)
project(${PROJECT_NAME} CXX)

if(WIN32)
  add_definitions(-DNOMINMAX)
endif(WIN32)
if(UNIX)
  add_definitions(-DUNIX)
endif(UNIX)

  #
  # The Eclipse CDT Error Parser cannot handle error messages that span more
  # than one line, which is the default gcc behavior. Force gcc to generate
  # single line error messages with no line wrapping.
  #
  # http://www.cmake.org/Wiki/CMake:Eclipse
  #
if(CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0")
endif(CMAKE_COMPILER_IS_GNUCC)
if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

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
  include(CPack)
endif(WIN32)
