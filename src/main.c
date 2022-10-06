#include "command_parser.h"

#include <linenoise.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


static void
update_history(linenoise_st * const linenoise_ctx, char const * const line)
{
    linenoiseHistoryAdd(linenoise_ctx, line);
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

static void completion_cb(char const * const buf, linenoiseCompletions * const lc)
{
    if (buf[0] == 'h') {
        linenoiseAddCompletion(lc,"hello");
        linenoiseAddCompletion(lc,"hello there");
    }
}

#ifdef WITH_HINTS
char *hints_cb(const char *buf, int *color, int *bold)
{
    if (!strcasecmp(buf,"hello")) {
        *color = 35;
        *bold = 0;
        return " World";
    }
    return NULL;
}
#endif

#define TAB 9
static bool
tab_handler(linenoise_st * const linenoise_ctx,
            char const key,
            void * const user_ctx)
{
    linenoiseCompletions * lc = linenoise_completions_get(linenoise_ctx);

    linenoiseAddCompletion(lc, "hello");
    linenoiseAddCompletion(lc, "1help");
    linenoiseAddCompletion(lc, "2help");
    linenoiseAddCompletion(lc, "3help");
    linenoiseAddCompletion(lc, "4help");
    linenoiseAddCompletion(lc, "5help");
    linenoiseAddCompletion(lc, "6help");
    linenoiseAddCompletion(lc, "7help");
    linenoiseAddCompletion(lc, "8help");
    linenoiseAddCompletion(lc, "9help");
    linenoiseAddCompletion(lc, "0help");
    linenoiseAddCompletion(lc, "ahelp");
    linenoiseAddCompletion(lc, "bhelp");
    linenoiseAddCompletion(lc, "chelp");
    linenoiseAddCompletion(lc, "dhelp");
    linenoiseAddCompletion(lc, "elast");

    return true;
}

static void
run_commands_via_prompt(bool const print_raw_codes)
{
    FILE * output_fp;
    int output_pipe[2] = { 0 };

    if (print_raw_codes)
    {
        pipe2(output_pipe, O_NONBLOCK);

        output_fp = fdopen(output_pipe[1], "wb");
    }
    else
    {
        output_fp = stdout;
    }

    linenoise_st * linenoise_ctx = linenoise_new(stdin, output_fp);
    bool const enable_beep = false;
    bool const multiline_mode = true;
    linenoiseBeepControl(linenoise_ctx, enable_beep);
    linenoiseSetMultiLine(linenoise_ctx, multiline_mode);
    linenoiseSetCompletionCallback(linenoise_ctx, completion_cb);
    linenoise_bind_key(linenoise_ctx, TAB, tab_handler, NULL);

#ifdef WITH_HINTS
    linenoiseSetHintsCallback(linenoise_ctx, hints_cb);
#endif

    fprintf(stdout, "'q' to quit\n");

    char * line;
    while ((line = linenoise(linenoise_ctx, "prompt>")) != NULL)
    {
        fprintf(stdout, "got line\n");
        if (print_raw_codes)
        {
            fflush(output_fp);
            print_tinyrl_output(output_pipe[0]);
        }

        if (line[0] == 'q')
        {
            free(line);
            break;
        }

        if (line[0] != '\0')
        {
            update_history(linenoise_ctx, line);
            run_command_line(line);
        }

        linenoiseFree(line);
    }

    linenoise_delete(linenoise_ctx);
}

int
main(int argc, char * * argv)
{
    int exit_code;
    bool print_raw_codes = false;

    if (argc > 1 && strcmp(argv[1], "raw") == 0)
    {
        argc--;
        argv++;
        print_raw_codes = true;
    }
    if (argc > 1)
    {
        exit_code = run_command_line(argv[1]) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    else
    {
        run_commands_via_prompt(print_raw_codes);
        exit_code = EXIT_SUCCESS;
    }

done:
    return exit_code;
}
