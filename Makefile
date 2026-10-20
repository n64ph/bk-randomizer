# Makefile do mod (MIPS), baseado no BKRecompModTemplate.
#   make                         # compila build/mod.elf
#   make nrm                     # roda o RecompModTool -> build/ph_bk_randomizer.nrm
#   make logic                   # regenera core/generated a partir de logic/*.json
#   make check                   # testes Python + núcleo + mips-check + mips-selftest + conflitos de patch
#   make package WEBSITE=https://github.com/SEU_USUARIO/bk-randomizer   # pacote da Thunderstore em dist/
# Use um shell POSIX (no Windows: o Git Bash, que é o shell do Claude Code).
BUILD_DIR := build

ifeq ($(OS),Windows_NT)
    CC      := clang
    LD      := ld.lld
    PYTHON  ?= python
else ifneq ($(shell uname),Darwin)
    CC      := clang
    LD      := ld.lld
    PYTHON  ?= python3
else
    CC      ?= clang
    LD      ?= ld.lld
    PYTHON  ?= python3
endif
RECOMP_MOD_TOOL ?= RecompModTool
BANJORECOMP_DIR ?= ../BanjoRecomp
WEBSITE ?=

TARGET  := $(BUILD_DIR)/mod.elf
LDSCRIPT := mod.ld
ARCHFLAGS := -target mips -mips2 -mabi=32 -O2 -G0 -mno-abicalls -mno-odd-spreg -mno-check-zero-division \
             -fomit-frame-pointer -ffast-math -fno-unsafe-math-optimizations -fno-builtin -funsigned-char -fno-builtin-sinf -fno-builtin-cosf
WARNFLAGS := -Wall -Wextra -Wno-incompatible-library-redeclaration -Wno-unused-parameter -Wno-unknown-pragmas -Wno-unused-variable \
             -Wno-missing-braces -Wno-unsupported-floating-point-opt -Wno-unknown-warning-option -Wno-cast-function-type-mismatch -Werror=section -Wno-visibility
CFLAGS   := $(ARCHFLAGS) $(WARNFLAGS) -D_LANGUAGE_C -nostdinc -ffunction-sections
CPPFLAGS := -nostdinc -DMIPS -DF3DEX_GBI -I include -I include/dummy_headers \
            -I bk-decomp/include -I bk-decomp/include/2.0L -I bk-decomp/include/2.0L/PR \
            -I core/include -I core/generated
LDFLAGS  := -nostdlib -T $(LDSCRIPT) -Map $(BUILD_DIR)/mod.map --unresolved-symbols=ignore-all --emit-relocs -e 0 --no-nmagic -gc-sections

MOD_SRCS  := $(sort $(wildcard src/*.c))
# Núcleo portátil sem o spoiler (o spoiler só existe na CLI do PC).
CORE_SRCS := $(filter-out core/src/bkr_spoiler.c,$(sort $(wildcard core/src/*.c))) core/generated/bkr_logic_data.c
C_SRCS := $(MOD_SRCS) $(CORE_SRCS)
C_OBJS := $(addprefix $(BUILD_DIR)/, $(C_SRCS:.c=.o))
C_DEPS := $(C_OBJS:.o=.d)

.PHONY: all nrm logic check package clean

all: $(TARGET)

$(TARGET): $(C_OBJS) $(LDSCRIPT)
	$(LD) $(C_OBJS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -MMD -MF $(@:.o=.d) -c -o $@

nrm: $(TARGET)
	$(RECOMP_MOD_TOOL) mod.toml $(BUILD_DIR)

logic:
	$(PYTHON) tools/compile_logic.py --raw logic/lighthouse_raw.json --overrides logic/overrides.json \
		--out core/generated --report logic/compiled_report.json

check:
	$(PYTHON) -m unittest discover -s tools/tests
	$(MAKE) -C core test mips-check mips-selftest PYTHON=$(PYTHON)
	$(PYTHON) tools/check_patch_conflicts.py --banjorecomp $(BANJORECOMP_DIR) --mod src

package: nrm
	$(if $(WEBSITE),,$(error defina WEBSITE com a URL pública do repositório, ex.: make package WEBSITE=https://github.com/SEU_USUARIO/bk-randomizer))
	$(PYTHON) tools/package_thunderstore.py --nrm $(BUILD_DIR)/ph_bk_randomizer.nrm --website $(WEBSITE) --out dist

clean:
	rm -rf $(BUILD_DIR) dist

-include $(C_DEPS)
