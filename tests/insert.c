#define URLROUTER_ASSERT
#define URLROUTER_IO
#include "urlrouter.h"

#include <stdio.h>
#include <stdlib.h>

static int RES_OK = 0;
static int RES_ERR_PATH_EXIST = URLROUTER_ERR_PATH_EXISTS;
static int RES_ERR_MALFORMED_PATH = URLROUTER_ERR_MALFORMED_PATH;
static const char *ERRS[] = {
	"OK",
	"PATH_EXISTS",
	"BUFF_FULL",
	"MALFORMED_PATH",
};

#define INSERT_TEST(name, ...)                                                                     \
	static void name(void)                                                                         \
	{                                                                                              \
		printf("\nTesting %s:\n", #name);                                                          \
		const void *routes[] = {__VA_ARGS__};                                                      \
		const size_t n = sizeof(routes) / sizeof(routes[0]);                                       \
		urlrouter router = {0};                                                                    \
		char buf[1024 * 4] = {0};                                                                  \
		urlrouter_init(&router, buf, 1024 * 4);                                                    \
		for (size_t i = 0; i < n; i += 2)                                                          \
		{                                                                                          \
			const char *route = (const char *)routes[i];                                           \
			const unsigned int expected = *(unsigned int *)routes[i + 1];                          \
			printf("\tAdding route: %s\n", route);                                                 \
			const int res = urlrouter_add(&router, route, 1);                                      \
			const unsigned int err = res < 0 ? res : 0;                                            \
			if (expected != err)                                                                   \
			{                                                                                      \
				urlrouter_print(&router);                                                          \
				fprintf(stderr, "Test %s failed: %s, expected %s, found: %s\n", #name, route,      \
						ERRS[-expected], ERRS[-err]);                                              \
				exit(1);                                                                           \
			}                                                                                      \
		}                                                                                          \
		urlrouter_print(&router);                                                                  \
	}

// clang-format off

INSERT_TEST(wildcard_conflict,
			"/cmd/{tool}/{sub}", 					&RES_OK,
			"/cmd/vet", 							&RES_OK,
			"/foo/bar", 							&RES_OK,
			"/foo/{name}", 							&RES_OK,
			"/foo/{names}", 						&RES_ERR_PATH_EXIST,
			"/cmd/{xxx}/names", 					&RES_OK,
			"/cmd/{tool}/{xxx}/foo", 				&RES_OK,
			"/src/{file}", 							&RES_OK,
			"/src/{files}", 						&RES_ERR_PATH_EXIST,
			"/src/static.json",						&RES_OK,
			"/src/$filepathx", 						&RES_OK,
			"/src/", 								&RES_OK,
			"/src/foo/bar", 						&RES_OK,
			"/src1/", 								&RES_OK,
			"/src2/", 								&RES_OK,
			"/src2", 								&RES_OK,
			"/src3", 								&RES_OK,
			"/search/{query}", 						&RES_OK,
			"/search/valid", 						&RES_OK,
			"/user_{name}", 						&RES_OK,
			"/user_x", 								&RES_OK,
			"/user_{bar}", 							&RES_ERR_PATH_EXIST,
			"/id{id}", 								&RES_OK,
			"/id/{id}", 							&RES_OK,
)


INSERT_TEST(child_conflict,
	"/cmd/vet", 									&RES_OK,
	"/cmd/{tool}", 									&RES_OK,
	"/cmd/{tool}/{sub}", 							&RES_OK,
	"/cmd/{tool}/misc", 							&RES_OK,
	"/cmd/{tool}/{bad}", 							&RES_ERR_PATH_EXIST,
	"/src/AUTHORS", 								&RES_OK,
	"/user_x", 										&RES_OK,
	"/user_{name}", 								&RES_OK,
	"/id/{id}", 									&RES_OK,
	"/id{id}", 										&RES_OK,
	"/{id}", 										&RES_OK,
)

INSERT_TEST(duplicates,
	"/", 											&RES_OK,
	"/", 											&RES_ERR_PATH_EXIST,
	"/doc/", 										&RES_OK,
	"/doc/", 										&RES_ERR_PATH_EXIST,
	"/search/{query}", 								&RES_OK,
	"/search/{query}", 								&RES_ERR_PATH_EXIST,
	"/user_{name}", 								&RES_OK,
	"/user_{name}", 								&RES_ERR_PATH_EXIST,
)

INSERT_TEST(unnamed_param,
	"/{}", 											&RES_ERR_MALFORMED_PATH,
	"/user{}", 										&RES_ERR_MALFORMED_PATH,
	"/cmd{}", 										&RES_ERR_MALFORMED_PATH,
	"/src{*}", 										&RES_ERR_MALFORMED_PATH,
)

INSERT_TEST(normalized_conflict,
	"/x/{foo}/bar", 								&RES_OK,
	"/x/{bar}/bar", 								&RES_ERR_PATH_EXIST,
	"/{y}/bar/baz", 								&RES_OK,
	"/{y}/baz/baz", 								&RES_OK,
	"/{z}/bar/bat", 								&RES_OK,
	"/{z}/bar/baz", 								&RES_ERR_PATH_EXIST,
)

INSERT_TEST(more_conflicts,
	"/con{tact}", 									&RES_OK,
	"/who/foo/hello", 								&RES_OK,
	"/whose/{users}/{name}", 						&RES_OK,
	"/who/are/foo", 								&RES_OK,
	"/who/are/foo/bar", 							&RES_OK,
	"/con{nection}", 								&RES_ERR_PATH_EXIST,
	"/whose/{users}/{user}", 						&RES_ERR_PATH_EXIST
)

INSERT_TEST(duplicate_conflict,
	"/hey", 										&RES_OK,
	"/hey/users", 									&RES_OK,
	"/hey/user", 									&RES_OK,
	"/hey/user", 									&RES_ERR_PATH_EXIST,
)

INSERT_TEST(invalid_param,
	"{", 											&RES_ERR_MALFORMED_PATH,
	"}", 											&RES_ERR_MALFORMED_PATH,
	"x{y", 											&RES_ERR_MALFORMED_PATH,
	"x}", 											&RES_ERR_MALFORMED_PATH,
	"/{foo}s", 										&RES_ERR_MALFORMED_PATH,
)

INSERT_TEST(escaped_param,
	"{{", 											&RES_OK,
	"}}", 											&RES_OK,
	"xx}}", 										&RES_OK,
	"}}yy", 										&RES_OK,
	"}}yy{{}}", 									&RES_OK,
	"}}yy{{}}{{}}y{{", 								&RES_OK,
	"}}yy{{}}{{}}y{{", 								&RES_ERR_PATH_EXIST,
	"/{{yy", 										&RES_OK,
	"/{yy}", 										&RES_OK,
	"/foo", 										&RES_OK,
	"/foo/{{", 										&RES_OK,
	"/foo/{{/{x}", 									&RES_OK,
	"/foo/{ba{{r}", 								&RES_OK,
	"/bar/{ba}}r}", 								&RES_OK,
	"/xxx/{x{{}}y}", 								&RES_OK,
)

INSERT_TEST(double_params,
	"/{foo}{bar}", 									&RES_ERR_MALFORMED_PATH,
	"/{foo}{bar}/", 								&RES_ERR_MALFORMED_PATH,
	// {{ is an escape for {, leaving *bar} with an unmatched }
	"/{foo}{{*bar}/", 								&RES_ERR_MALFORMED_PATH,
)

// Routes without a leading slash are valid (matchit: missing_leading_slash_conflict)
INSERT_TEST(no_leading_slash,
	"foo",											&RES_OK,
	"foo/",											&RES_OK,
	"foo",											&RES_ERR_PATH_EXIST,
	"{foo}/",										&RES_OK,
	"{bar}/",										&RES_ERR_PATH_EXIST, // normalizes same as {foo}/
)

// Trailing slash creates a distinct route from the same path without it
INSERT_TEST(trailing_slash,
	"/a/{x}",										&RES_OK,
	"/a/{x}/",										&RES_OK,   // different: trailing slash
	"/a/{y}",										&RES_ERR_PATH_EXIST,
	"/a/{y}/",										&RES_ERR_PATH_EXIST,
	"/b",											&RES_OK,
	"/b/",											&RES_OK,
	"/b",											&RES_ERR_PATH_EXIST,
	"/b/",											&RES_ERR_PATH_EXIST,
)

// Conflicts between routes with a static prefix before a parameter
// (matchit: prefix_suffix_conflict, suffix cases excluded — we reject suffix-after-param)
INSERT_TEST(prefix_param_conflict,
	// Pure param and prefix+param at the same level are distinct (different first char)
	"/a/{x}",										&RES_OK,
	"/a/prefix{x}",									&RES_OK,
	// Same prefix, different param name: conflict (normalizes the same)
	"/a/prefix{y}",									&RES_ERR_PATH_EXIST,
	// Different prefix: no conflict
	"/a/other{x}",									&RES_OK,
	"/a/other{y}",									&RES_ERR_PATH_EXIST,
	// Suffix after param is invalid
	"/a/{x}y",										&RES_ERR_MALFORMED_PATH,
	// Multi-segment with and without prefix
	"/b/{x}/end",									&RES_OK,
	"/b/{y}/end",									&RES_ERR_PATH_EXIST,
	"/b/prefix{x}/end",								&RES_OK,
	"/b/prefix{y}/end",								&RES_ERR_PATH_EXIST,
	// prefix+param and pure param in the same segment don't conflict
	"/c/prefix{x}",									&RES_OK,
	"/c/{x}",										&RES_OK,
)

int main(void)
{
	wildcard_conflict();
	child_conflict();
	duplicates();
	unnamed_param();
	normalized_conflict();
	more_conflicts();
	duplicate_conflict();
	invalid_param();
	escaped_param();
	double_params();
	no_leading_slash();
	trailing_slash();
	prefix_param_conflict();

	return 0;
}
