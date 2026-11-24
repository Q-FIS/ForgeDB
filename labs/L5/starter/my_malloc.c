// See README.md.
#include "my_malloc.h"

#include <stdint.h>
#include <sys/mman.h>

void *my_malloc(size_t size) { (void)size; return NULL; }
void my_free(void *ptr) { (void)ptr; }
void *my_realloc(void *ptr, size_t size) { (void)ptr; (void)size; return NULL; }
