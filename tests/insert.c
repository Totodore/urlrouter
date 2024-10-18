#define URLROUTER_ASSERT
#define URLROUTER_IO
#include "urlrouter.h"

#include <stdio.h>
#include <stdlib.h>

#define INSERT_TEST(name, ...)                                                                     \
	static void name(void)                                                                         \
	{                                                                                              \
		const void *routes[] = {__VA_ARGS__};                                                      \
		const size_t n = sizeof(routes) / sizeof(routes[0]);                                       \
		urlrouter router = {0};                                                                    \
		char buf[1024 * 4] = {0};                                                                  \
		urlrouter_init(&router, buf, 1024 * 4);                                                    \
		for (size_t i = 0; i < n; i += 2)                                                          \
		{                                                                                          \
			const char *route = (const char *)routes[i];                                           \
			const int expected = (int)routes[i + 1];                                               \
			printf("Adding route: %s\n", route);                                                   \
			const int err = urlrouter_add(&router, route, NULL);                                   \
			if (expected != err && expected < 0)                                                   \
			{                                                                                      \
				fprintf(stderr, "Test %s failed: %s, expected %d, found: %d\n", #name, route,      \
						expected, err);                                                            \
				exit(1);                                                                           \
			}                                                                                      \
		}                                                                                          \
	}

// clang-format off

INSERT_TEST(wildcard_conflict,
			"/cmd/{tool}/{sub}", 					0,
			"/cmd/{tool}/{sub}", 					0,
			"/cmd/vet", 							0,
			"/foo/bar", 							0,
			"/foo/{name}", 							0,
			"/foo/{names}", 						URLROUTER_ERR_PATH_EXISTS,
			"/cmd/{xxx}/names", 					0,
			"/cmd/{tool}/{xxx}/foo", 				0,
			"/src/{file}", 							0,
			"/src/{files}", 						URLROUTER_ERR_PATH_EXISTS,
			"/src/static.json",						0,
			"/src/$filepathx", 						0,
			"/src/", 								0,
			"/src/foo/bar", 						0,
			"/src1/", 								0,
			"/src2/", 								0,
			"/src2", 								0,
			"/src3", 								0,
			"/search/{query}", 						0,
			"/search/valid", 						0,
			"/user_{name}", 						0,
			"/user_x", 								0,
			"/user_{bar}", 							URLROUTER_ERR_PATH_EXISTS,
			"/id{id}", 								0,
			"/id/{id}", 							0
)

INSERT_TEST(invalid_catchall,
	"/non-leading-{*catchall}", 					0,
	"/foo/bar{*catchall}", 							0,
	"/src/{*filepath}/x", 							URLROUTER_ERR_MALFORMED_PATH,
	"/src2/", 										0,
	"/src2/{*filepath}/x", 							URLROUTER_ERR_MALFORMED_PATH
)

INSERT_TEST(catchall_root_conflict,
	"/", 											0,
	"/{*filepath}", 								URLROUTER_ERR_PATH_EXISTS
)


INSERT_TEST(child_conflict,
	"/cmd/vet", 									0,
	"/cmd/{tool}", 									0,
	"/cmd/{tool}/{sub}", 							0,
	"/cmd/{tool}/misc", 							0,
	"/cmd/{tool}/{bad}", 							URLROUTER_ERR_PATH_EXISTS,
	"/src/AUTHORS", 								0,
	"/src/{*filepath}", 							0,
	"/user_x", 										0,
	"/user_{name}", 								0,
	"/id/{id}", 									0,
	"/id{id}", 										0,
	"/{id}", 										0,
	"/{*filepath}", 								URLROUTER_ERR_PATH_EXISTS
)

INSERT_TEST(duplicates,
	"/", 											0,
	"/", 											URLROUTER_ERR_PATH_EXISTS,
	"/doc/", 										0,
	"/doc/", 										URLROUTER_ERR_PATH_EXISTS,
	"/src/{*filepath}", 							0,
	"/src/{*filepath}", 							URLROUTER_ERR_PATH_EXISTS,
	"/search/{query}", 								0,
	"/search/{query}", 								URLROUTER_ERR_PATH_EXISTS,
	"/user_{name}", 								0,
	"/user_{name}", 								URLROUTER_ERR_PATH_EXISTS
)

INSERT_TEST(unnamed_param,
	"/{}", 											URLROUTER_ERR_MALFORMED_PATH,
	"/user{}", 										URLROUTER_ERR_MALFORMED_PATH,
	"/cmd{}", 										URLROUTER_ERR_MALFORMED_PATH,
	"/src{*}", 										URLROUTER_ERR_MALFORMED_PATH
)

INSERT_TEST(double_params,
	"/{foo}{bar}", 									URLROUTER_ERR_MALFORMED_PATH,
	"/{foo}{bar}/", 								URLROUTER_ERR_MALFORMED_PATH,
	"/{foo}{{*bar}/", 								URLROUTER_ERR_MALFORMED_PATH
)

INSERT_TEST(normalized_conflict,
	"/x/{foo}/bar", 								0,
	"/x/{bar}/bar", 								URLROUTER_ERR_PATH_EXISTS,
	"/{y}/bar/baz", 								0,
	"/{y}/baz/baz", 								0,
	"/{z}/bar/bat", 								0,
	"/{z}/bar/baz", 								URLROUTER_ERR_PATH_EXISTS
)

INSERT_TEST(more_conflicts,
	"/con{tact}", 									0,
	"/who/are/{*you}", 								0,
	"/who/foo/hello", 								0,
	"/whose/{users}/{name}", 						0,
	"/who/are/foo", 								0,
	"/who/are/foo/bar", 							0,
	"/con{nection}", 								URLROUTER_ERR_PATH_EXISTS,
	"/whose/{users}/{user}", 						URLROUTER_ERR_PATH_EXISTS
)

INSERT_TEST(catchall_static_overlap_1,
	"/bar", 										0,
	"/bar/", 										0,
	"/bar/{*foo}", 									0
)

INSERT_TEST(catchall_static_overlap_2,
	"/foo", 										0,
	"/{*bar}", 										0,
	"/bar", 										0,
	"/baz", 										0,
	"/baz/{split}", 								0,
	"/", 											0,
	"/{*bar}", 										URLROUTER_ERR_PATH_EXISTS,
	"/{*zzz}", 										URLROUTER_ERR_PATH_EXISTS,
	"/{xxx}", 										URLROUTER_ERR_PATH_EXISTS
)

INSERT_TEST(catchall_static_overlap_3,
	"/{*bar}", 										0,
	"/bar", 										0,
	"/bar/x", 										0,
	"/bar_{x}", 									0,
	"/bar_{x}", 									URLROUTER_ERR_PATH_EXISTS,
	"/bar_{x}/y", 									0,
	"/bar/{x}", 									0
)

INSERT_TEST(duplicate_conflict,
	"/hey", 										0,
	"/hey/users", 									0,
	"/hey/user", 									0,
	"/hey/user", 									URLROUTER_ERR_PATH_EXISTS
)

INSERT_TEST(invalid_param,
	"{", 											URLROUTER_ERR_MALFORMED_PATH,
	"}", 											URLROUTER_ERR_MALFORMED_PATH,
	"x{y", 											URLROUTER_ERR_MALFORMED_PATH,
	"x}", 											URLROUTER_ERR_MALFORMED_PATH,
	"/{foo}s", 										URLROUTER_ERR_MALFORMED_PATH
)

INSERT_TEST(escaped_param,
	"{{", 											0,
	"}}", 											0,
	"xx}}", 										0,
	"}}yy", 										0,
	"}}yy{{}}", 									0,
	"}}yy{{}}{{}}y{{", 								0,
	"}}yy{{}}{{}}y{{", 								URLROUTER_ERR_PATH_EXISTS,
	"/{{yy", 										0,
	"/{yy}", 										0,
	"/foo", 										0,
	"/foo/{{", 										0,
	"/foo/{{/{x}", 									0,
	"/foo/{ba{{r}", 								0,
	"/bar/{ba}}r}", 								0,
	"/xxx/{x{{}}y}", 								0
)

INSERT_TEST(bare_catchall,
	"{*foo}", 										0,
	"foo/{*bar}", 									0
)

int main(void)
{
	wildcard_conflict();
	invalid_catchall();
	catchall_root_conflict();
	child_conflict();
	duplicates();
	unnamed_param();
	double_params();
	normalized_conflict();
	more_conflicts();
	catchall_static_overlap_1();
	catchall_static_overlap_2();
	catchall_static_overlap_3();
	duplicate_conflict();
	invalid_param();
	escaped_param();
	bare_catchall();

	return 0;
}
