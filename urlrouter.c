#include "urlrouter.h"

#ifdef URLROUTER_IO
#include <stdio.h>
#endif

#ifdef URLROUTER_ASSERT
#include <assert.h>
#else
#define assert(expr) ((void)0)
#endif


typedef char bool;

// Check if the current frag is exhausted. The node should be the one corresponding with the frag.
static inline bool is_node_frag_end(urlrouter_node *node, const char *frag)
{
	return frag - node->frag == node->frag_len || *frag == '\0';
}
// A node param has a fragment that starts with '{' and ends with '}'.
// TODO: check with escaping characters
static inline bool is_node_param(urlrouter_node *node)
{
	return node->frag[node->frag_len - 1] == '}' || node->frag[0] == '{';
}

// Only alphanumeric characters are allowed in path parameters
static inline bool is_valid_param(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

static inline unsigned long rem_space(urlrouter *router)
{
	return router->len - router->cursor * sizeof(urlrouter_node);
}

static inline unsigned int str_len(const char *s)
{
	const char *p = s;
	while (*++p != '\0')
		;
	return p - s;
}
static inline char readc(const char **cursor) { return *(*cursor)++; }
static inline char peekc(const char **cursor) { return **cursor; }
static inline void advancec(const char **cursor) { (*cursor)++; }

// Create a new node and insert it in the router buffer.
// If there is not enough space, returns NULL.
static inline urlrouter_node *create_node(urlrouter *router, const char *frag,
										  unsigned int frag_len, const void *data)
{
	if (rem_space(router) < sizeof(urlrouter_node))
		return NULL;

	char *p = (char *)router->buffer + router->cursor * sizeof(urlrouter_node);
	for (unsigned long i = 0; i < sizeof(urlrouter_node); i++)
		((char *)p)[i] = 0;
	urlrouter_node *node = (urlrouter_node *)p;

	node->frag = frag;
	node->frag_len = frag_len;
	node->data = data;
	router->cursor++;
	return node;
}

// Check if the path is malformed:
// - Path parameters should be closed
// - Path parameters should only contain alphanumeric characters
static inline int verify_path(const char *p)
{
	if (*p == '\0')
		return URLROUTER_ERR_MALFORMED_PATH;

	bool is_param = *p == '{';
	int param_len = 0;
	while (*p++)
	{
		if (is_param && ((!is_valid_param(*p) && *p != '}') || (*p == '}' && param_len < 2)))
			return URLROUTER_ERR_MALFORMED_PATH;
		else if (is_param)
			param_len++;

		// Path parameter is closed or opened
		if (*p == '}')
			is_param = param_len = 0;
		else if (*p == '{')
			is_param = 1;
	}

	if (is_param) // Path parameter is not closed
		return URLROUTER_ERR_MALFORMED_PATH;

	return 0;
}

void urlrouter_init(urlrouter *router, void *buffer, unsigned long len)
{
	router->buffer = buffer;
	router->len = len;
	router->cursor = 0;
}

int urlrouter_add(urlrouter *router, const char *path, const void *data)
{

	const char *p = path;
	urlrouter_node *node = router->root, *previous = NULL;
	bool frag_param = 0, path_param = 0;

	// root is null (initial conditions)
	if (node == NULL)
	{
		int err = verify_path(p);
		if (err != 0)
			return err;

		node = create_node(router, p, str_len(p), data);
		if (node == NULL)
			return URLROUTER_ERR_BUFF_FULL;

		router->root = (urlrouter_node *)router->buffer;
		assert(router->root == node);
		return rem_space(router);
	}

	const char *frag = node->frag;
	while (*p) // Iterate over the path
	{
		// If the fragment is different, go to the next sibling
		if (*frag != *p && node->next_sibling)
		{
			previous = node;
			node = node->next_sibling;
			frag = node->frag;
			continue;
		}
		// If the fragment is different and there is no sibling, create a new
		// sibling
		else if (*frag != *p && !node->next_sibling)
		{
			if (verify_path(p) == URLROUTER_ERR_MALFORMED_PATH)
				return URLROUTER_ERR_MALFORMED_PATH;
			urlrouter_node *new_node = create_node(router, p, str_len(p), data);
			if (new_node == NULL)
				return URLROUTER_ERR_BUFF_FULL;

			// If current node is a parameter we should move it to the beggining to respect
			// priority.
			bool is_param = is_node_param(node);
			if (is_param && previous && previous->first_child == node)
			{
				// the current node was the first child. We just set the new to be the first one.
				new_node->next_sibling = node;
				previous->first_child = new_node;
			}
			else if (is_param && previous && previous->next_sibling == node)
			{
				// the current node is one of the siblings. We just set the new to be the next one.
				new_node->next_sibling = node;
				previous->next_sibling = new_node;
			}
			else
				node->next_sibling = new_node;

			return rem_space(router);
		}

		// Iterate over the fragment until it the path and current fragment
		// don't match
		do
		{
			// TODO: implement escaping with {{ }}
			if (*frag == '{' || *frag == '}')
				frag_param = !frag_param;
			if (*p == '{' || *p == '}')
				path_param = !path_param;
			frag++;
			p++;
		} while (*frag == *p && !is_node_frag_end(node, frag) && *p != '\0' && !frag_param &&
				 !path_param);

		while (frag_param && *frag++ != '}' && !is_node_frag_end(node, frag))
			;
		do
		{
			if (!is_valid_param(*p) && path_param)
				return URLROUTER_ERR_MALFORMED_PATH;
		} while (path_param && *p++ != '}' && *p != '\0');
		path_param = frag_param = 0;

		if (is_node_frag_end(node, frag) && *p == '\0')
			return URLROUTER_ERR_PATH_EXISTS;

		if (node->first_child && is_node_frag_end(node, frag))
		{
			previous = node;
			node = node->first_child;
			frag = node->frag;
		}
		// If the frag is exhausted and there is no children we can append a new
		// child
		else if (!node->first_child && is_node_frag_end(node, frag))
		{
			if (verify_path(p) == URLROUTER_ERR_MALFORMED_PATH)
				return URLROUTER_ERR_MALFORMED_PATH;
			urlrouter_node *new_node = create_node(router, p, str_len(p), data);
			if (new_node == NULL)
				return URLROUTER_ERR_BUFF_FULL;

			node->first_child = new_node;
			return rem_space(router);
		}
		// If the path is exhausted, we should split the current child to add the
		// new node as a sibling
		else if (!is_node_frag_end(node, frag))
		{
			if (*p && verify_path(p) == URLROUTER_ERR_MALFORMED_PATH)
				return URLROUTER_ERR_MALFORMED_PATH;

			int node_cnt = *p != '\0' ? 1 : 2;
			if (rem_space(router) < node_cnt * sizeof(urlrouter_node))
				return URLROUTER_ERR_BUFF_FULL;

			urlrouter_node *splited_node =
				create_node(router, node->frag + (frag - node->frag),
							node->frag_len - (frag - node->frag), node->data);
			assert(splited_node != NULL);

			splited_node->first_child = node->first_child;

			// Transform the current node into a parent subset
			node->frag_len = frag - node->frag;

			if (*p != '\0')
			{
				urlrouter_node *new_node = create_node(router, p, str_len(p), data);
				assert(new_node != NULL);

				// If the splitted node is a parameter we should put at the end to
				// respect priority
				if (!is_node_param(splited_node))
				{
					splited_node->next_sibling = new_node;
					node->first_child = splited_node;
				}
				else
				{
					new_node->next_sibling = splited_node;
					node->first_child = new_node;
				}
			}
			else // The path is exhausted, we can just add the data to the parent subset node
			{
				node->first_child = splited_node;
				node->data = data;
			}

			return rem_space(router);
		}
		else if (node->first_child)
		{
			previous = node;
			node = node->first_child;
			frag = node->frag;
		}
	}

	return rem_space(router);
}

const void *urlrouter_find(const urlrouter *router, const char *path, urlparam *params,
						   const unsigned int len, unsigned int *param_cnt)
{
	// cannot have param_cnt set but not params
	assert((params != NULL && param_cnt != NULL) || params == NULL);

	const char *p = path;
	unsigned int param_i = 0;
	urlrouter_node *node = router->root;
	if (!node)
		return NULL;

	while (*p)
	{

		const char *frag = node->frag;
		if (*frag == '{' && !is_node_frag_end(node, frag))
		{
			if (params && param_i < len)
			{
				params[param_i].value = p++;
				params[param_i].len = 1;
				if (param_cnt)
					++*param_cnt;
			}
			while (*frag++ != '}' && !is_node_frag_end(node, frag))
				;
			// Bench between local var and constant deref in loop
			while (*p != '/' && *p != '\0')
			{
				if (params && param_i < len)
					params[param_i].len++;
				p++;
			}
			param_i++;
		}

		while (*frag == *p && !is_node_frag_end(node, frag) && *p != '\0')
		{
			frag++;
			p++;
			if (*frag == '{' && !is_node_frag_end(node, frag))
			{
				if (params && param_i < len)
				{
					params[param_i].value = p++;
					params[param_i].len = 1;
					if (param_cnt)
						++*param_cnt;
				}
				while (*frag++ != '}' && !is_node_frag_end(node, frag))
					;
				// Bench between local var and constant deref in loop
				while (*p != '/' && *p != '\0')
				{
					if (params && param_i < len)
						params[param_i].len++;
					p++;
				}
				param_i++;
			}
		}

		if (*frag != *p)
		{
			if (node->first_child && is_node_frag_end(node, frag))
			{
				node = node->first_child;
				continue;
			}
			else if (node->next_sibling)
			{
				node = node->next_sibling;
				continue;
			}
			else
				return NULL;
		}
		else if (is_node_frag_end(node, frag) && *p == '\0')
			return node->data;
		else if (node->first_child)
			node = node->first_child;
		else
			return NULL;
	}

	return NULL;
}

static inline void print_node(const urlrouter_node *node, int depth)
{
	while (node != NULL)
	{
		// Print the indentation for the current depth
		printf("|");
		for (int i = 0; i < depth; ++i)
			printf(" ");

		if (depth > 0)
			printf("└");
		else
			printf("-");
		// Print the fragment
		printf("%.*s", node->frag_len, node->frag);

		for (int i = 0; i < 50 - node->frag_len - depth; ++i)
			printf(" ");
		printf("-> %p\n", node->data);

		// Print the first child with increased depth
		if (node->first_child != NULL)
		{
			print_node(node->first_child, depth + node->frag_len);
		}

		// Move to the next sibling
		node = node->next_sibling;
	}
}
void urlrouter_print(const urlrouter *router)
{
	printf("URL Router:\n");
	print_node(router->root, 0);
}

#ifdef URLROUTER_TEST

void test_verify_path(void)
{
	assert(verify_path("test") == 0);
	assert(verify_path("azd{azdazd}") == 0);
	assert(verify_path("azd{azdazd}") == 0);
	assert(verify_path("checkaz{") == URLROUTER_ERR_MALFORMED_PATH);
	assert(verify_path("checkaz}") == 0);
	assert(verify_path("azdiazd}") == 0);
	assert(verify_path("a?zdiaz") == 0);
	assert(verify_path("aézdiaz") == 0);
	assert(verify_path("aézdi/{éazdazd}/az") == URLROUTER_ERR_MALFORMED_PATH);
	assert(verify_path("\0") == URLROUTER_ERR_MALFORMED_PATH);
}
void test_node_frag_end(void)
{
	const char frag[] = "test";
	urlrouter_node node = {.frag = frag, .frag_len = sizeof("test")};
	assert(!is_node_frag_end(&node, frag));
	assert(is_node_frag_end(&node, frag + 4));
	assert(is_node_frag_end(&node, ""));
}

int main()
{
	test_verify_path();
	test_node_frag_end();
	printf("All tests passed!\n");
	return 0;
}

#endif // URLROUTER_TEST
