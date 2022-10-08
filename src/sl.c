#include "sl.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

char **_sl_new(const char *s, ...)
{
	char **l;
	char *p;
	va_list ap;
	int n;

	if (!s) {
		l = malloc(sizeof(*l));
		*l = NULL;
		return l;
	}

	n = 0;
	va_start(ap, s);
	do {
		++n;
	} while (va_arg(ap, char *));
	va_end(ap);
	l = malloc((n + 1) * sizeof(*l));

	n = 0;
	l[0] = strdup(s);
	va_start(ap, s);
	do {
		p = va_arg(ap, char *);
		l[++n] = p ? strdup(p) : NULL;
	} while (p);
	va_end(ap);

	return l;
}

char ** sl_copy(char **s)
{
	char **l;
	int i, n;

	n = sl_len(s);
	l = malloc((n + 1) * sizeof(*l));
	for (i = 0; i < n; i++)
		l[i] = strdup(s[i]);
	l[n] = NULL;

	return l;
}

char **sl_concat(char **l1, char **l2)
{
	int n1, n2, n;
	char **l;

	n1 = sl_len(l1);
	n2 = sl_len(l2);
	n = n1 + n2;

	l = realloc(l1, (n + 1) * sizeof(*l));
	memcpy(l + n1, l2, (n2 + 1) * sizeof(*l));
	free(l2);

	return l;
}

void sl_free(char **l)
{
	char **s;

	if (!l)
		return;

	for (s = l; *s; s++)
		free(*s);
	free(l);
}

int sl_len(char **l)
{
	int i;

	if (!l)
		return 0;

	i = 0;
	while (*l++)
		i++;

	return i;
}

int sl_indexof(char **l, char *s)
{
	int i = 0;

	if (l && s) {
		while (*l) {
			if (!strcmp(*l, s))
				return i;
			l++;
			i++;
		}
	}
	return -1;
}

static char **sl_split_internal(const char *s, char sep, bool keep)
{
	const char *p, *q;
	char **l;
	int n;

	if (!s)
		return NULL;

	n = 0;
	p = s;
	do {
		p = strchr(p, sep);
		if (p)
			++p;
		++n;
	} while (p);
	l = malloc((n + 1) * sizeof(*l));

	n = 0;
	p = s;
	do {
		q = strchr(p, sep);
		if (q) {
			l[n] = strndup(p, q - p + (keep ? 1 : 0));
			++q;
		} else
			l[n] = strdup(p);
		p = q;
		++n;
	} while (p);
	l[n] = NULL;

	return l;
}

char **sl_split(const char *s, char sep)
{
	return sl_split_internal(s, sep, false);
}

char **sl_split_lines(const char *s, bool keep)
{
	return sl_split_internal(s, '\n', keep);
}

char *sl_join(char **l, const char *sep)
{
	char **p;
	char *s;
	char *t;
	int seplen = strlen(sep);
	int len = 0;

	if (!l || !*l)
		return strdup("");

	len = strlen(*l) + 1;
	for (p = l + 1; *p; p++)
		len += seplen + strlen(*p);
	s = malloc(len);

	t = s;
	len = strlen(*l);
	memcpy(t, *l, len);
	t += len;
	for (p = l + 1; *p; p++) {
		len = strlen(*p);
		memcpy(t, sep, seplen);
		t += seplen;
		memcpy(t, *p, len);
		t += len;
	}
	*t = '\0';

	return s;
}

bool sl_find(char **l, const char *s)
{
	char **p;

	if (!l)
		return false;

	for (p = l; *p; p++)
		if (strcmp(*p, s) == 0)
			return true;

	return false;
}

static int sl_strcmp(const void *a, const void *b)
{
	return strcmp(*(char **)a, *(char **)b);
}

void sl_sort(char **l)
{
	if (!l || !*l)
		return;
	qsort(l, sl_len(l), sizeof(*l), sl_strcmp);
}

bool sl_equal(char **l1, char **l2)
{
	char **p1, **p2;

	if (!l1)
		return !l2;
	if (!l2)
		return false;

	for (p1 = l1, p2 = l2; *p1 && *p2; p1++, p2++)
		if (strcmp(*p1, *p2) != 0)
			return false;
	return !*p1 && !*p2;
}

char **_sl_append(char **l, ...)
{
	char *p;
	va_list ap;
	int len;
	int n;

	len = sl_len(l);

	n = len;
	va_start(ap, l);
	do {
		n++;
	} while (va_arg(ap, char *));
	va_end(ap);
	l = realloc(l, n * sizeof(*l));

	n = len;
	va_start(ap, l);
	do {
		p = va_arg(ap, char *);
		l[n++] = p ? strdup(p) : NULL;
	} while (p);
	va_end(ap);

	return l;
}

char **sl_append_len(char **l, const char *s, size_t len)
{
	int n;

	n = sl_len(l) + 1;
	l = realloc(l, (n + 1) * sizeof(*l));
	l[n-1] = strndup(s, len);
	l[n] = NULL;

	return l;
}

char **sl_append_printf(char **l, const char *fmt, ...)
{
	va_list ap;
	char *s;
	int n;

	va_start(ap, fmt);
	xvasprintf(&s, fmt, ap);
	va_end(ap);

	n = sl_len(l) + 1;
	l = realloc(l, (n + 1) * sizeof(*l));
	l[n-1] = s;
	l[n] = NULL;

	return l;
}

char *sl_pop(char **l)
{
	char *s;
	int i;

	if (!l)
		return NULL;

	i = sl_len(l);
	if (!i)
		return NULL;

	i--;
	s = l[i];
	l[i] = NULL;
	return s;
}

char *sl_remove(char **l)
{
	char *s;
	int i;

	if (!l)
		return NULL;

	i = sl_len(l);
	if (!i)
		return NULL;

	s = l[0];
	memmove(l, l + 1, i * sizeof(*l));
	return s;
}
