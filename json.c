#include <assert.h>
#include <err.h>
#include <stdio.h>
#include "json.h"

struct json_member {
	string_t key;
	struct json value;
};

static void free_object(struct json_object *obj);
static int parse_element(str_t *s, struct json *j);
static void free_json_inner(struct json *j);
static void free_array(struct json_array *arr);
static void stringify_element(string_t *s, int depth, const struct json *j);

static _Bool isws(char ch) {
	return ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t';
}

static void skip_space(str_t *s) {
	while (s->len && isws(s->ptr[0])) {
		s->ptr++;
		s->len--;
	}
}

static char shift(str_t *s) {
	char ret;

	if (s->len == 0)
		errx(1, "BUG: tried to shift an empty string");

	ret = s->ptr[0];
	s->len -= 1;
	s->ptr += 1;

	return ret;
}

static int unescape(str_t *s, char *ch) {
	if (s->len == 0)
		return 1;

	switch (shift(s)) {
	case '"':
		*ch = '"';
		break;
	case '\\':
		*ch = '\\';
		break;
	case '/':
		*ch = '/';
		break;
	case 'b':
		*ch = '\b';
		break;
	case 'f':
		*ch = '\f';
		break;
	case 'n':
		*ch = '\n';
		break;
	case 'r':
		*ch = '\r';
		break;
	case 't':
		*ch = '\t';
		break;
	case 'u':
		if (s->len < 4)
			return 1;
		errx(1, "TODO: support \\u 4HEXDIG");
		break;
	default:
		return 1;
	}

	return 0;
}

static int parse_char(str_t *s, char ch) {
	if (s->len && s->ptr[0] == ch) {
		s->len -= 1;
		s->ptr += 1;
		return 0;
	}
	return 1;
}

static int parse_string(str_t *s, string_t *string) {
	string->ptr = NULL;
	string->len = 0;
	string->cap = 0;

	if (parse_char(s, '"'))
		return 1;

	while (s->len && s->ptr[0] != '"') {
		char ch = shift(s);

		if (ch == '\\' && unescape(s, &ch))
			goto err_free_string;

		string_append_char(string, ch);
	}

	if (parse_char(s, '"'))
		goto err_free_string;

	return 0;
err_free_string:
	free_string(*string);
	return 1;
}

static int parse_member(str_t *s, struct json_member *m) {
	if (parse_string(s, &m->key))
		return 1;

	skip_space(s);

	if (parse_char(s, ':'))
		goto err_free_string;

	if (parse_element(s, &m->value))
		goto err_free_string;

	return 0;
err_free_string:
	free(m->key.ptr);
	return 1;
}

static int parse_members(str_t *s, struct json_object *obj) {
	assert(s->len != 0);
	obj->members = NULL;
	obj->members_len = 0;

	do {
		obj->members = realloc(obj->members, sizeof(obj->members[0]) * (obj->members_len + 1));
		if (parse_member(s, obj->members + obj->members_len))
			goto err_free_object;
		obj->members_len += 1;
	} while (parse_char(s, ',') == 0);

	return 0;
err_free_object:
	free_object(obj);
	return 1;
}

static int parse_object(str_t *s, struct json_object *obj) {
	if (parse_char(s, '{'))
		return 1;

	skip_space(s);

	if (s->len && s->ptr[0] != '}' && parse_members(s, obj))
		return 1;

	if (parse_char(s, '}'))
		goto err_free_object;

	return 0;
err_free_object:
	free_object(obj);
	return 1;
}

static int parse_elements(str_t *s, struct json_array *arr) {
	assert(s->len != 0);
	arr->items = NULL;
	arr->len = 0;

	do {
		arr->items = realloc(arr->items, sizeof(arr->items[0]) * (arr->len + 1));
		if (parse_element(s, arr->items + arr->len))
			goto err_free_array;
		arr->len += 1;
	} while (parse_char(s, ',') == 0);

	return 0;
err_free_array:
	free_array(arr);
	return 1;
}

static int parse_array(str_t *s, struct json_array *arr) {
	if (parse_char(s, '['))
		return 1;

	skip_space(s);

	if (s->len && s->ptr[0] != ']' && parse_elements(s, arr))
		return 1;

	if (parse_char(s, ']'))
		goto err_free_array;

	return 0;
err_free_array:
	free_array(arr);
	return 1;
}

static int parse_number(str_t *s, double *num) {
	int sign = 1;
	if (parse_char(s, '-') == 0)
		sign = -1;

	errx(1, "TODO: parse numbers");(void)num;(void)sign;
}

static int parse_element(str_t *s, struct json *j) {
	skip_space(s);

	if (s->len == 0)
		return 1;

	switch (s->ptr[0]) {
	case '{':
		if (parse_object(s, &j->as.object))
			return 1;
		j->tag = JSON_OBJECT;
		break;
	case '[':
		if (parse_array(s, &j->as.array))
			return 1;
		j->tag = JSON_ARRAY;
		break;
	case '"':
		if (parse_string(s, &j->as.string))
			return 1;
		j->tag = JSON_STRING;
		break;
	case 't':
		if (parse_char(s, 't')
			|| parse_char(s, 'r')
			|| parse_char(s, 'u')
			|| parse_char(s, 'e'))
				return 1;
		j->tag = JSON_BOOL;
		j->as.bool = 1;
		break;
	case 'f':
		if (parse_char(s, 'f')
			|| parse_char(s, 'a')
			|| parse_char(s, 'l')
			|| parse_char(s, 's')
			|| parse_char(s, 'e'))
				return 1;
		j->tag = JSON_BOOL;
		j->as.bool = 0;
		break;
	case 'n':
		if (parse_char(s, 'n')
			|| parse_char(s, 'u')
			|| parse_char(s, 'l')
			|| parse_char(s, 'l'))
				return 1;
		j->tag = JSON_NULL;
		break;
	case '-':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		if (parse_number(s, &j->as.number))
			return 1;
		j->tag = JSON_NUMBER;
		break;
	}

	skip_space(s);

	return 0;
}

struct json *json_parse(str_t s) {
	struct json *j = malloc(sizeof(*j));

	if (parse_element(&s, j)) {
		free(j);
		return NULL;
	}

	if (s.len != 0) {
		json_free(j);
		return NULL;
	}

	return j;
}

static void free_object(struct json_object *obj) {
	for (size_t i = 0; i < obj->members_len; i++) {
		free(obj->members[i].key.ptr);
		free_json_inner(&obj->members[i].value);
	}
	free(obj->members);

	obj->members = NULL;
	obj->members_len = 0;
}

static void free_array(struct json_array *arr) {
	for (size_t i = 0; i < arr->len; i++)
		free_json_inner(&arr->items[i]);
	free(arr->items);

	arr->len = 0;
	arr->items = NULL;
}

static void free_json_inner(struct json *j) {
	if (j == NULL)
		return;

	switch (j->tag) {
	case JSON_OBJECT:
		free_object(&j->as.object);
		break;
	case JSON_ARRAY:
		free_array(&j->as.array);
		break;
	case JSON_STRING:
		free_string(j->as.string);
		break;
	case JSON_NUMBER:
	case JSON_BOOL:
	case JSON_NULL:
		break;
	}
}

void json_free(struct json *j) {
	free_json_inner(j);
	free(j);
}

static void stringify_string_char(string_t *s, char ch) {
	switch (ch) {
	case '"':
		string_append(s, STR("\\\""));
		return;
	case '\\':
		string_append(s, STR("\\\\"));
		return;
	case '\b':
		string_append(s, STR("\\b"));
		return;
	case '\f':
		string_append(s, STR("\\f"));
		return;
	case '\n':
		string_append(s, STR("\\n"));
		return;
	case '\r':
		string_append(s, STR("\\r"));
		return;
	case '\t':
		string_append(s, STR("\\t"));
		return;
	}

	if (ch > 0x1f) {
		string_append_char(s, ch);
		return;
	}

	char ubuf[3];
	snprintf(ubuf, sizeof(ubuf), "%02x", (int)ch);

	str_t ustr = { .ptr = ubuf, .len = 2 };
	string_append(s, STR("\\u00"));
	string_append(s, ustr);
}

static void stringify_string(string_t *s, str_t val) {
	string_append_char(s, '"');
	for (size_t i = 0; i < val.len; i++)
		stringify_string_char(s, val.ptr[i]);
	string_append_char(s, '"');
}

static void indent(string_t *s, int n) {
	while (n--)
		string_append_char(s, '\t');
}

static void stringify_array(string_t *s, int depth, const struct json_array *arr) {
	string_append_char(s, '[');
	if (arr->len)
		string_append_char(s, '\n');

	for (size_t i = 0; i < arr->len; i++) {
		indent(s, depth + 1);
		stringify_element(s, depth + 1, &arr->items[i]);
		if (i != arr->len - 1)
			string_append_char(s, ',');
		string_append_char(s, '\n');
	}

	if (arr->len)
		indent(s, depth);
	string_append_char(s, ']');
}

static void stringify_object(string_t *s, int depth, const struct json_object *obj) {
	string_append_char(s, '{');
	if (obj->members_len)
		string_append_char(s, '\n');

	for (size_t i = 0; i < obj->members_len; i++) {
		const struct json_member *m = &obj->members[i];
		indent(s, depth + 1);
		stringify_string(s, tostr(m->key));
		string_append(s, STR(": "));
		stringify_element(s, depth + 1, &m->value);
		if (i != obj->members_len - 1)
			string_append_char(s, ',');
		string_append_char(s, '\n');
	}

	if (obj->members_len)
		indent(s, depth);
	string_append_char(s, '}');
}

static void stringify_number(string_t *s, double val) {
	errx(1, "TODO: stringify numbers"); (void)s;(void)val;
}

static void stringify_element(string_t *s, int depth, const struct json *j) {
	switch (j->tag) {
	case JSON_OBJECT:
		stringify_object(s, depth, &j->as.object);
		break;
	case JSON_ARRAY:
		stringify_array(s, depth, &j->as.array);
		break;
	case JSON_STRING:
		stringify_string(s, tostr(j->as.string));
		break;
	case JSON_NUMBER:
		stringify_number(s, j->as.number);
		break;
	case JSON_BOOL:
		if (j->as.bool)
			string_append(s, STR("true"));
		else
			string_append(s, STR("false"));
		break;
	case JSON_NULL:
		string_append(s, STR("null"));
		break;
	}
}

// caller frees returned string
string_t json_stringify_pretty(const struct json *j) {
	string_t s = { 0 };
	stringify_element(&s, 0, j);
	return s;
}

struct json *json_object_get(struct json_object *obj, str_t key) {
	for (size_t i = 0; i < obj->members_len; i++) {
		if (str_eq(key, tostr(obj->members[i].key)))
			return &obj->members[i].value;
	}
	return NULL;
}
