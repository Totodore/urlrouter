# urlrouter
A header-only, zero-allocation, no std efficient url router in C99.
It is implemented with a [radix trie](https://en.wikipedia.org/wiki/Radix_tree) and scales well with a high number of routes.

## Example
```c
#define URLROUTER_IMPLEMENTATION
#include <urlrouter.h>

int main(void) {
    urlrouter router;
    char buff[1024 * 4];
    urlrouter_init(&router, buff, 2024 * 4);
    urlrouter_add(&router, "/user/{id}", "user Val");
    urlrouter_add(&router, "/user", "user root");

    const char *val = urlrouter_find(&router, "/user/168");

    return 0;
}
```
