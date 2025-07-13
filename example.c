#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "json.h"

int main(void) {
	struct json *j = json_parse(STR(" { \"some_key\" : { \"a\" : [ \"foo\" , \"bar\"]},\"another_key\":\"multi line\\nstring\" } "));
	assert(j != NULL);
	string_t string = json_stringify_pretty(j);

	printf("%.*s\n", PRSTR(string));

	assert(j->tag == JSON_OBJECT);
	struct json *x = json_object_get(&j->as.object, STR("another_key"));
	assert(x != NULL);
	assert(x->tag == JSON_STRING);
	printf("..%.*s..\n", PRSTR(x->as.string));

	free_string(string);
	json_free(j);
}
