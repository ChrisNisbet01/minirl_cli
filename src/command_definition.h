#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct args
{
	size_t num_args;
	char const * * args;
};

struct command
{
	char const * name;
	struct args * args;
};

enum condition_t
{
	condition_none,
	condition_and,
	condition_or,
    condition_COUNT
};

struct expression_side
{
    struct command * command;
    size_t num_expressions;
    struct command_expression * * expressions;
};

struct command_expression
{
    bool is_a_group;
    enum condition_t condition;
    struct expression_side left;
    struct expression_side right;
};

struct command_list
{
    size_t num_expressions;
    struct command_expression * * expressions;
};


void
args_append(struct args * args, char const * arg);

void
args_free(struct args * args);

struct args *
args_new(void);

void
command_free(struct command * command);

struct command *
command_new(char const * name, struct args * args);

void
expression_append_command(
    struct command_expression * expression, struct command * command);

void
expression_append_expression_left(
    struct command_expression * expression,
    struct command_expression * left);

void
expression_append_expression_right(
    struct command_expression * expression,
    struct command_expression * right);

void
expression_free(struct command_expression * expression);

struct command_expression *
expression_new(void);

void
command_list_append_expression(
    struct command_list * list, struct command_expression * expression);

void
command_list_free(struct command_list * list);

struct command_list *
command_list_new(void);

void
command_list_print(struct command_list const * list, FILE * const stream);

