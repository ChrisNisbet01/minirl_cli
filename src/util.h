#ifndef LIBCONFIG_UTIL_H
#define LIBCONFIG_UTIL_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

void xvasprintf(char **strp, const char *fmt, va_list ap);
void xasprintf(char **strp, const char *fmt, ...);

#if 0
char *file_read(const char *name);
char *file_read_buf(const char *name, int *len);
void file_write(const char *name, const char *val);
bool file_write_buf(const char *name, const char *val, int len);
int file_printf(const char *name, const char *format, ...);
int file_cmp(const char *filename1, const char *filename2);
bool file_exists(const char *filename);
bool file_replace(bool do_cmp, const char *new,
	const char *current, const char *old);

char *stream_read(FILE *f);
char *stream_read_buf(FILE *f, int *len);
#endif

#endif
