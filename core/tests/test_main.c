/*
 * test_main.c — Executor dos testes do núcleo (host).
 *
 * A lista de testes é gerada pelo core/Makefile em build/bkr_test_list.inc a
 * partir de toda função "void test_<nome>(void)" em tests/test_*.c; para
 * criar um teste novo basta escrever a função.
 *
 * Uso: ./build/bkr_tests [trecho]   (roda só os testes cujo nome contém o trecho)
 */
#include "bkr_test.h"

int bkr_test_failures = 0;
int bkr_test_checks = 0;

#define BKR_TEST(name) void test_##name(void);
#include "bkr_test_list.inc"
#undef BKR_TEST

static const BkrTestCase cases[] = {
#define BKR_TEST(name) { #name, test_##name },
#include "bkr_test_list.inc"
#undef BKR_TEST
};

static int name_matches(const char* name, const char* filter) {
    bkr_u32 n;
    if (filter == NULL) {
        return 1;
    }
    n = bkr_strlen(filter);
    for (; *name != '\0'; name++) {
        bkr_u32 i = 0;
        while (i < n && name[i] == filter[i]) {
            i++;
        }
        if (i == n) {
            return 1;
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    const char* filter = argc > 1 ? argv[1] : NULL;
    unsigned i;
    unsigned ran = 0;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        int before = bkr_test_failures;
        if (!name_matches(cases[i].name, filter)) {
            continue;
        }
        cases[i].fn();
        ran++;
        printf("%s %s\n", bkr_test_failures == before ? "ok  " : "FAIL", cases[i].name);
    }
    if (ran == 0) {
        printf("nenhum teste corresponde a '%s'\n", filter != NULL ? filter : "");
        return 1;
    }
    printf("\n%u testes, %d verificações, %d falhas\n", ran, bkr_test_checks, bkr_test_failures);
    return bkr_test_failures == 0 ? 0 : 1;
}
