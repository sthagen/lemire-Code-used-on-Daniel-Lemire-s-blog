#include <malloc.h>
#include <stdio.h>

#if __linux__
#include <malloc.h>
size_t available(const void *ptr) {
  return malloc_usable_size(ptr);
}
#elif defined(__APPLE__) || defined(__FreeBSD__)
#include <malloc/malloc.h>
size_t available(const void *ptr) {
  return malloc_size(ptr);
}
#endif
int main() {
  for (size_t i = 1; i < 1000; i++) {
    char *c = (char *)malloc(i);
    printf("%d %d %d\n", i, available(c), available(c) - i);
    free(c);
  }
}
