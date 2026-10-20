# CLAUDE.md

Mod **Banjo-Kazooie Randomizer** para o **Banjo: Recompiled** (BanjoRecomp v1.0.2). O seed é gerado
dentro do jogo por um núcleo C portátil (`core/`); o mesmo núcleo roda nos testes e na CLI `bkrgen`.
Especificação e plano completos: `docs/superpowers/plans/2026-10-17-bk-randomizer.md`.

## Preferências do dono do projeto
- Tudo em PT-BR: documentação, comentários, mensagens de commit e textos para o jogador.
- Identificadores (funções, tipos, variáveis, nomes de arquivo) em inglês; comentários e docstrings em português.
- Entrega completa: nada de TODO, "implementar depois", stub ou exemplo pela metade.
- Pergunte só em ambiguidade crítica. Fora disso, decida, registre a premissa (no commit ou em `docs/`) e siga.

## Upstream: política de IA do Banjo: Recompiled
O README do BanjoRecomp declara uma postura "No-AI" e o CONTRIBUTING.md proíbe código gerado por IA em
contribuições ao projeto. Este repositório é um mod independente, feito sobre o template CC0 e a API pública de mods.
- Nunca abra issue, PR ou discussão em `github.com/BanjoRecomp/*` a partir de uma sessão com IA, nem envie
  para lá código gerado aqui.
- Se achar um bug do base, registre em `docs/` para o dono do projeto avaliar e reportar por conta própria.
- O README do mod declara o uso de IA no desenvolvimento; mantenha essa declaração.

## Mapa
```
core/               núcleo portátil (C99, sem libc), testes host e CLI
  include/bkr/      headers do núcleo
  src/              implementação
  generated/        tabelas de lógica GERADAS por tools/compile_logic.py (não edite à mão)
  tests/            test_*.c (host), mips_selftest.c/.ld (autoteste MIPS)
  cli/bkrgen.c      gerador e spoiler no PC
src/                código do mod (MIPS): hooks, patches, entrega de itens, UI
logic/              lighthouse_raw.json (importado), overrides.json (política), compiled_report.json
tools/              importador, compilador de lógica, checagem de patches, autoteste MIPS (Python)
docs/               auditoria da lógica, checklist de QA, plano
include/, mod.ld    vindos do BKRecompModTemplate (não edite)
BanjoRecompSyms/, bk-decomp/   submódulos fixados (6820055 e 351ca15)
```

## Comandos
```bash
make check                          # testes Python + núcleo + mips-check + mips-selftest + conflitos de patch
make nrm                            # build/mod.elf e build/ph_bk_randomizer.nrm (RecompModTool no PATH)
make logic                          # regenera core/generated a partir de logic/*.json
make -C core test TEST=share_code   # roda só os testes cujo nome contém o trecho
make -C core cli && core/build/bkrgen --seed BANJO --spoiler spoiler.txt
make package WEBSITE=https://github.com/SEU_USUARIO/bk-randomizer   # pacote da Thunderstore em dist/
python3 tools/import_lighthouse.py --lighthouse ../lighthouse --out logic/lighthouse_raw.json
```
No Windows, rode tudo no Git Bash (o shell do Claude Code). Os Makefiles escolhem `gcc` e `python` sozinhos.
`make check` espera o checkout do BanjoRecomp em `../BanjoRecomp` (ou `BANJORECOMP_DIR=...`).

## Regras do núcleo (`core/`)
- C99 puro, sem libc e sem `<stdint.h>` (o mod compila com `-nostdinc -fno-builtin`).
- Sem ponto flutuante, sem divisão ou módulo de 64 bits, sem recursão.
- A pilha da thread principal do jogo tem 0x17F0 bytes: nada de arrays grandes na pilha (use `static`).
  O autoteste MIPS falha se o pico de pilha passar de 3064 bytes.
- Sem atribuição de structs grandes (o compilador emitiria `memcpy`, que o jogo não exporta): copie campo a campo.
- Tudo determinístico: mesmas opções e semente geram o mesmo placement no PC e no jogo.
- `make -C core mips-check` exige zero símbolos externos.

## Regras do mod (`src/`)
- Prefira `RECOMP_HOOK` e `RECOMP_HOOK_RETURN`. Hooks não mudam argumentos nem retorno (o runtime restaura o
  contexto depois de cada hook); eles só leem e escrevem memória.
- `RECOMP_PATCH` só em funções que o base não altera; `tools/check_patch_conflicts.py` (dentro do `make check`)
  garante isso. Ao patchear, copie o corpo original do decomp e cite a origem em comentário.
- Hooks em funções já alteradas pelo base são permitidos.
- RecompUI: toda criação ou mudança de elemento acontece entre `recompui_open_context` e
  `recompui_close_context`; abrir o mesmo contexto duas vezes derruba o jogo.
- Nenhum estado extra no save: tudo deriva do seed e dos bits vanilla de "localização checada".
- A extensão de save do base (`SaveFileExtensionData`) é reservada para ele; não grave nela.
- Itens de localizações locais são entregues no mesmo quadro em que o jogo marca a localização
  (`src/mod_collect.c`): o bit salvo e o item recebido nunca ficam dessincronizados.

## Valores-ouro (mudam juntos)
| Valor | Onde | Muda quando |
|---|---|---|
| `BKR_LOGIC_DATA_HASH` = `0x5C82A342` | `core/generated/bkr_logic_data.h` | `logic/*.json` ou `tools/compile_logic.py` mudam a lógica |
| impressão digital de "BANJO" = `623A1517` | `core/tests/test_settings.c`, `tools/tests/test_rng_reference.py` | os dados de lógica ou `BKR_GENERATOR_VERSION` mudam |
| `PLACEMENT_GOLDEN` = `31098721` | `core/Makefile` | o algoritmo de geração ou os dados mudam |

Mudança proposital: suba `BKR_GENERATOR_VERSION` (`core/include/bkr/bkr_settings.h`), atualize os valores,
anote no `CHANGELOG.md` e explique no commit. Mudança sem querer é bug: investigue antes de "atualizar o ouro".

## Fluxo
- TDD: escreva primeiro o teste que falha (`core/tests/test_*.c` ou `tools/tests/test_*.py`).
- Antes de cada commit: `make check` (e `make nrm` se mexeu em `src/` ou `mod.toml`).
- Commits pequenos, mensagem em PT-BR no imperativo (ex.: "adiciona fila de entrega de itens").
- Não edite `core/generated/*` à mão: mude `logic/overrides.json` e rode `make logic`.
