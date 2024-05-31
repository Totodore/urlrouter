
#ifndef URLROUTER_H
#define URLROUTER_H

#define URLROUTER_VERSION_MAJOR 0
#define URLROUTER_VERSION_MINOR 1
#define URLROUTER_VERSION_PATCH 0

#ifndef NULL
#define NULL 0
#endif

#ifdef URLROUTER_IO
#include <stdio.h>
#endif

#ifdef URLROUTER_ASSERT
#include <assert.h>
#else
#define assert(expr) ((void)0)
#endif

#ifdef __cplusplus
extern "C"
{
#endif
	enum
	{
		// The provided path already exists in the router
		URLROUTER_ERR_PATH_EXISTS = -1,

		// The buffer is full
		URLROUTER_ERR_BUFF_FULL = -2,

		// Path parameter is not closed or contains non alphanumeric characters
		URLROUTER_ERR_MALFORMED_PATH = -3
	};
	typedef struct urlrouter_node
	{
		int dead : 1;
		const char *frag;
		unsigned int frag_len : 7; // max frag 127
		const void *data;
		struct urlrouter_node *first_child;
		struct urlrouter_node *next_sibling;
	} urlrouter_node;

	typedef struct
	{
		urlrouter_node *root;
		void *buffer;
		unsigned long len;
		unsigned long cursor;
	} urlrouter;

	typedef struct
	{
		const char *value;
		unsigned int len;
	} urlparam;

	/**
	 * @brief Initialize the router with a buffer and its size
	 * @param router The router to initialize
	 * @param buffer The buffer to store the nodes
	 * @param len The size of the buffer
	 */
	void urlrouter_init(urlrouter *router, void *buffer, unsigned long len);

	/**
	 * @brief Add a path to the router
	 * @param router The router to add the path to
	 * @param path The path to add. It should be a null-terminated string that lasts until the end of the router
	 * @param data The data to associate with the path
	 * @returns The remaining space in the buffer or URLROUTER_ERR_PATH_EXISTS if path is already existing in the buffer
	 * or URLROUTER_ERR_BUFF_FULL if there is no more room in the buffer.
	 */
	int urlrouter_add(urlrouter *router, const char *path, const void *data);

	/**
	 * @brief Find a path in the router and return its associated value and path params.
	 * @param router The router to search in
	 * @param path A null-terminated C string to search for
	 * @param params An array that will be populated with each encountered params.
	 * Can also be null if you don't care about params
	 * @param len The length of the array
	 * @param param_cnt A pointer that will be set to the number of encountered params
	 * @returns The data associated with the path or NULL if the path is not found
	 */
	const void *urlrouter_find(
		const urlrouter *router, const char *path,
		urlparam *params, const unsigned int len, unsigned int *param_cnt);

#ifdef URLROUTER_IO
	/**
	 * @brief Print the router to the standard output with printf
	 */
	void urlrouter_print(const urlrouter *router);
#endif

#ifdef URLROUTER_IMPLEMENTATION
#define IS_FRAG_END(node) (frag - node->frag == node->frag_len || *frag == '\0')
#define IS_VALID_PATH(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '}')
#define IS_PARAM(node) (node->frag[node->frag_len - 1] == '}')
#define REM_SPACE (router->len - router->cursor * sizeof(urlrouter_node))

	static inline unsigned int urlrouter__strlen(const char *s)
	{
		const char *p = s;
		while (*++p != '\0')
			;
		return p - s;
	}
	static inline urlrouter_node *urlrouter__create_node(urlrouter *router, const char *frag, unsigned int frag_len, const void *data)
	{
		urlrouter_node node = {0};
		node.frag = frag;
		node.frag_len = frag_len;
		node.data = data;

		char *d = (char *)router->buffer + router->cursor * sizeof(urlrouter_node);
		if (d + sizeof(urlrouter_node) > (char *)router->buffer + router->len)
			return NULL;

		const char *s = (const char *)&node;
		unsigned long n = sizeof(urlrouter_node);
		while (n--)
			*d++ = *s++;

		router->cursor++;
		return (urlrouter_node *)router->buffer + router->cursor - 1;
	}

	// Check if the path is malformed:
	// - Path parameters should be closed
	// - Path parameters should only contain alphanumeric characters
	static inline int urlrouter__verify_path(const char *p)
	{
		char path_param = *p == '{';
		while (*p++)
		{
			if (path_param && !IS_VALID_PATH(*p))
				return URLROUTER_ERR_MALFORMED_PATH;
			if (*p == '{' || *p == '}')
				path_param = !path_param;
		}
		if (path_param)
			return URLROUTER_ERR_MALFORMED_PATH;

		return 0;
	}

	void urlrouter_init(urlrouter *router, void *buffer, unsigned long len)
	{
		// memset the buffer to 0, on most recent compilers this will be optimized to a memset call
		for (unsigned long i = 0; i < len; i++)
			((char *)buffer)[i] = 0;

		router->buffer = buffer;
		router->len = len;
		router->cursor = 0;
		router->root = NULL;
	}

	int urlrouter_add(urlrouter *router, const char *path, const void *data)
	{

		const char *p = path;
		urlrouter_node *node = router->root, *previous = NULL;
		char frag_param = 0, path_param = 0;

		if (node == NULL)
		{
			if (urlrouter__verify_path(p) == URLROUTER_ERR_MALFORMED_PATH)
				return URLROUTER_ERR_MALFORMED_PATH;
			node = urlrouter__create_node(router, p, urlrouter__strlen(p), data);
			if (node == NULL)
				return URLROUTER_ERR_BUFF_FULL;

			router->root = router->buffer;
			return REM_SPACE;
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
			// If the fragment is different and there is no sibling, create a new sibling
			else if (*frag != *p && !node->next_sibling)
			{
				if (urlrouter__verify_path(p) == URLROUTER_ERR_MALFORMED_PATH)
					return URLROUTER_ERR_MALFORMED_PATH;
				urlrouter_node *new_node = urlrouter__create_node(router, p, urlrouter__strlen(p), data);
				if (new_node == NULL)
					return URLROUTER_ERR_BUFF_FULL;

				// If current node is a parameter we should move it to the end to respect priority
				int is_param = IS_PARAM(node);
				if (is_param && previous && previous->first_child == node) // node is first sibling
				{
					new_node->next_sibling = node;
					previous->first_child = new_node;
				}
				else if (is_param && previous && previous->next_sibling == node) // node is one of the siblings
				{
					new_node->next_sibling = node;
					previous->next_sibling = new_node;
				}
				else
					node->next_sibling = new_node;

				return REM_SPACE;
			}

			do // Iterate over the fragment
			{
				// TODO: implement escaping with {{ }}
				if (*frag == '{' || *frag == '}')
					frag_param = !frag_param;
				if (*p == '{' || *p == '}')
					path_param = !path_param;
				frag++;
				p++;
			} while (*frag == *p && !IS_FRAG_END(node) && *p != '\0' && !frag_param && !path_param);

			while (frag_param && *frag++ != '}' && !IS_FRAG_END(node))
				;
			do
			{
				if (!IS_VALID_PATH(*p) && path_param)
					return URLROUTER_ERR_MALFORMED_PATH;
			} while (path_param && *p++ != '}' && *p != '\0');
			path_param = frag_param = 0;

			if (IS_FRAG_END(node) && *p == '\0')
				return URLROUTER_ERR_PATH_EXISTS;

			if (node->first_child && IS_FRAG_END(node))
			{
				previous = node;
				node = node->first_child;
				frag = node->frag;
			}
			// If the frag is exhausted and there is no children we can append a new child
			else if (!node->first_child && IS_FRAG_END(node))
			{
				if (urlrouter__verify_path(p) == URLROUTER_ERR_MALFORMED_PATH)
					return URLROUTER_ERR_MALFORMED_PATH;
				urlrouter_node *new_node = urlrouter__create_node(router, p, urlrouter__strlen(p), data);
				if (new_node == NULL)
					return URLROUTER_ERR_BUFF_FULL;

				node->first_child = new_node;
				return REM_SPACE;
			}
			// If the path is exhausted, we should split the current child to add the new node as a sibling
			else if (!IS_FRAG_END(node))
			{
				if (urlrouter__verify_path(p) == URLROUTER_ERR_MALFORMED_PATH)
					return URLROUTER_ERR_MALFORMED_PATH;

				int node_cnt = *p != '\0' ? 1 : 2;
				if (REM_SPACE < node_cnt * sizeof(urlrouter_node))
					return URLROUTER_ERR_BUFF_FULL;

				urlrouter_node *splited_node = urlrouter__create_node(router,
																	  node->frag + (frag - node->frag),
																	  node->frag_len - (frag - node->frag),
																	  node->data);
				assert(splited_node != NULL);

				splited_node->first_child = node->first_child;

				// Transform the current node into a parent subset
				node->frag_len = frag - node->frag;

				if (*p != '\0')
				{
					urlrouter_node *new_node = urlrouter__create_node(router, p, urlrouter__strlen(p), data);
					assert(new_node != NULL);

					// If the splitted node is a parameter we should put at the end to respect priority
					if (!IS_PARAM(splited_node))
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

				return REM_SPACE;
			}
			else if (node->first_child)
			{
				previous = node;
				node = node->first_child;
				frag = node->frag;
			}
		}

		return REM_SPACE;
	}

	const void *urlrouter_find(const urlrouter *router, const char *path,
							   urlparam *params, const unsigned int len, unsigned int *param_cnt)
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
			while (*frag == *p && !IS_FRAG_END(node) && *p != '\0')
			{
				frag++;
				p++;
				if (*frag == '{')
				{
					if (params && param_cnt && param_i < len)
					{
						params[param_i].value = p++;
						params[param_i].len = 1;
						++*param_cnt;
					}
					while (*frag++ != '}' && !IS_FRAG_END(node))
						;
					// Bench between local var and constant deref in loop
					while (*p != '/' && *p != '\0')
					{
						if (params && param_i < len)
							params[param_i].len++;
						p++;
					}
				}
			}

			if (*frag != *p)
			{
				if (node->first_child && IS_FRAG_END(node))
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
			else if (IS_FRAG_END(node) && *p == '\0')
				return node->data;
			else if (node->first_child)
				node = node->first_child;
			else
				return NULL;
		}

		return NULL;
	}

#ifdef URLROUTER_IO

	static inline void urlrouter__print_node(const urlrouter_node *node, int depth)
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
				urlrouter__print_node(node->first_child, depth + node->frag_len);
			}

			// Move to the next sibling
			node = node->next_sibling;
		}
	}
	void urlrouter_print(const urlrouter *router)
	{
		printf("URL Router:\n");
		urlrouter__print_node(router->root, 0);
	}
#endif

#undef IS_FRAG_END
#undef IS_VALID_PATH
#undef IS_PARAM
#undef REM_SPACE
#endif // URLROUTER_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif // URLROUTER_H