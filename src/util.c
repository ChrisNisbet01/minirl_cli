#include "util.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#if 0
#include "log.h"
#endif

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

void xvasprintf(char **strp, const char *fmt, va_list ap)
{
	int ret;

	ret = vasprintf(strp, fmt, ap);
	assert(ret >= 0);
}

void xasprintf(char **strp, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	xvasprintf(strp, fmt, ap);
	va_end(ap);
}

#if 0
char *file_read(const char *name)
{
	int len;
	return file_read_buf(name, &len);
}

char *file_read_buf(const char *name, int *len)
{
	FILE *f = NULL;
	char *buf = NULL;

	f = fopen(name, "rb");
	if (!f) {
		log_error("unable to open %s: %m", name);
		goto error;
	}

	if (fseek(f, 0, SEEK_END) == -1) {
		log_error("failed to seek to end of %s: %m", name);
		goto error;
	}
	*len = ftell(f);
	if (*len == -1) {
		log_error("failed to read size of file %s: %m", name);
		goto error;
	}
	if (fseek(f, 0, SEEK_SET) == -1) {
		log_error("failed to seek to start of %s: %m", name);
		goto error;
	}

	buf = malloc(*len + 1);
	if (!buf) {
		log_error("failed to allocate buffer for %s: %m", name);
		goto error;
	}

	if (fread(buf, *len, 1, f) != 1) {
		log_error("failed to read from %s: %m", name);
		goto error;
	}
	fclose(f);

	buf[*len] = 0;
	return buf;

error:
	if (f)
		fclose(f);
	free(buf);
	return NULL;
}

void file_write(const char *name, const char *val)
{
	file_write_buf(name, val, strlen(val));
}

bool file_write_buf(const char *name, const char *val, int len)
{
	FILE *f;
	size_t n;

	f = fopen(name, "wb");
	if (!f) {
		log_error("unable to open %s: %m", name);
		return false;
	}

	n = fwrite(val, len, 1, f);
	fclose(f);
	return n == 1;
}

int file_printf(const char *name, const char *format, ...)
{
	FILE *f;
	va_list arg;
	int done;

	f = fopen(name, "w");
	if (!f) {
		log_error("unable to open %s: %m", name);
		return -1;
	}

	va_start(arg, format);
	done = vfprintf(f, format, arg);
	va_end (arg);

	fclose(f);
	return done;
}

int file_cmp(const char *filename1, const char *filename2)
{
	struct stat st1, st2;
	void *data1, *data2;
	int fd1, fd2;
	int ret1, ret2;

	fd1 = open(filename1, O_RDONLY);
	fd2 = open(filename2, O_RDONLY);
	if (fd1 == -1 && fd2 == -1) {
		return 0;
	} else if (fd1 == -1) {
		close(fd2);
		return -1;
	} else if (fd2 == -1) {
		close(fd1);
		return -1;
	}

	ret1 = fstat(fd1, &st1);
	ret2 = fstat(fd2, &st2);
	if (ret1 == -1 || ret2 == -1 || st1.st_size != st2.st_size) {
		close(fd1);
		close(fd2);
		return -1;
	}
	if (st1.st_size == 0) {
		close(fd1);
		close(fd2);
		return 0;
	}

	data1 = mmap(NULL, st1.st_size, PROT_READ, MAP_PRIVATE, fd1, 0);
	data2 = mmap(NULL, st2.st_size, PROT_READ, MAP_PRIVATE, fd2, 0);
	if (data1 == MAP_FAILED || data2 == MAP_FAILED) {
		if (data1 != MAP_FAILED) munmap(data1, st1.st_size);
		if (data2 != MAP_FAILED) munmap(data2, st2.st_size);
		close(fd1);
		close(fd2);
		return -1;
	}

	ret1 = memcmp(data1, data2, st1.st_size);

	munmap(data1, st1.st_size);
	munmap(data2, st2.st_size);
	close(fd1);
	close(fd2);

	return ret1;
}

bool file_exists(const char *filename)
{
	struct stat st;
	return stat(filename, &st) != -1 ? S_ISREG(st.st_mode) : false;
}

bool file_replace(bool do_cmp, const char *new,
	const char *current, const char *old)
{
	if (do_cmp && file_cmp(current, new) == 0) {
		if (file_exists(new) && unlink(new) == -1)
			log_error("unable to unlink %s: %m", new);
		return false;
	}

	if (old)
		rename(current, old);
	if (file_exists(new) && rename(new, current) == -1)
		log_error("unable to rename %s to %s: %m", new, current);
	return true;
}

char *stream_read(FILE *f)
{
	int len;
	return stream_read_buf(f, &len);
}

char *stream_read_buf(FILE *f, int *len)
{
	char buffer[128];
	size_t read_len;
	char *data = NULL;
	*len = 0;

	while ((read_len = fread(buffer, sizeof *buffer, ARRAY_SIZE(buffer), f))) {
		read_len *= sizeof *buffer;
		data = realloc(data, *len + read_len + 1);
		memcpy(data + *len, buffer, read_len);
		*len += read_len;
		if (read_len < sizeof buffer)
			break;
	}

	if (!feof(f)) {
		free(data);
		data = NULL;
		*len = 0;
	}

	if (data)
		data[*len] = '\0';

	return data;
}
#endif

