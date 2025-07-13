#ifndef __STR_H
#define __STR_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define PRSTR(STR) (int)(STR).len, (STR).ptr
#define STR(LIT) (str_t){ .ptr = ""LIT"", .len = sizeof(LIT) - 1 }

#define __STRING_INITIAL_CAP 8

typedef struct {
	const char *ptr;
	size_t len;
} str_t;

typedef struct {
	char *ptr;
	size_t len;
	size_t cap;
} string_t;

static inline void free_string(string_t s) {
	free((void *)s.ptr);
}

static inline str_t tostr(string_t s) {
	str_t ret = { .ptr = s.ptr, .len = s.len };
	return ret;
}

static inline void string_grow(string_t *s, size_t needed) {
	if (s->cap >= needed)
		return;

	size_t newcap = s->cap * 2;
	if (newcap == 0)
		newcap = __STRING_INITIAL_CAP;

	if (newcap < needed)
		newcap = needed;
	s->cap = newcap;
	s->ptr = realloc(s->ptr, newcap);
}

static inline void string_append(string_t *a, str_t b) {
	if (b.len == 0)
		return;
	string_grow(a, a->len + b.len);
	memcpy(a->ptr + a->len, b.ptr, b.len);
	a->len += b.len;
}

static inline void string_append_char(string_t *a, char ch) {
	str_t str = { .ptr = &ch, .len = 1 };
	string_append(a, str);
}

static inline _Bool str_eq(str_t a, str_t b) {
	if (a.len != b.len)
		return 0;

	for (size_t i = 0; i < a.len; i++) {
		if (a.ptr[i] != b.ptr[i])
			return 0;
	}

	return 1;
}

#endif
