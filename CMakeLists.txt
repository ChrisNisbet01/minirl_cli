cmake_minimum_required(VERSION 3.18)
project(tinyrl_cli)

add_subdirectory(src)

# Don't want support for 'hints' 
set(QUERY_TERMINAL_FOR_WIDTH OFF CACHE INTERNAL "" FORCE)

add_subdirectory(linenoise)

IF(UNIT_TESTING)
  ENABLE_TESTING()
  add_subdirectory(tests)
ENDIF()


