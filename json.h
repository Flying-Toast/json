#ifndef __JSON_H
#define __JSON_H

#include "str.h"

struct json;

enum json_kind {
	JSON_OBJECT,
	JSON_ARRAY,
	JSON_STRING,
	JSON_NUMBER,
	JSON_BOOL,
	JSON_NULL,
};

struct json_member;

struct json_object {
	size_t members_len;
	struct json_member *members;
};

struct json_array {
	size_t len;
	struct json *items;
};

struct json {
	enum json_kind tag;
	union {
		struct json_object object;
		struct json_array array;
		string_t string;
		double number;
		_Bool bool;
	} as;
};

struct json *json_parse(str_t s);
void json_free(struct json *j);
string_t json_stringify_pretty(const struct json *j);
struct json *json_object_get(struct json_object *obj, str_t key);

#endif
