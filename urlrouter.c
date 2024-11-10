// MIT License
//
// Copyright (c) 2024 Théodore Prévot
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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

// Check if the current frag is exhausted.
// The given node should match the given frag.
static inline bool is_node_frag_end(urlrouter_node *node, const char *frag)
{
	return frag - node->frag == node->frag_len;
}
// A node param has a fragment that starts with '{' and ends with '}'.
// TODO: check with escaping characters
static inline bool is_node_param(urlrouter_node *node)
{
	return node->frag[node->frag_len - 1] == '}' || node->frag[0] == '{';
}
static inline bool is_param_start(const char *str) { return *str == '{' && *(str + 1) != '{'; }
static inline bool is_param_end(const char *str) { return *str == '}' && *(str + 1) != '}'; }

// Only alphanumeric characters are allowed in path parameters
static inline bool is_valid_param(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '{' ||
		   c == '}';
}
static inline bool is_param_escape_start(const char *str)
{
	return *str == '{' && *(str + 1) == '{';
}
static inline bool is_param_escape_end(const char *str) { return *str == '}' && *(str + 1) == '}'; }

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

#define DBG(x) printf("DEBUG [%d]: %s\n", __LINE__, x);

// Check if the path is malformed:
// - Path parameters should be closed
// - Path parameters should only contain alphanumeric characters
static inline int verify_path(const char *p)
{
	if (*p == '\0' || (*p == '}' && *(p + 1) == '\0'))
		return URLROUTER_ERR_MALFORMED_PATH;

	bool is_param = is_param_start(p);
	int param_len = 0;
	while (*p)
	{
		// Check for escaped characters ('{{' and '}}')
		if (is_param_escape_start(p) || is_param_escape_end(p))
		{
			p += 2; // Skip escaped braces
			continue;
		}

		if (is_param)
		{
			param_len++;
			if (is_param_end(p))
			{
				char next = *(p + 1);
				// When closing a path parameter, the next character
				// should be the end of a fragment
				if ((next != '/' && next != '\0') || param_len < 2)
					return URLROUTER_ERR_MALFORMED_PATH;
				else
					is_param = param_len = 0;
			}
			else if (!is_valid_param(*p))
				return URLROUTER_ERR_MALFORMED_PATH;
		}
		else
		{
			if (is_param_end(p)) // Closing a path parameter that was not opened
				return URLROUTER_ERR_MALFORMED_PATH;
			else
				is_param = is_param_start(p);
		}

		p++;
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
	assert(path != NULL);

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

		// Iterate over the fragment until the path and current fragment
		// don't match
		bool frag_param_escape, path_param_escape = 0;
		while (*frag == *p && !is_node_frag_end(node, frag) && *p != '\0')
		{
			// If we match a parameter we consume the entire parameter name.
			if (is_param_start(p) || is_param_start(frag))
			{
				// If a frag parameter is detected we iterate over it to consume it.
				while (*frag++ != '}' && !is_node_frag_end(node, frag))
					;
				assert(*frag != '}'); // it should be the end of the param
				// If a path_param is detected we iterate over it to consume it and we
				// verify that the param is valid
				while (*p++ != '}' && *p != '\0')
				{
					if (!is_valid_param(*p))
						return URLROUTER_ERR_MALFORMED_PATH;
				}
				assert(*p != '}'); // it should be the end of the param
			}

			if (*p != '\0')
				p++;
			if (*frag != '\0')
				frag++;
		}

		if (is_node_frag_end(node, frag) && *p == '\0')
		{
			// If this is the end of the fragment and the path but the node has no data
			// We can set the data, otherwise it means that the path already exists.
			if (node->data == NULL)
			{
				node->data = data;
				return rem_space(router);
			}
			else
				return URLROUTER_ERR_PATH_EXISTS;
		}

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
			printf("splitting_node: %s\n", p);
			if (*p && verify_path(p) == URLROUTER_ERR_MALFORMED_PATH)
				return URLROUTER_ERR_MALFORMED_PATH;

			int node_cnt = *p != '\0' ? 1 : 2;
			if (rem_space(router) < node_cnt * sizeof(urlrouter_node))
				return URLROUTER_ERR_BUFF_FULL;

			unsigned int split_idx = frag - node->frag;
			urlrouter_node *splited_node =
				create_node(router, node->frag + split_idx, node->frag_len - split_idx, node->data);
			assert(splited_node != NULL);

			splited_node->first_child = node->first_child;

			// Transform the current node into a parent subset
			node->frag_len = frag - node->frag;
			node->data = NULL;

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

	// We iterate over the path
	while (*p)
	{
		const char *frag = node->frag;
		if (*frag == '{' && !is_node_frag_end(node, frag))
		{
			// If we want to store the parameters
			// We start to store the parameter value
			if (params && param_i < len)
			{
				params[param_i].value = p++;
				params[param_i].len = 1;
				if (param_cnt)
					++*param_cnt;
			}
			// We consume the parameter
			while (*frag++ != '}' && !is_node_frag_end(node, frag))
				;
			// TODO: Bench between local var and constant deref in loop
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

#ifdef URLROUTER_IO
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

#endif // URLROUTER_IO

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
	assert(verify_path("{") == URLROUTER_ERR_MALFORMED_PATH);
	assert(verify_path("}") == URLROUTER_ERR_MALFORMED_PATH);
	assert(verify_path("") == URLROUTER_ERR_MALFORMED_PATH);
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
	// test_node_frag_end();
	printf("All tests passed!\n");
	return 0;
}

#endif // URLROUTER_TEST
