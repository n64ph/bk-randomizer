/* bkr_test.h — Mini framework de testes do núcleo (somente host). */
#ifndef BKR_TEST_H
#define BKR_TEST_H

#include <stdio.h>
#include "bkr/bkr_types.h"

extern int bkr_test_failures;
extern int bkr_test_checks;

#define BKR_CHECK(cond)                                                          \
    do {                                                                         \
        bkr_test_checks++;                                                       \
        if (!(cond)) {                                                           \
            printf("FALHA %s:%d: %s\n", __FILE__, __LINE__, #cond);              \
            bkr_test_failures++;                                                 \
        }                                                                        \
    } while (0)

#define BKR_CHECK_EQ(a, b)                                                       \
    do {                                                                         \
        unsigned long long va_ = (unsigned long long)(a);                        \
        unsigned long long vb_ = (unsigned long long)(b);                        \
        bkr_test_checks++;                                                       \
        if (va_ != vb_) {                                                        \
            printf("FALHA %s:%d: %s == %s (0x%llX != 0x%llX)\n", __FILE__,       \
                   __LINE__, #a, #b, va_, vb_);                                  \
            bkr_test_failures++;                                                 \
        }                                                                        \
    } while (0)

typedef struct BkrTestCase {
    const char* name;
    void (*fn)(void);
} BkrTestCase;

#endif
