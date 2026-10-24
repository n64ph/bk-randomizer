# Banjo-Kazooie Randomizer — continuação no Claude Code local

> **Para agentes:** SUB-SKILL OBRIGATÓRIA: use superpowers:executing-plans para executar a Parte B Task por Task, na ordem. Os passos usam caixas de seleção (`- [ ]`) para acompanhamento. A Task L5 é interativa (o dono do projeto joga e informa os resultados): não a delegue a subagentes.

**Objetivo:** trazer para o PC o trabalho feito numa sessão do Claude Code na nuvem (20/10/2026), confirmar que build e testes dão no PC o mesmo resultado e concluir a v0.1.0: QA no jogo (Fase 5 do roteiro) e empacotamento, integração e publicação (Fase 6).

**Ponto de partida (24/10/2026):** repositório `https://github.com/n64ph/bk-randomizer`, branch `claude/tender-shannon-vad9lh`. Ele contém as Tasks 2 a 22 do roteiro, os passos 1 a 3 da Task 23 e este arquivo. Nenhum item do QA no jogo foi rodado ainda.

**Documento-mãe:** o roteiro, `docs/superpowers/plans/2026-10-17-bk-randomizer.md` (SHA-256 `871e0bd9b8408599314fb2eca1ca683a9a51f1c0f63d0cc51e8618f8bdc50b92`). Este arquivo não o substitui: registra o que já foi feito e diz como continuar. Quando uma Task daqui disser "como no roteiro", abra a Task correspondente lá.

---

## Sumário
- [Como usar este arquivo](#como-usar-este-arquivo)
- [Parte A — O que foi feito na nuvem](#parte-a--o-que-foi-feito-na-nuvem)
- [Parte B — Plano para o Claude Code local](#parte-b--plano-para-o-claude-code-local)
  - Task L1 — Clonar e restaurar os branches do roteiro
  - Task L2 — Ferramentas
  - Task L3 — Referências
  - Task L4 — Verificação no PC
  - Task L5 — QA no jogo
  - Task L6 — Thunderstore, integração e publicação
- [Checklist final](#checklist-final)
- [Apêndices](#apêndices)

## Como usar este arquivo
1. Salve este arquivo, por exemplo em `Downloads/2026-10-24-continuacao-local.md`. Depois do clone (Task L1) ele também estará no repositório, em `docs/superpowers/plans/`.
2. Abra o Claude Code na pasta de projetos onde ficarão `bk-randomizer/`, `lighthouse/` e `BanjoRecomp/` (ex.: `C:\projetos` ou `~/projetos`). Se já existir uma pasta `bk-randomizer` ali, o Claude Code deve parar e perguntar antes de mexer nela.
3. Cole o prompt abaixo, trocando o caminho pelo lugar onde você salvou este arquivo:

```text
/using-superpowers
O plano de continuação está em: C:\Users\SEU_USUARIO\Downloads\2026-10-24-continuacao-local.md
1. Leia "Como usar este arquivo" e a Parte A inteira. Depois execute a Parte B com superpowers:executing-plans, uma Task por vez e na ordem. Rode as verificações e só avance quando a saída bater.
2. Se uma saída não bater, pare e investigue com superpowers:systematic-debugging antes de mudar qualquer coisa. Nunca "corrija" um valor-ouro para fazer um teste passar.
3. Tudo em PT-BR; identificadores em inglês. Depois do clone (Task L1), siga o CLAUDE.md do repositório.
4. Nunca abra issue, PR ou discussão no GitHub do Banjo: Recompiled.
5. Na Task L5 (QA no jogo), me guie item por item do checklist; eu jogo e informo os resultados.
6. Push, release e envio à Thunderstore só com a minha confirmação, um de cada vez.
```

### Convenções
As mesmas do roteiro: **Rodar** são comandos a partir da raiz do repositório (`bk-randomizer/`), salvo indicação; **Esperado** são linhas que precisam aparecer na saída, com `…` nos trechos que variam; **Esperado (falha)** é o passo vermelho do TDD. No Windows, use o Git Bash (o shell do Claude Code) e troque `python3` por `python`.

## Parte A — O que foi feito na nuvem

### A1. Resumo
- Foi seguido o caminho completo do roteiro (Tasks 1 a 22, na ordem), não o atalho do zip. O zip `bk-randomizer-esqueleto-v0.1.0.zip` (SHA-256 `407a9e367179778f78dea9a8c1166b8572f2e81c2af8deeb014d1d6a4e041d66`) serviu de oráculo.
- Em cada Task com TDD, o teste falhou primeiro com a mensagem esperada e depois passou com a saída esperada.
- Os três valores-ouro conferem: `BKR_LOGIC_DATA_HASH` = `0x5C82A342`, impressão digital de `BANJO` = `623A1517` e `PLACEMENT_GOLDEN` = `31098721`.
- `make check` verde e `make nrm` gerando `build/ph_bk_randomizer.nrm` (saídas no Apêndice 2).
- São 20 commits sobre os 9 do template, todos enviados ao GitHub. Nada foi enviado aos repositórios do BanjoRecomp.

### A2. Ambiente usado na nuvem

| Ferramenta | Versão usada | O roteiro pede |
|---|---|---|
| Sistema | Ubuntu 24.04, x86-64 | Windows 10/11, Linux ou macOS |
| clang, ld.lld, llvm-nm | 18.1.3 (Ubuntu) | LLVM 18 (no Windows, 18.1.8) |
| gcc (testes no host) | 13.3.0 | gcc ou clang |
| GNU Make / Git | 4.3 / 2.43.0 | — |
| Python | 3.13.16 | 3.10 ou mais novo |
| unicorn / pyelftools | 2.1.4 / 0.33 | — |
| RecompModTool | release `mod-tool-release`, binário Linux | idem (no Windows, `.exe`) |

### A3. Adaptações em relação ao roteiro
1. **Branches.** A sessão na nuvem só podia enviar para `claude/tender-shannon-vad9lh`, então tudo está nele, em sequência linear. Equivalência com o fluxo do roteiro: `main` = `83c4c51` (fim da Task 3; a Task 4 não faz commit) e `feat/randomizer-v1` = topo do branch. A Task L1 recria os dois no PC.
2. **Task 2.** Em vez de clonar o template e renomear `origin` para `template`, o template foi trazido para dentro do repositório que já existia no GitHub: `git remote add template …`, `git fetch template` e `git checkout -B claude/tender-shannon-vad9lh 06bbc59c840ecdd0f2d094fdf5df50710c1475b4`. O histórico é o mesmo (os 9 commits do template). O remoto `template` não vai junto no clone; a Task L1 o recria.
3. **Mensagens de commit.** Os assuntos são exatamente os do roteiro. Cada commit traz também os trailers `Co-Authored-By` e `Claude-Session` da sessão.
4. **Execução.** superpowers:executing-plans (alternativa prevista no roteiro), sem subagentes. Os arquivos foram gravados a partir dos blocos do roteiro por script, byte a byte (a ferramenta está no Apêndice 1), e comparados com o zip.
5. **Submódulo aninhado com CRLF.** O arquivo `bk-decomp/tools/n64splat/src/splat/segtypes/n64/i1.py` foi versionado no n64splat com CRLF, mas o `.gitattributes` do n64splat diz `* text eol=lf`. Por isso o Git o marca como modificado logo após o checkout (`git status` mostra ` m bk-decomp`). Não é mudança nossa: na nuvem isso foi neutralizado só no clone local, e a Task L1 (passo 3) faz o mesmo. Nunca faça commit dentro dos submódulos.
6. **Referências.** `../lighthouse` (`c09a1f1`) e `../BanjoRecomp` (`v1.0.2`), como no roteiro.

### A4. Registro por Task

| Task | Commit | Verificação feita na nuvem |
|---|---|---|
| 1 | — | versões de A2; `Usage: RecompModTool [mod toml] [output folder]` |
| 2 | `10b342d` remove o exemplo do template | submódulos `6820055…` e `351ca15…` |
| 3 | `83c4c51` adiciona regras do projeto, licença MIT e roteiro | 7 arquivos e a cópia do roteiro |
| 4 | — | `c09a1f11041f50c2fd86d8aa60f8ee15a3ca13fd`, `ec859632cfa584e2272d32e46051543429a2baf4` e os dois `ls` |
| 5 | `29fa2ec` adiciona importador da lógica do Lighthouse | falha `No module named 'import_lighthouse'`; depois `Ran 6 tests OK`, 125 regiões, contagens por tipo e SHA-256 `86be1903…2b49` |
| 6 | `2f8ee26` adiciona compilador de lógica, correções auditadas e tabelas geradas | falha `No module named 'compile_logic'`; depois `Ran 16 tests OK` e `profundidade máxima de regra: 8  hash: 0x5C82A342` |
| 7 | `60d2a81` adiciona tipos do núcleo, executor de testes e testes das tabelas | falha cita `bkr/bkr_world.h`; depois `2 testes, 2850 verificações, 0 falhas` |
| 8 | `4464f0d` adiciona FNV-1a e construtor de texto do núcleo | falha cita `bkr/bkr_hash.h`; depois `5 testes, 2872 verificações, 0 falhas` |
| 9 | `11befa4` adiciona RNG xoshiro128** com referência em Python | referência `Ran 5 tests OK`; falha cita `bkr/bkr_rng.h`; depois `8 testes, 9986 verificações, 0 falhas` |
| 10 | `6ed0e32` adiciona nomes de itens, mundos e palavras do hash | erro de link em `bkr_item_name`; depois `11 testes, 10003 verificações, 0 falhas` |
| 11 | `32a7269` adiciona opções, código de compartilhamento e impressão digital | falha cita `bkr/bkr_settings.h`; depois `15 testes, 10173 verificações, 0 falhas` |
| 12 | `8187f45` adiciona avaliador de regras e busca por ponto fixo | falha cita `bkr/bkr_logic.h`; depois `20 testes, 10704 verificações, 0 falhas` |
| 13 | `3120ce5` adiciona geração por preenchimento assumido | falha cita `bkr/bkr_generate.h`; depois tentativas máximas 9, 9, 13 e 7 nos presets 0 a 3 e `29 testes, 637826 verificações, 0 falhas` (18 s) |
| 14 | `c4ac2d4` adiciona spoiler com esferas de coleta | falha cita `bkr/bkr_spoiler.h`; depois `30 testes, 637838 verificações, 0 falhas` |
| 15 | `7464b9a` adiciona CLI bkrgen | saída igual à esperada; `10` esferas no spoiler em inglês; opção inválida sai com código 2 |
| 16 | — | `mips-check: ok (nenhum símbolo externo)` |
| 17 | `1097cb3` adiciona autoteste do núcleo MIPS no Unicorn | `hash MIPS=31098721 esperado=31098721 tempo=0.16s pilha=268 bytes` |
| 18 | `cfbf6c3` adiciona checagem de conflitos de RECOMP_PATCH com o base | falha `No module named 'check_patch_conflicts'`; depois `Ran 26 tests OK` |
| 19 | `ef64317` adiciona build do mod, manifesto com opções e inicialização | `make` termina com `-o build/mod.elf` |
| 20 | `2723a61` adiciona hooks de coleta, supressão e entrega de itens | `make` sem avisos |
| 21 | `0636dd8` adiciona movimentos sorteados nos molehills | `make` sem avisos |
| 22 | `d13fa2e` adiciona interface e pulso por quadro do mod | `make nrm` e `make check` completos (Apêndice 2) |
| 23, passos 1–3 | `c04c97a` adiciona checklist de QA no jogo | `placement_hash=31098721`, `spoiler=spoiler.txt` |
| 23, passos 4–5 | — | pendente: QA no jogo (Task L5) |
| 24 | — | pendente (Task L6) |

Depois deles vem o commit que adiciona este arquivo.

### A5. Conferência com o esqueleto (zip)
- Os 63 blocos "Arquivo … (criar/substituir)" do roteiro são idênticos byte a byte aos arquivos correspondentes do zip, e a cópia do roteiro dentro do zip é idêntica ao roteiro.
- Os arquivos gerados também saíram idênticos: `logic/lighthouse_raw.json`, `logic/compiled_report.json`, `core/generated/bkr_logic_data.c` e `core/generated/bkr_logic_data.h`.
- No repositório, 66 dos 68 arquivos do zip estão idênticos. Faltam `tools/package_thunderstore.py` e `tools/tests/test_package_thunderstore.py`, que são da Task 24.
- O que está no repositório e não está no zip veio do template: `include/`, `mod.ld`, `.vscode/`, `generate_symbols.toml`, `.gitmodules` e os submódulos.

### A6. Revisão do mod contra o decomp, antes do QA
Conferido no decomp `351ca15` e no BanjoRecomp `v1.0.2`. Nada disso exigiu mudança de código.
- **Fluxos de coleta:** jiggy em `src/core2/ba/marker.c:548-549`, favo em `marker.c:575-582`, token em `src/core2/code_59A80.c:30-34`. São os únicos chamadores de `jiggyscore_setCollected`, `honeycombscore_set` e `mumboscore_set`.
- **Contadores:** `item_inc` é `item_adjustByDiffWithHud(item, 1)` (`src/core2/code_BEF20.c`). Somar em `ITEM_26_JIGGY_TOTAL` também soma em `ITEM_2B`, e a supressão vale para os dois.
- **Vida máxima:** `func_8034789C` (`code_BEF20.c:527-547`) faz `ITEM_13 = total % 6` e vida = `5 + MIN(3, total / 6)` (16 com vida dupla). Roda a cada carga e troca de mapa (`src/core2/gc/section.c:279`). `saveData_load` carrega os favos antes dos tokens (`src/core2/code_B5040.c:353-366`), o que justifica o `RECOMP_HOOK_RETURN("saveData_load")`.
- **6º favo:** a animação (`src/core2/fx/honeycarrierscore.c:232-238`) desconta 6, sobe a vida e chama `gcpausemenu_80314AC8(1)` 1,5 s depois. O contador que trava a pausa fica em `src/core2/gc/pauseMenu.c:1518`.
- **Molehills:** `moleTable` tem 10 entradas e o índice é `actorTypeSpecificField - 9`. O índice 7 é BGS (botas, nó 0x11) e o 8 é GV (tênis, nó 0x29). O patch de `chmole_additionalAbilityLearnActions` mantém o corpo original de `src/core2/ch/mole.c:182-218` e só acrescenta a guarda da câmera.
- **Tokens duplicados:** o base troca `0x3D` por `0x74` (MMM, dentro do Loggo) e `0x5E` por `0x5D` (CCW, primavera) em `patches/mumbo_token_patches.c`. A correção vem ligada por padrão, e o mod a liga de novo.
- **Modo de jogo:** os hooks só agem em `GAME_MODE_3_NORMAL`, então a demo de abertura e a reprodução da seleção de arquivo não trocam itens.
- **Código inválido:** o exemplo de código inválido do QA (`BKR1-000000-X`) é rejeitado pelo `bkrgen`.
- **Versões upstream:** em 24/10/2026 o Banjo: Recompiled continua em `v1.0.2` (HEAD `dbd5215`) e o template em `06bbc59`.

### A7. Estado do QA e o que o jogador deve esperar
- Em 20/10 o dono recebeu o `.nrm` gerado na nuvem (SHA-256 `f52d33b998a5bf7ed23d7094f6117ff88d269582d96603a8b0be5757e4756c0c`), o `spoiler.txt` do seed `BANJO` e as instruções de A1 e A2. Nenhum resultado foi informado, então o QA começa em A1, com o `.nrm` gerado no PC (Task L4).
- **Comportamentos esperados na v0.1** (não são falha):
  1. O coletável mantém aparência, som e diálogo originais. Um favo que contém um token toca a música de favo, e um jiggy que contém um token faz o Banjo dançar. O aviso na tela mostra o que veio.
  2. Os contadores de jiggies por mundo (HUD e pausa) contam lugares visitados, não itens recebidos.
  3. As falas do Bottles de "aprendeu todos os movimentos deste mundo" e as dicas do Covil sobre movimentos (`src/core2/code_DA760.c`) usam os movimentos vanilla de cada mundo (`chmole_learnedAllLevelAbilities`), não os sorteados.
  4. As limitações do README continuam valendo: custos acumulados em ordem crescente, e o quadro de vida dupla deve ficar para o fim no modo "Apenas vencível".

## Parte B — Plano para o Claude Code local

### Task L1: Clonar o repositório e restaurar os branches do roteiro

**Arquivos:** nenhum. Cria `bk-randomizer/` na pasta de projetos.

- [ ] **Passo 1: Clonar com os submódulos** (na pasta de projetos; o Git basta, o resto vem na Task L2)

**Rodar:**

```bash
git clone --recurse-submodules -b claude/tender-shannon-vad9lh https://github.com/n64ph/bk-randomizer.git bk-randomizer
cd bk-randomizer
git log --oneline -2
git submodule status
```

**Esperado:**

```text
… adiciona plano de continuação no Claude Code local
c04c97a adiciona checklist de QA no jogo
 6820055ca076e94e30e53d917bd9e5f71c28ca20 BanjoRecompSyms…
 351ca1580c10e550160ac11c77824fa9a498015e bk-decomp…
```

Se o repositório for privado, o Git pede login (no Windows, pelo Git Credential Manager).

- [ ] **Passo 2: Recriar `main`, `feat/randomizer-v1` e o remoto `template`**

**Rodar:**

```bash
git remote add template https://github.com/BanjoRecomp/BKRecompModTemplate.git
git branch main 83c4c5106711e72069dbb0989bedacc06c00dc80
git switch -c feat/randomizer-v1
git log --oneline -1 main
git branch --show-current
```

**Esperado:**

```text
83c4c51 adiciona regras do projeto, licença MIT e roteiro
feat/randomizer-v1
```

Daqui em diante, os commits vão para `feat/randomizer-v1`, como no roteiro. O branch `claude/tender-shannon-vad9lh` fica como está no GitHub.

- [ ] **Passo 3: Árvore limpa**

**Rodar:**

```bash
git status --short
```

**Esperado:** nenhuma linha. Se aparecer ` m bk-decomp` (o caso de A3, item 5), neutralize só no clone local e confira de novo:

```bash
GD="$(git -C bk-decomp/tools/n64splat rev-parse --absolute-git-dir)"
mkdir -p "$GD/info"
echo 'src/splat/segtypes/n64/i1.py -text' >> "$GD/info/attributes"
git status --short
```

Se outro arquivo de submódulo aparecer como modificado, confira com `git -C <submódulo> diff --ignore-cr-at-eol --stat`. Se a saída for vazia, a diferença é só de fim de linha: acrescente o caminho com `-text` ao `info/attributes` daquele submódulo. Se não for vazia, pare e investigue.

### Task L2: Ferramentas (Task 1 do roteiro)

**Arquivos:** nenhum.

- [ ] **Passo 1: Instalar o que faltar**

Siga a "Task 1: Instalar as ferramentas" do roteiro (`docs/superpowers/plans/2026-10-17-bk-randomizer.md`): no Windows, Chocolatey com `git make mingw python` e `llvm --version 18.1.8`, `pip install unicorn pyelftools` e o `RecompModTool.exe` no `PATH`; no Linux e no macOS, os comandos da mesma Task. Se este PC já tem tudo, vá direto ao passo 2.

- [ ] **Passo 2: Conferir as versões** (no Windows, troque `python3` por `python`)

**Rodar:**

```bash
clang --version | head -1
ld.lld --version
llvm-nm --version | head -2
make --version | head -1
git --version
python3 --version
python3 -c "import unicorn, elftools; print('unicorn e pyelftools ok')"
RecompModTool 2>&1 | head -1
```

**Esperado:**

```text
clang version 18…
LLD 18…
LLVM version 18…
GNU Make …
git version …
Python 3.…
unicorn e pyelftools ok
Usage: …RecompModTool… [mod toml] [output folder]
```

### Task L3: Referências ao lado do repositório (Task 4 do roteiro)

**Arquivos:** nenhum no repositório. Cria `../lighthouse` e `../BanjoRecomp`.

- [ ] **Passo 1: Clones esparsos nos commits validados**

**Rodar:**

```bash
git clone -q --filter=blob:none --no-checkout https://github.com/HarbourMasters/lighthouse.git ../lighthouse
git -C ../lighthouse sparse-checkout set --no-cone /LICENSE.md /README.md /src/port/Rando/ /src/lair/
git -C ../lighthouse checkout -q c09a1f11041f50c2fd86d8aa60f8ee15a3ca13fd
git clone -q --filter=blob:none --no-checkout https://github.com/BanjoRecomp/BanjoRecomp.git ../BanjoRecomp
git -C ../BanjoRecomp sparse-checkout set --no-cone /patches/ /banjo.us.rev0.toml /README.md /CONTRIBUTING.md
git -C ../BanjoRecomp checkout -q v1.0.2
```

- [ ] **Passo 2: Conferir**

**Rodar:**

```bash
git -C ../lighthouse rev-parse HEAD
git -C ../BanjoRecomp rev-parse HEAD
ls ../lighthouse/src/port/Rando/StaticData/Checks.cpp ../BanjoRecomp/patches/mumbo_token_patches.c
```

**Esperado:**

```text
c09a1f11041f50c2fd86d8aa60f8ee15a3ca13fd
ec859632cfa584e2272d32e46051543429a2baf4
../lighthouse/src/port/Rando/StaticData/Checks.cpp
../BanjoRecomp/patches/mumbo_token_patches.c
```

### Task L4: Verificação no PC

**Arquivos:** nenhum versionado. Gera `build/` e `spoiler.txt`, que o `.gitignore` já ignora.

- [ ] **Passo 1: Bateria completa**

**Rodar:**

```bash
make check
```

**Esperado:**

```text
Ran 26 tests
OK
30 testes, 637838 verificações, 0 falhas
mips-check: ok (nenhum símbolo externo)
hash MIPS=31098721 esperado=31098721 tempo=…s pilha=268 bytes
patches do Banjo: Recompiled: 145 funções + 6 patches de instrução | patches do mod: 4 -> ['chmole_additionalAbilityLearnActions', 'honeycombscore_get_total', 'item_adjustByDiffWithHud', 'item_adjustByDiffWithoutHud']
ok: nenhum conflito de RECOMP_PATCH
```

O hash MIPS tem que ser `31098721`. A pilha deu 268 bytes com o LLVM 18.1.3. Se der outro valor com o hash certo, anote a versão do LLVM e investigue antes de seguir (o limite do autoteste é 3064 bytes).

- [ ] **Passo 2: Pacote do mod**

**Rodar:**

```bash
make nrm
ls build/ph_bk_randomizer.nrm
```

**Esperado:**

```text
RecompModTool mod.toml build
build/ph_bk_randomizer.nrm
```

- [ ] **Passo 3: Seed padrão e spoiler**

**Rodar:**

```bash
make -C core cli
./core/build/bkrgen --seed BANJO --spoiler spoiler.txt
```

**Esperado:**

```text
codigo=BKR1-GG000M-BANJO
fingerprint=623A1517
save=bkr_623A1517
placement_hash=31098721
tentativas=1
hash=Blubber Cheato Grunty Tiptup Bottles
spoiler=spoiler.txt
```

- [ ] **Passo 4 (opcional, se você ainda tem o zip): comparar com o esqueleto**

Extraia `bk-randomizer-esqueleto-v0.1.0.zip` numa pasta fora do repositório, por exemplo `../esqueleto-v0.1.0` (pelo Explorer, `Expand-Archive` ou `unzip`).

**Rodar:**

```bash
Z=../esqueleto-v0.1.0
same=0; diff=0; missing=""
for f in $(cd "$Z" && find . -type f | sed 's|^\./||' | sort); do
  if [ -f "$f" ]; then
    if cmp -s "$Z/$f" "$f"; then same=$((same+1)); else diff=$((diff+1)); echo "DIFERENTE: $f"; fi
  else
    missing="$missing $f"
  fi
done
echo "iguais: $same; diferentes: $diff; ausentes:$missing"
```

**Esperado:**

```text
iguais: 66; diferentes: 0; ausentes: tools/package_thunderstore.py tools/tests/test_package_thunderstore.py
```

### Task L5: QA no jogo (Task 23 do roteiro, passos 3 a 5)

**Arquivos:**
- Criar depois da rodada: `docs/qa/0.1.0.md` (cópia de `docs/qa-checklist.md` com os resultados)

O checklist é `docs/qa-checklist.md`. Siga-o na ordem (A → G), um item por vez: diga ao jogador o que fazer, onde e o que observar, e anote "ok" ou "falhou + observação". Este é o roteiro de jogo; as tabelas abaixo dizem onde testar cada item.

- [ ] **Passo 1: Preparar a rodada**
1. Use o `build/ph_bk_randomizer.nrm` gerado na Task L4, não o da nuvem: o QA tem que validar o mesmo arquivo que vai para o pacote. Instale pelo menu **Mods → Install Mods** do Banjo: Recompiled 1.0.2; se a cópia de 20/10 estiver instalada, ela é substituída (mesmo id `ph_bk_randomizer`).
2. Opções do mod no padrão, com **Log de depuração = Ligado**. As opções são lidas quando a partida começa: depois de mudar qualquer opção, feche e abra o jogo.
3. Abra o jogo com o console: no Windows, `BanjoRecompiled.exe --show-console`; no Linux e no macOS, pelo terminal.
4. O `spoiler.txt` do seed `BANJO` é o da Task L4.

- [ ] **Passo 2: Rodar o checklist com o jogador**

**Onde testar cada item no seed `BANJO`** (opções padrão; "esfera" é a rodada do spoiler em que a localização fica alcançável, calculada com o mesmo laço do `bkr_spoiler.c`):

| Item | Onde e como | O que conferir |
|---|---|---|
| A1 | console, logo depois de iniciar | `[bkr] codigo=BKR1-GG000M-BANJO fingerprint=623A1517 placement_hash=31098721 tentativas=1 save=bkr_623A1517` |
| A2 | seleção de arquivo | o painel mostra o texto exato do item A2 do checklist: o código `BKR1-GG000M-BANJO` e as palavras `Blubber Cheato Grunty Tiptup Bottles`; as fichas aparecem vazias (save próprio do seed) |
| A3 | salvar pela pausa (salvar e sair) | existe `saves/ph_bk_randomizer/bkr_623A1517.bin` na pasta de dados (Windows `%LOCALAPPDATA%\BanjoRecompiled`, Linux `~/.config/BanjoRecompiled`, macOS `~/Library/Application Support/BanjoRecompiled`) |
| A4 | opção **Código de compartilhamento** = `BKR1-000000-X`, reiniciar | painel `Randomizer: seed ou codigo invalido (veja as opcoes do mod)`; console `[bkr] codigo de compartilhamento invalido; randomizer desligado`; saves normais. Depois esvazie o código. |
| A5 | **Randomizer** = Desligado, reiniciar | nenhum texto do mod; saves normais. Depois religue. |
| B1 | `GL Jiggy - Lobby` (esfera 1, o primeiro jiggy do Covil) → Token | Banjo dança; aviso `Voce recebeu: Token do Mumbo`; tokens +1; jiggies para gastar (pausa) iguais |
| B2 | `MM Token - Behind Pink Jinjo` (esfera 2, regra `true`) → Jiggy; alternativa: `MM Token - Inside Tickers Tower` | aviso de Jiggy; jiggies para gastar +1 |
| B3 | favos de SM (esfera 1): Colliwobble, Stump e Waterfall → Jiggy; Quarries, Tree e Underwater → Token | o contador de favos não muda; o item sorteado chega |
| B4 | `MM Jiggy - Mumbos Skull` → Jiggy, ou `MM Token - Behind Ruins` → Token (esfera 2) | nenhum aviso; comportamento vanilla |
| B5 | seed auxiliar `QA1090` (procedimento abaixo) | animação de vida extra; vida máxima +1; depois dela a pausa abre normalmente |
| B6 | qualquer localização trocada, ex.: `MM Jiggy - Chimpy` → Favo (esfera 2) | salvar e sair logo depois; ao carregar, o item recebido continua lá |
| B7 | localização trocada durante um pulo, sob dano ou logo antes de uma transição de mapa | o item chega, o contador fica certo depois da troca de mapa, o jogo não trava |
| C1 | molehills de MM (esfera 2): Beak Buster → Trote de Garras; Talon Trot → Salto de Mola; Use Eggs → Disparo de Ovos (igual ao vanilla) | texto do movimento ensinado |
| C2 | `TTC Molehill - Flight` (esfera 3) ensina Botas Pernaltas; `BGS Molehill - Wading Boots` (esfera 8) ensina Tênis Turbo | o Bottles aparece enquadrado e a câmera não vai para outro lugar |
| C3 | falar de novo com um molehill já usado | lembrete do movimento ensinado |
| C4 | Disparo de Ovos em MM (+50 ovos); Voo em `TTC Molehill - Shock Jump` (+25 penas vermelhas); Asa Maravilha em CC (esfera 4, +5 penas douradas) | os brindes acompanham o texto |
| C5 | depois de aprender Salto de Mola (MM) e Voo (TTC) | os pads aparecem nos mundos |
| C6 | voltar à seleção de arquivo e carregar | o molehill continua ensinando o movimento sorteado |
| D1–D3 | como no checklist | — |
| D4 | sai naturalmente ao alternar entre `BANJO` e `QA1090` | cada seed com o próprio save |
| E1 | quadro de MM (1 jiggy) com um jiggy recebido de favo de SM | o mundo abre |
| E2 | cupim (5 tokens): os 4 tokens da esfera 1 + 1 de MM | desconta 5 |
| F1 | seguir as esferas 1 a 4 do spoiler | nada fora da lógica é necessário |
| F2 | antes da 1.0 | partida completa |
| G1, G2 | como no checklist | — |

**B5 com o seed auxiliar `QA1090`.** No seed `BANJO`, o 6º favo só chega na esfera 7. No `QA1090` (opções padrão, só a semente muda), ele chega cedo:
1. Opções: **Semente** = `QA1090`, código vazio. Feche e abra o jogo.
2. Console: `[bkr] codigo=BKR1-GG000M-QA1090 fingerprint=BDAFD739 placement_hash=8AE8A2AB tentativas=1 save=bkr_BDAFD739`. Seleção de arquivo: `BKR1-GG000M-QA1090  |  Tumblar Tumblar Nipper Dingpot Loggo`. Spoiler: `./core/build/bkrgen --seed QA1090 --spoiler spoiler_qa1090.txt`.
3. Em SM, os favos Quarries, Stump e Waterfall dão favo (fluxo vanilla: 3 favos). Underwater dá o jiggy do quadro de MM; Colliwobble e Tree dão tokens.
4. No Covil, `GL Jiggy - Lobby` dá favo (4º).
5. Em MM, só com os movimentos de SM: `MM Token - Stump Near Conga` dá favo (5º) e `MM Jiggy - Hill` dá favo (6º). O 6º vir de um jiggy é o caso de risco do roteiro: dança e animação de vida extra ao mesmo tempo. Sobra `MM Jiggy - Mumbos Skull` (favo).
6. Conferir: animação de vida extra, vida máxima de 5 para 6 e, depois dela, a pausa abre normalmente.
7. Opcional: em outra ficha do mesmo seed, inverta a ordem (Hill como 5º e Stump Near Conga como 6º) para testar o 6º favo vindo de um token.
8. Se for prático, faça B6 e B7 aqui também. Depois volte para **Semente** = `BANJO`; isso já testa D4.

**Quando um item falha:** pare a rodada e colete o que a seção "Quando algo falha" do checklist pede (código do seed, versões do mod e do jogo, passo exato, saída do console e o save). Investigue com superpowers:systematic-debugging antes de mudar código. Depois de corrigir: `make check`, `make nrm`, reinstale e repita os itens afetados. Correções em `src/` não mudam os valores-ouro; se uma correção mudar o gerador ou a lógica, siga a regra dos valores-ouro do CLAUDE.md. Bug do Banjo: Recompiled vai para `docs/` como nota para o dono avaliar.

- [ ] **Passo 3: Registrar o resultado** (só depois da rodada)

Copie `docs/qa-checklist.md` para `docs/qa/0.1.0.md`, marque cada item com o resultado e acrescente no topo a versão do jogo, o sistema e a data. Para B5, registre também o seed `QA1090`.

**Rodar:**

```bash
git add docs/qa/0.1.0.md
git commit -q -m "registra a rodada de QA da 0.1.0"
git log --oneline -1
```

**Esperado:**

```text
… registra a rodada de QA da 0.1.0
```

### Task L6: Thunderstore, integração e publicação (Task 24 do roteiro)

**Arquivos:**
- Criar: `tools/tests/test_package_thunderstore.py`, `tools/package_thunderstore.py` (blocos da Task 24 do roteiro)
- Modificar: `CHANGELOG.md` (data da versão)

Só comece depois do QA sem item "falhou" em aberto.

- [ ] **Passo 1: Ferramenta de extração**

Grave o script do Apêndice 1 em `../bkr-tools/extract_plan_blocks.py` (fora do repositório) e confira que ele lê o roteiro inteiro.

**Rodar:**

```bash
python3 ../bkr-tools/extract_plan_blocks.py docs/superpowers/plans/2026-10-17-bk-randomizer.md --list | tail -2
python3 ../bkr-tools/extract_plan_blocks.py docs/superpowers/plans/2026-10-17-bk-randomizer.md --task 23 --compare .
```

**Esperado:**

```text
Task 24  linha  7138    2928 bytes  tools/tests/test_package_thunderstore.py
Task 24  linha  7229    6646 bytes  tools/package_thunderstore.py
1 blocos comparados, 0 diferenças
```

- [ ] **Passo 2: Testes primeiro, ver falhar** (passos 1 e 2 da Task 24)

**Rodar:**

```bash
python3 ../bkr-tools/extract_plan_blocks.py docs/superpowers/plans/2026-10-17-bk-randomizer.md --path tools/tests/test_package_thunderstore.py --out .
python3 -m unittest discover -s tools/tests -v
```

**Esperado (falha):**

```text
ModuleNotFoundError: No module named 'package_thunderstore'
```

- [ ] **Passo 3: Implementar e ver passar** (passos 3 e 4 da Task 24)

**Rodar:**

```bash
python3 ../bkr-tools/extract_plan_blocks.py docs/superpowers/plans/2026-10-17-bk-randomizer.md --path tools/package_thunderstore.py --out .
python3 -m unittest discover -s tools/tests -v
python3 ../bkr-tools/extract_plan_blocks.py docs/superpowers/plans/2026-10-17-bk-randomizer.md --task 24 --compare .
```

**Esperado:**

```text
Ran 32 tests
OK
2 blocos comparados, 0 diferenças
```

- [ ] **Passos 4 a 6: CHANGELOG datado, commit, verificação final e pacote** (passos 5 e 6 da Task 24)

Rode o passo 5 da Task 24 do roteiro exatamente como está, no dia da publicação (ele grava a data de hoje no `CHANGELOG.md` e faz o commit). No passo 6, a URL é a do repositório que já existe:

```bash
make check
make package WEBSITE=https://github.com/n64ph/bk-randomizer
```

**Esperado:** a saída do passo 6 da Task 24, que termina com:

```text
Ran 32 tests
30 testes, 637838 verificações, 0 falhas
mips-check: ok (nenhum símbolo externo)
hash MIPS=31098721 esperado=31098721 tempo=…s pilha=268 bytes
ok: nenhum conflito de RECOMP_PATCH
pacote: dist/BK_Randomizer-0.1.0.zip
conteúdo: ph_bk_randomizer.nrm, icon.png, manifest.json, README.md, CHANGELOG.md
```

Se ainda tiver o zip extraído, repita o passo 4 da Task L4: agora devem dar `iguais: 67; diferentes: 1` (o `CHANGELOG.md`, que ganhou a data; no zip ele diz "não publicada") e nenhum ausente.

- [ ] **Passo 7: Integrar o branch e marcar a versão** (passo 7 da Task 24, sem mudança)

**Rodar:**

```bash
git switch main
git merge -q --no-ff feat/randomizer-v1 -m "integra o Banjo-Kazooie Randomizer 0.1.0"
git tag -a v0.1.0 -m "Banjo-Kazooie Randomizer 0.1.0"
git log --oneline -1
```

**Esperado:**

```text
… integra o Banjo-Kazooie Randomizer 0.1.0
```

- [ ] **Passo 8: Publicar** (passo 8 da Task 24, com adaptações; cada envio só com a confirmação do dono)
1. Antes de tudo, confira se saiu versão nova do Banjo: Recompiled (`git ls-remote --tags https://github.com/BanjoRecomp/BanjoRecomp.git`). Se sim, faça checkout da nova tag em `../BanjoRecomp`, rode `make check` de novo e repita no jogo os itens afetados.
2. **GitHub:** o repositório já existe e o `origin` veio do clone, então pule o `git remote add origin`. Confira se ele está público (o `manifest.json` aponta para ele). Depois, com confirmação:

```bash
git push -u origin main --tags
```

   Em **Settings → General → Default branch**, troque o branch padrão para `main`. Hoje o padrão é `claude/tender-shannon-vad9lh`, o primeiro branch enviado. Apagar esse branch no GitHub depois é opcional e decisão do dono.
3. **Release `v0.1.0`:** anexe `build/ph_bk_randomizer.nrm` e `dist/BK_Randomizer-0.1.0.zip`. Na descrição, inclua o código de exemplo `BKR1-GG000M-BANJO`, as limitações do README e a frase sobre o uso de IA.
4. **Thunderstore:** comunidade **Banjo: Recompiled** (`thunderstore.io/c/banjo-recompiled`), **Upload**, enviar `dist/BK_Randomizer-0.1.0.zip`.
5. Não divulgue o mod nos canais oficiais do Banjo: Recompiled como se fosse parte do projeto (ele é independente).

## Checklist final
Estado em 24/10/2026 dos itens de "Antes de chamar a v0.1.0 de pronta" do roteiro:
- [x] `make check` verde: 26 testes Python (32 depois da Task 24); 30 testes C com 0 falhas; `mips-check`; autoteste MIPS com `31098721`; nenhum conflito.
- [x] `make nrm` gera `build/ph_bk_randomizer.nrm`.
- [ ] Checklist de QA completo e registrado em `docs/qa/0.1.0.md`, sem item "falhou" em aberto (Task L5).
- [ ] CHANGELOG datado, tag `v0.1.0` e branch integrado em `main` (Task L6).
- [x] O README declara o uso de IA; nada foi enviado aos repositórios do BanjoRecomp.

## Apêndices

### Apêndice 1: `extract_plan_blocks.py`
Grava, byte a byte, os arquivos que o roteiro descreve nos blocos "Arquivo … (criar/substituir)". Fica fora do repositório (`../bkr-tools/`), porque não faz parte do mod. Usa só a biblioteca padrão.

```python
"""Extrai byte a byte os arquivos descritos no roteiro em blocos
"Arquivo `caminho` (criar):" ou "Arquivo `caminho` (substituir):".

Uso, a partir da raiz do repositório (no Windows, troque python3 por python):
  python3 ../bkr-tools/extract_plan_blocks.py ROTEIRO --list
  python3 ../bkr-tools/extract_plan_blocks.py ROTEIRO --path tools/package_thunderstore.py --out .
  python3 ../bkr-tools/extract_plan_blocks.py ROTEIRO --task 24 --compare ../esqueleto-v0.1.0
ROTEIRO = docs/superpowers/plans/2026-10-17-bk-randomizer.md
"""
import argparse
import re
import sys
from pathlib import Path

HEADER = re.compile(r"^Arquivo `([^`]+)` \((criar|substituir)\):$")
TASK = re.compile(r"^### Task (\d+):")
FENCE = re.compile(r"^(`{3,})")


def parse_blocks(plan_path):
    """Lista os blocos de arquivo do roteiro, na ordem em que aparecem."""
    lines = Path(plan_path).read_text(encoding="utf-8").replace("\r\n", "\n").split("\n")
    blocks = []
    task = None
    i = 0
    while i < len(lines):
        m = TASK.match(lines[i])
        if m:
            task = int(m.group(1))
        m = HEADER.match(lines[i].rstrip())
        if m:
            j = i + 1
            while not FENCE.match(lines[j]):
                if lines[j].strip():
                    sys.exit(f"linha {j + 1}: texto entre o cabeçalho e o bloco de código")
                j += 1
            fence = FENCE.match(lines[j]).group(1)
            k = j + 1
            while lines[k] != fence:
                k += 1
            blocks.append({"task": task, "path": m.group(1), "line": i + 1,
                           "content": "\n".join(lines[j + 1:k]) + "\n"})
            i = k
        i += 1
    return blocks


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("plan", help="caminho do roteiro")
    ap.add_argument("--task", type=int, help="só os blocos desta Task")
    ap.add_argument("--path", action="append", help="só este arquivo (pode repetir)")
    ap.add_argument("--list", action="store_true", help="lista os blocos")
    ap.add_argument("--out", help="grava os blocos sob esta pasta (normalmente a raiz do repositório)")
    ap.add_argument("--compare", help="compara os blocos com os arquivos desta pasta")
    args = ap.parse_args()
    blocks = parse_blocks(args.plan)
    if args.task is not None:
        blocks = [b for b in blocks if b["task"] == args.task]
    if args.path:
        missing = set(args.path) - {b["path"] for b in blocks}
        if missing:
            sys.exit(f"sem bloco no roteiro para: {', '.join(sorted(missing))}")
        blocks = [b for b in blocks if b["path"] in args.path]
    for b in blocks:
        data = b["content"].encode("utf-8")
        if args.list:
            print(f"Task {b['task']:>2}  linha {b['line']:>5}  {len(data):>6} bytes  {b['path']}")
        if args.out:
            target = Path(args.out) / b["path"]
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
    if args.compare:
        diff = 0
        for b in blocks:
            other = Path(args.compare) / b["path"]
            if not other.is_file() or other.read_bytes() != b["content"].encode("utf-8"):
                diff += 1
                print(f"diferente ou ausente: {b['path']}")
        print(f"{len(blocks)} blocos comparados, {diff} diferenças")
        return 1 if diff else 0
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

### Apêndice 2: saídas de referência da nuvem
`make check` (Task 22, linhas de resumo):

```text
Ran 26 tests in 0.109s
OK
30 testes, 637838 verificações, 0 falhas
mips-check: ok (nenhum símbolo externo)
hash MIPS=31098721 esperado=31098721 tempo=0.29s pilha=268 bytes
patches do Banjo: Recompiled: 145 funções + 6 patches de instrução | patches do mod: 4 -> ['chmole_additionalAbilityLearnActions', 'honeycombscore_get_total', 'item_adjustByDiffWithHud', 'item_adjustByDiffWithoutHud']
ok: nenhum conflito de RECOMP_PATCH
```

Início do `spoiler.txt` do seed `BANJO`:

```text
Banjo-Kazooie Randomizer - Spoiler
Codigo: BKR1-GG000M-BANJO
Hash: Blubber Cheato Grunty Tiptup Bottles
Impressao digital: 623A1517
Tentativas: 1
Opcoes: logica=glitchless shuffle=mixed scope=game moves=molehills access=all

== Esferas de coleta ==
Esfera 1: +3 jiggies, +4 tokens, +0 favos
Esfera 2: +3 jiggies, +5 tokens, +3 favos
  MM Molehill - Beak Buster: Trote de Garras
  MM Molehill - Talon Trot: Salto de Mola
  MM Molehill - Use Eggs: Disparo de Ovos
Esfera 3: +10 jiggies, +10 tokens, +1 favos
  TTC Molehill - Shock Jump: Voo
  TTC Molehill - Flight: Botas Pernaltas
Esfera 4: +7 jiggies, +12 tokens, +1 favos
  CC Molehill - Wonderwing: Asa Maravilha
```

Conteúdo do `.nrm` gerado na nuvem (só para referência: com outro build do LLVM, `mod_binary.bin` pode mudar sem que isso seja erro; o `.nrm` inteiro muda a cada build, porque o zip guarda a hora):

| Arquivo | Bytes | SHA-256 |
|---|---|---|
| `mod_binary.bin` | 43732 | `a1c1daacc9cbc709fa0dff2e78dab0a2961c65e4de30c9da436a2b65ec344c7d` |
| `mod_syms.bin` | 21212 | `0cbeb43d8fe569a2ea3444adc93d4d819aa589caeb8ee02dc4c566d0f9a938bd` |
| `mod.json` | 5448 | `83457a01f2b3eade2d56cd4a4038b396220ae09c0ec63af2cd1b63c01e0a729d` |

### Apêndice 3: solução de problemas desta continuação
Vale também a tabela "Solução de problemas" do roteiro.

| Sintoma | Causa provável | O que fazer |
|---|---|---|
| `git clone` pede usuário e senha | repositório privado | entre com a conta do GitHub (no Windows, pelo Git Credential Manager) |
| `git status` mostra ` m bk-decomp` | CRLF do n64splat (A3, item 5) | Task L1, passo 3 |
| `Filename too long` no clone (Windows) | limite de caminho do Windows | `git config --global core.longpaths true`, apague a pasta e clone de novo |
| `pilha=` diferente de 268 com o hash certo | outro build do LLVM 18 | anote a versão, investigue antes de seguir; o limite é 3064 bytes |
| com o randomizer ligado, a seleção de arquivo mostra os saves normais | a troca de save não aconteceu | confira a linha `[bkr]` no console e registre como falha de A2/A3 |
| `extract_plan_blocks.py` diz "sem bloco no roteiro" | caminho digitado diferente do cabeçalho | rode com `--list` e copie o caminho exato |
