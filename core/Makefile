# Makefile do núcleo portátil: testes (host), CLI e checagens de portabilidade MIPS.
#   make -C core test                      # todos os testes (500 seeds por preset nas propriedades)
#   make -C core test TEST=share_code      # só os testes cujo nome contém "share_code"
#   make -C core test BKR_PROPERTY_SEEDS=5000
#   make -C core cli                       # gera core/build/bkrgen
#   make -C core mips-check                # compila com as flags do mod e exige zero símbolos externos
#   make -C core mips-selftest             # roda o núcleo MIPS no Unicorn e compara o hash com o do host
# Use sempre um shell POSIX (no Windows: o Git Bash, que é o shell do Claude Code).

ifeq ($(OS),Windows_NT)
    HOST_CC ?= gcc
    PYTHON ?= python
    EXE := .exe
else
    HOST_CC ?= cc
    PYTHON ?= python3
    EXE :=
endif
MIPS_CC ?= clang
MIPS_LD ?= ld.lld
NM ?= llvm-nm
BKR_PROPERTY_SEEDS ?= 500
TEST ?=
# Valor-ouro do seed "BANJO" com as opções padrão. Fonte única: o teste C e o
# autoteste MIPS recebem este valor. Só muda junto com BKR_GENERATOR_VERSION
# ou com os dados de lógica (BKR_LOGIC_DATA_HASH).
PLACEMENT_GOLDEN ?= 31098721

BUILD := build
CPPFLAGS := -Iinclude -Igenerated
HOST_CFLAGS := -std=c99 -O2 -Wall -Wextra -Wpedantic -Werror
# Mesmas flags de arquitetura do Makefile do mod (BKRecompModTemplate) + -fno-builtin.
MIPS_CFLAGS := -target mips -mips2 -mabi=32 -O2 -G0 -mno-abicalls -mno-odd-spreg \
               -mno-check-zero-division -fomit-frame-pointer -ffast-math \
               -fno-unsafe-math-optimizations -fno-builtin -funsigned-char -nostdinc \
               -ffunction-sections -DMIPS -std=c99 -Wall -Wextra \
               -Wno-unsupported-floating-point-opt -Werror

CORE_SRC := $(sort $(wildcard src/*.c)) generated/bkr_logic_data.c
# O mod não leva o spoiler (o texto do spoiler é gerado pela CLI no PC).
MOD_CORE_SRC := $(filter-out src/bkr_spoiler.c,$(CORE_SRC))
TEST_SRC := $(sort $(filter-out tests/test_main.c,$(wildcard tests/test_*.c)))
HEADERS := $(wildcard include/bkr/*.h generated/*.h tests/*.h)

.PHONY: all test cli mips-check mips-selftest clean

all: test cli mips-check

$(BUILD) $(BUILD)/mips $(BUILD)/mips/selftest:
	mkdir -p $@

# Lista de testes: uma linha BKR_TEST(nome) por função "void test_nome(void)".
$(BUILD)/bkr_test_list.inc: $(TEST_SRC) | $(BUILD)
	$(if $(TEST_SRC),,$(error nenhum arquivo tests/test_*.c encontrado))
	grep -h -o '^void test_[a-z0-9_]*(void)' $(TEST_SRC) \
		| sed 's/^void test_\([a-z0-9_]*\)(void)$$/BKR_TEST(\1)/' > $@

$(BUILD)/bkr_tests$(EXE): $(CORE_SRC) $(TEST_SRC) tests/test_main.c $(HEADERS) $(BUILD)/bkr_test_list.inc Makefile | $(BUILD)
	$(HOST_CC) $(HOST_CFLAGS) $(CPPFLAGS) -Itests -I$(BUILD) -DBKR_PROPERTY_SEEDS=$(BKR_PROPERTY_SEEDS) \
		-DBKR_PLACEMENT_GOLDEN=0x$(PLACEMENT_GOLDEN)u $(CORE_SRC) $(TEST_SRC) tests/test_main.c -o $@

test: $(BUILD)/bkr_tests$(EXE)
	./$(BUILD)/bkr_tests$(EXE) $(TEST)

$(BUILD)/bkrgen$(EXE): $(CORE_SRC) cli/bkrgen.c $(HEADERS) | $(BUILD)
	$(HOST_CC) $(HOST_CFLAGS) $(CPPFLAGS) $(CORE_SRC) cli/bkrgen.c -o $@

cli: $(BUILD)/bkrgen$(EXE)

MIPS_CHECK_OBJS := $(addprefix $(BUILD)/mips/,$(notdir $(CORE_SRC:.c=.o)))

mips-check: | $(BUILD)/mips
	@for f in $(CORE_SRC); do \
		$(MIPS_CC) $(MIPS_CFLAGS) $(CPPFLAGS) -c $$f -o $(BUILD)/mips/$$(basename $$f .c).o || exit 1; \
	done
	$(MIPS_LD) -r $(MIPS_CHECK_OBJS) -o $(BUILD)/core_all_mips.o
	@undef="$$($(NM) -u $(BUILD)/core_all_mips.o)"; \
	if [ -n "$$undef" ]; then echo "ERRO: símbolos externos no núcleo MIPS:"; echo "$$undef"; exit 1; \
	else echo "mips-check: ok (nenhum símbolo externo)"; fi

SELFTEST_SRC := $(MOD_CORE_SRC) tests/mips_selftest.c
SELFTEST_OBJS := $(addprefix $(BUILD)/mips/selftest/,$(notdir $(SELFTEST_SRC:.c=.o)))

$(BUILD)/mips/selftest.elf: $(SELFTEST_SRC) tests/mips_selftest.ld $(HEADERS) | $(BUILD)/mips/selftest
	@for f in $(SELFTEST_SRC); do \
		$(MIPS_CC) $(MIPS_CFLAGS) $(CPPFLAGS) -c $$f -o $(BUILD)/mips/selftest/$$(basename $$f .c).o || exit 1; \
	done
	$(MIPS_LD) -T tests/mips_selftest.ld --gc-sections $(SELFTEST_OBJS) -o $@

mips-selftest: $(BUILD)/mips/selftest.elf
	$(PYTHON) ../tools/mips_selftest.py $< --expect $(PLACEMENT_GOLDEN)

clean:
	rm -rf $(BUILD)
