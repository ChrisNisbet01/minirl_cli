set(FLEX_EXECUTABLE flex)
set(BISON_EXECUTABLE bison)

# Create custom command for flex/lex (note the outputs)
add_custom_command(
 OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/command_lexer.c"
 COMMAND ${FLEX_EXECUTABLE} --prefix=command_parser_yy --header-file=command_parser.lex.h -o "${CMAKE_CURRENT_BINARY_DIR}/command_lexer.c" "${CMAKE_CURRENT_SOURCE_DIR}/command_parser.l"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/command_parser.l" "${CMAKE_CURRENT_SOURCE_DIR}/command_parser.y"
)

add_custom_target(CommandLexer
  echo "Creating command_lexer c files"
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/command_lexer.c"
)

set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/command_lexer.c" GENERATED)
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/command_parser.c" GENERATED)

# Create custom command for bison/yacc (note the DEPENDS)
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/command_parser.c"
  COMMAND ${BISON_EXECUTABLE} --defines=command_parser.yacc.h -p command_parser_yy "${CMAKE_CURRENT_SOURCE_DIR}/command_parser.y" -o "${CMAKE_CURRENT_BINARY_DIR}/command_parser.c"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/command_parser.y"
)

add_custom_target(CommandParser 
  echo "Creating command_parser c files"
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/command_parser.c" "${CMAKE_CURRENT_SOURCE_DIR}/command_definition.h"
)

add_dependencies(CommandParser CommandLexer)


set(SOURCE_FILES 
  main.c
  command_parser.h
  command_definition.c
  command_definition.h
  command_parser.c
  command_lexer.c
)

add_compile_definitions(_GNU_SOURCE)
add_executable(tinyrl_cli ${SOURCE_FILES})
add_dependencies(tinyrl_cli CommandParser)

target_link_libraries(tinyrl_cli LINK_PUBLIC tinyrl)
target_include_directories(tinyrl_cli 
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}")

