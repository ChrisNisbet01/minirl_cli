#include "command_parser.h"
#include "cli_helpers.h"
#include "sl.h"

#include <minirl.h>

#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ESCAPESTR "\x1b"

#define TAB 0x09
#define LF  0x0a
#define CR  0x0d
#define ESC 0x1b


static void
update_history(minirl_st * const minirl, char const * const line)
{
    minirl_history_add(minirl, line);
}

static bool
run_command_line(char const * const line)
{
	return true;
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

typedef struct line_buf_st
{
    size_t len;
    size_t capacity;
    uint8_t * buf;
} line_buf_st;

static void
line_buf_reset(line_buf_st * const line_buf)
{
    free(line_buf->buf);
    line_buf->buf = NULL;
    line_buf->len = 0;
    line_buf->capacity = 0;
}

static void
line_buf_init(line_buf_st * const line_buf)
{
    line_buf->buf = NULL;
    line_buf_reset(line_buf);
}

static void
line_buf_append(line_buf_st * const line_buf, uint8_t b)
{
    if (line_buf->len >= line_buf->capacity)
    {
        size_t const new_capacity = line_buf->capacity + 100;

        line_buf->buf = realloc(line_buf->buf, new_capacity + 1);
        line_buf->capacity = new_capacity;
    }
    line_buf->buf[line_buf->len] = b;
    line_buf->len++;
    line_buf->buf[line_buf->len] = '\0';
}

static void
print_tinyrl_output(int const fd)
{
    int bytes_read;
    line_buf_st line_buf;

    line_buf_init(&line_buf);

    fprintf(stdout, "reading pipe\n");
    do
    {
        char c;
        bytes_read = read(fd, &c, sizeof c);
        if (bytes_read > 0)
        {
            if (isprint(c))
            {
                fprintf(stdout, "<0x%x>", (unsigned)c);
            }
            else if (c == ESC)
            {
                fprintf(stdout, "<ESC>");
            }
            else if (c == TAB)
            {
                fprintf(stdout, "<TAB>");
            }
            else if (c == CR)
            {
                fprintf(stdout, "<CR>");
            }
            else if (c == LF)
            {
                fprintf(stdout, "<LF>");
            }
            else
            {
                fprintf(stdout, "<0x%x>", (unsigned)c);
            }
            line_buf_append(&line_buf, c);
        }
    }
    while (bytes_read > 0);

    fprintf(stdout, "\ndone reading pipe.\n");

    fprintf(stdout, "%s", line_buf.buf);
    line_buf_reset(&line_buf);
}

struct cli_match
{
    unsigned start; /* Start of the word that was matched */
    char ** matches;
    bool finished; /* We have a match but we may continue matching */
};

static char * *
cli_cmd_complete(char * * const words, char const * const partial, bool * finished)
{
    *finished = true;
    char * * matches = NULL;
    char ** w = words;

    if (w == NULL || *w == NULL || *w[0] == '\0')
    {
        matches = sl_new(NULL);
        matches = sl_append(matches,
                            "config",
                            "exit",
                            "analyzer",
                            "cat",
                            "clear",
                            "container",
                            "cp",
                            "help",
                            "ls",
                            "mkdir",
                            "modem",
                            "monitoring",
                            "more",
                            "mv",
                            "ping",
                            "poweroff",
                            "reboot",
                            "rm",
                            "scp",
                            "show",
                            "speedtest",
                            "ssh",
                            "system",
                            "telnet",
                            "traceroute");
        goto done;
    }
    if (partial != NULL)
    {
        matches = sl_new(NULL);
        if (strncmp(partial, "hello", strlen(partial)) == 0)
        {
            matches = sl_append(matches, "hello");
        }
        if (strncmp(partial, "help", strlen(partial)) == 0)
        {
            matches = sl_append(matches, "help");
        }
        if (strncmp(partial, "hellelujah", strlen(partial)) == 0)
        {
            matches = sl_append(matches, "hellelujah");
        }
    }

done:
    sl_sort(matches);

    return matches;
}
static void cli_match(minirl_st * const minirl,
                      struct cli_split * split,
                      struct cli_match * match)
{
    unsigned len;
    bool quoted;
    char ** matches;
    char ** s;

#if 0
    /* Update the config (if modified) so that the command completion is not
     * using a stale config. */
    if (cli->view == &cli_root_view)
    cli_reload(cli);

#endif
    matches = cli_cmd_complete(split->words, split->partial, &match->finished);

    /* Accept all matches that start with the partial word.
     * When the partial word contains quotes or escapes,
     * only accept exact matches */
    if (split->partial)
    {
        len = strlen(split->partial);
        quoted = strcmp(split->partial, split->partial_raw) != 0;
        for (s = matches; s && *s;)
        {
            if (quoted && strcmp(*s, split->partial) == 0)
            {
                sl_free(matches);
                matches = sl_new(split->partial_raw);
                break;
            }
            else if (!quoted && strncmp(*s, split->partial, len) == 0)
            {
                s++;
            }
            else
            {
                free(sl_remove(s));
            }
        }
    }

    match->matches = matches;
    match->start = minirl_point_get(minirl);
    if (split->partial_raw != NULL)
    {
        match->start -= strlen(split->partial_raw);
    }
}

static void cli_match_free(struct cli_match * match)
{
    sl_free(match->matches);
}

static void cli_split(minirl_st * const minirl, bool partial, struct cli_split * split)
{
    char * s;

    if (partial)
        s = strndup(minirl_line_get(minirl), minirl_point_get(minirl));
    else
        s = strdup(minirl_line_get(minirl));
    cli_split_line(s, partial, split);
    free(s);
}

static bool cli_complete(minirl_st * const minirl, bool allow_prefix)
{
    struct cli_split split;
    struct cli_match match;
    bool ret;

    cli_split(minirl, true, &split);
    cli_match(minirl, &split, &match);

    if (allow_prefix
        && split.partial != NULL
        && sl_find(match.matches, split.partial))
    {
        /* Return success without showing completions when the user presses
         * space on an exact match. */
        ret = true;
    }
    else
    {
        ret = minirl_complete(minirl, match.start, match.matches, allow_prefix);
        if (!allow_prefix && !match.finished)
        {
            /* Don't jump to next arg when the user presses tab and the parser
             * has not finished providing completions. */
            ret = false;
        }
    }

    cli_match_free(&match);
    cli_split_free(&split);
    return ret;
}

static bool cli_is_quoting(minirl_st * const minirl, bool end)
{
	const char *text;
	unsigned i, point;
	bool quote = false;
	bool escape = false;

    text = minirl_line_get(minirl);
    point = end ? strlen(text) : minirl_point_get(minirl);

	for (i = 0; i < point; i++) {
		if (escape) {
			escape = false;
		} else if (text[i] == '\\') {
			escape = true;
		} else if (text[i] == '"') {
			quote = !quote;
		}
	}

	return quote || escape;
}

static bool
tab_handler(minirl_st * const minirl,
	    uint32_t * const flags,
	    char const * const key,
	    void * const user_ctx)
{
	if (cli_is_quoting(minirl, false)) {
		return minirl_insert_text(minirl, "\t");
	}

	if (!cli_complete(minirl, false)) {
		return true;
	}

	return minirl_insert_text(minirl, " ");
}

static bool space_handler(
    minirl_st * const minirl,
    uint32_t * const flags,
    char const * const key,
    void * const user_ctx)
{
	const char * const line = minirl_line_get(minirl);

    if (cli_is_quoting(minirl, false) || (line && *line == '#'))
        return minirl_insert_text(minirl, " ");

    if (!cli_complete(minirl, true))
    {
		return false;
    }

	return minirl_insert_text(minirl, " ");
}

static bool cli_enter(
    minirl_st * const minirl,
    uint32_t * const flags,
    char const * const key,
    void * const user_ctx)
{
	const char *line;
	minirl_point_set(minirl, minirl_end_get(minirl));

	if (cli_is_quoting(minirl, false))
		return minirl_insert_text(minirl, "\n");

    line = minirl_line_get(minirl);
	if (!line || !*line) {
        *flags |= minirl_key_handler_done;
		return false;
	}

	if (*line == '#') {
        *flags |= minirl_key_handler_done;
		return true;
	}

    *flags |= minirl_key_handler_done;
	return true;
}

static bool ctrl_right_handler(
    minirl_st * const minirl,
    uint32_t * const flags,
    char const * const key,
    void * const user_ctx)
{
    return true;
}

static bool ctrl_left_handler(
    minirl_st * const minirl,
    uint32_t * const flags,
    char const * const key,
    void * const user_ctx)
{
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

    minirl_st * minirl = minirl_new(stdin, output_fp);
    bool const multiline_mode = true;
    minirl_bind_key(minirl, TAB, tab_handler, NULL);
    minirl_bind_key(minirl, ' ', space_handler, NULL);
    minirl_bind_key(minirl, '\r', cli_enter, NULL);
    minirl_bind_key(minirl, '\n', cli_enter, NULL);
    minirl_bind_keyseq(minirl, ESCAPESTR "[1;5C", ctrl_right_handler, NULL);
    minirl_bind_keyseq(minirl, ESCAPESTR "[1;5D", ctrl_left_handler, NULL);

    fprintf(stdout, "'q' to quit\n");

    char * line;
    while ((line = minirl_readline(minirl, "prompt>")) != NULL)
    {
        fprintf(stdout, "\ngot line\n");
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
            update_history(minirl, line);
            run_command_line(line);
        }

        minirl_free(line);
    }

    minirl_delete(minirl);
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
