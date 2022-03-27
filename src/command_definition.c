#include "command_definition.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
free_const(void const * p)
{
    free((void *)p);
}

void
args_append(struct args * const args, char const * const arg)
{
    args->args = realloc(args->args, sizeof *args->args * (args->num_args + 1));
    args->args[args->num_args] = arg;
    args->num_args++;
}

void
args_free(struct args * const args)
{
    if (args == NULL)
    {
        goto done;
    }

    for (size_t i = 0; i < args->num_args; i++)
    {
        free_const(args->args[i]);
    }
    free(args->args);
    free(args);

done:
    return;
}

struct args *
args_new(void)
{
    struct args * const args = calloc(1, sizeof *args);

    return args;
}


void
command_free(struct command * const command)
{
    if (command == NULL)
    {
        goto done;
    }

    free_const(command->name);
    args_free(command->args);
    free(command);

done:
    return;
}

struct command *
command_new(char const * const name, struct args * const args)
{
    struct command * const command = calloc(1, sizeof *command);

    command->name = strdup(name);
    command->args = args;

    return command;
}

static void
expression_append_expression_side(
    struct expression_side * const side,
    struct command_expression * const expression)
{
    side->expressions =
        realloc(
            side->expressions,
            sizeof *side->expressions * (side->num_expressions + 1));
    side->expressions[side->num_expressions] = expression;
    side->num_expressions++;
}

void
expression_append_expression_left(
    struct command_expression * const expression,
    struct command_expression * const left)
{
    expression_append_expression_side(&expression->left, left);
}

void
expression_append_expression_right(
    struct command_expression * const expression,
    struct command_expression * const right)
{
    expression_append_expression_side(&expression->right, right);
}

void
expression_append_command(
    struct command_expression * const expression,
    struct command * const command)
{
    /*
     * Single command expressions are only used if there is no condfition,
     * and therefore no RHS expression.
     */
    struct expression_side * const side = &expression->left;

    side->command = command;
}

void
expression_free(struct command_expression * const expression)
{
    if (expression == NULL)
    {
        goto done;
    }

    struct expression_side * const left = &expression->left;

    command_free(left->command);
    for (size_t i = 0; i < left->num_expressions; i++)
    {
        expression_free(left->expressions[i]);
    }
    free(left->expressions);

    struct expression_side * const right = &expression->right;

    command_free(right->command);
    for (size_t i = 0; i < right->num_expressions; i++)
    {
        expression_free(right->expressions[i]);
    }
    free(right->expressions);

    free(expression);
done:
    return;
}

struct command_expression *
expression_new(void)
{
    struct command_expression * const expression =
        calloc(1, sizeof *expression);

    expression->condition = condition_none;

    return expression;
}

void
command_list_append_expression(
    struct command_list * const list,
    struct command_expression * const expression)
{
    list->expressions =
        realloc(
            list->expressions,
            sizeof *list->expressions * (list->num_expressions + 1));
    list->expressions[list->num_expressions] = expression;
    list->num_expressions++;
}

void
command_list_free(struct command_list * const list)
{
    if (list == NULL)
    {
        goto done;
    }

    for (size_t i = 0; i < list->num_expressions; i++)
    {
        expression_free(list->expressions[i]);
    }
    free(list->expressions);
    free(list);

done:
    return;
}

struct command_list *
command_list_new(void)
{
    struct command_list * const list = calloc(1, sizeof *list);

    return list;
}

static void
command_print(struct command const * const command, FILE * const stream)
{
    fprintf(stream, "%s", command->name);
    for (size_t i = 0; i < command->args->num_args; i++)
    {
        fprintf(stream, " %s", command->args->args[i]);
    }
}

static void
expression_print(
    struct command_expression const * const expression, FILE * const stream);

static void
expression_side_print(
    struct expression_side const * const side, FILE * const stream)
{
    if (side->command != NULL)
    {
        command_print(side->command, stream);
    }
    else
    {
        for (size_t i = 0; i < side->num_expressions; i++)
        {
            expression_print(side->expressions[i], stream);
        }
    }
}

static void
expression_print(
    struct command_expression const * const expression, FILE * const stream)
{
    if (expression->is_a_group)
    {
        fprintf(stream, "(");
    }
    expression_side_print(&expression->left, stream);
    if (expression->condition != condition_none)
    {
        static char const * const condition_strs[condition_COUNT] =
        {
            [condition_or] = "||",
            [condition_and] = "&&"
        };

        fprintf(stream, " %s ", condition_strs[expression->condition]);
        expression_side_print(&expression->right, stream);
    }
    if (expression->is_a_group)
    {
        fprintf(stream, ")");
    }
}

void
command_list_print(
    struct command_list const * const list, FILE * const stream)
{
    for (size_t i = 0; i < list->num_expressions; i++)
    {
        struct command_expression const * const expression =
            list->expressions[i];

        if (i != 0)
        {
            fprintf(stream, " ; ");
        }
        expression_print(expression, stream);
    }
}
