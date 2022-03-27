%define api.pure
%locations
%lex-param{void *scanner}
%parse-param{void *scanner}

%code requires {

typedef struct YYLTYPE {
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define YYLTYPE_IS_DECLARED 1 /* alert the parser that we have our own definition */

struct parse_state
{
    struct command_list * list;
};


}

%{
#include "command_parser.h"
#include "command_definition.h"
#include "command_parser.yacc.h"
#include "command_parser.lex.h"

#include <stdio.h>
#include <stdarg.h>

#define COMMAND_PARSE_DEBUG 0

int yydebug = 0;

void yyerror(YYLTYPE * yylloc, yyscan_t scanner, char *s, ...);

int command_parser_yywrap(yyscan_t const yyscanner)
{
	return 1;
}

#if COMMAND_PARSE_DEBUG
static void dbg_printf(char * const s, ...)
{
    va_list ap;

    va_start(ap, s);
    vfprintf(stderr, s, ap);
    va_end(ap);

    fflush(stderr);
}
#else
#define dbg_printf(s, ...) do {} while(0)
#endif

struct command_list *
parse_command_line(char const * const line_buffer)
{
    yyscan_t scanner;
    struct parse_state parse_state = {0};
    struct command_list * list;

    list = parse_state.list = command_list_new();
    
    if (list == NULL)
    {
        goto done;
    }

    command_parser_yylex_init_extra(&parse_state, &scanner);

    YY_BUFFER_STATE buf = command_parser_yy_scan_string(line_buffer, scanner);
    bool const parse_error = yyparse(scanner) != 0;

    if (parse_error)
    {
        dbg_printf("parse error\n");
    }

    command_parser_yy_delete_buffer(buf, scanner);

    dbg_printf("parse complete\n");

    command_parser_yylex_destroy(scanner);

    if (parse_error)
    {
        command_list_free(list);
        list = NULL;
    }

done:
    return list;
}

%}

%union
{
    struct command_list * list;
    struct command_expression * expression;
    enum condition_t condition;
    struct command * command;
    char * command_name;
	char * string;
    struct args * args;
    struct arg * arg;
}

%type <list> expression_list
%type <condition> condition
%type <expression> expressions expression
%type <command> command
%type <args> args
%type <string> arg
%type <string> command_name

%token TOK_EOL TOK_SEMICOLON TOK_AND TOK_OR TOK_LBRACE TOK_RBRACE
%token <string> TOK_STRING TOK_DOUBLE_QUOTED_STRING TOK_SINGLE_QUOTED_STRING

%start expression_list

%%

expression_list:
    expressions
    {
        dbg_printf("got expressions list\n");
        struct parse_state * const parse_state = 
            command_parser_yyget_extra(scanner);
        struct command_list * const list = parse_state->list;

        command_list_append_expression(list, $<expression>1);
        $<list>$ = list;
    }
    | expression_list TOK_SEMICOLON expressions
    {
        dbg_printf("got expressions list -> expressions\n");
        struct command_list * const list = $<list>1;

        command_list_append_expression(list, $<expression>3);
        $<list>$ = list;
    }
    | expression_list TOK_EOL
    {
        dbg_printf("expressions got EOL\n");
        $<list>$ = $1;
        YYACCEPT;
    }

expressions:
    expression
    {
        dbg_printf("got lone expression\n");
        $<expression>$ = $<expression>1;
    }
    | expressions condition expression
    {
        dbg_printf("expressions condition (%d) expression\n", $2);
        struct command_expression * expression = expression_new();

        expression_append_expression_left(expression, $<expression>1);
        expression->condition = $<condition>2;
        expression_append_expression_right(expression, $<expression>3);

        $<expression>$ = expression;
    }

expression:
    lbrace expressions rbrace
    {
        dbg_printf("got bracketed expression in $2\n");
        struct command_expression * const expression = $<expression>2;

        expression->is_a_group = true;
        $<expression>$ = expression;
    }
    | command
    {
        dbg_printf("got expression with single command\n");
        struct command_expression * const expression = expression_new();
 
        expression_append_command(expression, $<command>1);
        $<expression>$ = expression;
    }

lbrace:
    TOK_LBRACE
    {
        dbg_printf("got brace open\n");
    }

rbrace:
    TOK_RBRACE
    {
        dbg_printf("got brace close\n");
    }

condition:
    TOK_OR
    {
        dbg_printf("got OR condition\n");
        $<condition>$ = condition_or;
    }
    | TOK_AND
    {
        dbg_printf("got AND condition\n");
        $<condition>$ = condition_and;
    }

command: 
	command_name args
    {
        dbg_printf("handle command with args\n");
        struct args * const args = $<args>2;
        char const * const command_name = $<string>1;

        struct command * const command = command_new(command_name, args);

        dbg_printf("got command %s with %zu args\n", 
                   command->name, command->args->num_args);
        $<command>$ = command;
    }

command_name:
    TOK_STRING
    {
        dbg_printf("got command name: %s\n", $1);
        $<string>$ = strdup($1);
    }

args:
    /* Empty */
    {
        dbg_printf("handle empty args\n");
        $<args>$ = args_new();
    }
    | args arg
    {
        dbg_printf("got args\n");
        struct args * const args = $<args>1;
        char * const arg = $<string>2;

        args_append(args, arg);
        $<args>$ = args;
    }

arg:
    TOK_DOUBLE_QUOTED_STRING
    {
        dbg_printf("got dbl quote arg: %s\n", $1);
        /* Quotes are kept. */
        $<string>$ = strdup($1);
    }
    | TOK_SINGLE_QUOTED_STRING
    {
        /* Quotes are kept. */
        dbg_printf("got sgl quote arg: %s\n", $1);
        $<string>$ = strdup($1);
    }
    | TOK_STRING
    {
        dbg_printf("got arg: %s\n", $1);
        $<string>$ = strdup($1);
    }

%%

void yyerror(YYLTYPE * const yylloc, yyscan_t const scanner, char * const s, ...)
{
    if (yydebug != 0)
    {
    	va_list ap;

        va_start(ap, s);
    	vfprintf(stderr, s, ap);
    	va_end(ap);

    	fprintf(stderr, "\n");
    }
}

