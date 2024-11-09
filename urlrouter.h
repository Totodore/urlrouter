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

#ifndef URLROUTER_H
#define URLROUTER_H

#define URLROUTER_VERSION_MAJOR 0
#define URLROUTER_VERSION_MINOR 1
#define URLROUTER_VERSION_PATCH 0

#ifndef NULL
#define NULL 0
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
		// Data buffer
		void *buffer;
		// Buffer length
		unsigned long len;
		// Internal node cursor for the buffer
		unsigned long cursor;
	} urlrouter;

	/**
	 * The name of the parameter and its length.
	 * It is a slice of the original path given to urlrouter_find.
	 */
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
	 * @brief Add a path to the router.
	 * @param router The router to add the path to
	 * @param path The path to add. It should be a null-terminated string that has
	 * at least the lifetime of the router
	 * @param data The data to associate with the path
	 * @returns The remaining space in the buffer or URLROUTER_ERR_PATH_EXISTS if
	 * path is already existing in the buffer or URLROUTER_ERR_BUFF_FULL if there is
	 * no more room in the buffer.
	 */
	int urlrouter_add(urlrouter *router, const char *path, const void *data);

	/**
	 * @brief Find a path in the router and return its associated value and path
	 * params.
	 * @param router The router to search in
	 * @param path A null-terminated C string to search for
	 * @param params An array that will be populated with each encountered params.
	 * Can also be null if you don't care about params
	 * @param len The length of the array
	 * @param param_cnt A pointer that will be set to the number of encountered
	 * params
	 * @returns The data associated with the path or NULL if the path is not found
	 */
	const void *urlrouter_find(const urlrouter *router, const char *path, urlparam *params,
							   const unsigned int len, unsigned int *param_cnt);

#ifdef URLROUTER_IO
	/**
	 * @brief Print the router tree to the standard output with printf
	 */
	void urlrouter_print(const urlrouter *router);
#endif

#ifdef __cplusplus
}
#endif

#endif // URLROUTER_H
