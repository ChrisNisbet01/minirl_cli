cmake_minimum_required(VERSION 3.18)
project(tinyrl_cli)

add_subdirectory(tinyrl)
add_subdirectory(src)

IF(UNIT_TESTING)
  ENABLE_TESTING()
  add_subdirectory(tests)
ENDIF()


