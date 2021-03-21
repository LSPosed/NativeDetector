#include <new>
#include <malloc.h>

void* operator new(std::size_t size) {
    return malloc(size);
}

void operator delete(void* ptr) {
    free(ptr);
}
