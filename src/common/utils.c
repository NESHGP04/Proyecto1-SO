//src/common/utils.c

#include "../../include/common.h"

void safe_copy(char *dst, const char *src, size_t n) {
    if (!dst || !src) return;
    strncpy(dst, src, n - 1);
    dst[n - 1] = '\0';
}