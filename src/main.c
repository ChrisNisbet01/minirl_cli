#include "command_parser.h"

#include <linenoise.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


static void
update_history(
    char const * const line)
{
    linenoiseHistoryAdd(line);
}

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
print_tinyrl_output(int const fd)
{
    int bytes_read;
    char buf[200];
    size_t buf_len = 0;

    fprintf(stdout, "reading pipe\n");
    do
    {
        char c;
        bytes_read = read(fd, &c, sizeof c);
        if (bytes_read > 0)
        {
            fprintf(stdout, "0x%x ", (unsigned)c);
            buf[buf_len] = c;
            buf_len++;
        }
    } while (bytes_read > 0);

    fprintf(stdout, "\ndone reading pipe.\n");

    buf[buf_len] = '\0';
    fprintf(stdout, "%s", buf);
}

static void
run_commands_via_prompt(void)
{
    int output_pipe[2];

    pipe2(output_pipe, O_NONBLOCK);

    FILE * const output_fp = fdopen(output_pipe[1], "wb");

    linenoise_st * linenoise_ctx = linenoise_new(stdin, output_fp);
    bool const enable_beep = false;
    linenoiseBeepControl(linenoise_ctx, enable_beep);

    fprintf(stdout, "'q' to quit\n");

    char * line;
    while ((line = linenoise(linenoise_ctx, "prompt>")) != NULL)
    {
        fprintf(stdout, "got line\n");
        fflush(output_fp);
        print_tinyrl_output(output_pipe[0]);

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

        linenoiseFree(line);
    }

    linenoise_delete(linenoise_ctx);
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
