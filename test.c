
#define URLROUTER_IMPLEMENTATION
#define URLROUTER_IO
#include "urlrouter.h"

#include <assert.h>
#define BUF_SIZE 1024 * 16

int main(void)
{
	char data[BUF_SIZE];
	urlrouter router;
	urlrouter_init(&router, data, BUF_SIZE);
	// urlrouter_add(&router, "/a/b/c", "/a/b/c");
	urlrouter_add(&router, "/a/b/c/e/ef", "/a/b/c/e/ef");
	// urlrouter_add(&router, "/blabl", "/blabl");
	// urlrouter_add(&router, "/blabl/7", "/blabl/7");
	// urlrouter_add(&router, "/blabl/14", "/blabl/14");
	// urlrouter_add(&router, "/blabl/14", "/blabl/14");
	// urlrouter_add(&router, "/blabl/28", "/blabl/28");
	urlrouter_add(&router, "/azd/{i}", "/azd/{id}");
	urlrouter_add(&router, "/azd/{id/azd}/edit", "/azd/{id}/edit");
	urlrouter_print(&router);

	urlrouter_add(&router, "/cmd/{tool}/{sub}", "/cmd/{tool}/{sub}"); // /a/b
	urlrouter_add(&router, "/cmd/vet", "/cmd/vet");
	urlrouter_add(&router, "/foo/bar", "/foo/bar");
	urlrouter_add(&router, "/foo/{name}", "/foo/{name}");
	urlrouter_add(&router, "/foo/{names}", "/foo/{names}");

	assert(urlrouter_add(&router, "/cmd/{*path}", "/cmd/{*path}") == URLROUTER_ERR_MALFORMED_PATH); // NOT SUPPORTED
	urlrouter_add(&router, "/cmd/{xxx}/names", "/cmd/{xxx}/names");
	urlrouter_add(&router, "/cmd/{tool}/{xxx}/foo", "/cmd/{tool}/{xxx}/foo");
	urlrouter_add(&router, "/src/{*filepath}", "/src/{*filepath}");
	urlrouter_add(&router, "/src/{file}", "/src/{file}");
	urlrouter_add(&router, "/src/static.json", "/src/static.json");
	urlrouter_add(&router, "/src/$filepathx", "/src/$filepathx");
	urlrouter_add(&router, "/src/", "/src/");
	urlrouter_add(&router, "/src/foo/bar", "/src/foo/bar");
	urlrouter_add(&router, "/src1/", "/src1/");
	urlrouter_add(&router, "/src1/{*filepath}", "/src1/{*filepath}");
	urlrouter_add(&router, "/src2{*filepath}", "/src2{*filepath}");
	urlrouter_add(&router, "/src2/{*filepath}", "/src2/{*filepath}");
	urlrouter_add(&router, "/src2/", "/src2/");
	urlrouter_add(&router, "/src2", "/src2");
	urlrouter_add(&router, "/src3", "/src3");
	urlrouter_add(&router, "/src3/{*filepath}", "/src3/{*filepath}");
	urlrouter_print(&router);
	urlrouter_add(&router, "/search/{query}", "/search/{query}");
	urlrouter_print(&router);
	urlrouter_add(&router, "/search/valid", "/search/valid");
	urlrouter_print(&router);
	urlrouter_add(&router, "/user_{name}", "/user_{name}");
	urlrouter_add(&router, "/user_x", "/user_x");
	// assert(urlrouter_add(&router, "/user_{bar}", "/user_{bar}") == URLROUTER_ERR_PATH_EXISTS);
	urlrouter_add(&router, "/id{id}", "/id{id}");
	int res = urlrouter_add(&router, "/id/{id}", "/id/{id}");
	if (res < 0)
		printf("Error adding route %d\n", res);
	urlrouter_print(&router);

	// const char *route = urlrouter_find(&router, "/blabl/28");
	// if (route)
	// 	printf("Found route: %s\n", route);
	// else
	// 	printf("Route not found\n");
	return 0;
}