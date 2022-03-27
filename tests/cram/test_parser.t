Check that the parser is producing the expected results:

  $ [ -n "$TEST_BIN_DIR" ] && export PATH="$TEST_BIN_DIR:$PATH"

Simple simgle word command
  $ tinyrl_cli "abc"
  abc

Single command with single unquoted arg
  $ tinyrl_cli "abc def"
  abc def

Expression with single condition
  $ tinyrl_cli "abc 123||def"
  abc 123 || def

Bracketed expresssion
  $ tinyrl_cli "(abc 123||def) || ghi"
  (abc 123 || def) || ghi

Nested bracketed expressions
  $ tinyrl_cli "(abc 123|| (def && xyz 234)) || ghi"
  (abc 123 || (def && xyz 234)) || ghi

Multiple commands in sequence - separated by ';'
  $ tinyrl_cli "abc;ghi"
  abc ; ghi

Second command in sequence with args
  $ tinyrl_cli "abc;ghi 123"
  abc ; ghi 123

Command with double-quoted arg
  $ tinyrl_cli "abc \"def\""
  abc "def"

Command with single quoted arg
  $ tinyrl_cli "abc 'def'"
  abc 'def'

Bracketed command without condition
  $ tinyrl_cli "(abc 'def')"
  (abc 'def')

Command with invalid syntax
  $ tinyrl_cli "abc("
  unable to parse line
  [1]

Strip leading/trailing whitespace
  $ tinyrl_cli " abc "
  abc

Command with args with mixed quotes
  $ tinyrl_cli "abc 123 \"456\" '789'"
  abc 123 "456" '789'

