#ifndef LIBCONFIG_SL_H
#define LIBCONFIG_SL_H

#include <alloca.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

char **_sl_new(const char *s, ...);
#define sl_new(s, args...) _sl_new(s, ##args, NULL)
char ** sl_copy(char **l);
char **sl_concat(char **l1, char **l2);
void sl_free(char **l);
int sl_len(char **l);
int sl_indexof(char **l, char *s);

char **sl_split(const char *s, char sep);
char **sl_split_lines(const char *s, bool keep);
char *sl_join(char **l, const char *sep);

bool sl_find(char **l, const char *s);
void sl_sort(char **l);
bool sl_equal(char **l1, char **l2);

char **_sl_append(char **l, ...);
#define sl_append(l, args...) _sl_append(l, ##args, NULL)
char **sl_append_len(char **l, const char *s, size_t len);
char **sl_append_printf(char **l, const char *fmt, ...);
char *sl_pop(char **l);
char *sl_remove(char **l);

#define sl_alloca(arg) ({					\
		typeof(arg) *_l;				\
		va_list _ap;					\
		int _n;						\
								\
		_n = 0;						\
		va_start(_ap, arg);				\
		do { ++_n;					\
		} while (va_arg(_ap, typeof(arg)));		\
		va_end(_ap);					\
		_l = alloca((_n + 1) * sizeof(*_l));		\
								\
		_n = 0;						\
		_l[0] = arg;					\
		va_start(_ap, arg);				\
		do { _l[++_n] = va_arg(_ap, typeof(arg));	\
		} while (_l[_n]);				\
		va_end(_ap);					\
		_l;						\
	})

#endif
