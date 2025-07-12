#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "json.h"

int main(void) {
	struct json *j = json_parse(STR(" { \"some_key\" : { \"a\" : [ \"foo\" , \"bar\"]},\"another_key\":\"stringystring\" } "));
	assert(j != NULL);
	string_t string = json_stringify_pretty(j);

	printf("%.*s", PRSTR(string));

	free_string(string);
	json_free(j);
}
