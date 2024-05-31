#include <stdio.h>

#define URLROUTER_IMPLEMENTATION
#include "urlrouter.h"

typedef void (*user_cb_t)(urlparam id);

void user(urlparam id)
{
	if (id.value)
		printf("New req with id: %.*s\n", id.len, id.value);
	else
		printf("New req without id\n");
}

int main(void)
{
	urlrouter router;
	char buff[1024 * 4]; // This buffer is enough to store ~50 routes
	urlrouter_init(&router, buff, 1024 * 4);
	urlrouter_add(&router, "/user/{id}/test", user);
	urlrouter_add(&router, "/user", user);

	urlparam params[10] = {0};
	user_cb_t cb = urlrouter_find(&router, "/user/168/test", params, 10, NULL);
	if (cb)
		cb(params[0]);
	else
		printf("Route not found\n");

	return 0;
}