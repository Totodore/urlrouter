#define _POSIX_C_SOURCE 199309L

#define URLROUTER_IMPLEMENTATION
#define URLROUTER_IO
#define URLROUTER_ASSERT
#include "urlrouter.h"

#include <assert.h>
#include <time.h>

#define BUF_SIZE 1024 * 16

int main(void)
{
	char data[BUF_SIZE];
	urlrouter router;
	urlrouter_init(&router, data, BUF_SIZE);
	urlrouter_add(&router, "zqd/{azd}/test", "zqd/{azd}/test");
	urlrouter_add(&router, "/a/b/c", "/a/b/c");
	urlrouter_add(&router, "/a/b/c/e/ef", "/a/b/c/e/ef");
	urlrouter_add(&router, "/blabl", "/blabl");
	urlrouter_add(&router, "/blabl/7", "/blabl/7");
	urlrouter_add(&router, "/blabl/14", "/blabl/14");
	urlrouter_add(&router, "/blabl/14", "/blabl/14");
	urlrouter_add(&router, "/blabl/28", "/blabl/28");
	urlrouter_add(&router, "/azd/{i}", "/azd/{id}");
	urlrouter_add(&router, "/azd/{id}/edit", "/azd/{id}/edit");
	urlrouter_add(&router, "/azd/{id/azd}/edit", "/azd/{id}/edit");

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
	// urlrouter_add(&router, "/src2{*filepath}", "/src2{*filepath}");
	// urlrouter_add(&router, "/src2/{*filepath}", "/src2/{*filepath}");
	urlrouter_add(&router, "/src2/", "/src2/");
	urlrouter_add(&router, "/src2", "/src2");
	urlrouter_add(&router, "/src3", "/src3");
	urlrouter_add(&router, "/src3/{*filepath}", "/src3/{*filepath}");
	urlrouter_add(&router, "/search/{query}", "/search/{query}");
	urlrouter_add(&router, "/search/valid", "/search/valid");
	urlrouter_add(&router, "/user_{name}", "/user_{name}");
	urlrouter_add(&router, "/user_x", "/user_x");
	// assert(urlrouter_add(&router, "/user_{bar}", "/user_{bar}") == URLROUTER_ERR_PATH_EXISTS);
	urlrouter_add(&router, "/id{id}", "/id{id}");
	urlrouter_add(&router, "/id/{id}", "/id/{id}");
	urlrouter_add(&router, "/src/foo/{bar}", "/src/foo/{bar}");
	urlrouter_print(&router);

	urlrouter_print(&router);

	urlparam params[5] = {0};
	unsigned int param_cnt = 0;

	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);
	const char *route = urlrouter_find(&router, "/azd/azdoinazdoin/edit", params, 5, &param_cnt);
	clock_gettime(CLOCK_MONOTONIC, &end);

	if (route)
		printf("Found route: %s, params: [", route);
	else
		printf("Route not found\n");

	for (unsigned int i = 0; i < param_cnt; i++)
		printf("%.*s,", params[i].len, params[i].value);
	printf("]\n");

	// double t_ns = (double)(end.tv_sec - start.tv_sec) * 1.0e9 +
	//   (double)(end.tv_nsec - start.tv_nsec);
	printf("time: %ldns\n", (end.tv_nsec - start.tv_nsec));

	return 0;
}