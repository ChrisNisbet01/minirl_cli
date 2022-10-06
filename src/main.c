#include "command_parser.h"

#include <linenoise.h>
#if 0
#include <history.h>
#endif

#include <stdlib.h>
#include <string.h>

#if 0
static char const *
get_previous_line(struct tinyrl_history const * const rl_history)
{
    char const * previous_line;
    size_t const history_length = tinyrl_history_length(rl_history);

    if (history_length == 0)
    {
        previous_line = NULL;
        goto done;
    }

    size_t const last_index = history_length - 1;
    previous_line = tinyrl_history_get(rl_history, last_index);

done:
    return previous_line;
}

static bool
line_differs_from_previous(
    struct tinyrl_history const * const rl_history, char const * const new_line)
{
    char const * const previous_line = get_previous_line(rl_history);
    bool line_differs = previous_line == NULL
        || strcmp(new_line, previous_line) != 0;

    return line_differs;
}

static void
update_history(
    struct tinyrl_history * const rl_history, char const * const line)
{
    if (line_differs_from_previous(rl_history, line))
    {
        tinyrl_history_add(rl_history, line);
    }

}
#else

static void
update_history(
    char const * const line)
{
#if 0
    if (line_differs_from_previous(rl_history, line))
    {
        tinyrl_history_add(rl_history, line);
    }
#else
    linenoiseHistoryAdd(line);
#endif

}

#endif

static bool
run_command_line(char const * const line)
{
    struct command_list * const list = parse_command_line(line);
    bool success;

    if (list != NULL)
    {
        command_list_print(list, stdout);
        fprintf(stdout, "\n");
        fflush(stdout);
        command_list_free(list);
        success = true;
    }
    else
    {
        fprintf(stdout, "unable to parse line\n");
        success = false;
    }

    return success;
}

static void
run_commands_via_prompt(void)
{
#if 0
    struct tinyrl * const rl = tinyrl_new(stdin, stdout);
    struct tinyrl_history * const rl_history = tinyrl_history_new(rl, 10);
    char * line;

    fprintf(stdout, "'q' to quit\n");

    while ((line=tinyrl_readline(rl, "prompt>")) != NULL)
    {
        fprintf(stdout, "got line: %s\n", line);

        if (line[0] == 'q')
        {
            free(line);
            break;
        }

        if (line[0] != '\0')
        {
            update_history(rl_history, line);
            run_command_line(line);
        }

        free(line);
    }

	tinyrl_history_delete(rl_history);
	tinyrl_delete(rl);
#else
    linenoise_st * linenoise_ctx = linenoise_new(stdin, stdout);
    bool const enable_beep = false;
    linenoiseBeepControl(linenoise_ctx, enable_beep);

    fprintf(stdout, "'q' to quit\n");

    char * line;
    while ((line = linenoise("prompt>")) != NULL)
    {
        fprintf(stdout, "got line: %s\n", line);

        if (line[0] == 'q')
        {
            free(line);
            break;
        }

        if (line[0] != '\0')
        {
            update_history(line);
            run_command_line(line);
        }

        free(line);
    }

    linenoise_delete(linenoise_ctx);

#endif

}

int
main(int const argc, char * * const argv)
{
    int exit_code;

    if (argc > 1)
    {
        exit_code = run_command_line(argv[1]) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    else
    {
        run_commands_via_prompt();
        exit_code = EXIT_SUCCESS;
    }

done:
    return exit_code;
}
