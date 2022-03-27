%option reentrant
%option yylineno
%option bison-bridge
%option bison-locations

%{

#include <stdio.h>
#include "command_definition.h"
#include "command_parser.yacc.h"

%}

%x DBL_QUOTE_STRING
%x SINGLE_QUOTE_STRING

SEMICOLON ;
AND_CONDITION \&\&
OR_CONDITION \|\|
LBRACE \(
RBRACE \)
DOUBLE_QUOTE \"
SINGLE_QUOTE \'
STRING [_a-zA-Z0-9]*
EOL \n
WHITESPACE [ \t]+

%%

{SEMICOLON} {
    return TOK_SEMICOLON;
}

{AND_CONDITION} {
    return TOK_AND;
}

{OR_CONDITION} {
    return TOK_OR;
}

{LBRACE} {
    return TOK_LBRACE;
}

{RBRACE} {
    return TOK_RBRACE;
}

{DOUBLE_QUOTE} {
    yymore();
    BEGIN DBL_QUOTE_STRING;
}

<DBL_QUOTE_STRING>\\{DOUBLE_QUOTE} {
    yymore();
}

<DBL_QUOTE_STRING>{DOUBLE_QUOTE} {
    yymore();
    yylval->string = yytext;
    BEGIN 0;
    return TOK_DOUBLE_QUOTED_STRING;
}

<DBL_QUOTE_STRING>. {
    yymore();
}

{SINGLE_QUOTE} {
    yymore();
    BEGIN SINGLE_QUOTE_STRING;
}

<SINGLE_QUOTE_STRING>\\{SINGLE_QUOTE} {
    yymore();
}

<SINGLE_QUOTE_STRING>{SINGLE_QUOTE} {
    yymore();
    yylval->string = yytext;
    BEGIN 0;
    return TOK_SINGLE_QUOTED_STRING;
}

<SINGLE_QUOTE_STRING>. {
    yymore();
}

{STRING} {
    yylval->string = yytext;
    return TOK_STRING;
}

{EOL} {
   return TOK_EOL;
}

{WHITESPACE} {
   /* ignore whitespace */
}

%%

