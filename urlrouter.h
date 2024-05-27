
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

#ifdef __cplusplus
extern "C"
{
#endif
	enum
	{
		URLROUTER_OK = 0,
		URLROUTER_ERR_PATH_EXISTS = -1,
		URLROUTER_ERR_BUFF_FULL = -2,
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

	void urlrouter_init(urlrouter *router, void *buffer, unsigned long buffer_size);
	int urlrouter_add(urlrouter *router, const char *path, const void *data);
	const void *urlrouter_find(const urlrouter *router, const char *path);

#ifdef URLROUTER_IO
	void urlrouter_print(const urlrouter *router);
#endif

#ifdef URLROUTER_IMPLEMENTATION
#define IS_FRAG_END(node) (frag - node->frag == node->frag_len || *frag == '\0')

	static inline unsigned int urlrouter_strlen(const char *s)
	{
		const char *p = s;
		while (*p++ != '\0')
			;
		return p - s;
	}
	static inline urlrouter_node *urlrouter_create_node(urlrouter *router, const char *frag, unsigned int frag_len, const void *data)
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

	void urlrouter_init(urlrouter *router, void *buffer, unsigned long buffer_size)
	{
		for (unsigned long i = 0; i < buffer_size; i++)
			((char *)buffer)[i] = 0;
		router->buffer = buffer;
		router->len = buffer_size;
		router->cursor = 0;
		router->root = NULL;
	}

	int urlrouter_add(urlrouter *router, const char *path, const void *data)
	{
#define REM_SPACE router->len - router->cursor * sizeof(urlrouter_node)

		const char *p = path;
		urlrouter_node *node = router->root;

		if (node == NULL)
		{
			node = urlrouter_create_node(router, p, urlrouter_strlen(p), data);
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
				node = node->next_sibling;
				frag = node->frag;
				continue;
			}
			// If the fragment is different and there is no sibling, create a new sibling
			else if (*frag != *p && !node->next_sibling)
			{
				urlrouter_node *new_node = urlrouter_create_node(router, p, urlrouter_strlen(p), data);
				if (new_node == NULL)
					return URLROUTER_ERR_BUFF_FULL;

				node->next_sibling = new_node;
				return REM_SPACE;
			}

			do // Iterate over the fragment
			{
				frag++;
				p++;
			} while (*frag == *p && !IS_FRAG_END(node) && *p != '\0');

			if (IS_FRAG_END(node) && *p == '\0')
				return URLROUTER_ERR_PATH_EXISTS;

			if (node->first_child && IS_FRAG_END(node))
			{
				node = node->first_child;
				frag = node->frag;
			}
			// If the frag is exhausted and there is no children we can append a new child
			else if (!node->first_child && IS_FRAG_END(node))
			{
				urlrouter_node *new_node = urlrouter_create_node(router, p, urlrouter_strlen(p), data);
				if (new_node == NULL)
					return URLROUTER_ERR_BUFF_FULL;

				node->first_child = new_node;
				return REM_SPACE;
			}
			// If the path is exhausted, we should split the current child to add the new node as a sibling
			else if (!IS_FRAG_END(node))
			{
				urlrouter_node *splited_node = urlrouter_create_node(router,
																	 node->frag + (frag - node->frag),
																	 node->frag_len - (frag - node->frag), node->data);
				if (splited_node == NULL)
					return URLROUTER_ERR_BUFF_FULL;

				splited_node->first_child = node->first_child;

				// Transform the current node into a parent subset
				node->frag_len = frag - node->frag;
				node->first_child = splited_node;

				urlrouter_node *new_node = urlrouter_create_node(router, p, urlrouter_strlen(p), data);
				if (new_node == NULL)
					return URLROUTER_ERR_BUFF_FULL;

				splited_node->next_sibling = new_node;

				return REM_SPACE;
			}
			else if (node->first_child)
			{
				node = node->first_child;
				frag = node->frag;
			}
		}

		return REM_SPACE;

#undef REM_SPACE
	}

	const void *urlrouter_find(const urlrouter *router, const char *path)
	{
		const char *p = path;
		urlrouter_node *node = router->root;

		while (*p)
		{
			const char *frag = node->frag;
			while (*frag == *p && !IS_FRAG_END(node) && *p != '\0')
			{
				frag++;
				p++;
			}

			if (*frag != *p)
			{
				if (node->next_sibling)
				{
					node = node->next_sibling;
					continue;
				}
				else if (node->first_child)
				{
					node = node->first_child;
					continue;
				}
				else
				{
					return NULL;
				}
			}

			else if (IS_FRAG_END(node) && *p == '\0')
			{
				return node->data;
			}
			else if (node->first_child)
			{
				node = node->first_child;
			}
			else
			{
				return NULL;
			}
		}

		return NULL;
	}

#ifdef URLROUTER_IO

	static inline void urlrouter_print_node(const urlrouter_node *node, int depth)
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
			printf("%.*s\n", node->frag_len, node->frag);

			// Print the first child with increased depth
			if (node->first_child != NULL)
			{
				urlrouter_print_node(node->first_child, depth + node->frag_len);
			}

			// Move to the next sibling
			node = node->next_sibling;
		}
	}
	void urlrouter_print(const urlrouter *router)
	{
		printf("URL Router:\n");
		urlrouter_print_node(router->root, 0);
	}
#endif

#undef IS_FRAG_END
#endif // URLROUTER_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif // URLROUTER_H