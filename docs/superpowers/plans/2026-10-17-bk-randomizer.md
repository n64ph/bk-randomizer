# Banjo-Kazooie Randomizer para Banjo: Recompiled — Roteiro para o Claude Code

> **Para agentes:** SUB-SKILL OBRIGATÓRIA: use superpowers:subagent-driven-development (recomendado) ou superpowers:executing-plans para implementar este plano tarefa por tarefa. Os passos usam caixas de seleção (`- [ ]`) para acompanhamento.

**Objetivo:** criar, com testes, um mod randomizer com lógica para o Banjo: Recompiled que embaralha jiggies, favos vazios, tokens do Mumbo e os movimentos ensinados nos molehills, gera o seed dentro do jogo e oferece no PC uma CLI que produz o mesmo seed e o spoiler.

**Arquitetura:** a lógica de regiões do Lighthouse (CC0) é importada para JSON, corrigida por um arquivo de política revisado e compilada em bytecode RPN dentro de tabelas C. Um núcleo C99 sem libc gera o seed por preenchimento assumido (o algoritmo do OoT Randomizer) e roda igual no PC (testes e CLI) e no jogo (MIPS). O mod intercepta a coleta com hooks, troca o item de cada localização no mesmo quadro, reescreve a tabela dos molehills e usa um arquivo de save por seed.

**Stack:** C99 (clang/lld 18 para MIPS; gcc ou clang no PC), Python 3.10+ (só biblioteca padrão, mais `unicorn` e `pyelftools` no autoteste MIPS), GNU Make, Git, RecompModTool (N64Recomp), Banjo: Recompiled 1.0.2.

---

## Sumário
- [Como usar este roteiro](#como-usar-este-roteiro)
- [Parte A — Especificação](#parte-a--especificação)
- [Parte B — Plano de implementação](#parte-b--plano-de-implementação)
  - Fase 0 — Ambiente e repositório (Tasks 1–4)
  - Fase 1 — Pipeline de lógica (Tasks 5–6)
  - Fase 2 — Núcleo C portátil (Tasks 7–15)
  - Fase 3 — Portabilidade MIPS (Tasks 16–17)
  - Fase 4 — Mod (Tasks 18–22)
  - Fase 5 — QA no jogo (Task 23)
  - Fase 6 — Empacotamento e publicação (Task 24)
- [Autorrevisão e checklist final](#autorrevisão-e-checklist-final)
- [Apêndices](#apêndices)

## Como usar este roteiro

### O que você precisa ter
- Um PC com Windows 10/11, Linux ou macOS.
- O Banjo: Recompiled 1.0.2 instalado e funcionando com a sua ROM (Banjo-Kazooie US 1.0, `.z64`, SHA-1 `1fe1632098865f639e22c11b9a81ee8f29c75d7a`).
- O Claude Code com o plugin **superpowers** (o mesmo que fornece `/using-superpowers`).

### Passo a passo
1. Salve este arquivo, por exemplo em `Downloads/roteiro-bk-randomizer.md`.
2. Crie uma pasta para projetos (ex.: `C:\projetos` ou `~/projetos`) e abra o Claude Code nela.
3. Cole o prompt abaixo, trocando o caminho pelo lugar onde você salvou o roteiro:

```text
/using-superpowers
O roteiro completo do projeto está em: C:\Users\SEU_USUARIO\Downloads\roteiro-bk-randomizer.md
1. Leia "Como usar este roteiro" e a Parte A inteira. Não carregue a Parte B de uma vez: extraia cada Task na hora de executá-la.
2. Execute a Parte B com superpowers:subagent-driven-development, uma Task por vez e na ordem. Cada Task traz os arquivos completos, os comandos e a saída esperada: reproduza exatamente, rode as verificações e só avance quando a saída bater.
3. Se uma saída não bater, pare e investigue com superpowers:systematic-debugging antes de mudar qualquer coisa. Nunca "corrija" um valor-ouro para fazer um teste passar.
4. Tudo em PT-BR; identificadores em inglês. A partir da Task 3, siga o CLAUDE.md do repositório.
5. Nunca abra issue, PR ou discussão no GitHub do Banjo: Recompiled.
6. Na Fase 5 (QA no jogo), me guie item por item do checklist; eu jogo e informo os resultados.
```

4. Acompanhe pela lista de tarefas do Claude Code. As Fases 0–4 rodam sozinhas; a Fase 5 precisa de você jogando; a Fase 6 publica.

### Convenções deste roteiro
- **Arquivo `caminho` (criar):** crie o arquivo com exatamente o conteúdo do bloco. **(substituir):** o arquivo já existe (veio do template ou de uma Task anterior) e deve ser sobrescrito por inteiro.
- **Rodar:** comandos para o terminal, sempre a partir da raiz do repositório (`bk-randomizer/`), salvo quando o bloco diz outra coisa. No Windows, use o **Git Bash** (o shell do Claude Code) e troque `python3` por `python`.
- **Esperado:** linhas que precisam aparecer na saída; `…` marca trechos que variam (tempo, caminhos, versões). **Esperado (falha):** o comando precisa falhar com essa mensagem: é o passo "vermelho" do TDD. A mensagem exata de erro de compilação varia entre gcc e clang; o que importa é o nome do arquivo ou símbolo citado.
- **Valores-ouro:** `0x5C82A342` (hash dos dados de lógica), `623A1517` (impressão digital do seed `BANJO`) e `31098721` (hash do placement desse seed). Eles só mudam de propósito (ver CLAUDE.md, Task 3).

### Atalho opcional: o pacote do esqueleto
Se você recebeu junto o arquivo `bk-randomizer-esqueleto-v0.1.0.zip`, ele contém exatamente os arquivos que as Tasks 3–24 criam (incluindo este roteiro e as tabelas geradas), já verificados. Ele substitui `Makefile`, `mod.toml`, `README.md`, `LICENSE` e `.gitignore` do template e não toca em `include/`, `mod.ld` nem nos submódulos. Para usá-lo: execute as Tasks 1, 2 e 4; dentro de `bk-randomizer/`, rode `git switch -c feat/randomizer-v1` e `unzip -o CAMINHO/bk-randomizer-esqueleto-v0.1.0.zip -d .`; confira com os passos "Rodar" da Task 22 (`make nrm` e `make check`; com o esqueleto, o `make check` mostra `Ran 32 tests`, porque já inclui os testes da Task 24); faça `git add -A && git commit -m "importa o esqueleto verificado da v0.1.0"`; siga para a Fase 5 e, na Task 24, pule os passos 1 a 4 (os arquivos já vieram no zip). O caminho completo (Tasks 3–22) continua sendo o recomendado se você quer que o Claude Code construa e entenda cada parte.

## Parte A — Especificação

### A1. O que o mod faz na v0.1
- **Localizações embaralhadas (249):** os 100 jiggies, os 24 favos de mel vazios (incluindo os 6 de Spiral Mountain), os 116 tokens do Mumbo e os 9 molehills dos mundos.
- **Itens:** os mesmos 240 coletáveis mais os 9 movimentos que o Bottles ensina nos mundos (Disparo de Ovos, Bico-Martelo, Trote de Garras, Salto de Mola, Voo, Asa Maravilha, Botas Pernaltas, Bico-Bomba e Tênis Turbo).
- **Opções:** lógica (Glitchless ou Sem lógica); coletáveis (Vanilla, Por tipo ou Misturado); escopo (Jogo inteiro ou Por mundo); movimentos (Vanilla ou Entre molehills); acessibilidade (Todas as localizações ou Apenas vencível); idioma dos avisos; avisos ligados ou não; log de depuração.
- **Seed gerado no jogo:** na inicialização, a partir de opções + semente. O mesmo núcleo C roda na CLI `bkrgen`, então PC e jogo produzem exatamente o mesmo seed.
- **Compartilhamento:** código `BKR1-XXXXXX-SEMENTE` e cinco palavras de conferência na tela de seleção de arquivo.
- **Save por seed:** `saves/ph_bk_randomizer/bkr_<impressão digital>.bin`; os saves normais ficam intocados.
- **Avisos na tela** (RecompUI) quando o item recebido difere do coletável original.
- **Spoiler** em PT-BR ou inglês, com esferas de coleta e a lista de localizações por mundo.
- Com o randomizer desligado, o jogo e os saves são os vanilla.

**Fora da v0.1** (detalhes em A16): notas, Jinjos, páginas do Cheato e Stop 'n' Swop embaralhados; movimentos de Spiral Mountain; custos aleatórios de quadros, portas e transformações; ordem dos mundos; aparência do coletável conforme o conteúdo; dicas; rastreador; multiworld (Archipelago); plando.

**Critérios de pronto:** `make check` verde (32 testes Python; 30 testes C com 500 seeds por preset; `mips-check`; autoteste MIPS com o mesmo hash do PC; nenhum conflito de patch); `make nrm` gera o `.nrm`; o checklist de QA no jogo (Task 23) passa.

### A2. Fatos verificados (em 17/10/2026)

| Fato | Valor | Onde foi conferido |
|---|---|---|
| Versão do Banjo: Recompiled | 1.0.2, publicada em 31/07/2026 (tag `v1.0.2` = `ec85963`). O HEAD (`dbd5215`, 25/09) só muda o README. | releases e git do BanjoRecomp |
| ROM aceita | Banjo-Kazooie US 1.0 (NTSC-U), SHA-1 `1fe1632098865f639e22c11b9a81ee8f29c75d7a` | `BUILDING.md:39` e README do BanjoRecomp |
| Template de mod | BKRecompModTemplate `06bbc59` (CC0), submódulos BanjoRecompSyms `6820055` e bk-decomp `351ca15` | git do template; são os mesmos do BanjoRecomp v1.0.2 |
| Runtime do v1.0.2 | N64ModernRuntime `ca568b6`, RecompFrontend `d0d90ba`; hooks, saves e RecompUI iguais até agosto de 2026 | git dos submódulos |
| RecompModTool | release `mod-tool-release` do N64Recomp (Windows `.exe` de 19/11/2025; Linux de 04/05/2025). Com o binário Linux, o `.nrm` deste mod sai com conteúdo idêntico ao gerado pelo N64Recomp `ffb39cd` compilado do código. No Windows ele usa o `Compress-Archive` do PowerShell; no Linux e no macOS chama `zip`. | teste local e `RecompModTool/main.cpp` |
| Clang no Windows | LLVM **18.1.8** (o binário oficial 19.1.0 não gera MIPS corretamente) | README do template |
| Pasta do jogo | Windows `%LOCALAPPDATA%\BanjoRecompiled`, Linux `~/.config/BanjoRecompiled`, macOS `~/Library/Application Support/BanjoRecompiled`; mods em `mods/`, saves em `saves/` | `main.cpp`, `pi.cpp:99-113` |
| Console no Windows | `BanjoRecompiled.exe --show-console` | `src/main/main.cpp:646` |
| Pilha da thread principal | `MAIN_THREAD_STACK_SIZE` = 0x17F0 | decomp `src/core1/code_0.c:8` |
| Thunderstore | comunidade `banjo-recompiled`; pacote = `.nrm` + `icon.png` + `manifest.json` + `README.md` na raiz do zip | pacote publicado inspecionado |

### A3. Política de IA do Banjo: Recompiled
O README do BanjoRecomp declara que a equipe não usa IA generativa, e o `CONTRIBUTING.md` proíbe código gerado por IA em contribuições ao projeto, citando incerteza de licença, regressões e a exigência de precisão de hardware. O mesmo arquivo traz um aviso dirigido a agentes de IA. O que isso significa aqui:
- Este mod é um projeto separado, feito sobre o template de mods (CC0) e a API pública de mods. Nenhum código do BanjoRecomp é copiado.
- Nada deste trabalho vai para os repositórios do BanjoRecomp: nada de issue, PR, discussão ou patch enviado a partir de uma sessão com IA. O CLAUDE.md (Task 3) torna isso regra do projeto.
- Se você achar um bug do base, reproduza-o você mesmo e relate com suas palavras, sem código gerado.
- Ao publicar, declare que o mod foi feito com auxílio de IA (o README da Task 3 já declara).

### A4. Referências e o que aproveitamos de cada uma

| Referência | O que aproveitamos | O que evitamos |
|---|---|---|
| **OoT Randomizer** (`OoTRandomizer/OoT-Randomizer`, MIT) | Lógica como dados (regiões, saídas, eventos e regras em texto compiladas para avaliação rápida); preenchimento assumido (`Fill.py`: `distribute_items_restrictive` e `fill_restrictive`), com os itens mais restritos primeiro; esferas de coleta no spoiler (`Spoiler.create_playthrough`); string de opções compacta (a nossa acrescenta 6 bits de checagem); ícones de hash para conferir o seed entre jogadores (aqui, palavras). | Patcher de ROM em Python: aqui o jogo é recompilado, então o seed nasce dentro dele. |
| **Lighthouse** (`HarbourMasters/Lighthouse` `c09a1f1`, CC0) | Tabela de 1208 checagens e 125 regiões com regras para BK, base da nossa lógica (importada, não reescrita à mão). | Sete problemas encontrados na auditoria (A13), corrigidos num arquivo de política separado. |
| **Archipelago-BK** (`RecompRando/archipelago-bk` `a20c2a9`, MIT) | Confirmação independente de duas regras de acesso (Jinjo verde de CC e tumba de Sandybutt). | O modelo de transformações como itens e a regra de tokens só pelo custo próprio, menos segura que o custo acumulado. |
| **MMRecompRando** (`RecompRando/MMRecompRando`, GPLv3) | Padrões de mod de randomizer em recomp: um save por seed via `recomp_change_save_file`, avisos com RecompUI. | Nenhum código copiado (licença diferente). |
| **decomp de Banjo-Kazooie** (`351ca15`, CC0) | Todos os fatos do jogo usados aqui (fluxos de coleta, tabelas, custos), com arquivo e linha citados. | — |

### A5. Arquitetura

```text
PC (build e testes)
  ../lighthouse (CC0) ──tools/import_lighthouse.py──► logic/lighthouse_raw.json
                                                        │  + logic/overrides.json (correções e política)
                                                        ▼
                               tools/compile_logic.py ──► core/generated/ (tabelas + bytecode RPN)
                                                        │
  core/ (C99, sem libc): RNG · opções · lógica · geração ◄┘
     ├─► testes no PC (core/tests; 500 seeds × 4 presets)
     ├─► CLI bkrgen (seed + spoiler)
     └─► MIPS: mips-check (zero símbolos externos) + autoteste no Unicorn (mesmo hash)

Jogo (Banjo: Recompiled + ph_bk_randomizer.nrm)
  recomp_on_init ─► lê opções ─► bkr_generate() ─► troca o save ─► reescreve moleTable
  hooks de coleta ─► entrega o item sorteado no mesmo quadro ─► suprime o +1 vanilla
  honeycombscore_get_total ─► favos recebidos (estado derivado)
  mainLoop ─► desarma a supressão · confere moleTable · UI (código, hash e avisos)
```

**Componentes e responsabilidades**

| Pasta/arquivo | Responsabilidade |
|---|---|
| `tools/import_lighthouse.py` | Traduz o C++ do Lighthouse para JSON e regras na nossa DSL, sem corrigir nada. |
| `logic/overrides.json` | Política e correções revisadas por gente (cada uma com `_motivo`). |
| `tools/compile_logic.py` | Aplica a política, valida invariantes e gera tabelas C + bytecode RPN + hash dos dados. |
| `core/` | Núcleo portátil: RNG, hash, opções e código de compartilhamento, avaliador de regras, busca, geração, spoiler. |
| `core/tests/`, `tools/tests/` | Testes (unitários, propriedades com milhares de seeds, valores-ouro, autoteste MIPS). |
| `src/` | Mod MIPS: inicialização, hooks de coleta, entrega, molehills, UI e pulso por quadro. |

### A6. Princípios de projeto
1. **Determinismo total.** Seed = função pura de (versão do gerador, hash dos dados de lógica, opções, semente). Sem relógio, sem float, sem dependência de endianness (o FNV mistura bytes por deslocamento). Prova: o autoteste MIPS gera o mesmo hash de placement que o PC.
2. **Estado derivado.** Nada extra é salvo. Os bits vanilla de "localização coletada" já são persistidos pelo jogo; o placement vem do seed; os contadores de jiggies para gastar, tokens e movimentos também são do jogo. Os favos recebidos são recontados (placement + bits) sempre que o jogo pergunta o total.
3. **Entrega atômica.** O item sorteado é entregue dentro do hook, no mesmo quadro em que o jogo marca a localização. Não existe janela em que o bit esteja salvo e o item não (salvar e sair logo depois é seguro).
4. **Patches mínimos.** Hooks sempre que possível; `RECOMP_PATCH` só em quatro funções que o base não altera, com o corpo original copiado do decomp e a origem citada. A checagem de conflitos roda no `make check`.
5. **Lógica como dados.** Regras ficam em JSON; o compilador falha alto em qualquer inconsistência (checagem repetida, evento sem definição, região sem entrada, contagem errada).
6. **Núcleo à prova de MIPS.** C99 sem libc nem `<stdint.h>`, memória estática, sem recursão, pico de pilha medido (268 bytes) bem abaixo do limite de projeto (metade de 0x17F0).

### A7. Modelo de dados e identificadores
- **Mundos** (`level_e` do decomp): MM 1, TTC 2, CC 3, BGS 4, FP 5, Covil 6, GV 7, CCW 8, RBB 9, MMM 10, SM 11.
- **Movimentos** (`ability_e`): 0x00–0x13 (Investida de Bico … Portas de Notas). Os 10 de SM entram como iniciais.
- **Tipos de localização:** JIGGY 1, HONEYCOMB 2, TOKEN 3, MOLEHILL 4.
- **Itens:** JIGGY 1, HONEYCOMB 2, TOKEN 3, movimento = 0x20 + `ability_e`.
- **Id de jogo por localização:** `jiggy_e` (1–100), `honeycomb_e` (1–24), uid do token (1–116, com as correções de A13 #6) ou índice em `moleTable` (0–8). Tabelas reversas (`bkr_loc_by_jiggy[101]`, `bkr_loc_by_honeycomb[25]`, `bkr_loc_by_token[128]`, `bkr_loc_by_mole[10]`) levam do id do jogo ao índice da localização em O(1) dentro dos hooks.
- **Regiões:** cada região tem faixas contíguas de saídas, eventos, grupos de notas e localizações (`BkrRegionDef`), o que deixa a busca linear e sem alocação.

### A8. DSL de lógica, bytecode e custos
Regras do Lighthouse como `CAN_USE_ABILITY(ABILITY_6_EGGS) && CAN_UNLOCK_NOTE_DOOR(180)` viram `(has(EGGS) and note_door(180))`. Primitivas da DSL:

| DSL | Significado | Bytecode |
|---|---|---|
| `has(X)` | movimento X aprendido | `HAS x` |
| `event(X)` | evento X alcançado (interruptores, quadros, Jinjos…) | `EVENT i` |
| `region(X)` | região alcançada | `REGION i` |
| `note_door(N)` | notas ≥ custo da porta N **e** a porta alcançada | `NOTE_DOOR k` + `EVENT` |
| `world(X)` | jiggies acumulados ≥ custo do quadro de X **e** o quadro alcançado | `WORLD_COST s` + `EVENT` |
| `transform(X)` | tokens acumulados ≥ custo da transformação **e** o crânio do Mumbo alcançado | `TRANSFORM_COST t` + `REGION` |
| `jinjos(X)` | os 5 Jinjos do mundo | 5× `EVENT` |
| `can_attack`, `extend_jump`, `break(X)`, `kill(X)` | atalhos definidos em `helpers` | expandidos na compilação |
| `and`, `or`, `not`, `true`, `false` | lógica booleana | `AND`, `OR`, `NOT`, `TRUE`, `FALSE` |

O compilador expande atalhos, simplifica constantes, achata `and`/`or`, deduplica regras idênticas (715 referências viram 174 regras distintas, 1769 palavras de bytecode) e emite RPN com profundidade máxima 8 (limite 32: a pilha do avaliador C é um único `bkr_u32`).

**Custos acumulados (vanilla):** quadros na ordem de `PICTURE_INFO` — MM 1, TTC 3, CC 8, BGS 15, FP 23, GV 32, MMM 42, RBB 54, CCW 69, porta da Gruntilda 94; portas de notas 50, 180, 260, 350, 450, 640, 765, 810, 828, 846, 864, 882; transformações em ordem de custo — cupim 5, crocodilo 15, morsa 30, abóbora 50, abelha 75. "Acumulado" significa que a lógica só considera o quadro de CCW aberto quando o jogador já pode ter pago todos os anteriores.

**Notas e Jinjos** não são embaralhados, mas entram na lógica: as 900 notas viram 118 grupos (região + regra, com contagem) e os 45 Jinjos viram eventos.

### A9. Algoritmo de geração (preenchimento assumido)
```text
fingerprint = FNV("BKR|", versão do gerador, hash dos dados, bloco de opções, "|", semente)
rng = xoshiro128**(splitmix32(fingerprint))
repita até 40 tentativas:
  pool = itens vanilla das localizações embaralhadas; travadas recebem o próprio item
  progressão = movimentos + jiggies + tokens (embaralhados; movimentos primeiro); resto = favos
  assumido = movimentos iniciais + toda a progressão
  para cada item de progressão:
      remove o item de "assumido"
      varre o mundo a partir de "assumido" + itens já colocados (ponto fixo)
      candidatas = vazias, permitidas para o item (tipo, escopo) e alcançáveis
      sem candidata → nova tentativa
      coloca o item numa candidata sorteada
  favos vão para vazias permitidas, ao acaso
  verifica: objetivo alcançável e, com "Todas as localizações", as 249 alcançáveis
```
Restrições: molehill só recebe movimento; movimento só vai para molehill (v0.1); "Por tipo" mantém cada coletável no seu tipo de localização; "Por mundo" mantém cada item no mundo de origem. Medido em 2000 seeds por preset: 0 falhas, no máximo 13 tentativas, cerca de 10 ms por geração no PC.

### A10. Pontos de interceptação no jogo
Fluxos vanilla (decomp `351ca15`), todos dentro de uma única chamada:

| Coletável | Sequência original | Arquivo |
|---|---|---|
| Jiggy | `jiggyscore_setCollected(id, TRUE)` → `item_adjustByDiffWithoutHud(ITEM_26_JIGGY_TOTAL, 1)` → dança | `src/core2/ba/marker.c:548-549` |
| Favo vazio | `honeycombscore_set(id, 1)` → música e diálogo agendados → `item_inc(ITEM_13_EMPTY_HONEYCOMB)` → se ≥ 6, `gcpausemenu_80314AC8(0)` | `marker.c:575-582` |
| Token | `mumboscore_set(uid, TRUE)` → som e diálogo agendados → `item_inc(ITEM_1C_MUMBO_TOKEN)` | `src/core2/code_59A80.c:26-35` |

`item_inc(x)` é `item_adjustByDiffWithHud(x, 1)` (`code_BEF20.c:31-33`). O mod:
1. **Hook** em `jiggyscore_setCollected`, `honeycombscore_set` e `mumboscore_set` (rodam antes da função; só agem se a localização ainda não foi coletada). Se o item sorteado for diferente do vanilla, entrega-o na hora e arma uma supressão de uso único para o próximo `+1` do contador vanilla.
2. **Patch** de uma linha em `item_adjustByDiffWithHud` e `item_adjustByDiffWithoutHud`: se a supressão estiver armada para aquele item e `diff == 1`, consome-a e não incrementa. O hook de `mainLoop` desarma qualquer supressão no início de cada quadro.
3. **Favos derivados:** patch em `honeycombscore_get_total` (usado pela vida máxima em `func_8034789C`, a cada carga e troca de mapa, e pela pausa) devolve os favos recebidos. `RECOMP_HOOK_RETURN("saveData_load")` recalcula a vida no fim do carregamento, porque o jogo calcula a vida antes de ler os tokens.
4. **Pausa:** ao entregar o 6º favo, o mod repete o que o vanilla faz (`gcpausemenu_80314AC8(0)`); a animação de vida extra destrava (`honeycarrierscore.c:232-238`) e a troca de mapa zera o contador (`section.c:281`).
5. **Molehills:** `moleTable` (`src/core2/ch/mole.c:34-45`, 10 entradas de 6 bytes) controla texto de ensino, lembrete, nó de câmera e habilidade. O mod reescreve as 9 entradas dos mundos com o texto e a habilidade sorteados, mantendo o nó de câmera de cada molehill, e confere a tabela a cada quadro. `chmole_learnAbility` (`mole.c:221-245`) usa a tabela para decidir entre ensino e lembrete, então nada mais muda. O patch em `chmole_additionalAbilityLearnActions` (`mole.c:182-218`) evita apontar a câmera para as botas (nó 0x11) ou os tênis (nó 0x29) quando esses textos são ensinados fora de BGS/GV.
6. **Inicialização:** o evento `recomp_on_init` dispara em `dummy_func_8025AFB0`, depois de carregar o core2 e antes de ler o save (`BanjoRecomp/patches/init_patches.c:16-25`). Ali o mod lê as opções, gera o seed, liga a correção de tokens do base, troca o arquivo de save e aplica `moleTable`.

**Armadilhas conhecidas da API de mods**
- Hooks não mudam argumentos nem retorno: o runtime copia o contexto antes do hook e o restaura depois (`N64ModernRuntime/librecomp/src/mod_hooks.cpp`).
- `RECOMP_PATCH` numa função que o base já substitui impede o mod de carregar; hooks nessas funções são permitidos.
- RecompUI: abrir o mesmo contexto duas vezes é erro fatal (`RecompFrontend/recompui/src/core/ui_context.cpp`).
- O jogo não tem `memset`; com `-fno-builtin` e cópias campo a campo o compilador não gera chamadas escondidas (o `mips-check` prova).

### A11. Persistência e saves
- `recomp_change_save_file("bkr_623A1517")` grava em `<pasta do jogo>/saves/ph_bk_randomizer/bkr_623A1517.bin` (`pi.cpp:107-113`); o save normal é `saves/bk.bin`. Cada seed (e cada versão do gerador) tem o próprio arquivo, então um save nunca é carregado com outro placement.
- A extensão de save do base (`SaveFileExtensionData`, 320 bytes; `SaveGlobalExtensionData`, 256 bytes) é reservada para ele; o mod não grava nela.
- Bits livres para o futuro: o placar de tokens aceita uids 1–125 e só 116 são usados; os bits 117–125 (9) são persistidos pelo jogo e podem guardar, numa v2, "movimento recebido fora de molehill".

### A12. Opções, código de compartilhamento e spoiler
- Opções empacotadas em 6 bits: `logic | shuffle<<1 | scope<<3 | moves<<4 | access<<5` (48 combinações válidas). Padrão = 20.
- Bloco do código: 24 bits de opções + 6 bits de checagem (FNV dos bits), em 6 caracteres base32 Crockford (aceita minúsculas e lê O como 0, I e L como 1). Semente: `[A-Z0-9_]`, até 24 caracteres; espaço vira `_`. Exemplo: `BKR1-GG000M-BANJO`.
- Impressão digital = FNV-1a de `BKR|` + versão do gerador + hash dos dados + bloco + `|` + semente. Ela semeia o RNG, nomeia o save e escolhe as 5 palavras do hash (5 bits por palavra).
- Spoiler: cabeçalho, esferas de coleta (como no OoTR: o que fica alcançável a cada rodada) com os movimentos de cada esfera, e a lista de localizações por mundo.

### A13. Auditoria da lógica importada
O arquivo `docs/logic-audit.md` (Task 6) documenta tudo com evidência. Resumo do Lighthouse `c09a1f1`:
1. Três eventos de interruptor da bruxa (CC, MMM, CCW) são exigidos e nunca definidos.
2. O jiggy dos Jinjos de BGS tem regra `true`.
3. Uma nota de BGS aparece duas vezes e outra nunca (`_LOG_2` repetida, `_LOG_3` ausente).
4. A região da comporta submersa de CC (Jinjo verde) não tem entrada.
5. A região do labirinto de Sandybutt (GV) não tem entrada; a pirâmide sobe com ovos (decomp `src/GV/code_6370.c`).
6. Tokens com id repetido no jogo original; o base corrige com `bkrecomp_set_mumbo_token_fixes_enabled` e o Lighthouse rotula dois deles trocados.
7. `CanOpenWorld` troca os custos dos quadros de MMM e CCW.

Todas as correções moram em `logic/overrides.json`, com `_motivo`, e são aplicadas pelo compilador; o JSON importado nunca é editado à mão.

### A14. Interface (RecompUI)
Um contexto próprio, sem capturar entrada nem mouse, com um rótulo superior (código + palavras do hash, visível só na seleção de arquivo, ou aviso de configuração inválida) e um rótulo inferior para os avisos de item (180 quadros). Toda alteração acontece entre `recompui_open_context` e `recompui_close_context`. Os textos não usam acentos para evitar problemas de fonte.

### A15. Testes e valores-ouro

| Camada | O que prova | Onde |
|---|---|---|
| Python | tradução de regras, importação de uma árvore mínima, compilador (precedência, simplificação, RPN), dados reais (contagens e hash 0x5C82A342), opcodes iguais aos do C, referência independente do RNG/FNV/código/impressão digital, checagem de conflitos, empacotamento | `tools/tests/` |
| C (host) | utilidades, RNG, nomes, opções, tabelas, avaliador, alcançabilidade (tudo alcançável com todos os itens; vanilla vence; sem itens quase nada), geração (determinismo, restrições, balanço, valor-ouro), propriedades (500 seeds × 4 presets no `make check`) e spoiler | `core/tests/` |
| MIPS | zero símbolos externos; o mesmo hash de placement no Unicorn; pico de pilha | `make -C core mips-check mips-selftest` |
| Mod | símbolos e alvos de hook/patch válidos (RecompModTool); nenhum conflito com o base | `make nrm`, `make check` |
| Jogo | entrega, pausa, molehills, persistência, desempenho | `docs/qa-checklist.md` |

Valores-ouro: `BKR_LOGIC_DATA_HASH = 0x5C82A342`, impressão digital de `BANJO` = `623A1517`, `PLACEMENT_GOLDEN = 31098721`. Mudou sem querer? É bug. Mudou de propósito? Suba `BKR_GENERATOR_VERSION`, atualize os três e o CHANGELOG.

### A16. Riscos, limitações e roadmap

**Riscos que só o jogo confirma** (cobertos pela Task 23): animação do 6º favo durante outra animação; câmera e textos dos molehills fora do lugar original; legibilidade dos avisos; tempo de geração no início.

**Limitações da v0.1**
- Recursos gastáveis usam custo acumulado em ordem crescente. Comprar a abelha antes da morsa, por exemplo, pode deixar o jogador sem tokens para o que a lógica espera. Folga: 41 tokens e 6 jiggies.
- O quadro de vida dupla (4 jiggies) não entra na conta; com "Apenas vencível", deixe-o para o fim.
- O coletável mantém a aparência original (um jiggy pode conter um token).
- Os contadores de jiggies por mundo (HUD e pausa) contam lugares visitados, não itens recebidos.

**Roadmap (v0.2+), com o ponto de entrada técnico de cada item**
1. **Ordem de compra garantida:** recusar uma transformação se uma mais barata não foi paga (`FILEPROG_90_PAID_TERMITE_COST` em diante) e exigir na lógica a transformação anterior; elimina a limitação de recursos de tokens.
2. **Custos aleatórios de quadros e portas:** `BkrCosts` já é parâmetro do avaliador; no jogo, `PICTURE_INFO` (`src/lair/jigsawpicture.c:40`) e `D_8039347C` (`src/lair/code_0.c:198`) ficam no overlay do covil e podem ser reescritos no carregamento do overlay.
3. **Movimentos de SM e movimentos fora de molehills:** usar os bits livres 117–125 do placar de tokens para registrar "recebido" e entregar com `ability_unlock`.
4. **Notas e Jinjos:** o base 1.0.2 exporta funções e eventos de notas salvas (`bkrecomp_note_collected_event` e afins).
5. **Aparência fiel ao conteúdo:** trocar o modelo do ator do coletável conforme o item sorteado.
6. **Dicas e rastreador:** dicas na Brentilda (textos dela no covil) e uma tela de rastreador em RecompUI.
7. **Archipelago:** biblioteca nativa no estilo do MMRecompRando, com fila de itens remotos entregues só em estado seguro, compatível com o apworld do Archipelago-BK.
8. **Plando:** placements fixos lidos de um arquivo na pasta do mod.

## Parte B — Plano de implementação

### Mapa de arquivos

| Arquivo | Responsabilidade | Task |
|---|---|---|
| `CLAUDE.md`, `README.md`, `LICENSE`, `THIRD_PARTY_NOTICES.md`, `CHANGELOG.md`, `.gitignore`, `.gitattributes` | regras para o Claude Code, documentação, licenças | 3 |
| `tools/import_lighthouse.py` + teste | importa a lógica do Lighthouse para `logic/lighthouse_raw.json` | 5 |
| `core/include/bkr/bkr_rules.h`, `tools/compile_logic.py` + teste, `logic/overrides.json`, `docs/logic-audit.md` | DSL, política, compilação para `core/generated/` | 6 |
| `core/Makefile`, `core/tests/bkr_test.h`, `core/tests/test_main.c` | build e executor dos testes do núcleo | 7 |
| `core/include/bkr/bkr_types.h`, `bkr_ids.h`, `bkr_world.h`, `core/src/bkr_util.c` | tipos, ids estáveis, estruturas das tabelas, utilidades | 7 |
| `bkr_hash.h/.c`, `bkr_text.h/.c` | FNV-1a e construtor de texto sem libc | 8 |
| `bkr_rng.h/.c`, `tools/tests/test_rng_reference.py` | xoshiro128** + referência independente em Python | 9 |
| `core/src/bkr_names.c` | nomes de itens e mundos (PT-BR/EN) e palavras do hash | 10 |
| `bkr_settings.h/.c` | opções, código de compartilhamento, impressão digital | 11 |
| `bkr_logic.h/.c` | estado, avaliador RPN, busca por ponto fixo | 12 |
| `bkr_generate.h/.c` | geração por preenchimento assumido + verificação | 13 |
| `bkr_spoiler.h/.c` | spoiler com esferas | 14 |
| `core/cli/bkrgen.c` | CLI no PC | 15 |
| `core/tests/mips_selftest.c/.ld`, `tools/mips_selftest.py` | autoteste MIPS no Unicorn | 17 |
| `tools/check_patch_conflicts.py` + teste | proíbe `RECOMP_PATCH` em função alterada pelo base | 18 |
| `Makefile`, `mod.toml`, `src/bkr_mod.h`, `src/mod_init.c` | build do mod, manifesto, opções, inicialização | 19 |
| `src/mod_collect.c`, `src/mod_grant.c` | hooks de coleta, supressão, entrega e favos derivados | 20 |
| `src/mod_mole.c` | movimentos nos molehills | 21 |
| `src/mod_ui.c`, `src/mod_tick.c` | RecompUI e pulso por quadro | 22 |
| `docs/qa-checklist.md` | QA no jogo | 23 |
| `tools/package_thunderstore.py` + teste | pacote da Thunderstore | 24 |

Testes de cada parte: `core/tests/test_*.c` e `tools/tests/test_*.py`, criados na mesma Task do código que testam.

### Fluxo de branches
As Tasks 1–4 preparam `main`. A Task 5 cria `feat/randomizer-v1`, onde ficam as Tasks 5–24; no fim, superpowers:finishing-a-development-branch integra o branch em `main` (Task 24).

### Fase 0 — Ambiente e repositório

### Task 1: Instalar as ferramentas

**Arquivos:** nenhum.

- [ ] **Passo 1: Instalar conforme o sistema**

Windows (PowerShell como administrador, com o [Chocolatey](https://chocolatey.org/install)). Use o LLVM 18.1.8: o binário oficial 19.1.0 não gera MIPS corretamente.

```powershell
choco install -y git make mingw python
choco install -y llvm --version 18.1.8
```

Depois, no Git Bash:

```bash
python -m pip install unicorn pyelftools
```

Baixe `RecompModTool.exe` da release [`mod-tool-release` do N64Recomp](https://github.com/N64Recomp/N64Recomp/releases/tag/mod-tool-release) e coloque-o numa pasta do `PATH` (por exemplo `C:\tools`, adicionada às variáveis de ambiente do usuário). O `.nrm` é compactado pelo PowerShell, que já vem no Windows.

Linux (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install -y git make gcc clang lld llvm python3 python3-pip zip
python3 -m pip install --user unicorn pyelftools
mkdir -p ~/.local/bin
curl -L -o ~/.local/bin/RecompModTool https://github.com/N64Recomp/N64Recomp/releases/download/mod-tool-release/RecompModTool
chmod +x ~/.local/bin/RecompModTool
```

Garanta que `~/.local/bin` está no `PATH`.

macOS (Homebrew). O clang da Apple não gera MIPS: o mod usa o LLVM 18 do Homebrew, passado ao `make` por `CC` e `LD` (o Makefile do template espera isso no macOS). Não há RecompModTool pronto para macOS; compile-o do código, no commit validado, antes de mudar `CC`.

```bash
brew install llvm@18 python git zip cmake ninja
python3 -m pip install --user unicorn pyelftools
git clone https://github.com/N64Recomp/N64Recomp.git ~/src/N64Recomp
cd ~/src/N64Recomp
git checkout ffb39cdad1da5de07eaaa48bd1db4a89a7986771
git submodule update --init --recursive
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target RecompModTool
mkdir -p ~/.local/bin && cp build/RecompModTool ~/.local/bin/
cat >> ~/.zprofile <<'EOF'
export PATH="$HOME/.local/bin:$(brew --prefix llvm@18)/bin:$PATH"
export CC="$(brew --prefix llvm@18)/bin/clang"
export LD="$(brew --prefix llvm@18)/bin/ld.lld"
EOF
source ~/.zprofile
```

Se o `pip` recusar com "externally-managed-environment" (Linux ou macOS), repita o comando com `--break-system-packages`: a instalação fica só na sua pasta de usuário.

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

No Linux, uma versão mais nova do clang da distribuição costuma funcionar; a Fase 3 (`mips-check` e autoteste MIPS) confirma. Python precisa ser 3.10 ou mais novo.

### Task 2: Criar o repositório a partir do template

**Arquivos:** remove `src/always_tumble_jump.c` (exemplo do template).

- [ ] **Passo 1: Clonar o template no commit validado** (rode na pasta de projetos, não dentro de um repositório)

**Rodar:**

```bash
git clone https://github.com/BanjoRecomp/BKRecompModTemplate.git bk-randomizer
cd bk-randomizer
git checkout -q 06bbc59c840ecdd0f2d094fdf5df50710c1475b4
git submodule update --init --recursive
git submodule status
```

**Esperado:**

```text
 6820055ca076e94e30e53d917bd9e5f71c28ca20 BanjoRecompSyms…
 351ca1580c10e550160ac11c77824fa9a498015e bk-decomp…
```

Os dois submódulos são os mesmos que o Banjo: Recompiled 1.0.2 usa. O `bk-decomp` vem do GitLab (`gitlab.com/banjo.decomp/banjo-kazooie`).

- [ ] **Passo 2: Transformar no seu projeto** (a partir daqui, todos os comandos rodam dentro de `bk-randomizer/`)

**Rodar:**

```bash
git switch -C main
git remote rename origin template
git rm -q src/always_tumble_jump.c
git commit -q -m "remove o exemplo do template"
git log --oneline -1
```

**Esperado:**

```text
… remove o exemplo do template
```

O remoto `template` continua apontando para o BKRecompModTemplate; o seu remoto (`origin`) entra na Task 24.

### Task 3: Documentos do projeto e regras para o Claude Code

**Arquivos:**
- Criar: `CLAUDE.md`, `THIRD_PARTY_NOTICES.md`, `CHANGELOG.md`, `.gitattributes`, `docs/superpowers/plans/2026-10-17-bk-randomizer.md`
- Substituir: `README.md`, `LICENSE`, `.gitignore`

- [ ] **Passo 1: Guardar este roteiro no repositório**

Copie este arquivo (o roteiro que você está lendo) para `docs/superpowers/plans/2026-10-17-bk-randomizer.md`. Exemplo no Git Bash, ajustando o caminho de origem:

```bash
mkdir -p docs/superpowers/plans
cp "/c/Users/SEU_USUARIO/Downloads/roteiro-bk-randomizer.md" docs/superpowers/plans/2026-10-17-bk-randomizer.md
```

- [ ] **Passo 2: Criar o `CLAUDE.md`**

Arquivo `CLAUDE.md` (criar):

````markdown
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
````

- [ ] **Passo 3: Substituir o `README.md`**

Arquivo `README.md` (substituir):

````markdown
# Banjo-Kazooie Randomizer para Banjo: Recompiled

Mod de randomizer com lógica para o [Banjo: Recompiled](https://github.com/BanjoRecomp/BanjoRecomp).
Ele embaralha os 100 jiggies, os 24 favos de mel vazios, os 116 tokens do Mumbo e os 9 movimentos que o
Bottles ensina nos mundos, e garante que o seed pode ser vencido (por padrão, com 100% das localizações
alcançáveis). O seed é gerado dentro do jogo; a ferramenta `bkrgen` gera o mesmo seed e o spoiler no PC.

> Projeto independente, sem vínculo com a equipe do Banjo: Recompiled. O código foi desenvolvido com auxílio
> de IA (Claude Code). Não envie bugs deste mod para o repositório do Banjo: Recompiled.

## Requisitos
- Banjo: Recompiled 1.0.2 ou mais novo, com a ROM que ele aceita (Banjo-Kazooie US 1.0).

## Instalação
1. Baixe `ph_bk_randomizer.nrm`.
2. No Banjo: Recompiled, abra **Mods**, clique em **Install Mods** e escolha o `.nrm` (ou copie o arquivo para a
   pasta aberta por **Open Mods Folder**).
3. Ative o mod e ajuste as opções **antes** de iniciar o jogo: elas são lidas na inicialização.

## Opções
| Opção | Valores | Padrão |
|---|---|---|
| Randomizer | Desligado, Ligado | Ligado |
| Semente | texto livre (letras, números, espaço e `_`) | `BANJO` |
| Código de compartilhamento | `BKR1-XXXXXX-SEMENTE` (substitui as opções abaixo) | vazio |
| Lógica | Glitchless, Sem lógica | Glitchless |
| Coletáveis | Vanilla, Por tipo, Misturado | Misturado |
| Escopo | Jogo inteiro, Por mundo | Jogo inteiro |
| Movimentos do Bottles | Vanilla, Entre molehills | Entre molehills |
| Acessibilidade | Todas as localizações, Apenas vencível | Todas as localizações |
| Idioma dos avisos | Português (Brasil), English | Português (Brasil) |
| Avisos de item | Desligado, Ligado | Ligado |
| Log de depuração | Desligado, Ligado | Desligado |

## Seeds, códigos e saves
- O código `BKR1-XXXXXX-SEMENTE` reúne opções e semente. Na tela de seleção de arquivo o mod mostra o código e
  cinco palavras de conferência ("hash"): se você e um amigo virem as mesmas palavras, estão no mesmo seed.
- Cada seed usa um save próprio, `bkr_<impressão digital>.bin`, dentro de `saves/ph_bk_randomizer/` na pasta de
  dados do Banjo: Recompiled. Seus saves normais não são tocados; com o mod desligado o jogo usa os saves de sempre.

## Como funciona no jogo
- Ao pegar um jiggy, um favo vazio ou um token, você recebe o item sorteado para aquele lugar; quando ele é
  diferente do original, um aviso mostra o que veio.
- Cada molehill dos mundos ensina o movimento sorteado para ele. Os movimentos de Spiral Mountain continuam os de sempre.
- Na pausa, o total de jiggies conta os lugares de jiggy já visitados; os jiggies para gastar nos quadros, os favos
  e os tokens contam o que você recebeu.
- Dica: compre as transformações do Mumbo em ordem de custo (cupim, crocodilo, morsa, abóbora, abelha) e preencha
  os quadros dos mundos em ordem. A lógica conta com custos acumulados nessa ordem (veja "Limitações").

## Spoiler e CLI
```bash
make -C core cli
core/build/bkrgen --seed BANJO --spoiler spoiler.txt          # mesmas opções padrão do mod
core/build/bkrgen --code BKR1-GG000M-BANJO --lang en --spoiler spoiler_en.txt
```
A CLI usa o mesmo núcleo do mod: o seed, a impressão digital e o nome do save são idênticos aos do jogo.

## Compilar
Veja o roteiro em `docs/superpowers/plans/2026-10-17-bk-randomizer.md` (ferramentas, comandos e verificação).
Resumo: `make check` (testes) e `make nrm` (gera `build/ph_bk_randomizer.nrm` com o RecompModTool).

## Limitações conhecidas (v0.1)
- Notas e Jinjos não são embaralhados (a lógica os considera nos lugares originais). Movimentos de Spiral
  Mountain, páginas do Cheato e itens do Stop 'n' Swop também ficam como no jogo original.
- Custos de quadros, portas de notas e transformações são os vanilla.
- A lógica assume que jiggies e tokens são gastos em ordem crescente de custo. Gastar fora dessa ordem (por
  exemplo, comprar a abelha antes da morsa) pode deixar você sem recursos para o que a lógica espera.
- Com "Apenas vencível", deixe o quadro de vida dupla (4 jiggies) para o fim.

## Créditos e licenças
Código sob licença MIT (veja `LICENSE`). Dados de lógica derivados do Lighthouse (HarbourMasters, CC0);
algoritmo de preenchimento inspirado no OoT Randomizer (MIT). Detalhes em `THIRD_PARTY_NOTICES.md`.
````

- [ ] **Passo 4: Licença MIT no lugar da CC0 do template, e os avisos de terceiros**

Arquivo `LICENSE` (substituir):

```text
MIT License

Copyright (c) 2026 PH

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

Arquivo `THIRD_PARTY_NOTICES.md` (criar):

```markdown
# Avisos de terceiros

O código deste repositório é MIT (veja `LICENSE`). Ele usa ou se baseia nos trabalhos abaixo.

| Projeto | Uso aqui | Licença |
|---|---|---|
| [BKRecompModTemplate](https://github.com/BanjoRecomp/BKRecompModTemplate) | ponto de partida do repositório: `include/`, `mod.ld`, base do `Makefile` e do `mod.toml` | CC0 1.0 |
| [BanjoRecompSyms](https://github.com/BanjoRecomp/BanjoRecompSyms) @ `6820055` | submódulo: símbolos do jogo para o RecompModTool | CC0 1.0 |
| [Banjo-Kazooie decomp](https://gitlab.com/banjo.decomp/banjo-kazooie) @ `351ca15` | submódulo: headers do jogo; corpos de função copiados nos patches de `src/` (origem citada em cada patch) | CC0 1.0 |
| [Lighthouse](https://github.com/HarbourMasters/Lighthouse) @ `c09a1f1` | dados de regiões, checagens e regras importados por `tools/import_lighthouse.py` para `logic/lighthouse_raw.json` | CC0 1.0 |
| [OoT Randomizer](https://github.com/OoTRandomizer/OoT-Randomizer) | referência de arquitetura (regras em dados, preenchimento assumido, esferas do spoiler); nenhum código copiado | MIT |
| [Archipelago-BK](https://github.com/RecompRando/archipelago-bk) @ `a20c2a9` | referência para duas regras de acesso (ver `docs/logic-audit.md`); nenhum código copiado | MIT |
| [N64Recomp / RecompModTool](https://github.com/N64Recomp/N64Recomp) | ferramenta de build que gera o `.nrm` (não é distribuída) | MIT |
| xoshiro128** (D. Blackman e S. Vigna) | gerador do RNG em `core/src/bkr_rng.c` | domínio público (CC0) |
| splitmix32: passo de Weyl + finalizador do MurmurHash3 (A. Appleby) | semeadura do RNG em `core/src/bkr_rng.c` | domínio público |
| FNV-1a (G. Fowler, L. C. Noll e K.-P. Vo) | hash em `core/src/bkr_hash.c` | domínio público |

O Banjo: Recompiled (GPLv3) é o programa que carrega o mod; nenhum código dele é copiado para cá. O mod
usa apenas a API pública de mods (`RECOMP_HOOK`, `RECOMP_PATCH`, `RECOMP_IMPORT` e exports documentados).

Banjo-Kazooie é marca da Microsoft/Rare. Este projeto não contém ROM nem assets do jogo.
```

- [ ] **Passo 5: CHANGELOG, `.gitignore` e `.gitattributes`**

Arquivo `CHANGELOG.md` (criar):

```markdown
# Changelog

Formato: versão do mod, data, mudanças. Toda mudança de `BKR_GENERATOR_VERSION` aparece aqui, porque ela
muda os seeds gerados para os mesmos códigos.

## 0.1.0 — não publicada
- Primeira versão: jiggies, favos vazios e tokens embaralhados (por tipo ou misturados, no jogo inteiro ou
  por mundo) e os 9 movimentos dos molehills dos mundos trocados entre si.
- Lógica glitchless importada do Lighthouse `c09a1f1`, com as correções de `docs/logic-audit.md`.
- `BKR_GENERATOR_VERSION` 1, `BKR_LOGIC_DATA_HASH` 0x5C82A342, seed `BKR1-GG000M-BANJO` com
  placement 31098721.
```

Arquivo `.gitignore` (substituir):

```gitignore
# Vindos do BKRecompModTemplate
*.d
*.o
*.elf
*.bin
*.exe
*.dll
*.lib
*.pdb
*.exp
./funcs.h
patches.map
build/
.vscode/settings.json

# Do randomizer
*.nrm
__pycache__/
*.pyc
spoiler*.txt
dist/
```

Arquivo `.gitattributes` (criar):

```text
# Fim de linha LF em todos os arquivos de texto, em qualquer sistema: os hashes
# dos arquivos gerados e os scripts do Git Bash dependem disso.
* text=auto eol=lf
*.png binary
*.nrm binary
*.zip binary
```

- [ ] **Passo 6: Commit**

**Rodar:**

```bash
git add CLAUDE.md README.md LICENSE THIRD_PARTY_NOTICES.md CHANGELOG.md .gitignore .gitattributes docs/superpowers/plans/2026-10-17-bk-randomizer.md
git commit -q -m "adiciona regras do projeto, licença MIT e roteiro"
git log --oneline -1
```

**Esperado:**

```text
… adiciona regras do projeto, licença MIT e roteiro
```

### Task 4: Clonar as referências ao lado do repositório

**Arquivos:** nenhum no repositório. Cria `../lighthouse` (só a lógica do randomizer, ~4 MB) e `../BanjoRecomp` (só `patches/` e documentos, ~1 MB).

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

O checkout do BanjoRecomp serve só para a checagem de conflitos de patch (Task 18). Leia o `README.md` e o `CONTRIBUTING.md` dele para conhecer a política do projeto (A3).

### Fase 1 — Pipeline de lógica

### Task 5: Importador da lógica do Lighthouse

**Arquivos:**
- Criar: `tools/import_lighthouse.py`, `tools/tests/test_import_lighthouse.py`
- Gerar: `logic/lighthouse_raw.json`

O importador só traduz: lê `Checks.cpp` (tabela de checagens) e `Regions/*.cpp` (regiões com checagens, saídas e eventos) e converte as macros C++ (`CAN_USE_ABILITY`, `CAN_ACCESS`, `CAN_UNLOCK_NOTE_DOOR`…) para a DSL do projeto. Correções ficam para a Task 6.

- [ ] **Passo 1: Criar o branch de trabalho**

**Rodar:**

```bash
git switch -c feat/randomizer-v1
```

- [ ] **Passo 2: Escrever os testes**

Arquivo `tools/tests/test_import_lighthouse.py` (criar):

```python
"""Testes do importador da lógica do Lighthouse (tools/import_lighthouse.py).

Rodar:  python3 -m unittest discover -s tools/tests -v
"""
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import import_lighthouse as il  # noqa: E402

CHECKS_CPP = """
namespace Rando { namespace StaticData {
    // comentário que deve ser ignorado: RC(RC_FAKE, RCTYPE_JIGGY, ...)
    RC(RC_MM_JIGGY_CHIMPY,   RCTYPE_JIGGY,       LEVEL_1_MUMBOS_MOUNTAIN, ACTOR_46_JIGGY,        JIGGY_9_MM_CHIMPY,        0, 0, 0),
    RC(RC_MM_MUMBO_TOKEN_X,  RCTYPE_MUMBO_TOKEN, LEVEL_1_MUMBOS_MOUNTAIN, ACTOR_2D_MUMBO_TOKEN,  MUMBOTOKEN_5D_MM_X,       10, -20, 30),
    RC(RC_MM_NOTE_A,         RCTYPE_MUSIC_NOTE,  LEVEL_1_MUMBOS_MOUNTAIN, ACTOR_51_MUSIC_NOTE,   NULL,                     1, 2, 3),
}}
"""

REGION_CPP = """
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_MM_START] = RandoRegion{ .regionName = "Mumbo's Mountain", .mapId = MAP_2_MM_MUMBOS_MOUNTAIN,
        .checks = {
            CHECK(RC_MM_JIGGY_CHIMPY,  CAN_USE_ABILITY(ABILITY_5_CLIMB) || CAN_USE_ABILITY(ABILITY_8_FLAP_FLIP)),
            CHECK(RC_MM_NOTE_A,        true),
        },
        .connections = {
            CONNECTION(RR_MM_TOP,      CAN_USE_ABILITY(ABILITY_10_TALON_TROT) && CAN_ATTACK),
        },
        .events = {
            EVENT(RA_MM_EVENT,         CAN_USE_TRANSFORMATION(TRANSFORM_2_TERMITE)),
        },
    };
    Regions[RR_MM_TOP] = RandoRegion{ .regionName = "Topo", .mapId = MAP_2_MM_MUMBOS_MOUNTAIN,
        .checks = {
            CHECK(RC_MM_MUMBO_TOKEN_X, CAN_ACCESS(RA_MM_EVENT)),
        },
    };
});
"""


class TranslatorTest(unittest.TestCase):
    def test_macros(self):
        rule = il.translate_rule("CAN_USE_ABILITY(ABILITY_6_EGGS) && (CAN_UNLOCK_NOTE_DOOR(180) || "
                                 "CAN_USE_TRANSFORMATION(TRANSFORM_6_BEE))")
        self.assertEqual(rule, "(has(EGGS) and (note_door(180) or transform(BEE)))")
        self.assertEqual(il.translate_rule("CAN_ACCESS(RA_WITCH_SWITCH_GOBIS_VALLEY)"),
                         "event(WITCH_SWITCH_GOBIS_VALLEY)")
        self.assertEqual(il.translate_rule("CAN_BREAK_OBJECT(RA_BREAK_OBJECT_GRATE)"), "break(GRATE)")
        self.assertEqual(il.translate_rule("CAN_UNLOCK_WORLD(LEVEL_8_CLICK_CLOCK_WOOD)"), "world(CCW)")
        self.assertEqual(il.translate_rule("!CAN_ATTACK"), "not can_attack")
        self.assertEqual(il.translate_rule("true"), "true")

    def test_unknown_macro_fails(self):
        with self.assertRaises(il.ImportErrorBkr):
            il.translate_rule("CAN_DO_MAGIC(X)")

    def test_unknown_character_fails(self):
        with self.assertRaises(il.ImportErrorBkr):
            il.translate_rule("CAN_ATTACK + 1")

    def test_collection_number(self):
        self.assertEqual(il.collection_number("JIGGY_9_MM_CHIMPY"), 9)
        self.assertEqual(il.collection_number("MUMBOTOKEN_5D_CCW_X"), 0x5D)
        self.assertEqual(il.collection_number("ABILITY_10_TALON_TROT"), 0x10)
        self.assertIsNone(il.collection_number("NULL"))


class SyntheticTreeTest(unittest.TestCase):
    def test_build_raw_from_minimal_tree(self):
        with tempfile.TemporaryDirectory() as tmp:
            rando = Path(tmp) / "src" / "port" / "Rando"
            (rando / "StaticData").mkdir(parents=True)
            (rando / "Logic" / "Regions").mkdir(parents=True)
            (rando / "StaticData" / "Checks.cpp").write_text(CHECKS_CPP, encoding="utf-8")
            (rando / "Logic" / "Regions" / "MumbosMountain.cpp").write_text(REGION_CPP, encoding="utf-8")
            raw = il.build_raw(Path(tmp))
        self.assertEqual(sorted(raw["checks"]), ["RC_MM_JIGGY_CHIMPY", "RC_MM_MUMBO_TOKEN_X", "RC_MM_NOTE_A"])
        token = raw["checks"]["RC_MM_MUMBO_TOKEN_X"]
        self.assertEqual((token["type"], token["world"], token["collection_number"], token["pos"]),
                         ("TOKEN", "MM", 0x5D, [10, -20, 30]))
        self.assertEqual([r["id"] for r in raw["regions"]], ["MM_START", "MM_TOP"])
        start = raw["regions"][0]
        self.assertEqual(start["file"], "MumbosMountain.cpp")
        self.assertEqual(start["map"], "MAP_2_MM_MUMBOS_MOUNTAIN")
        self.assertEqual(start["checks"][0], ["RC_MM_JIGGY_CHIMPY", "(has(CLIMB) or has(FLAP_FLIP))"])
        self.assertEqual(start["exits"], [["MM_TOP", "(has(TALON_TROT) and can_attack)"]])
        self.assertEqual(start["events"], [["MM_EVENT", "transform(TERMITE)"]])
        self.assertEqual(raw["regions"][1]["checks"], [["RC_MM_MUMBO_TOKEN_X", "event(MM_EVENT)"]])

    def test_unknown_check_reference_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            rando = Path(tmp) / "src" / "port" / "Rando"
            (rando / "StaticData").mkdir(parents=True)
            (rando / "Logic" / "Regions").mkdir(parents=True)
            (rando / "StaticData" / "Checks.cpp").write_text(CHECKS_CPP, encoding="utf-8")
            (rando / "Logic" / "Regions" / "MumbosMountain.cpp").write_text(
                REGION_CPP.replace("RC_MM_NOTE_A,", "RC_MM_NAO_EXISTE,"), encoding="utf-8")
            with self.assertRaises(il.ImportErrorBkr):
                il.build_raw(Path(tmp))


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Passo 3: Ver os testes falharem**

**Rodar:**

```bash
python3 -m unittest discover -s tools/tests -v
```

**Esperado (falha):**

```text
ModuleNotFoundError: No module named 'import_lighthouse'
```

- [ ] **Passo 4: Implementar o importador**

Arquivo `tools/import_lighthouse.py` (criar):

```python
#!/usr/bin/env python3
"""Importador da lógica do randomizer do Lighthouse (HarbourMasters, licença CC0-1.0).

Lê ``src/port/Rando/StaticData/Checks.cpp`` e ``src/port/Rando/Logic/Regions/*.cpp``
de um checkout do Lighthouse e grava um JSON bruto e neutro em
``logic/lighthouse_raw.json``. As regras C++ são traduzidas para a DSL do projeto
(``has(X)``, ``event(X)``, ``transform(X)``, ``world(X)``, ``note_door(N)``...).

Uso:
    python3 tools/import_lighthouse.py --lighthouse ../lighthouse --out logic/lighthouse_raw.json

O commit validado é ``PINNED_COMMIT``. Outro commit funciona, mas exige rodar a
auditoria de lógica de novo (docs/logic-audit.md).
"""
import argparse
import json
import re
import sys
from pathlib import Path

PINNED_COMMIT = "c09a1f11041f50c2fd86d8aa60f8ee15a3ca13fd"

ABILITY_NAMES = {
    "ABILITY_0_BARGE": "BEAK_BARGE",
    "ABILITY_1_BEAK_BOMB": "BEAK_BOMB",
    "ABILITY_2_BEAK_BUSTER": "BEAK_BUSTER",
    "ABILITY_3_CAMERA_CONTROL": "CAMERA_CONTROL",
    "ABILITY_4_CLAW_SWIPE": "CLAW_SWIPE",
    "ABILITY_5_CLIMB": "CLIMB",
    "ABILITY_6_EGGS": "EGGS",
    "ABILITY_7_FEATHERY_FLAP": "FEATHERY_FLAP",
    "ABILITY_8_FLAP_FLIP": "FLAP_FLIP",
    "ABILITY_9_FLIGHT": "FLIGHT",
    "ABILITY_A_HOLD_A_JUMP_HIGHER": "HIGH_JUMP",
    "ABILITY_B_RATATAT_RAP": "RATATAT_RAP",
    "ABILITY_C_ROLL": "ROLL",
    "ABILITY_D_SHOCK_JUMP": "SHOCK_JUMP",
    "ABILITY_E_WADING_BOOTS": "WADING_BOOTS",
    "ABILITY_F_DIVE": "DIVE",
    "ABILITY_10_TALON_TROT": "TALON_TROT",
    "ABILITY_11_TURBO_TALON": "TURBO_TALON",
    "ABILITY_12_WONDERWING": "WONDERWING",
    "ABILITY_13_1ST_NOTEDOOR": "NOTE_DOORS",
}

TRANSFORM_NAMES = {
    "TRANSFORM_2_TERMITE": "TERMITE",
    "TRANSFORM_3_PUMPKIN": "PUMPKIN",
    "TRANSFORM_4_WALRUS": "WALRUS",
    "TRANSFORM_5_CROC": "CROC",
    "TRANSFORM_6_BEE": "BEE",
}

LEVEL_NAMES = {
    "LEVEL_1_MUMBOS_MOUNTAIN": "MM",
    "LEVEL_2_TREASURE_TROVE_COVE": "TTC",
    "LEVEL_3_CLANKERS_CAVERN": "CC",
    "LEVEL_4_BUBBLEGLOOP_SWAMP": "BGS",
    "LEVEL_5_FREEZEEZY_PEAK": "FP",
    "LEVEL_6_LAIR": "LAIR",
    "LEVEL_7_GOBIS_VALLEY": "GV",
    "LEVEL_8_CLICK_CLOCK_WOOD": "CCW",
    "LEVEL_9_RUSTY_BUCKET_BAY": "RBB",
    "LEVEL_A_MAD_MONSTER_MANSION": "MMM",
    "LEVEL_B_SPIRAL_MOUNTAIN": "SM",
    "LEVEL_D_CUTSCENE": "NONE",
}

ENEMY_NAMES = {
    "ACTOR_124_SIR_SLUSH": "SIR_SLUSH",
    "ACTOR_29B_ZUBBA": "ZUBBA",
}

TYPE_NAMES = {
    "RCTYPE_UNKNOWN": "UNKNOWN",
    "RCTYPE_EMPTY_HONEYCOMB": "HONEYCOMB",
    "RCTYPE_JIGGY": "JIGGY",
    "RCTYPE_JINJO": "JINJO",
    "RCTYPE_MOLEHILL": "MOLEHILL",
    "RCTYPE_MUMBO_TOKEN": "TOKEN",
    "RCTYPE_MUSIC_NOTE": "NOTE",
    "RCTYPE_STOP_N_SWOP": "STOP_N_SWOP",
}

TOKEN_RE = re.compile(r"\s*(&&|\|\||!|\(|\)|[A-Za-z_][A-Za-z0-9_]*|-?\d+)")
CHECK_ROW_RE = re.compile(
    r"RC\(\s*(RC_[A-Z0-9_]+)\s*,\s*(RCTYPE_[A-Z_]+)\s*,\s*(LEVEL_[A-Z0-9_]+)\s*,"
    r"\s*(ACTOR_[A-Z0-9_]+)\s*,\s*([A-Za-z0-9_]+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\)"
)
COLLECTION_ID_RE = re.compile(r"^(JIGGY|HONEYCOMB|MUMBOTOKEN|ABILITY)_([0-9A-F]+)_")


class ImportErrorBkr(Exception):
    """Erro de importação com mensagem legível."""


def strip_comments(text):
    """Remove comentários // e /* */ preservando strings simples."""
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def find_matching(text, open_index, open_ch, close_ch):
    """Retorna o índice do fechamento que casa com ``text[open_index]``."""
    depth = 0
    for i in range(open_index, len(text)):
        ch = text[i]
        if ch == open_ch:
            depth += 1
        elif ch == close_ch:
            depth -= 1
            if depth == 0:
                return i
    raise ImportErrorBkr(f"delimitador '{open_ch}' sem fechamento na posição {open_index}")


def split_first_arg(inner):
    """Separa ``A, resto`` no primeiro vírgula de nível zero."""
    depth = 0
    for i, ch in enumerate(inner):
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        elif ch == "," and depth == 0:
            return inner[:i].strip(), inner[i + 1:].strip()
    raise ImportErrorBkr(f"entrada sem vírgula: {inner!r}")


def collection_number(collection):
    """Converte JIGGY_9_MM_CHIMPY em 9, MUMBOTOKEN_5D_... em 0x5D etc."""
    match = COLLECTION_ID_RE.match(collection)
    if not match:
        return None
    return int(match.group(2), 16)


def parse_checks(checks_cpp):
    """Lê a tabela de checagens (id, tipo, mundo, ator, coleção, posição)."""
    text = strip_comments(checks_cpp.read_text(encoding="utf-8"))
    checks = {}
    for match in CHECK_ROW_RE.finditer(text):
        rc_id, rc_type, level, actor, collection, x, y, z = match.groups()
        if rc_id in checks:
            raise ImportErrorBkr(f"checagem duplicada: {rc_id}")
        checks[rc_id] = {
            "type": TYPE_NAMES[rc_type],
            "world": LEVEL_NAMES[level],
            "actor": actor,
            "collection": collection,
            "collection_number": collection_number(collection),
            "pos": [int(x), int(y), int(z)],
        }
    if not checks:
        raise ImportErrorBkr("nenhuma checagem encontrada; o formato de Checks.cpp mudou?")
    return checks


class RuleTranslator:
    """Traduz uma expressão booleana C++ do Lighthouse para a DSL do projeto."""

    def __init__(self, source):
        self.tokens = [t for t in TOKEN_RE.findall(source) if t.strip()]
        rebuilt = "".join(self.tokens)
        compact = re.sub(r"\s+", "", source)
        if rebuilt != compact:
            raise ImportErrorBkr(f"caractere não reconhecido na regra: {source!r}")
        self.pos = 0
        self.source = source

    def peek(self):
        return self.tokens[self.pos] if self.pos < len(self.tokens) else None

    def take(self, expected=None):
        tok = self.peek()
        if tok is None or (expected is not None and tok != expected):
            raise ImportErrorBkr(f"esperado {expected!r}, achado {tok!r} em {self.source!r}")
        self.pos += 1
        return tok

    def translate(self):
        out = self.parse_or()
        if self.peek() is not None:
            raise ImportErrorBkr(f"sobra de tokens em {self.source!r}: {self.tokens[self.pos:]}")
        return out

    def parse_or(self):
        parts = [self.parse_and()]
        while self.peek() == "||":
            self.take("||")
            parts.append(self.parse_and())
        return parts[0] if len(parts) == 1 else "(" + " or ".join(parts) + ")"

    def parse_and(self):
        parts = [self.parse_not()]
        while self.peek() == "&&":
            self.take("&&")
            parts.append(self.parse_not())
        return parts[0] if len(parts) == 1 else "(" + " and ".join(parts) + ")"

    def parse_not(self):
        if self.peek() == "!":
            self.take("!")
            return "not " + self.parse_not()
        return self.parse_atom()

    def parse_atom(self):
        tok = self.take()
        if tok == "(":
            inner = self.parse_or()
            self.take(")")
            return inner
        if tok == "true":
            return "true"
        if tok == "false":
            return "false"
        if tok == "CAN_ATTACK":
            return "can_attack"
        if tok == "CAN_EXTEND_JUMP_DISTANCE":
            return "extend_jump"
        if self.peek() != "(":
            raise ImportErrorBkr(f"identificador solto {tok!r} em {self.source!r}")
        self.take("(")
        arg = self.take()
        self.take(")")
        return self.translate_call(tok, arg)

    def translate_call(self, macro, arg):
        if macro == "CAN_USE_ABILITY":
            return f"has({ABILITY_NAMES[arg]})"
        if macro in ("CAN_USE_TRANSFORMATION", "GET_CURRENT_TRANSFORMATION"):
            return f"transform({TRANSFORM_NAMES[arg]})"
        if macro == "CAN_ACCESS":
            return f"event({arg[3:] if arg.startswith('RA_') else arg})"
        if macro == "CAN_BREAK_OBJECT":
            return f"break({arg.replace('RA_BREAK_OBJECT_', '')})"
        if macro == "CAN_COLLECT_JINJOS":
            return f"jinjos({LEVEL_NAMES[arg]})"
        if macro == "CAN_KILL_ENEMY":
            return f"kill({ENEMY_NAMES[arg]})"
        if macro == "CAN_UNLOCK_NOTE_DOOR":
            return f"note_door({int(arg)})"
        if macro == "CAN_UNLOCK_WORLD":
            return f"world({LEVEL_NAMES[arg]})"
        raise ImportErrorBkr(f"macro desconhecida {macro}({arg}) em {self.source!r}")


def translate_rule(source):
    """Atalho: traduz uma regra C++ em DSL."""
    return RuleTranslator(source).translate()


def parse_entries(block, macro):
    """Extrai pares (primeiro_argumento, regra_traduzida) de CHECK/CONNECTION/EVENT."""
    entries = []
    pattern = re.compile(r"\b" + macro + r"\s*\(")
    pos = 0
    while True:
        match = pattern.search(block, pos)
        if not match:
            break
        open_index = match.end() - 1
        close_index = find_matching(block, open_index, "(", ")")
        first, rule = split_first_arg(block[open_index + 1:close_index])
        entries.append([first, translate_rule(rule)])
        pos = close_index + 1
    return entries


def section(body, name):
    """Retorna o texto de ``.name = { ... }`` dentro do corpo de uma região."""
    match = re.search(r"\." + name + r"\s*=\s*\{", body)
    if not match:
        return ""
    open_index = match.end() - 1
    return body[open_index + 1:find_matching(body, open_index, "{", "}")]


def parse_regions(region_dir):
    """Lê todas as regiões, na ordem dos arquivos e das declarações."""
    regions = []
    seen = set()
    for path in sorted(region_dir.glob("*.cpp")):
        text = strip_comments(path.read_text(encoding="utf-8"))
        for match in re.finditer(r"Regions\[(RR_[A-Z0-9_]+)\]\s*=\s*RandoRegion\s*\{", text):
            region_id = match.group(1)[3:]
            if region_id in seen:
                raise ImportErrorBkr(f"região duplicada: {region_id}")
            seen.add(region_id)
            open_index = match.end() - 1
            body = text[open_index + 1:find_matching(text, open_index, "{", "}")]
            name = re.search(r'\.regionName\s*=\s*"([^"]*)"', body)
            map_id = re.search(r"\.mapId\s*=\s*(MAP_[A-Z0-9_]+)", body)
            checks = parse_entries(section(body, "checks"), "CHECK")
            exits = parse_entries(section(body, "connections"), "CONNECTION")
            events = parse_entries(section(body, "events"), "EVENT")
            regions.append({
                "id": region_id,
                "name": name.group(1) if name else region_id,
                "map": map_id.group(1) if map_id else None,
                "file": path.name,
                "checks": checks,
                "exits": [[target[3:], rule] for target, rule in exits],
                "events": [[event[3:], rule] for event, rule in events],
            })
    if not regions:
        raise ImportErrorBkr("nenhuma região encontrada; o formato de Regions/*.cpp mudou?")
    return regions


def build_raw(lighthouse_root):
    """Monta o dicionário bruto a partir de um checkout do Lighthouse."""
    rando = lighthouse_root / "src" / "port" / "Rando"
    checks = parse_checks(rando / "StaticData" / "Checks.cpp")
    regions = parse_regions(rando / "Logic" / "Regions")
    referenced = {check for region in regions for check, _ in region["checks"]}
    unknown = sorted(referenced - set(checks))
    if unknown:
        raise ImportErrorBkr(f"regiões citam checagens inexistentes: {unknown[:10]}")
    return {
        "source": {
            "project": "HarbourMasters/Lighthouse",
            "license": "CC0-1.0",
            "pinned_commit": PINNED_COMMIT,
        },
        "checks": checks,
        "regions": regions,
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--lighthouse", required=True, type=Path, help="checkout do Lighthouse")
    parser.add_argument("--out", required=True, type=Path, help="arquivo JSON de saída")
    args = parser.parse_args(argv)
    try:
        raw = build_raw(args.lighthouse)
    except ImportErrorBkr as exc:
        print(f"ERRO: {exc}", file=sys.stderr)
        return 1
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(raw, indent=1, ensure_ascii=False) + "\n", encoding="utf-8", newline="\n")
    by_type = {}
    for check in raw["checks"].values():
        by_type[check["type"]] = by_type.get(check["type"], 0) + 1
    print(f"regiões: {len(raw['regions'])}")
    print("checagens por tipo: " + ", ".join(f"{k}={v}" for k, v in sorted(by_type.items())))
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Passo 5: Ver os testes passarem**

**Rodar:**

```bash
python3 -m unittest discover -s tools/tests -v
```

**Esperado:**

```text
Ran 6 tests
OK
```

- [ ] **Passo 6: Importar a lógica real e conferir o hash do arquivo**

**Rodar:**

```bash
python3 tools/import_lighthouse.py --lighthouse ../lighthouse --out logic/lighthouse_raw.json
python3 -c "import hashlib; print(hashlib.sha256(open('logic/lighthouse_raw.json','rb').read()).hexdigest())"
```

**Esperado:**

```text
regiões: 125
checagens por tipo: HONEYCOMB=24, JIGGY=100, JINJO=45, MOLEHILL=15, NOTE=900, STOP_N_SWOP=7, TOKEN=116, UNKNOWN=1
86be19037dc36af8e76c1006061b54e78a9d0ec0b277bf6351d746a595b62b49
```

Se o hash for outro, o checkout do Lighthouse não está no commit `c09a1f1` (Task 4) ou o arquivo foi gravado com fim de linha CRLF.

- [ ] **Passo 7: Commit**

**Rodar:**

```bash
git add tools/import_lighthouse.py tools/tests/test_import_lighthouse.py logic/lighthouse_raw.json
git commit -q -m "adiciona importador da lógica do Lighthouse"
```

### Task 6: Compilador de lógica, política e auditoria

**Arquivos:**
- Criar: `core/include/bkr/bkr_rules.h`, `tools/compile_logic.py`, `tools/tests/test_compile_logic.py`, `logic/overrides.json`, `docs/logic-audit.md`
- Gerar: `core/generated/bkr_logic_data.h`, `core/generated/bkr_logic_data.c`, `logic/compiled_report.json`

O compilador aplica `logic/overrides.json` (correções com motivo e política: movimentos iniciais, tipos de localização, custos, atalhos), valida invariantes (100 jiggies, 24 favos, 116 tokens, 9 molehills, 100 notas por mundo, 45 Jinjos, nenhum evento sem definição) e gera as tabelas C com o bytecode RPN das regras. O `bkr_rules.h` define os opcodes; o teste confere que o Python e o C concordam.

- [ ] **Passo 1: Escrever os testes**

Arquivo `tools/tests/test_compile_logic.py` (criar):

```python
"""Testes do compilador de lógica (tools/compile_logic.py).

Rodar:  python3 -m unittest discover -s tools/tests -v
"""
import json
import re
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import compile_logic as cl  # noqa: E402


class OpcodeSyncTest(unittest.TestCase):
    def test_opcodes_match_c_header(self):
        header = (ROOT / "core" / "include" / "bkr" / "bkr_rules.h").read_text(encoding="utf-8")
        c_ops = {m.group(1): int(m.group(2)) for m in re.finditer(r"BKR_OP_([A-Z_]+)\s*=\s*(\d+)", header)}
        self.assertEqual(c_ops, cl.OPCODES)
        depth = re.search(r"#define BKR_RULE_MAX_DEPTH (\d+)", header)
        self.assertEqual(int(depth.group(1)), cl.MAX_DEPTH)


class ParserTest(unittest.TestCase):
    def test_precedence(self):
        node = cl.Parser("has(EGGS) or has(CLIMB) and not has(DIVE)").parse()
        self.assertEqual(node, ("or", [("call", "has", "EGGS"),
                                       ("and", [("call", "has", "CLIMB"), ("not", ("call", "has", "DIVE"))])]))

    def test_rejects_garbage(self):
        with self.assertRaises(cl.LogicError):
            cl.Parser("has(EGGS) &&").parse()
        with self.assertRaises(cl.LogicError):
            cl.Parser("has(EGGS) has(DIVE)").parse()

    def test_simplify_constants(self):
        self.assertEqual(cl.simplify(("and", [("true",), ("op", "HAS", 6)])), ("op", "HAS", 6))
        self.assertEqual(cl.simplify(("or", [("true",), ("op", "HAS", 6)])), ("true",))
        self.assertEqual(cl.simplify(("not", ("false",))), ("true",))
        self.assertEqual(cl.simplify(("and", [("op", "HAS", 6), ("and", [("op", "HAS", 6), ("op", "HAS", 8)])])),
                         ("and", [("op", "HAS", 6), ("op", "HAS", 8)]))

    def test_emit_rpn(self):
        node = ("and", [("op", "HAS", 6), ("or", [("op", "HAS", 8), ("op", "TOKENS", 5)])])
        rpn, depth = cl.emit_rpn(node, {}, {})
        ops = cl.OPCODES
        self.assertEqual(rpn, [ops["HAS"], 6, ops["HAS"], 8, ops["TOKENS"], 5, ops["OR"], ops["AND"], ops["END"]])
        self.assertEqual(depth, 3)

    def test_display_name(self):
        self.assertEqual(cl.display_name("RC_MM_JIGGY_TICKERS_TOWER", "MM", "JIGGY"), "MM Jiggy - Tickers Tower")
        self.assertEqual(cl.display_name("RC_GL_MUMBO_TOKEN_ABOVE_CC_ENTRANCE", "LAIR", "TOKEN"),
                         "GL Token - Above CC Entrance")
        self.assertEqual(cl.display_name("RC_TTC_EMPTY_HONEYCOMB_UNDERWATER", "TTC", "HONEYCOMB"),
                         "TTC Honeycomb - Underwater")


class RealDataTest(unittest.TestCase):
    """Compila os dados reais (logic/*.json) e confere os valores-ouro do roteiro."""

    @classmethod
    def setUpClass(cls):
        cls.raw = json.loads((ROOT / "logic" / "lighthouse_raw.json").read_text(encoding="utf-8"))
        cls.ov = json.loads((ROOT / "logic" / "overrides.json").read_text(encoding="utf-8"))

    def test_counts_and_hash(self):
        _, _, report = cl.compile_all(self.raw, self.ov)
        self.assertEqual(report["locations_by_kind"], {"JIGGY": 100, "HONEYCOMB": 24, "TOKEN": 116, "MOLEHILL": 9})
        self.assertEqual(report["defines"]["BKR_REGION_COUNT"], 125)
        self.assertEqual(report["ignored_checks"], {"STOP_N_SWOP": 7, "MOLEHILL": 6})
        self.assertLessEqual(report["max_rule_depth"], cl.MAX_DEPTH)
        self.assertEqual(report["data_hash"], "0x5C82A342")

    def test_compile_is_pure(self):
        first = cl.compile_all(self.raw, self.ov)
        second = cl.compile_all(self.raw, self.ov)
        self.assertEqual(first, second)

    def test_lighthouse_fixes_are_applied(self):
        model = cl.build_model(self.raw, self.ov)
        rules = {loc["id"]: loc["rule"] for loc in model["locations"]}
        self.assertEqual(rules["RC_BGS_JIGGY_JINJO"], "jinjos(BGS)")
        events = {name for region in model["regions"] for name, _ in region["events"]}
        for switch in ("WITCH_SWITCH_CLANKERS_CAVERN", "WITCH_SWITCH_MAD_MONSTER_MANSION",
                       "WITCH_SWITCH_CLICK_CLOCK_WOOD"):
            self.assertIn(switch, events)
        tokens = {loc["id"]: loc["game_id"] for loc in model["locations"] if loc["kind"] == "TOKEN"}
        self.assertEqual(tokens["RC_MMM_MUMBO_TOKEN_INSIDE_LOGGO"], 116)

    def test_bad_override_fails(self):
        broken = json.loads(json.dumps(self.ov))
        broken["extra_exits"].append({"from": "NAO_EXISTE", "to": "GOBIS_VALLEY_UPPER_AREA", "rule": "true"})
        with self.assertRaises(cl.LogicError):
            cl.compile_all(self.raw, broken)


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Passo 2: Ver os testes falharem**

**Rodar:**

```bash
python3 -m unittest discover -s tools/tests -v
```

**Esperado (falha):**

```text
ModuleNotFoundError: No module named 'compile_logic'
```

- [ ] **Passo 3: Opcodes do bytecode (lado C)**

Arquivo `core/include/bkr/bkr_rules.h` (criar):

```c
/*
 * bkr_rules.h — Bytecode de regras (RPN) produzido por tools/compile_logic.py.
 * Os valores DEVEM coincidir com OPCODES em tools/compile_logic.py
 * (tools/tests/test_compile_logic.py confere automaticamente).
 */
#ifndef BKR_RULES_H
#define BKR_RULES_H

enum {
    BKR_OP_END = 0,            /* fim da regra */
    BKR_OP_TRUE = 1,
    BKR_OP_FALSE = 2,
    BKR_OP_HAS = 3,            /* arg: ability_e */
    BKR_OP_EVENT = 4,          /* arg: índice de evento */
    BKR_OP_REGION = 5,         /* arg: índice de região */
    BKR_OP_NOTE_DOOR = 6,      /* arg: índice da porta (0..11) -> notas >= custo */
    BKR_OP_WORLD_COST = 7,     /* arg: slot do quebra-cabeça (0..9) -> jiggies >= custo acumulado */
    BKR_OP_TRANSFORM_COST = 8, /* arg: transformation_e -> tokens >= custo acumulado */
    BKR_OP_TOKENS = 9,         /* arg: quantidade */
    BKR_OP_JIGGIES = 10,       /* arg: quantidade */
    BKR_OP_NOTES = 11,         /* arg: quantidade */
    BKR_OP_AND = 12,
    BKR_OP_OR = 13,
    BKR_OP_NOT = 14
};

#define BKR_RULE_MAX_DEPTH 32

#endif
```

- [ ] **Passo 4: Política e correções**

Cada correção tem `_motivo`; a evidência completa está em `docs/logic-audit.md` (Passo 7).

Arquivo `logic/overrides.json` (criar):

```json
{
 "_doc": "Política e correções aplicadas sobre logic/lighthouse_raw.json por tools/compile_logic.py. Cada correção tem motivo; revise docs/logic-audit.md antes de mudar.",
 "start_region": "SPIRAL_MOUNTAIN_ENTRANCE",
 "goal_event": "GAME_COMPLETE",
 "starting_moves": [
  "BEAK_BARGE",
  "CAMERA_CONTROL",
  "CLAW_SWIPE",
  "CLIMB",
  "DIVE",
  "FEATHERY_FLAP",
  "FLAP_FLIP",
  "HIGH_JUMP",
  "RATATAT_RAP",
  "ROLL"
 ],
 "location_types": [
  "JIGGY",
  "HONEYCOMB",
  "TOKEN",
  "MOLEHILL"
 ],
 "excluded_checks": [
  "RC_SM_MOLEHILL_ATTACK",
  "RC_SM_MOLEHILL_BEAK_BARGE",
  "RC_SM_MOLEHILL_CAMERA_CONTROL",
  "RC_SM_MOLEHILL_CLIMB",
  "RC_SM_MOLEHILL_DIVE",
  "RC_SM_MOLEHILL_JUMP"
 ],
 "token_uid_overrides": {
  "_motivo": "IDs efetivos no Banjo: Recompiled com bkrecomp_set_mumbo_token_fixes_enabled(TRUE) (patches/mumbo_token_patches.c).",
  "RC_MMM_MUMBO_TOKEN_INSIDE_LOGGO": 116,
  "RC_CCW_MUMBO_TOKEN_SPRING_THORN_FIELD_FRONT_OF_MUMBOS_SKULL": 93,
  "RC_CCW_MUMBO_TOKEN_SPRING_FRONT_OF_EYRIE": 94
 },
 "molehill_index": {
  "_motivo": "Índice em moleTable (src/core2/ch/mole.c) = actorTypeSpecificField - 9.",
  "RC_FP_MOLEHILL_BEAK_BOMB": 0,
  "RC_MM_MOLEHILL_USE_EGGS": 1,
  "RC_MM_MOLEHILL_BEAK_BUSTER": 2,
  "RC_MM_MOLEHILL_TALON_TROT": 3,
  "RC_TTC_MOLEHILL_SHOCK_JUMP": 4,
  "RC_TTC_MOLEHILL_FLIGHT": 5,
  "RC_CC_MOLEHILL_WONDERWING": 6,
  "RC_BGS_MOLEHILL_WADING_BOOTS": 7,
  "RC_GV_MOLEHILL_TURBO_TALON": 8
 },
 "duplicate_check_renames": {
  "_motivo": "Lighthouse c09a1f1 lista RC_BGS_NOTE_THIRD_SHORT_SWAMP_LOG_2 duas vezes e omite _3; a segunda ocorrência vira _3.",
  "RC_BGS_NOTE_THIRD_SHORT_SWAMP_LOG_2": "RC_BGS_NOTE_THIRD_SHORT_SWAMP_LOG_3"
 },
 "check_rule_patches": {
  "_motivo": "Correções de regra encontradas na auditoria.",
  "RC_BGS_JIGGY_JINJO": "jinjos(BGS)"
 },
 "extra_events": [
  {
   "_motivo": "Lighthouse c09a1f1 não define este evento; o interruptor fica no quarto do interruptor dentro do Clanker (mapa 0x21, acesso pelo blowhole).",
   "region": "CLANKERS_CAVERN_INTERIOR_CLANKER_SLOW_SAWBLADE",
   "event": "WITCH_SWITCH_CLANKERS_CAVERN",
   "rule": "has(BEAK_BUSTER)"
  },
  {
   "_motivo": "Lighthouse c09a1f1 não define este evento; o interruptor fica no alto da igreja (mapa 0x1C).",
   "region": "MAD_MONSTER_MANSION_CHURCH_INTERIOR_RAFTERS",
   "event": "WITCH_SWITCH_MAD_MONSTER_MANSION",
   "rule": "has(BEAK_BUSTER)"
  },
  {
   "_motivo": "Lighthouse c09a1f1 não define este evento; o interruptor fica no tronco da árvore no inverno.",
   "region": "CLICK_CLOCK_WOOD_WINTER_UPPER_TREE",
   "event": "WITCH_SWITCH_CLICK_CLOCK_WOOD",
   "rule": "has(BEAK_BUSTER)"
  }
 ],
 "extra_exits": [
  {
   "_motivo": "Lighthouse c09a1f1 não tem entrada para esta região (Jinjo verde de CC); o Archipelago-BK exige só nadar.",
   "from": "CLANKERS_CAVERN_NEAR_CLANKER",
   "to": "CLANKERS_CAVERN_UNDERWATER_LOCK",
   "rule": "has(DIVE)"
  },
  {
   "_motivo": "Lighthouse c09a1f1 não tem entrada para a tumba de Sandybutt; o Archipelago-BK exige ovos a partir da área superior.",
   "from": "GOBIS_VALLEY_UPPER_AREA",
   "to": "GOBIS_VALLEY_INTERIOR_MAZE_PYRAMID",
   "rule": "has(EGGS)"
  }
 ],
 "worlds": {
  "NONE": 0,
  "MM": 1,
  "TTC": 2,
  "CC": 3,
  "BGS": 4,
  "FP": 5,
  "LAIR": 6,
  "GV": 7,
  "CCW": 8,
  "RBB": 9,
  "MMM": 10,
  "SM": 11
 },
 "moves": {
  "BEAK_BARGE": 0,
  "BEAK_BOMB": 1,
  "BEAK_BUSTER": 2,
  "CAMERA_CONTROL": 3,
  "CLAW_SWIPE": 4,
  "CLIMB": 5,
  "EGGS": 6,
  "FEATHERY_FLAP": 7,
  "FLAP_FLIP": 8,
  "FLIGHT": 9,
  "HIGH_JUMP": 10,
  "RATATAT_RAP": 11,
  "ROLL": 12,
  "SHOCK_JUMP": 13,
  "WADING_BOOTS": 14,
  "DIVE": 15,
  "TALON_TROT": 16,
  "TURBO_TALON": 17,
  "WONDERWING": 18,
  "NOTE_DOORS": 19
 },
 "puzzle_slots": {
  "_motivo": "Ordem de PICTURE_INFO (src/lair/jigsawpicture.c). NÃO derive do level_e: MMM e CCW trocariam de custo.",
  "MM": 0,
  "TTC": 1,
  "CC": 2,
  "BGS": 3,
  "FP": 4,
  "GV": 5,
  "MMM": 6,
  "RBB": 7,
  "CCW": 8,
  "LAIR": 9
 },
 "puzzle_events": {
  "MM": "PUZZLE_BOARD_MUMBOS_MOUNTAIN",
  "TTC": "PUZZLE_BOARD_TREASURE_TROVE_COVE",
  "CC": "PUZZLE_BOARD_CLANKERS_CAVERN",
  "BGS": "PUZZLE_BOARD_BUBBLEGLOOP_SWAMP",
  "FP": "PUZZLE_BOARD_FREEZEEZY_PEAK",
  "GV": "PUZZLE_BOARD_GOBIS_VALLEY",
  "MMM": "PUZZLE_BOARD_MAD_MONSTER_MANSION",
  "RBB": "PUZZLE_BOARD_RUSTY_BUCKET_BAY",
  "CCW": "PUZZLE_BOARD_CLICK_CLOCK_WOOD",
  "LAIR": "PUZZLE_BOARD_GRUNTILDA"
 },
 "note_doors": [
  50,
  180,
  260,
  350,
  450,
  640,
  765,
  810,
  828,
  846,
  864,
  882
 ],
 "transformations": {
  "TERMITE": {
   "id": 2,
   "skulls": [
    "MUMBOS_MOUNTAIN_INTERIOR_MUMBOS_SKULL"
   ]
  },
  "CROC": {
   "id": 5,
   "skulls": [
    "BUBBLEGLOOP_SWAMP_INTERIOR_MUMBOS_SKULL"
   ]
  },
  "WALRUS": {
   "id": 4,
   "skulls": [
    "FREEZEEZY_PEAK_INTERIOR_MUMBOS_SKULL"
   ]
  },
  "PUMPKIN": {
   "id": 3,
   "skulls": [
    "MAD_MONSTER_MANSION_INTERIOR_MUMBOS_SKULL"
   ]
  },
  "BEE": {
   "id": 6,
   "skulls": [
    "CLICK_CLOCK_WOOD_SPRING_INTERIOR_MUMBOS_SKULL",
    "CLICK_CLOCK_WOOD_SUMMER_INTERIOR_MUMBOS_SKULL",
    "CLICK_CLOCK_WOOD_AUTUMN_INTERIOR_MUMBOS_SKULL"
   ]
  }
 },
 "helpers": {
  "_motivo": "Equivalentes a CAN_ATTACK, CAN_EXTEND_JUMP_DISTANCE, CanBreakObject e CanKillEnemy (Lighthouse Logic.h).",
  "can_attack": "has(RATATAT_RAP) or has(CLAW_SWIPE) or has(EGGS) or has(BEAK_BUSTER) or has(ROLL) or has(WONDERWING) or has(BEAK_BARGE)",
  "extend_jump": "has(FEATHERY_FLAP) or has(RATATAT_RAP) or has(TALON_TROT)",
  "break(BOULDER)": "has(BEAK_BARGE) or has(EGGS) or has(BEAK_BUSTER) or has(WONDERWING)",
  "break(BRICK_WALL)": "has(BEAK_BARGE) or has(WONDERWING) or has(RATATAT_RAP) or has(EGGS)",
  "break(CELLAR_CASK)": "has(BEAK_BARGE) or has(WONDERWING) or has(RATATAT_RAP) or has(EGGS)",
  "break(WOODEN_DOOR)": "has(BEAK_BARGE) or has(WONDERWING) or has(RATATAT_RAP) or has(EGGS)",
  "break(GNAWTYS_BOULDER)": "has(BEAK_BARGE) or has(BEAK_BUSTER) or has(EGGS) or has(WONDERWING)",
  "break(GRATE)": "has(EGGS) or has(RATATAT_RAP)",
  "break(IRON_GATE)": "has(BEAK_BARGE) or has(WONDERWING) or has(RATATAT_RAP)",
  "break(WEB)": "has(EGGS)",
  "break(WINDOWS)": "has(EGGS) or has(WONDERWING) or has(RATATAT_RAP)",
  "kill(SIR_SLUSH)": "has(BEAK_BOMB) and has(FLIGHT)",
  "kill(ZUBBA)": "has(BEAK_BARGE) or has(WONDERWING) or has(RATATAT_RAP)"
 }
}
```

- [ ] **Passo 5: Implementar o compilador**

Arquivo `tools/compile_logic.py` (criar):

```python
#!/usr/bin/env python3
"""Compilador da lógica do randomizer: JSON (DSL) -> tabelas C para o núcleo portátil.

Entrada:  logic/lighthouse_raw.json (gerado por import_lighthouse.py)
          logic/overrides.json      (política e correções revisadas por humanos)
Saída:    core/generated/bkr_logic_data.h
          core/generated/bkr_logic_data.c
          logic/compiled_report.json

Uso:
    python3 tools/compile_logic.py --raw logic/lighthouse_raw.json \
        --overrides logic/overrides.json --out core/generated --report logic/compiled_report.json
"""
import argparse
import copy
import json
import re
import sys
from pathlib import Path

# Opcodes: DEVEM ser idênticos a core/include/bkr/bkr_rules.h (tools/tests verifica).
OPCODES = {
    "END": 0, "TRUE": 1, "FALSE": 2, "HAS": 3, "EVENT": 4, "REGION": 5,
    "NOTE_DOOR": 6, "WORLD_COST": 7, "TRANSFORM_COST": 8, "TOKENS": 9,
    "JIGGIES": 10, "NOTES": 11, "AND": 12, "OR": 13, "NOT": 14,
}
MAX_DEPTH = 32  # BKR_RULE_MAX_DEPTH: a pilha do avaliador C é um único bkr_u32

LOC_KINDS = {"JIGGY": 1, "HONEYCOMB": 2, "TOKEN": 3, "MOLEHILL": 4}
ITEMS = {"JIGGY": 1, "HONEYCOMB": 2, "TOKEN": 3}
ITEM_MOVE_BASE = 0x20
FILE_WORLDS = {
    "MumbosMountain.cpp": "MM", "TreasureTroveCove.cpp": "TTC", "ClankersCavern.cpp": "CC",
    "BubblegloopSwamp.cpp": "BGS", "FreezeezyPeak.cpp": "FP", "GruntildasLair.cpp": "LAIR",
    "GobisValley.cpp": "GV", "ClickClockWood.cpp": "CCW", "RustyBucketBay.cpp": "RBB",
    "MadMonsterMansion.cpp": "MMM", "SpiralMountain.cpp": "SM",
}
TYPE_LABELS = {"JIGGY": "Jiggy", "HONEYCOMB": "Honeycomb", "TOKEN": "Token", "MOLEHILL": "Molehill"}
WORLD_ACRONYMS = {"MM", "TTC", "CC", "BGS", "FP", "GV", "MMM", "RBB", "CCW", "SM", "GL"}
DSL_TOKEN_RE = re.compile(r"\s*([A-Za-z_][A-Za-z0-9_]*|\d+|\(|\))")


class LogicError(Exception):
    """Erro de compilação da lógica."""


# ----------------------------------------------------------------------------- DSL

def tokenize(text):
    """Quebra uma regra da DSL em tokens; falha em caracteres desconhecidos."""
    tokens = []
    pos = 0
    text = text.strip()
    while pos < len(text):
        match = DSL_TOKEN_RE.match(text, pos)
        if not match or match.end() == pos:
            raise LogicError(f"caractere inválido na posição {pos} de {text!r}")
        tokens.append(match.group(1))
        pos = match.end()
        while pos < len(text) and text[pos].isspace():
            pos += 1
    return tokens


class Parser:
    """Analisador descendente: or > and > not > átomo."""

    def __init__(self, text):
        self.text = text
        self.tokens = tokenize(text)
        self.pos = 0

    def peek(self):
        return self.tokens[self.pos] if self.pos < len(self.tokens) else None

    def take(self, expected=None):
        tok = self.peek()
        if tok is None or (expected is not None and tok != expected):
            raise LogicError(f"esperado {expected!r}, achado {tok!r} em {self.text!r}")
        self.pos += 1
        return tok

    def parse(self):
        node = self.parse_or()
        if self.peek() is not None:
            raise LogicError(f"tokens sobrando em {self.text!r}: {self.tokens[self.pos:]}")
        return node

    def parse_or(self):
        items = [self.parse_and()]
        while self.peek() == "or":
            self.take("or")
            items.append(self.parse_and())
        return items[0] if len(items) == 1 else ("or", items)

    def parse_and(self):
        items = [self.parse_not()]
        while self.peek() == "and":
            self.take("and")
            items.append(self.parse_not())
        return items[0] if len(items) == 1 else ("and", items)

    def parse_not(self):
        if self.peek() == "not":
            self.take("not")
            return ("not", self.parse_not())
        return self.parse_atom()

    def parse_atom(self):
        tok = self.take()
        if tok == "(":
            node = self.parse_or()
            self.take(")")
            return node
        if tok in ("true", "false"):
            return (tok,)
        if self.peek() == "(":
            self.take("(")
            arg = self.take()
            self.take(")")
            return ("call", tok, arg)
        return ("ident", tok)


class Lowerer:
    """Expande helpers e converte chamadas da DSL em primitivas numéricas."""

    def __init__(self, ctx):
        self.ctx = ctx

    def lower(self, node, depth=0):
        if depth > 40:
            raise LogicError("expansão de helpers recursiva demais")
        kind = node[0]
        if kind in ("true", "false"):
            return node
        if kind in ("and", "or"):
            return (kind, [self.lower(n, depth + 1) for n in node[1]])
        if kind == "not":
            return ("not", self.lower(node[1], depth + 1))
        if kind == "ident":
            return self.expand_helper(node[1], depth)
        if kind == "call":
            return self.lower_call(node[1], node[2], depth)
        raise LogicError(f"nó desconhecido {node}")

    def expand_helper(self, key, depth):
        helpers = self.ctx["helpers"]
        if key not in helpers:
            raise LogicError(f"helper desconhecido: {key}")
        return self.lower(Parser(helpers[key]).parse(), depth + 1)

    def lower_call(self, name, arg, depth):
        ctx = self.ctx
        if name == "has":
            if arg not in ctx["moves"]:
                raise LogicError(f"movimento desconhecido: {arg}")
            return ("op", "HAS", ctx["moves"][arg])
        if name == "event":
            ctx["referenced_events"].add(arg)
            return ("event_ref", arg)
        if name == "region":
            return ("region_ref", arg)
        if name == "note_door":
            cost = int(arg)
            if cost not in ctx["note_doors"]:
                raise LogicError(f"porta de notas desconhecida: {cost}")
            index = ctx["note_doors"].index(cost)
            ctx["referenced_events"].add(f"NOTE_DOOR_{cost}")
            return ("and", [("op", "NOTE_DOOR", index), ("event_ref", f"NOTE_DOOR_{cost}")])
        if name == "world":
            slot = ctx["puzzle_slots"][arg]
            event = ctx["puzzle_events"][arg]
            ctx["referenced_events"].add(event)
            return ("and", [("op", "WORLD_COST", slot), ("event_ref", event)])
        if name == "transform":
            info = ctx["transformations"][arg]
            skulls = [("region_ref", r) for r in info["skulls"]]
            reach = skulls[0] if len(skulls) == 1 else ("or", skulls)
            return ("and", [("op", "TRANSFORM_COST", info["id"]), reach])
        if name == "jinjos":
            events = sorted(e for e in ctx["jinjo_events"] if e.startswith(f"JINJO_{arg}_"))
            if len(events) != 5:
                raise LogicError(f"esperados 5 Jinjos em {arg}, achados {len(events)}")
            for e in events:
                ctx["referenced_events"].add(e)
            return ("and", [("event_ref", e) for e in events])
        if name in ("break", "kill"):
            return self.expand_helper(f"{name}({arg})", depth)
        if name in ("tokens", "jiggies", "notes"):
            return ("op", name.upper(), int(arg))
        raise LogicError(f"função desconhecida na DSL: {name}({arg})")


def simplify(node):
    """Dobra constantes e achata and/or aninhados."""
    kind = node[0]
    if kind == "not":
        inner = simplify(node[1])
        if inner[0] == "true":
            return ("false",)
        if inner[0] == "false":
            return ("true",)
        return ("not", inner)
    if kind in ("and", "or"):
        absorbing = "false" if kind == "and" else "true"
        neutral = "true" if kind == "and" else "false"
        items = []
        for child in (simplify(n) for n in node[1]):
            if child[0] == absorbing:
                return (absorbing,)
            if child[0] == neutral:
                continue
            if child[0] == kind:
                items.extend(child[1])
            else:
                items.append(child)
        unique = []
        for item in items:
            if item not in unique:
                unique.append(item)
        if not unique:
            return (neutral,)
        return unique[0] if len(unique) == 1 else (kind, unique)
    return node


def emit_rpn(node, event_index, region_index):
    """Gera RPN (lista de u16) e a profundidade máxima de pilha."""
    out = []

    def walk(n):
        kind = n[0]
        if kind == "true":
            out.append(OPCODES["TRUE"])
            return 1
        if kind == "false":
            out.append(OPCODES["FALSE"])
            return 1
        if kind == "op":
            out.extend([OPCODES[n[1]], n[2]])
            return 1
        if kind == "event_ref":
            if n[1] not in event_index:
                raise LogicError(f"evento não definido: {n[1]}")
            out.extend([OPCODES["EVENT"], event_index[n[1]]])
            return 1
        if kind == "region_ref":
            if n[1] not in region_index:
                raise LogicError(f"região não definida: {n[1]}")
            out.extend([OPCODES["REGION"], region_index[n[1]]])
            return 1
        if kind == "not":
            depth = walk(n[1])
            out.append(OPCODES["NOT"])
            return depth
        if kind in ("and", "or"):
            depth = walk(n[1][0])
            for i, child in enumerate(n[1][1:]):
                depth = max(depth, i + 1 + walk(child))
                out.append(OPCODES["AND" if kind == "and" else "OR"])
            return depth
        raise LogicError(f"nó não emitível: {n}")

    depth = walk(node)
    out.append(OPCODES["END"])
    if depth > MAX_DEPTH:
        raise LogicError(f"regra excede a pilha de {MAX_DEPTH}: {node}")
    return out, depth


# ----------------------------------------------------------------------- modelo

def display_name(rc_id, world, loc_type):
    """RC_MM_JIGGY_TICKERS_TOWER -> 'MM Jiggy - Tickers Tower' (o covil usa o prefixo GL)."""
    world_label = "GL" if world == "LAIR" else world
    body = rc_id[3:]
    prefix = f"{world_label}_"
    if body.startswith(prefix):
        body = body[len(prefix):]
    type_tokens = {"JIGGY": "JIGGY_", "HONEYCOMB": "EMPTY_HONEYCOMB_", "TOKEN": "MUMBO_TOKEN_",
                   "MOLEHILL": "MOLEHILL_"}[loc_type]
    if body.startswith(type_tokens):
        body = body[len(type_tokens):]
    words = " ".join(w if w in WORLD_ACRONYMS else w.capitalize() for w in body.split("_"))
    return f"{world_label} {TYPE_LABELS[loc_type]} - {words}"


def build_model(raw, ov):
    """Aplica a política do overrides.json sobre o JSON bruto (sem alterar ``raw``)."""
    raw = copy.deepcopy(raw)
    checks = raw["checks"]
    regions = raw["regions"]
    renames = {k: v for k, v in ov["duplicate_check_renames"].items() if not k.startswith("_")}
    rule_patches = {k: v for k, v in ov["check_rule_patches"].items() if not k.startswith("_")}
    excluded = set(ov["excluded_checks"])
    kinds = set(ov["location_types"])
    token_over = {k: v for k, v in ov["token_uid_overrides"].items() if not k.startswith("_")}
    mole_index = {k: v for k, v in ov["molehill_index"].items() if not k.startswith("_")}

    seen = set()
    for region in regions:
        fixed = []
        for check_id, rule in region["checks"]:
            if check_id in seen and check_id in renames:
                check_id = renames[check_id]
            if check_id in seen:
                raise LogicError(f"checagem repetida sem correção: {check_id}")
            seen.add(check_id)
            fixed.append([check_id, rule_patches.get(check_id, rule)])
        region["checks"] = fixed
        region["world"] = FILE_WORLDS[region["file"]]
    for region in regions:
        for check_id, _ in region["checks"]:
            if check_id not in checks and check_id in renames.values():
                original = next(k for k, v in renames.items() if v == check_id)
                checks[check_id] = dict(checks[original])

    region_ids = [r["id"] for r in regions]
    region_index = {rid: i for i, rid in enumerate(region_ids)}
    for extra in ov["extra_events"]:
        if extra["region"] not in region_index:
            raise LogicError(f"extra_events cita região inexistente: {extra['region']}")
        regions[region_index[extra["region"]]]["events"].append([extra["event"], extra["rule"]])
    for extra in ov.get("extra_exits", []):
        if extra["from"] not in region_index or extra["to"] not in region_index:
            raise LogicError(f"extra_exits cita região inexistente: {extra}")
        regions[region_index[extra["from"]]]["exits"].append([extra["to"], extra["rule"]])

    locations, jinjo_events, note_groups, ignored = [], [], [], {}
    for r_i, region in enumerate(regions):
        groups = {}
        for check_id, rule in region["checks"]:
            check = checks[check_id]
            ctype = check["type"]
            if ctype == "JINJO":
                name = "JINJO_" + check_id[3:].replace("_JINJO_", "_", 1)
                jinjo_events.append((r_i, name, rule))
            elif ctype == "NOTE":
                groups.setdefault(rule, {"count": 0, "world": check["world"]})
                groups[rule]["count"] += 1
            elif ctype in kinds and check_id not in excluded:
                locations.append(make_location(check_id, check, r_i, rule, token_over, mole_index, ov))
            else:
                ignored[ctype] = ignored.get(ctype, 0) + 1
        for rule, info in groups.items():
            note_groups.append((r_i, rule, info["count"], info["world"]))
    return {
        "regions": regions, "region_index": region_index, "locations": locations,
        "jinjo_events": jinjo_events, "note_groups": note_groups, "ignored": ignored,
    }


def make_location(check_id, check, region_i, rule, token_over, mole_index, ov):
    """Cria uma localização com id de jogo e item vanilla."""
    ctype = check["type"]
    if ctype == "TOKEN":
        game_id = token_over.get(check_id, check["collection_number"])
    elif ctype == "MOLEHILL":
        game_id = mole_index[check_id]
    else:
        game_id = check["collection_number"]
    if game_id is None:
        raise LogicError(f"sem id de jogo para {check_id} ({check['collection']})")
    if ctype == "MOLEHILL":
        vanilla_item = ITEM_MOVE_BASE + ov["moves"][collection_move(check["collection"])]
    else:
        vanilla_item = ITEMS[ctype]
    return {
        "id": check_id, "kind": ctype, "world": check["world"], "region": region_i,
        "rule": rule, "game_id": game_id, "vanilla_item": vanilla_item,
        "name": display_name(check_id, check["world"], ctype),
    }


def collection_move(collection):
    """ABILITY_10_TALON_TROT -> TALON_TROT (nomes da DSL)."""
    table = {
        "ABILITY_0_BARGE": "BEAK_BARGE", "ABILITY_1_BEAK_BOMB": "BEAK_BOMB",
        "ABILITY_2_BEAK_BUSTER": "BEAK_BUSTER", "ABILITY_3_CAMERA_CONTROL": "CAMERA_CONTROL",
        "ABILITY_4_CLAW_SWIPE": "CLAW_SWIPE", "ABILITY_5_CLIMB": "CLIMB", "ABILITY_6_EGGS": "EGGS",
        "ABILITY_7_FEATHERY_FLAP": "FEATHERY_FLAP", "ABILITY_8_FLAP_FLIP": "FLAP_FLIP",
        "ABILITY_9_FLIGHT": "FLIGHT", "ABILITY_A_HOLD_A_JUMP_HIGHER": "HIGH_JUMP",
        "ABILITY_B_RATATAT_RAP": "RATATAT_RAP", "ABILITY_C_ROLL": "ROLL",
        "ABILITY_D_SHOCK_JUMP": "SHOCK_JUMP", "ABILITY_E_WADING_BOOTS": "WADING_BOOTS",
        "ABILITY_F_DIVE": "DIVE", "ABILITY_10_TALON_TROT": "TALON_TROT",
        "ABILITY_11_TURBO_TALON": "TURBO_TALON", "ABILITY_12_WONDERWING": "WONDERWING",
    }
    if collection not in table:
        raise LogicError(f"molehill com habilidade desconhecida: {collection}")
    return table[collection]


def validate(model, ov):
    """Invariantes do conjunto de localizações (aborta em qualquer desvio)."""
    by_kind = {}
    for loc in model["locations"]:
        by_kind.setdefault(loc["kind"], []).append(loc["game_id"])
    expected = {"JIGGY": set(range(1, 101)), "HONEYCOMB": set(range(1, 25)),
                "TOKEN": set(range(1, 117)), "MOLEHILL": set(range(0, 9))}
    for kind, ids in expected.items():
        got = by_kind.get(kind, [])
        if sorted(got) != sorted(ids):
            missing = sorted(ids - set(got))
            dups = sorted({x for x in got if got.count(x) > 1})
            raise LogicError(f"{kind}: faltando {missing[:10]} duplicados {dups[:10]}")
    notes = {}
    for _, _, count, world in model["note_groups"]:
        notes[world] = notes.get(world, 0) + count
    for world in ("MM", "TTC", "CC", "BGS", "FP", "GV", "MMM", "RBB", "CCW"):
        if notes.get(world) != 100:
            raise LogicError(f"{world} deveria ter 100 notas, tem {notes.get(world)}")
    if len(model["jinjo_events"]) != 45:
        raise LogicError(f"esperados 45 Jinjos, achados {len(model['jinjo_events'])}")
    if ov["start_region"] not in model["region_index"]:
        raise LogicError("start_region inexistente")


# --------------------------------------------------------------------- emissão C

def c_string(text):
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


def fnv1a(values):
    h = 0x811C9DC5
    for v in values:
        for shift in (0, 8):
            h ^= (v >> shift) & 0xFF
            h = (h * 0x01000193) & 0xFFFFFFFF
    return h


def compile_all(raw, ov):
    """Executa o pipeline completo e devolve (header, source, report)."""
    model = build_model(raw, ov)
    validate(model, ov)
    regions = model["regions"]
    region_index = model["region_index"]

    events = set()
    for region in regions:
        for name, _ in region["events"]:
            events.add(name)
    for _, name, _ in model["jinjo_events"]:
        events.add(name)
    event_names = sorted(events)
    event_index = {name: i for i, name in enumerate(event_names)}
    if ov["goal_event"] not in event_index:
        raise LogicError("goal_event não definido")

    ctx = {
        "helpers": {k: v for k, v in ov["helpers"].items() if not k.startswith("_")},
        "moves": ov["moves"], "note_doors": ov["note_doors"],
        "puzzle_slots": {k: v for k, v in ov["puzzle_slots"].items() if not k.startswith("_")},
        "puzzle_events": ov["puzzle_events"], "transformations": ov["transformations"],
        "jinjo_events": [name for _, name, _ in model["jinjo_events"]],
        "referenced_events": set(),
    }
    lowerer = Lowerer(ctx)
    code = []
    cache = {}
    max_depth = [0]

    def rule_offset(text):
        node = simplify(lowerer.lower(Parser(text).parse()))
        rpn, depth = emit_rpn(node, event_index, region_index)
        max_depth[0] = max(max_depth[0], depth)
        key = tuple(rpn)
        if key not in cache:
            cache[key] = len(code)
            code.extend(rpn)
        return cache[key]

    exits, region_rows, event_defs, groups, locs = [], [], [], [], []
    loc_by_region = {}
    for loc in model["locations"]:
        loc_by_region.setdefault(loc["region"], []).append(loc)
    jinjo_by_region = {}
    for r_i, name, rule in model["jinjo_events"]:
        jinjo_by_region.setdefault(r_i, []).append((name, rule))
    groups_by_region = {}
    for r_i, rule, count, world in model["note_groups"]:
        groups_by_region.setdefault(r_i, []).append((rule, count, world))

    world_ids = ov["worlds"]
    for r_i, region in enumerate(regions):
        row = {"exit_first": len(exits), "loc_first": len(locs),
               "event_first": len(event_defs), "group_first": len(groups)}
        for target, rule in region["exits"]:
            exits.append((region_index[target], rule_offset(rule)))
        for name, rule in region["events"] + jinjo_by_region.get(r_i, []):
            event_defs.append((event_index[name], rule_offset(rule)))
        for rule, count, world in groups_by_region.get(r_i, []):
            groups.append((rule_offset(rule), count, world_ids[world]))
        for loc in loc_by_region.get(r_i, []):
            locs.append(dict(loc, rule_off=rule_offset(loc["rule"])))
        row.update(exit_count=len(exits) - row["exit_first"], loc_count=len(locs) - row["loc_first"],
                   event_count=len(event_defs) - row["event_first"],
                   group_count=len(groups) - row["group_first"], world=world_ids[region["world"]])
        region_rows.append(row)

    undefined = sorted(ctx["referenced_events"] - set(event_names))
    if undefined:
        raise LogicError(f"eventos referenciados e não definidos: {undefined}")

    starting = 0
    for move in ov["starting_moves"]:
        starting |= 1 << ov["moves"][move]
    lookup = {"JIGGY": [0xFFFF] * 101, "HONEYCOMB": [0xFFFF] * 25, "TOKEN": [0xFFFF] * 128,
              "MOLEHILL": [0xFFFF] * 10}
    for i, loc in enumerate(locs):
        lookup[loc["kind"]][loc["game_id"]] = i

    numeric = list(code)
    for row in region_rows:
        numeric += [row[k] for k in ("exit_first", "exit_count", "loc_first", "loc_count",
                                     "event_first", "event_count", "group_first", "group_count",
                                     "world")]
    for e in exits:
        numeric += list(e)
    for loc in locs:
        numeric += [loc["region"], loc["rule_off"], loc["game_id"], LOC_KINDS[loc["kind"]],
                    world_ids[loc["world"]], loc["vanilla_item"]]
    for e in event_defs:
        numeric += list(e)
    for g in groups:
        numeric += list(g)
    data_hash = fnv1a(numeric)

    defines = {
        "BKR_REGION_COUNT": len(region_rows), "BKR_LOCATION_COUNT": len(locs),
        "BKR_EXIT_COUNT": len(exits), "BKR_EVENT_COUNT": len(event_names),
        "BKR_EVENT_DEF_COUNT": len(event_defs), "BKR_NOTE_GROUP_COUNT": len(groups),
        "BKR_RULE_CODE_SIZE": len(code), "BKR_START_REGION": region_index[ov["start_region"]],
        "BKR_GOAL_EVENT": event_index[ov["goal_event"]],
    }
    header = render_header(defines, starting, data_hash, raw["source"]["pinned_commit"])
    source = render_source(region_rows, exits, locs, event_defs, groups, code, regions,
                           event_names, lookup, world_ids)
    report = {
        "defines": defines, "max_rule_depth": max_depth[0], "ignored_checks": model["ignored"],
        "data_hash": f"0x{data_hash:08X}",
        "locations_by_kind": {k: sum(1 for l in locs if l["kind"] == k) for k in LOC_KINDS},
    }
    return header, source, report


def render_header(defines, starting, data_hash, commit):
    lines = [
        "/* Arquivo gerado por tools/compile_logic.py — NÃO EDITE À MÃO. */",
        "#ifndef BKR_LOGIC_DATA_H", "#define BKR_LOGIC_DATA_H", "",
        f'#define BKR_LOGIC_SOURCE_COMMIT "{commit}"',
    ]
    for key, value in defines.items():
        lines.append(f"#define {key} {value}")
    lines += [
        f"#define BKR_STARTING_MOVES 0x{starting:08X}u",
        f"#define BKR_LOGIC_DATA_HASH 0x{data_hash:08X}u",
        "", "#endif", "",
    ]
    return "\n".join(lines)


def render_source(region_rows, exits, locs, event_defs, groups, code, regions, event_names,
                  lookup, world_ids):
    out = ["/* Arquivo gerado por tools/compile_logic.py — NÃO EDITE À MÃO. */",
           '#include "bkr/bkr_world.h"', ""]
    out.append("const BkrRegionDef bkr_regions[BKR_REGION_COUNT] = {")
    for row, region in zip(region_rows, regions):
        out.append("    {{{exit_first}, {exit_count}, {loc_first}, {loc_count}, {event_first}, "
                   "{event_count}, {group_first}, {group_count}, {world}, 0}}, /* {name} */"
                   .format(name=region["id"], **row))
    out.append("};\n")
    out.append("const BkrExitDef bkr_exits[BKR_EXIT_COUNT] = {")
    out += [f"    {{{to}, {rule}}}," for to, rule in exits]
    out.append("};\n")
    out.append("const BkrLocDef bkr_locations[BKR_LOCATION_COUNT] = {")
    for i, loc in enumerate(locs):
        out.append(f"    {{{loc['region']}, {loc['rule_off']}, {loc['game_id']}, "
                   f"{LOC_KINDS[loc['kind']]}, {world_ids[loc['world']]}, {loc['vanilla_item']}, 0}},"
                   f" /* {i}: {loc['id']} */")
    out.append("};\n")
    out.append("const BkrEventDef bkr_event_defs[BKR_EVENT_DEF_COUNT] = {")
    out += [f"    {{{ev}, {rule}}}, /* {event_names[ev]} */" for ev, rule in event_defs]
    out.append("};\n")
    out.append("const BkrNoteGroupDef bkr_note_groups[BKR_NOTE_GROUP_COUNT] = {")
    out += [f"    {{{rule}, {count}, {world}}}," for rule, count, world in groups]
    out.append("};\n")
    out.append("const bkr_u16 bkr_rule_code[BKR_RULE_CODE_SIZE] = {")
    for i in range(0, len(code), 16):
        out.append("    " + ", ".join(str(v) for v in code[i:i + 16]) + ",")
    out.append("};\n")
    out.append("const char* const bkr_location_names[BKR_LOCATION_COUNT] = {")
    out += [f"    {c_string(loc['name'])}," for loc in locs]
    out.append("};\n")
    out.append("const char* const bkr_region_names[BKR_REGION_COUNT] = {")
    out += [f"    {c_string(r['id'])}," for r in regions]
    out.append("};\n")
    out.append("const char* const bkr_event_names[BKR_EVENT_COUNT] = {")
    out += [f"    {c_string(n)}," for n in event_names]
    out.append("};\n")
    for kind, size in (("JIGGY", 101), ("HONEYCOMB", 25), ("TOKEN", 128), ("MOLEHILL", 10)):
        name = {"JIGGY": "jiggy", "HONEYCOMB": "honeycomb", "TOKEN": "token", "MOLEHILL": "mole"}[kind]
        out.append(f"const bkr_u16 bkr_loc_by_{name}[{size}] = {{")
        values = lookup[kind]
        for i in range(0, size, 16):
            out.append("    " + ", ".join(f"0x{v:04X}" for v in values[i:i + 16]) + ",")
        out.append("};\n")
    return "\n".join(out)


def main(argv=None):
    parser = argparse.ArgumentParser(description="Compila a lógica do randomizer em C")
    parser.add_argument("--raw", required=True, type=Path)
    parser.add_argument("--overrides", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--report", required=True, type=Path)
    args = parser.parse_args(argv)
    raw = json.loads(args.raw.read_text(encoding="utf-8"))
    ov = json.loads(args.overrides.read_text(encoding="utf-8"))
    try:
        header, source, report = compile_all(raw, ov)
    except LogicError as exc:
        print(f"ERRO: {exc}", file=sys.stderr)
        return 1
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "bkr_logic_data.h").write_text(header, encoding="utf-8", newline="\n")
    (args.out / "bkr_logic_data.c").write_text(source, encoding="utf-8", newline="\n")
    args.report.write_text(json.dumps(report, indent=1, ensure_ascii=False) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(report["defines"]))
    print(f"profundidade máxima de regra: {report['max_rule_depth']}  hash: {report['data_hash']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Passo 6: Ver os testes passarem e gerar as tabelas**

**Rodar:**

```bash
python3 -m unittest discover -s tools/tests -v
python3 tools/compile_logic.py --raw logic/lighthouse_raw.json --overrides logic/overrides.json --out core/generated --report logic/compiled_report.json
```

**Esperado:**

```text
Ran 16 tests
OK
{"BKR_REGION_COUNT": 125, "BKR_LOCATION_COUNT": 249, "BKR_EXIT_COUNT": 243, "BKR_EVENT_COUNT": 104, "BKR_EVENT_DEF_COUNT": 105, "BKR_NOTE_GROUP_COUNT": 118, "BKR_RULE_CODE_SIZE": 1769, "BKR_START_REGION": 116, "BKR_GOAL_EVENT": 10}
profundidade máxima de regra: 8  hash: 0x5C82A342
```

`0x5C82A342` é o primeiro valor-ouro: o hash das tabelas numéricas (regiões, saídas, localizações, eventos, grupos de notas e bytecode). Ele entra na impressão digital de todo seed.

- [ ] **Passo 7: Documentar a auditoria**

Arquivo `docs/logic-audit.md` (criar):

```markdown
# Auditoria da lógica importada do Lighthouse

Fonte: `HarbourMasters/Lighthouse` @ `c09a1f11041f50c2fd86d8aa60f8ee15a3ca13fd` (CC0), arquivos
`src/port/Rando/StaticData/Checks.cpp` e `src/port/Rando/Logic/Regions/*.cpp`.
`tools/import_lighthouse.py` traduz esses arquivos para `logic/lighthouse_raw.json` sem alterar nada;
toda correção mora em `logic/overrides.json` e é aplicada por `tools/compile_logic.py`.

Reprodutibilidade: com o checkout esparso do commit acima, o importador gera um arquivo com
sha256 `86be19037dc36af8e76c1006061b54e78a9d0ec0b277bf6351d746a595b62b49`
(125 regiões; 1208 checagens: 100 jiggies, 24 favos, 116 tokens, 45 Jinjos, 15 molehills, 900 notas,
7 itens do Stop 'n' Swop e o marcador `RC_UNKNOWN`).

## Correções aplicadas

| # | Problema no Lighthouse c09a1f1 | Evidência | Correção (`logic/overrides.json`) |
|---|---|---|---|
| 1 | Os eventos `WITCH_SWITCH_CLANKERS_CAVERN`, `WITCH_SWITCH_MAD_MONSTER_MANSION` e `WITCH_SWITCH_CLICK_CLOCK_WOOD` são exigidos, mas nunca definidos (os outros seis são). Os três jiggies do covil ligados a esses interruptores ficariam inalcançáveis. | `GruntildasLair.cpp:131`, `:147`, `:263` usam `CAN_ACCESS(RA_WITCH_SWITCH_…)`; nenhum `EVENT(RA_WITCH_SWITCH_CLANKERS_CAVERN…)`, `…_MAD_MONSTER_MANSION` ou `…_CLICK_CLOCK_WOOD` existe nas regiões. | `extra_events`: CC no quarto do interruptor dentro do Clanker (`CLANKERS_CAVERN_INTERIOR_CLANKER_SLOW_SAWBLADE`), MMM no alto da igreja (`MAD_MONSTER_MANSION_CHURCH_INTERIOR_RAFTERS`), CCW no tronco no inverno (`CLICK_CLOCK_WOOD_WINTER_UPPER_TREE`); regra `has(BEAK_BUSTER)`, como os interruptores definidos. |
| 2 | O jiggy dos Jinjos de BGS tem regra `true`; nos outros mundos é `CAN_COLLECT_JINJOS(...)`. | `BubblegloopSwamp.cpp:14` | `check_rule_patches`: `RC_BGS_JIGGY_JINJO` = `jinjos(BGS)`. |
| 3 | `RC_BGS_NOTE_THIRD_SHORT_SWAMP_LOG_2` aparece duas vezes e `_3` nunca, embora `Checks.cpp` defina `_3`. Uma nota a menos em BGS (99). | `BubblegloopSwamp.cpp:117-118`; `Checks.cpp:152` | `duplicate_check_renames`: a segunda ocorrência vira `_3`. |
| 4 | A região `CLANKERS_CAVERN_UNDERWATER_LOCK` (Jinjo verde e 8 notas) não tem nenhuma entrada. | `ClankersCavern.cpp:193`; nenhum `CONNECTION(RR_CLANKERS_CAVERN_UNDERWATER_LOCK…)` | `extra_exits`: de `CLANKERS_CAVERN_NEAR_CLANKER` com `has(DIVE)` (o Archipelago-BK exige só nadar: `NormalRules.py:2518` @ `a20c2a9`). |
| 5 | A região `GOBIS_VALLEY_INTERIOR_MAZE_PYRAMID` (jiggy do labirinto, Jinjo rosa, token de Sandybutt, 7 notas e o interruptor do GV) não tem entrada. | `GobisValley.cpp:48` | `extra_exits`: de `GOBIS_VALLEY_UPPER_AREA` com `has(EGGS)`. A pirâmide sobe quando Banjo acerta ovos nas esfinges pequenas (decomp `src/GV/code_6370.c`, estado "eating"); o Archipelago-BK pede ovos (`NormalRules.py:5060`). |
| 6 | No jogo original, dois tokens repetem o id 0x3D e dois repetem 0x5E. O Banjo: Recompiled corrige com `bkrecomp_set_mumbo_token_fixes_enabled(TRUE)`: o token de dentro do Loggo vira 0x74 (116) e o do campo de espinhos de CCW (primavera) vira 0x5D (93). O Lighthouse não tem número para o do Loggo e rotula o do Eyrie como 5D e o do campo de espinhos como 5E. | `BanjoRecomp/patches/mumbo_token_patches.c:36-46` (v1.0.2); `Checks.cpp:298`, `:302`, `:845` | `token_uid_overrides`: Loggo = 116, campo de espinhos = 93, frente do Eyrie = 94. O mod liga a correção do base em `src/mod_init.c`. |
| 7 | `CanOpenWorld` deriva o índice do quadro de `level_e` e troca os custos de MMM e CCW (CCW recebe 10, MMM recebe 15). | `Logic.h:231-244` | A DSL `world(X)` usa `puzzle_slots` na ordem de `PICTURE_INFO` (decomp `src/lair/jigsawpicture.c:40-52`): MM 1, TTC 2, CC 5, BGS 7, FP 8, GV 9, MMM 10, RBB 12, CCW 15, Gruntilda 25. |

## Decisões de política (não são bugs)
- **Spiral Mountain:** os 6 molehills de SM ficam fora do embaralhamento (`excluded_checks`) e os 10 movimentos
  de SM entram como movimentos iniciais (`starting_moves`). Pular o tutorial ensina todos eles
  (`__chSmBottles_skipIntroTutorial`, decomp `src/SM/ch/smbottles.c:102`).
- **Molehills dos mundos:** `molehill_index` liga cada checagem ao índice de `moleTable` (`src/core2/ch/mole.c:34`).
- **Notas:** viram grupos por (região, regra) com contagem; as portas de notas usam `note_door(N)` e somam os grupos alcançados.
- **Jinjos:** viram eventos `JINJO_<MUNDO>_<COR>`; `jinjos(X)` exige os cinco do mundo.
- **Stop 'n' Swop:** ignorado (7 checagens).
- **Transformações:** `transform(X)` exige o crânio do Mumbo alcançável e tokens acumulados em ordem de custo
  (cupim 5, crocodilo 15, morsa 30, abóbora 50, abelha 75).

## Validação automatizada
- `test_all_items_reach_everything`: com todos os itens, as 249 localizações, as 900 notas e o objetivo são alcançáveis.
- `test_vanilla_placement_beats_game`: o placement vanilla vence o jogo e alcança 100%.
- `test_logic_actually_gates`: sem itens, menos de 40 localizações são alcançáveis e o objetivo não.
- `test_property_many_seeds`: 500 seeds por preset (4 presets) no `make check`; durante o desenvolvimento,
  2000 por preset: 0 falhas, no máximo 13 tentativas.

## Limitações conhecidas da lógica
- **Recursos gastáveis:** jiggies (quadros) e tokens (Mumbo) usam custo acumulado na ordem de custo. Gastar fora
  dessa ordem pode deixar o jogador sem recurso para o que a lógica espera. Folga: 6 jiggies (100 contra 94) e
  41 tokens (116 contra 75).
- **Quadro de vida dupla:** custa 4 jiggies e não entra na conta; com "Apenas vencível", recomende fazê-lo por último.
- **Glitchless** segue o julgamento do Lighthouse: truques difíceis não são exigidos, mas a lista não foi revisada
  movimento a movimento. Achados novos entram aqui e viram correções no `overrides.json`.
```

- [ ] **Passo 8: Commit**

**Rodar:**

```bash
git add core/include/bkr/bkr_rules.h tools/compile_logic.py tools/tests/test_compile_logic.py logic/overrides.json logic/compiled_report.json core/generated/bkr_logic_data.h core/generated/bkr_logic_data.c docs/logic-audit.md
git commit -q -m "adiciona compilador de lógica, correções auditadas e tabelas geradas"
```

### Fase 2 — Núcleo C portátil

Regras que valem para todo arquivo em `core/` (também estão no `bkr_types.h` e no CLAUDE.md): C99 sem libc e sem `<stdint.h>`; sem float, sem divisão de 64 bits, sem recursão; memória estática em vez de arrays grandes na pilha; structs copiadas campo a campo. O `core/Makefile` compila tudo o que existe em `core/src/` e gera a lista de testes a partir das funções `void test_<nome>(void)` de `core/tests/test_*.c`, então cada Task só acrescenta arquivos.

### Task 7: Tipos, tabelas geradas e executor de testes

**Arquivos:**
- Criar: `core/Makefile`, `core/tests/bkr_test.h`, `core/tests/test_main.c`, `core/tests/test_tables.c`
- Criar: `core/include/bkr/bkr_types.h`, `core/include/bkr/bkr_ids.h`, `core/include/bkr/bkr_world.h`, `core/src/bkr_util.c`

- [ ] **Passo 1: Build, mini framework de testes e executor**

Arquivo `core/Makefile` (criar):

```makefile
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
```

Arquivo `core/tests/bkr_test.h` (criar):

```c
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
```

Arquivo `core/tests/test_main.c` (criar):

```c
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
```

- [ ] **Passo 2: Escrever os testes das tabelas**

Arquivo `core/tests/test_tables.c` (criar):

```c
/* test_tables.c — Invariantes das tabelas geradas por tools/compile_logic.py. */
#include "bkr_test.h"
#include "bkr/bkr_world.h"
#include "bkr/bkr_ids.h"

void test_tables_consistent(void) {
    bkr_u32 r;
    bkr_u32 i;
    bkr_u32 kinds[5] = { 0, 0, 0, 0, 0 };
    bkr_u32 notes = 0;
    bkr_u32 next_loc = 0;
    for (r = 0; r < BKR_REGION_COUNT; r++) {
        const BkrRegionDef* reg = &bkr_regions[r];
        BKR_CHECK(reg->exit_first + reg->exit_count <= BKR_EXIT_COUNT);
        BKR_CHECK(reg->event_first + reg->event_count <= BKR_EVENT_DEF_COUNT);
        BKR_CHECK(reg->group_first + reg->group_count <= BKR_NOTE_GROUP_COUNT);
        BKR_CHECK_EQ(reg->loc_first, next_loc);
        next_loc += reg->loc_count;
        for (i = reg->loc_first; i < (bkr_u32)reg->loc_first + reg->loc_count; i++) {
            BKR_CHECK_EQ(bkr_locations[i].region, r);
        }
    }
    BKR_CHECK_EQ(next_loc, BKR_LOCATION_COUNT);
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        BKR_CHECK(bkr_locations[i].kind >= BKR_LOC_JIGGY && bkr_locations[i].kind <= BKR_LOC_MOLEHILL);
        kinds[bkr_locations[i].kind]++;
        BKR_CHECK(bkr_locations[i].rule < BKR_RULE_CODE_SIZE);
        BKR_CHECK(bkr_locations[i].world < BKR_WORLD_COUNT);
    }
    BKR_CHECK_EQ(kinds[BKR_LOC_JIGGY], 100u);
    BKR_CHECK_EQ(kinds[BKR_LOC_HONEYCOMB], 24u);
    BKR_CHECK_EQ(kinds[BKR_LOC_TOKEN], 116u);
    BKR_CHECK_EQ(kinds[BKR_LOC_MOLEHILL], 9u);
    for (i = 0; i < BKR_EXIT_COUNT; i++) {
        BKR_CHECK(bkr_exits[i].to < BKR_REGION_COUNT);
        BKR_CHECK(bkr_exits[i].rule < BKR_RULE_CODE_SIZE);
    }
    for (i = 0; i < BKR_EVENT_DEF_COUNT; i++) {
        BKR_CHECK(bkr_event_defs[i].event < BKR_EVENT_COUNT);
    }
    for (i = 0; i < BKR_NOTE_GROUP_COUNT; i++) {
        notes += bkr_note_groups[i].count;
    }
    BKR_CHECK_EQ(notes, 900u);
    BKR_CHECK(bkr_streq(bkr_region_names[BKR_START_REGION], "SPIRAL_MOUNTAIN_ENTRANCE"));
    BKR_CHECK(bkr_streq(bkr_event_names[BKR_GOAL_EVENT], "GAME_COMPLETE"));
}

void test_lookup_tables(void) {
    bkr_u32 id;
    BKR_CHECK_EQ(bkr_loc_by_jiggy[0], BKR_LOC_INVALID);
    for (id = 1; id <= 100u; id++) {
        bkr_u16 loc = bkr_loc_by_jiggy[id];
        BKR_CHECK(loc < BKR_LOCATION_COUNT);
        BKR_CHECK_EQ(bkr_locations[loc].kind, BKR_LOC_JIGGY);
        BKR_CHECK_EQ(bkr_locations[loc].game_id, id);
    }
    for (id = 1; id <= 24u; id++) {
        bkr_u16 loc = bkr_loc_by_honeycomb[id];
        BKR_CHECK(loc < BKR_LOCATION_COUNT);
        BKR_CHECK_EQ(bkr_locations[loc].kind, BKR_LOC_HONEYCOMB);
        BKR_CHECK_EQ(bkr_locations[loc].game_id, id);
    }
    for (id = 1; id <= 116u; id++) {
        bkr_u16 loc = bkr_loc_by_token[id];
        BKR_CHECK(loc < BKR_LOCATION_COUNT);
        BKR_CHECK_EQ(bkr_locations[loc].kind, BKR_LOC_TOKEN);
        BKR_CHECK_EQ(bkr_locations[loc].game_id, id);
    }
    for (id = 117; id < 128u; id++) {
        BKR_CHECK_EQ(bkr_loc_by_token[id], BKR_LOC_INVALID);
    }
    for (id = 0; id < 9u; id++) {
        bkr_u16 loc = bkr_loc_by_mole[id];
        BKR_CHECK(loc < BKR_LOCATION_COUNT);
        BKR_CHECK(BKR_ITEM_IS_MOVE(bkr_locations[loc].vanilla_item));
    }
    BKR_CHECK_EQ(bkr_loc_by_mole[9], BKR_LOC_INVALID);
    /* Ids efetivos no Banjo: Recompiled para os tokens que o jogo original duplicava. */
    BKR_CHECK(bkr_streq(bkr_location_names[bkr_loc_by_token[116]], "MMM Token - Inside Loggo"));
    BKR_CHECK(bkr_streq(bkr_location_names[bkr_loc_by_token[93]], "CCW Token - Spring Thorn Field Front Of Mumbos Skull"));
    /* Molehill 1 de moleTable ensina Disparo de Ovos em Mumbo's Mountain. */
    BKR_CHECK(bkr_streq(bkr_location_names[bkr_loc_by_mole[1]], "MM Molehill - Use Eggs"));
    BKR_CHECK_EQ(bkr_locations[bkr_loc_by_mole[1]].vanilla_item, BKR_ITEM_MOVE(BKR_ABILITY_EGGS));
}
```

- [ ] **Passo 3: Ver falhar** (no Windows, o Makefile usa `gcc` sozinho)

**Rodar:**

```bash
make -C core test
```

**Esperado (falha):**

```text
bkr/bkr_world.h
```

- [ ] **Passo 4: Tipos, ids estáveis e estruturas das tabelas**

Os valores de habilidade, mundo e transformação são os mesmos enums do decomp (`ability_e`, `level_e`, `transformation_e`): o mod passa esses números direto para o jogo.

Arquivo `core/include/bkr/bkr_types.h` (criar):

```c
/*
 * bkr_types.h — Tipos básicos do núcleo portátil do randomizer.
 *
 * Regras do núcleo (valem para TODOS os arquivos em core/):
 *  - C99 puro, sem libc, sem <stdint.h> (o mod compila com -nostdinc);
 *  - sem ponto flutuante e sem divisão/módulo de 64 bits;
 *  - sem recursão e sem arrays grandes na pilha (a pilha da thread principal
 *    do jogo tem 0x17F0 bytes);
 *  - sem atribuição de structs grandes (o compilador poderia emitir memcpy).
 * O mesmo código compila no host (testes e CLI) e em MIPS (mod).
 */
#ifndef BKR_TYPES_H
#define BKR_TYPES_H

typedef unsigned char  bkr_u8;
typedef signed char    bkr_s8;
typedef unsigned short bkr_u16;
typedef signed short   bkr_s16;
typedef unsigned int   bkr_u32;
typedef signed int     bkr_s32;

#define BKR_NULL ((void*)0)

#define BKR_STATIC_ASSERT(name, cond) typedef char bkr_static_assert_##name[(cond) ? 1 : -1]
BKR_STATIC_ASSERT(u8_size, sizeof(bkr_u8) == 1);
BKR_STATIC_ASSERT(u16_size, sizeof(bkr_u16) == 2);
BKR_STATIC_ASSERT(u32_size, sizeof(bkr_u32) == 4);

/* Conjuntos de bits em arrays de bkr_u32. */
#define BKR_WORDS(n) (((n) + 31u) / 32u)
#define BKR_BIT_GET(arr, i) (((arr)[(i) >> 5] >> ((i) & 31u)) & 1u)
#define BKR_BIT_SET(arr, i) ((arr)[(i) >> 5] |= (1u << ((i) & 31u)))
#define BKR_BIT_CLR(arr, i) ((arr)[(i) >> 5] &= ~(1u << ((i) & 31u)))

/* Zera/copia palavras sem depender de memset/memcpy. */
void bkr_words_clear(bkr_u32* dst, bkr_u32 count);
void bkr_words_copy(bkr_u32* dst, const bkr_u32* src, bkr_u32 count);

/* Strings mínimas (sem libc). */
bkr_u32 bkr_strlen(const char* s);
int bkr_streq(const char* a, const char* b);

#endif
```

Arquivo `core/include/bkr/bkr_ids.h` (criar):

```c
/*
 * bkr_ids.h — Identificadores estáveis do randomizer.
 * Os valores de habilidade, mundo e transformação são IDÊNTICOS aos enums do
 * decomp de Banjo-Kazooie (include/enums.h, commit 351ca158): ability_e,
 * level_e e transformation_e. Não renumere.
 */
#ifndef BKR_IDS_H
#define BKR_IDS_H

#include "bkr_types.h"

/* enum ability_e */
enum {
    BKR_ABILITY_BEAK_BARGE = 0x00,
    BKR_ABILITY_BEAK_BOMB = 0x01,
    BKR_ABILITY_BEAK_BUSTER = 0x02,
    BKR_ABILITY_CAMERA_CONTROL = 0x03,
    BKR_ABILITY_CLAW_SWIPE = 0x04,
    BKR_ABILITY_CLIMB = 0x05,
    BKR_ABILITY_EGGS = 0x06,
    BKR_ABILITY_FEATHERY_FLAP = 0x07,
    BKR_ABILITY_FLAP_FLIP = 0x08,
    BKR_ABILITY_FLIGHT = 0x09,
    BKR_ABILITY_HIGH_JUMP = 0x0A,
    BKR_ABILITY_RATATAT_RAP = 0x0B,
    BKR_ABILITY_ROLL = 0x0C,
    BKR_ABILITY_SHOCK_JUMP = 0x0D,
    BKR_ABILITY_WADING_BOOTS = 0x0E,
    BKR_ABILITY_DIVE = 0x0F,
    BKR_ABILITY_TALON_TROT = 0x10,
    BKR_ABILITY_TURBO_TALON = 0x11,
    BKR_ABILITY_WONDERWING = 0x12,
    BKR_ABILITY_NOTE_DOORS = 0x13,
    BKR_ABILITY_COUNT = 0x14
};

/* enum level_e */
enum {
    BKR_WORLD_NONE = 0,
    BKR_WORLD_MM = 1,
    BKR_WORLD_TTC = 2,
    BKR_WORLD_CC = 3,
    BKR_WORLD_BGS = 4,
    BKR_WORLD_FP = 5,
    BKR_WORLD_LAIR = 6,
    BKR_WORLD_GV = 7,
    BKR_WORLD_CCW = 8,
    BKR_WORLD_RBB = 9,
    BKR_WORLD_MMM = 10,
    BKR_WORLD_SM = 11,
    BKR_WORLD_COUNT = 12
};

/* enum transformation_e */
enum {
    BKR_TRANSFORM_TERMITE = 2,
    BKR_TRANSFORM_PUMPKIN = 3,
    BKR_TRANSFORM_WALRUS = 4,
    BKR_TRANSFORM_CROC = 5,
    BKR_TRANSFORM_BEE = 6,
    BKR_TRANSFORM_SLOTS = 8
};

/* Tipos de localização (iguais aos de tools/compile_logic.py). */
enum {
    BKR_LOC_NONE = 0,
    BKR_LOC_JIGGY = 1,
    BKR_LOC_HONEYCOMB = 2,
    BKR_LOC_TOKEN = 3,
    BKR_LOC_MOLEHILL = 4
};

/* Itens. Movimento = BKR_ITEM_MOVE_BASE + ability_e. */
enum {
    BKR_ITEM_NONE = 0,
    BKR_ITEM_JIGGY = 1,
    BKR_ITEM_HONEYCOMB = 2,
    BKR_ITEM_TOKEN = 3,
    BKR_ITEM_MOVE_BASE = 0x20
};
#define BKR_ITEM_MOVE(ability) ((bkr_u8)(BKR_ITEM_MOVE_BASE + (ability)))
#define BKR_ITEM_IS_MOVE(item) ((item) >= BKR_ITEM_MOVE_BASE && (item) < BKR_ITEM_MOVE_BASE + BKR_ABILITY_COUNT)
#define BKR_ITEM_ABILITY(item) ((item) - BKR_ITEM_MOVE_BASE)

/* Idiomas de texto. */
enum { BKR_LANG_PT_BR = 0, BKR_LANG_EN = 1 };

/* Nomes legíveis (bkr_names.c). Nunca retornam NULL. */
const char* bkr_item_name(bkr_u8 item, int lang);
const char* bkr_world_name(bkr_u8 world, int lang);
const char* bkr_hash_word(bkr_u32 index);

#endif
```

Arquivo `core/include/bkr/bkr_world.h` (criar):

```c
/*
 * bkr_world.h — Estruturas das tabelas de lógica geradas em
 * core/generated/bkr_logic_data.c por tools/compile_logic.py.
 * Cada região possui faixas contíguas de saídas, localizações, eventos e
 * grupos de notas.
 */
#ifndef BKR_WORLD_H
#define BKR_WORLD_H

#include "bkr_types.h"
#include "bkr_logic_data.h"

typedef struct BkrRegionDef {
    bkr_u16 exit_first, exit_count;
    bkr_u16 loc_first, loc_count;
    bkr_u16 event_first, event_count;
    bkr_u16 group_first, group_count;
    bkr_u8 world;
    bkr_u8 pad;
} BkrRegionDef;

typedef struct BkrExitDef {
    bkr_u16 to;
    bkr_u16 rule;
} BkrExitDef;

typedef struct BkrLocDef {
    bkr_u16 region;
    bkr_u16 rule;
    bkr_u16 game_id;      /* jiggy_e / honeycomb_e / uid do token / índice em moleTable */
    bkr_u8 kind;          /* BKR_LOC_* */
    bkr_u8 world;         /* level_e */
    bkr_u8 vanilla_item;  /* BKR_ITEM_* */
    bkr_u8 pad;
} BkrLocDef;

typedef struct BkrEventDef {
    bkr_u16 event;
    bkr_u16 rule;
} BkrEventDef;

typedef struct BkrNoteGroupDef {
    bkr_u16 rule;
    bkr_u8 count;
    bkr_u8 world;
} BkrNoteGroupDef;

#define BKR_LOC_INVALID 0xFFFFu
#define BKR_LOC_WORDS BKR_WORDS(BKR_LOCATION_COUNT)
#define BKR_REGION_WORDS BKR_WORDS(BKR_REGION_COUNT)
#define BKR_EVENT_WORDS BKR_WORDS(BKR_EVENT_COUNT)
#define BKR_EVDEF_WORDS BKR_WORDS(BKR_EVENT_DEF_COUNT)
#define BKR_GROUP_WORDS BKR_WORDS(BKR_NOTE_GROUP_COUNT)

extern const BkrRegionDef bkr_regions[BKR_REGION_COUNT];
extern const BkrExitDef bkr_exits[BKR_EXIT_COUNT];
extern const BkrLocDef bkr_locations[BKR_LOCATION_COUNT];
extern const BkrEventDef bkr_event_defs[BKR_EVENT_DEF_COUNT];
extern const BkrNoteGroupDef bkr_note_groups[BKR_NOTE_GROUP_COUNT];
extern const bkr_u16 bkr_rule_code[BKR_RULE_CODE_SIZE];
extern const char* const bkr_location_names[BKR_LOCATION_COUNT];
extern const char* const bkr_region_names[BKR_REGION_COUNT];
extern const char* const bkr_event_names[BKR_EVENT_COUNT];

/* Busca reversa: id do jogo -> índice de localização (BKR_LOC_INVALID se não houver). */
extern const bkr_u16 bkr_loc_by_jiggy[101];
extern const bkr_u16 bkr_loc_by_honeycomb[25];
extern const bkr_u16 bkr_loc_by_token[128];
extern const bkr_u16 bkr_loc_by_mole[10];

#endif
```

Arquivo `core/src/bkr_util.c` (criar):

```c
/* bkr_util.c — Utilidades sem libc: conjuntos de palavras e strings. */
#include "bkr/bkr_types.h"

void bkr_words_clear(bkr_u32* dst, bkr_u32 count) {
    bkr_u32 i;
    for (i = 0; i < count; i++) {
        dst[i] = 0;
    }
}

void bkr_words_copy(bkr_u32* dst, const bkr_u32* src, bkr_u32 count) {
    bkr_u32 i;
    for (i = 0; i < count; i++) {
        dst[i] = src[i];
    }
}

bkr_u32 bkr_strlen(const char* s) {
    bkr_u32 n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

int bkr_streq(const char* a, const char* b) {
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}
```

- [ ] **Passo 5: Ver passar**

**Rodar:**

```bash
make -C core test
```

**Esperado:**

```text
ok   tables_consistent
ok   lookup_tables
2 testes, 2850 verificações, 0 falhas
```

- [ ] **Passo 6: Commit**

**Rodar:**

```bash
git add core/Makefile core/tests/bkr_test.h core/tests/test_main.c core/tests/test_tables.c core/include/bkr/bkr_types.h core/include/bkr/bkr_ids.h core/include/bkr/bkr_world.h core/src/bkr_util.c
git commit -q -m "adiciona tipos do núcleo, executor de testes e testes das tabelas"
```

### Task 8: Texto e hash sem libc

**Arquivos:**
- Criar: `core/tests/test_text_hash.c`
- Criar: `core/include/bkr/bkr_hash.h`, `core/src/bkr_hash.c`, `core/include/bkr/bkr_text.h`, `core/src/bkr_text.c`

O FNV-1a de 32 bits nomeia saves e alimenta a impressão digital; `bkr_fnv1a_u32` mistura os bytes por deslocamento, então o resultado é igual no PC (little-endian) e no MIPS (big-endian). O construtor de texto substitui `printf` dentro do jogo.

- [ ] **Passo 1: Escrever os testes**

Arquivo `core/tests/test_text_hash.c` (criar):

```c
/* test_text_hash.c — Utilidades sem libc: strings, palavras, texto e FNV-1a. */
#include "bkr_test.h"
#include "bkr/bkr_hash.h"
#include "bkr/bkr_text.h"

void test_strings_and_words(void) {
    bkr_u32 a[3] = { 1u, 2u, 3u };
    bkr_u32 b[3] = { 9u, 9u, 9u };
    BKR_CHECK_EQ(bkr_strlen(""), 0u);
    BKR_CHECK_EQ(bkr_strlen("Banjo"), 5u);
    BKR_CHECK(bkr_streq("Kazooie", "Kazooie"));
    BKR_CHECK(!bkr_streq("Kazooie", "Kazoo"));
    BKR_CHECK(!bkr_streq("", "x"));
    bkr_words_copy(b, a, 3u);
    BKR_CHECK_EQ(b[0] + b[1] + b[2], 6u);
    bkr_words_clear(b, 2u);
    BKR_CHECK_EQ(b[0], 0u);
    BKR_CHECK_EQ(b[1], 0u);
    BKR_CHECK_EQ(b[2], 3u);
    BKR_BIT_SET(a, 40u);
    BKR_CHECK_EQ(BKR_BIT_GET(a, 40u), 1u);
    BKR_CHECK_EQ(a[1], 2u | (1u << 8));
    BKR_BIT_CLR(a, 40u);
    BKR_CHECK_EQ(a[1], 2u);
}

void test_fnv_vectors(void) {
    static const bkr_u8 bytes[3] = { 'f', 'o', 'o' };
    BKR_CHECK_EQ(bkr_fnv1a_str(BKR_FNV_OFFSET, ""), 0x811C9DC5u);
    BKR_CHECK_EQ(bkr_fnv1a_str(BKR_FNV_OFFSET, "a"), 0xE40C292Cu);
    BKR_CHECK_EQ(bkr_fnv1a_str(BKR_FNV_OFFSET, "foobar"), 0xBF9CF968u);
    BKR_CHECK_EQ(bkr_fnv1a_bytes(BKR_FNV_OFFSET, bytes, 3u), bkr_fnv1a_str(BKR_FNV_OFFSET, "foo"));
    /* fnv1a_u32 mistura em little-endian: igual ao FNV dos bytes 01 00 00 00. */
    BKR_CHECK_EQ(bkr_fnv1a_u32(BKR_FNV_OFFSET, 1u), 0xFB69B604u);
}

void test_text_builder(void) {
    char buf[16];
    BkrText t;
    bkr_text_init(&t, buf, sizeof(buf));
    bkr_text_put(&t, "N=");
    bkr_text_u32(&t, 4096u);
    bkr_text_putc(&t, ' ');
    bkr_text_hex(&t, 0xBEEFu, 4);
    BKR_CHECK(bkr_streq(buf, "N=4096 BEEF"));
    BKR_CHECK(!t.overflow);
    bkr_text_put(&t, "estoura o buffer");
    BKR_CHECK(t.overflow);
    BKR_CHECK_EQ(bkr_strlen(buf), 15u);
    bkr_text_init(&t, buf, sizeof(buf));
    bkr_text_u32(&t, 0u);
    bkr_text_putc(&t, '/');
    bkr_text_u32(&t, 4294967295u);
    BKR_CHECK(bkr_streq(buf, "0/4294967295"));
}
```

- [ ] **Passo 2: Ver falhar**

**Rodar:**

```bash
make -C core test
```

**Esperado (falha):**

```text
bkr/bkr_hash.h
```

- [ ] **Passo 3: Implementar**

Arquivo `core/include/bkr/bkr_hash.h` (criar):

```c
/*
 * bkr_hash.h — FNV-1a de 32 bits, usado em impressões digitais de seed,
 * nomes de arquivo de save e checagens de equivalência host/MIPS.
 */
#ifndef BKR_HASH_H
#define BKR_HASH_H

#include "bkr_types.h"

#define BKR_FNV_OFFSET 0x811C9DC5u
#define BKR_FNV_PRIME 0x01000193u

bkr_u32 bkr_fnv1a_bytes(bkr_u32 hash, const bkr_u8* data, bkr_u32 len);
bkr_u32 bkr_fnv1a_str(bkr_u32 hash, const char* text);
bkr_u32 bkr_fnv1a_u32(bkr_u32 hash, bkr_u32 value);

#endif
```

Arquivo `core/src/bkr_hash.c` (criar):

```c
/* bkr_hash.c — FNV-1a de 32 bits (byte a byte, independente de endianness). */
#include "bkr/bkr_hash.h"

bkr_u32 bkr_fnv1a_bytes(bkr_u32 hash, const bkr_u8* data, bkr_u32 len) {
    bkr_u32 i;
    for (i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= BKR_FNV_PRIME;
    }
    return hash;
}

bkr_u32 bkr_fnv1a_str(bkr_u32 hash, const char* text) {
    while (*text != '\0') {
        hash ^= (bkr_u8)*text;
        hash *= BKR_FNV_PRIME;
        text++;
    }
    return hash;
}

/* Mistura os 4 bytes do valor em ordem little-endian, calculados por
 * deslocamento: o resultado é o mesmo no host (LE) e no MIPS (BE). */
bkr_u32 bkr_fnv1a_u32(bkr_u32 hash, bkr_u32 value) {
    int i;
    for (i = 0; i < 4; i++) {
        hash ^= (value >> (i * 8)) & 0xFFu;
        hash *= BKR_FNV_PRIME;
    }
    return hash;
}
```

Arquivo `core/include/bkr/bkr_text.h` (criar):

```c
/*
 * bkr_text.h — Construtor de texto em buffer fixo, sem libc.
 * Sempre mantém o buffer terminado em zero; marca overflow em vez de estourar.
 */
#ifndef BKR_TEXT_H
#define BKR_TEXT_H

#include "bkr_types.h"

typedef struct BkrText {
    char* buf;
    bkr_u32 cap;
    bkr_u32 len;
    int overflow;
} BkrText;

void bkr_text_init(BkrText* t, char* buf, bkr_u32 cap);
void bkr_text_putc(BkrText* t, char c);
void bkr_text_put(BkrText* t, const char* s);
void bkr_text_u32(BkrText* t, bkr_u32 value);
void bkr_text_hex(BkrText* t, bkr_u32 value, int digits);

#endif
```

Arquivo `core/src/bkr_text.c` (criar):

```c
/* bkr_text.c — Construtor de texto em buffer fixo (sem printf/libc). */
#include "bkr/bkr_text.h"

void bkr_text_init(BkrText* t, char* buf, bkr_u32 cap) {
    t->buf = buf;
    t->cap = cap;
    t->len = 0;
    t->overflow = 0;
    if (cap > 0) {
        buf[0] = '\0';
    }
}

void bkr_text_putc(BkrText* t, char c) {
    if (t->len + 1 < t->cap) {
        t->buf[t->len] = c;
        t->len++;
        t->buf[t->len] = '\0';
    } else {
        t->overflow = 1;
    }
}

void bkr_text_put(BkrText* t, const char* s) {
    while (*s != '\0') {
        bkr_text_putc(t, *s);
        s++;
    }
}

void bkr_text_u32(BkrText* t, bkr_u32 value) {
    char digits[10];
    int n = 0;
    do {
        digits[n] = (char)('0' + (value % 10u));
        n++;
        value /= 10u;
    } while (value != 0);
    while (n > 0) {
        n--;
        bkr_text_putc(t, digits[n]);
    }
}

void bkr_text_hex(BkrText* t, bkr_u32 value, int digits) {
    static const char hex[] = "0123456789ABCDEF";
    int shift;
    for (shift = (digits - 1) * 4; shift >= 0; shift -= 4) {
        bkr_text_putc(t, hex[(value >> shift) & 0xFu]);
    }
}
```

- [ ] **Passo 4: Ver passar**

**Rodar:**

```bash
make -C core test
```

**Esperado:**

```text
ok   strings_and_words
ok   fnv_vectors
ok   text_builder
5 testes, 2872 verificações, 0 falhas
```

- [ ] **Passo 5: Commit**

**Rodar:**

```bash
git add core/tests/test_text_hash.c core/include/bkr/bkr_hash.h core/src/bkr_hash.c core/include/bkr/bkr_text.h core/src/bkr_text.c
git commit -q -m "adiciona FNV-1a e construtor de texto do núcleo"
```

### Task 9: Gerador pseudoaleatório (com referência independente)

**Arquivos:**
- Criar: `tools/tests/test_rng_reference.py`
- Criar: `core/tests/test_rng.c`, `core/include/bkr/bkr_rng.h`, `core/src/bkr_rng.c`

xoshiro128** semeado por splitmix32: só operações de 32 bits, mesma sequência em qualquer CPU. A referência em Python reimplementa RNG, FNV, bloco de opções e impressão digital de forma independente; os valores-ouro dos testes C saem dela.

- [ ] **Passo 1: Referência em Python** (passa já: ela é o oráculo dos valores)

Arquivo `tools/tests/test_rng_reference.py` (criar):

```python
"""Implementação de referência (Python) do RNG, do FNV-1a, do código de
compartilhamento e da impressão digital do seed. Os valores fixos daqui são os
mesmos dos testes C em core/tests/: se um lado mudar, os dois testes acusam.

Rodar:  python3 -m unittest discover -s tools/tests -v
"""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MASK = 0xFFFFFFFF
BASE32 = "0123456789ABCDEFGHJKMNPQRSTVWXYZ"
GENERATOR_VERSION = 1  # BKR_GENERATOR_VERSION em core/include/bkr/bkr_settings.h


def splitmix32(state):
    """Retorna (novo_estado, valor), como bkr_splitmix32."""
    state = (state + 0x9E3779B9) & MASK
    z = state
    z = ((z ^ (z >> 16)) * 0x85EBCA6B) & MASK
    z = ((z ^ (z >> 13)) * 0xC2B2AE35) & MASK
    return state, z ^ (z >> 16)


def rotl(x, k):
    return ((x << k) | (x >> (32 - k))) & MASK


class Xoshiro128:
    """xoshiro128** semeado por splitmix32, como bkr_rng_seed/bkr_rng_next."""

    def __init__(self, seed):
        x = seed
        self.s = []
        for _ in range(4):
            x, v = splitmix32(x)
            self.s.append(v)
        if not any(self.s):
            self.s[0] = 1

    def next(self):
        s = self.s
        result = (rotl((s[1] * 5) & MASK, 7) * 9) & MASK
        t = (s[1] << 9) & MASK
        s[2] ^= s[0]
        s[3] ^= s[1]
        s[1] ^= s[2]
        s[0] ^= s[3]
        s[2] ^= t
        s[3] = rotl(s[3], 11)
        return result


def fnv1a(data, h=0x811C9DC5):
    for b in data:
        h ^= b
        h = (h * 0x01000193) & MASK
    return h


def fnv1a_u32(value, h):
    """Mistura os 4 bytes em little-endian, como bkr_fnv1a_u32."""
    return fnv1a(bytes([(value >> (8 * i)) & 0xFF for i in range(4)]), h)


def settings_block(bits):
    """6 caracteres base32: 24 bits de opções + 6 bits de checagem."""
    check = fnv1a_u32(bits, 0x811C9DC5) & 0x3F
    value = (bits & 0xFFFFFF) | (check << 24)
    out = []
    for _ in range(6):
        out.append(BASE32[value & 31])
        value >>= 5
    return "".join(reversed(out))


def fingerprint(bits, seed, data_hash):
    """Como bkr_seed_fingerprint: FNV de "BKR|", versão, dados, bloco, "|", semente."""
    h = fnv1a(b"BKR|")
    h = fnv1a_u32(GENERATOR_VERSION, h)
    h = fnv1a_u32(data_hash, h)
    h = fnv1a(settings_block(bits).encode("ascii"), h)
    h = fnv1a(b"|", h)
    return fnv1a(seed.encode("ascii"), h)


# logic=glitchless(0) | shuffle=mixed(2)<<1 | scope=game(0)<<3 | moves=molehills(1)<<4 | access=all(0)<<5
DEFAULT_SETTINGS_BITS = 0 | (2 << 1) | (0 << 3) | (1 << 4) | (0 << 5)


class ReferenceTest(unittest.TestCase):
    def test_fnv_vectors(self):
        self.assertEqual(fnv1a(b""), 0x811C9DC5)
        self.assertEqual(fnv1a(b"a"), 0xE40C292C)
        self.assertEqual(fnv1a(b"foobar"), 0xBF9CF968)
        self.assertEqual(fnv1a_u32(1, 0x811C9DC5), 0xFB69B604)

    def test_splitmix_golden(self):
        x, a = splitmix32(0)
        x, b = splitmix32(x)
        self.assertEqual((a, b), (0x92CA2F0E, 0x3CD6E3F3))

    def test_xoshiro_golden(self):
        rng = Xoshiro128(12345)
        self.assertEqual([rng.next() for _ in range(3)], [0x1EEA3CC1, 0x1A40A62E, 0xFC4CD240])

    def test_share_block_golden(self):
        self.assertEqual(DEFAULT_SETTINGS_BITS, 20)
        self.assertEqual(settings_block(DEFAULT_SETTINGS_BITS), "GG000M")

    def test_fingerprint_golden(self):
        header = (ROOT / "core" / "generated" / "bkr_logic_data.h").read_text(encoding="utf-8")
        data_hash = int(re.search(r"BKR_LOGIC_DATA_HASH 0x([0-9A-F]{8})u", header).group(1), 16)
        self.assertEqual(data_hash, 0x5C82A342)
        self.assertEqual(fingerprint(DEFAULT_SETTINGS_BITS, "BANJO", data_hash), 0x623A1517)


if __name__ == "__main__":
    unittest.main()
```

**Rodar:**

```bash
python3 -m unittest tools/tests/test_rng_reference.py -v
```

**Esperado:**

```text
Ran 5 tests
OK
```

- [ ] **Passo 2: Escrever os testes C**

Arquivo `core/tests/test_rng.c` (criar):

```c
/* test_rng.c — xoshiro128** + splitmix32: valores-ouro, limites e embaralhamento. */
#include "bkr_test.h"
#include "bkr/bkr_rng.h"

void test_rng_reference(void) {
    /* Valores conferidos com a implementação Python em tools/tests/test_rng_reference.py. */
    BkrRng rng;
    bkr_u32 x = 0;
    BKR_CHECK_EQ(bkr_splitmix32(&x), 0x92CA2F0Eu);
    BKR_CHECK_EQ(bkr_splitmix32(&x), 0x3CD6E3F3u);
    bkr_rng_seed(&rng, 12345u);
    BKR_CHECK_EQ(bkr_rng_next(&rng), 0x1EEA3CC1u);
    BKR_CHECK_EQ(bkr_rng_next(&rng), 0x1A40A62Eu);
    BKR_CHECK_EQ(bkr_rng_next(&rng), 0xFC4CD240u);
}

void test_rng_below_bounds(void) {
    BkrRng rng;
    bkr_u32 i;
    bkr_u32 seen[7] = { 0, 0, 0, 0, 0, 0, 0 };
    bkr_rng_seed(&rng, 7u);
    for (i = 0; i < 7000u; i++) {
        bkr_u32 v = bkr_rng_below(&rng, 7u);
        BKR_CHECK(v < 7u);
        seen[v]++;
    }
    for (i = 0; i < 7u; i++) {
        BKR_CHECK(seen[i] > 800u && seen[i] < 1200u);
    }
    BKR_CHECK_EQ(bkr_rng_below(&rng, 1u), 0u);
}

void test_shuffle_is_permutation(void) {
    BkrRng rng;
    bkr_u16 v[50];
    bkr_u32 count[50];
    bkr_u32 i;
    for (i = 0; i < 50u; i++) {
        v[i] = (bkr_u16)i;
        count[i] = 0;
    }
    bkr_rng_seed(&rng, 99u);
    bkr_rng_shuffle_u16(&rng, v, 50u);
    for (i = 0; i < 50u; i++) {
        BKR_CHECK(v[i] < 50u);
        count[v[i]]++;
    }
    for (i = 0; i < 50u; i++) {
        BKR_CHECK_EQ(count[i], 1u);
    }
    BKR_CHECK(v[0] != 0 || v[1] != 1 || v[2] != 2);
}
```

- [ ] **Passo 3: Ver falhar**

**Rodar:**

```bash
make -C core test
```

**Esperado (falha):**

```text
bkr/bkr_rng.h
```

- [ ] **Passo 4: Implementar**

Arquivo `core/include/bkr/bkr_rng.h` (criar):

```c
/*
 * bkr_rng.h — Gerador pseudoaleatório determinístico (xoshiro128**),
 * semeado por splitmix32. Só usa operações de 32 bits, logo produz a mesma
 * sequência no host e no MIPS.
 */
#ifndef BKR_RNG_H
#define BKR_RNG_H

#include "bkr_types.h"

typedef struct BkrRng {
    bkr_u32 s[4];
} BkrRng;

/* Avança *x e retorna o próximo valor de splitmix32. */
bkr_u32 bkr_splitmix32(bkr_u32* x);
/* Inicializa o estado a partir de uma semente de 32 bits. */
void bkr_rng_seed(BkrRng* rng, bkr_u32 seed);
/* Próximo valor de 32 bits. */
bkr_u32 bkr_rng_next(BkrRng* rng);
/* Inteiro uniforme em [0, n) sem viés (n > 0). */
bkr_u32 bkr_rng_below(BkrRng* rng, bkr_u32 n);
/* Embaralha um array de bkr_u16 (Fisher–Yates). */
void bkr_rng_shuffle_u16(BkrRng* rng, bkr_u16* values, bkr_u32 count);

#endif
```

Arquivo `core/src/bkr_rng.c` (criar):

```c
/* bkr_rng.c — xoshiro128** (Blackman & Vigna) semeado por splitmix32. */
#include "bkr/bkr_rng.h"

static bkr_u32 bkr_rotl(bkr_u32 x, int k) {
    return (x << k) | (x >> (32 - k));
}

bkr_u32 bkr_splitmix32(bkr_u32* x) {
    bkr_u32 z;
    *x += 0x9E3779B9u;
    z = *x;
    z = (z ^ (z >> 16)) * 0x85EBCA6Bu;
    z = (z ^ (z >> 13)) * 0xC2B2AE35u;
    return z ^ (z >> 16);
}

void bkr_rng_seed(BkrRng* rng, bkr_u32 seed) {
    bkr_u32 x = seed;
    int i;
    for (i = 0; i < 4; i++) {
        rng->s[i] = bkr_splitmix32(&x);
    }
    if ((rng->s[0] | rng->s[1] | rng->s[2] | rng->s[3]) == 0) {
        rng->s[0] = 1;
    }
}

bkr_u32 bkr_rng_next(BkrRng* rng) {
    bkr_u32* s = rng->s;
    bkr_u32 result = bkr_rotl(s[1] * 5u, 7) * 9u;
    bkr_u32 t = s[1] << 9;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= t;
    s[3] = bkr_rotl(s[3], 11);
    return result;
}

bkr_u32 bkr_rng_below(BkrRng* rng, bkr_u32 n) {
    bkr_u32 threshold = (0u - n) % n;
    for (;;) {
        bkr_u32 r = bkr_rng_next(rng);
        if (r >= threshold) {
            return r % n;
        }
    }
}

void bkr_rng_shuffle_u16(BkrRng* rng, bkr_u16* values, bkr_u32 count) {
    bkr_u32 i;
    if (count < 2) {
        return;
    }
    for (i = count - 1; i > 0; i--) {
        bkr_u32 j = bkr_rng_below(rng, i + 1);
        bkr_u16 tmp = values[i];
        values[i] = values[j];
        values[j] = tmp;
    }
}
```

- [ ] **Passo 5: Ver passar**

**Rodar:**

```bash
make -C core test
```

**Esperado:**

```text
ok   rng_reference
ok   rng_below_bounds
ok   shuffle_is_permutation
8 testes, 9986 verificações, 0 falhas
```

- [ ] **Passo 6: Commit**

**Rodar:**

```bash
git add tools/tests/test_rng_reference.py core/tests/test_rng.c core/include/bkr/bkr_rng.h core/src/bkr_rng.c
git commit -q -m "adiciona RNG xoshiro128** com referência em Python"
```

### Task 10: Nomes de itens, mundos e palavras do hash

**Arquivos:**
- Criar: `core/tests/test_names.c`, `core/src/bkr_names.c`

Os textos não têm acento de propósito (fonte e console). As 32 palavras do hash são nomes de personagens e lugares do jogo; 5 palavras × 5 bits identificam o seed na tela.

- [ ] **Passo 1: Escrever os testes**

Arquivo `core/tests/test_names.c` (criar):

```c
/* test_names.c — Nomes de itens e mundos (PT-BR/EN) e palavras do hash. */
#include "bkr_test.h"
#include "bkr/bkr_ids.h"

void test_item_names(void) {
    BKR_CHECK(bkr_streq(bkr_item_name(BKR_ITEM_JIGGY, BKR_LANG_PT_BR), "Jiggy"));
    BKR_CHECK(bkr_streq(bkr_item_name(BKR_ITEM_HONEYCOMB, BKR_LANG_PT_BR), "Favo de Mel Vazio"));
    BKR_CHECK(bkr_streq(bkr_item_name(BKR_ITEM_HONEYCOMB, BKR_LANG_EN), "Empty Honeycomb"));
    BKR_CHECK(bkr_streq(bkr_item_name(BKR_ITEM_TOKEN, BKR_LANG_EN), "Mumbo Token"));
    BKR_CHECK(bkr_streq(bkr_item_name(BKR_ITEM_MOVE(BKR_ABILITY_TALON_TROT), BKR_LANG_PT_BR), "Trote de Garras"));
    BKR_CHECK(bkr_streq(bkr_item_name(BKR_ITEM_MOVE(BKR_ABILITY_BEAK_BUSTER), BKR_LANG_EN), "Beak Buster"));
    BKR_CHECK(bkr_streq(bkr_item_name(BKR_ITEM_NONE, BKR_LANG_PT_BR), "Nada"));
    BKR_CHECK(bkr_streq(bkr_item_name(0x7F, BKR_LANG_EN), "Nothing"));
    BKR_CHECK(BKR_ITEM_IS_MOVE(BKR_ITEM_MOVE(BKR_ABILITY_NOTE_DOORS)));
    BKR_CHECK(!BKR_ITEM_IS_MOVE(BKR_ITEM_MOVE_BASE + BKR_ABILITY_COUNT));
}

void test_world_names(void) {
    BKR_CHECK(bkr_streq(bkr_world_name(BKR_WORLD_LAIR, BKR_LANG_PT_BR), "Covil de Gruntilda"));
    BKR_CHECK(bkr_streq(bkr_world_name(BKR_WORLD_LAIR, BKR_LANG_EN), "Gruntilda's Lair"));
    BKR_CHECK(bkr_streq(bkr_world_name(BKR_WORLD_MMM, BKR_LANG_EN), "Mad Monster Mansion"));
    BKR_CHECK(bkr_streq(bkr_world_name(200, BKR_LANG_PT_BR), "Nenhum"));
}

void test_hash_words(void) {
    BKR_CHECK(bkr_streq(bkr_hash_word(0u), "Banjo"));
    BKR_CHECK(bkr_streq(bkr_hash_word(31u), "Dingpot"));
    BKR_CHECK(bkr_streq(bkr_hash_word(32u), "Banjo"));
}
```

- [ ] **Passo 2: Ver falhar** (os protótipos já estão em `bkr_ids.h`; falta a implementação, então o erro é de link)

**Rodar:**

```bash
make -C core test
```

**Esperado (falha):**

```text
bkr_item_name
```

- [ ] **Passo 3: Implementar**

Arquivo `core/src/bkr_names.c` (criar):

```c
/* bkr_names.c — Nomes de itens, mundos e palavras do hash (PT-BR / EN). */
#include "bkr/bkr_ids.h"

static const char* const bkr_move_names_pt[BKR_ABILITY_COUNT] = {
    "Investida de Bico", "Bico-Bomba", "Bico-Martelo", "Controle de Camera",
    "Patada", "Escalar", "Disparo de Ovos", "Bater de Penas", "Salto Mortal",
    "Voo", "Salto Alto", "Bicadas Rapidas", "Rolamento", "Salto de Mola",
    "Botas Pernaltas", "Mergulho", "Trote de Garras", "Tenis Turbo",
    "Asa Maravilha", "Portas de Notas"
};

static const char* const bkr_move_names_en[BKR_ABILITY_COUNT] = {
    "Beak Barge", "Beak Bomb", "Beak Buster", "Camera Control", "Claw Swipe",
    "Climb", "Egg Firing", "Feathery Flap", "Flap Flip", "Flight",
    "High Jump", "Rat-a-tat Rap", "Roll", "Shock Spring Jump", "Wading Boots",
    "Dive", "Talon Trot", "Turbo Talon Trot", "Wonderwing", "Note Doors"
};

static const char* const bkr_world_names[2][BKR_WORLD_COUNT] = {
    { "Nenhum", "Mumbo's Mountain", "Treasure Trove Cove", "Clanker's Cavern",
      "Bubblegloop Swamp", "Freezeezy Peak", "Covil de Gruntilda", "Gobi's Valley",
      "Click Clock Wood", "Rusty Bucket Bay", "Mad Monster Mansion", "Spiral Mountain" },
    { "None", "Mumbo's Mountain", "Treasure Trove Cove", "Clanker's Cavern",
      "Bubblegloop Swamp", "Freezeezy Peak", "Gruntilda's Lair", "Gobi's Valley",
      "Click Clock Wood", "Rusty Bucket Bay", "Mad Monster Mansion", "Spiral Mountain" }
};

static const char* const bkr_hash_words[32] = {
    "Banjo", "Kazooie", "Mumbo", "Bottles", "Tooty", "Grunty", "Klungo", "Brentilda",
    "Cheato", "Jinjo", "Jiggy", "Nota", "Favo", "Token", "Conga", "Chimpy",
    "Clanker", "Gobi", "Boggy", "Wozza", "Tiptup", "Nipper", "Leaky", "Blubber",
    "Snacker", "Tumblar", "Loggo", "Napper", "Nabnut", "Eyrie", "Gnawty", "Dingpot"
};

const char* bkr_item_name(bkr_u8 item, int lang) {
    int en = (lang == BKR_LANG_EN);
    if (BKR_ITEM_IS_MOVE(item)) {
        return en ? bkr_move_names_en[BKR_ITEM_ABILITY(item)] : bkr_move_names_pt[BKR_ITEM_ABILITY(item)];
    }
    switch (item) {
        case BKR_ITEM_JIGGY:
            return "Jiggy";
        case BKR_ITEM_HONEYCOMB:
            return en ? "Empty Honeycomb" : "Favo de Mel Vazio";
        case BKR_ITEM_TOKEN:
            return en ? "Mumbo Token" : "Token do Mumbo";
        default:
            return en ? "Nothing" : "Nada";
    }
}

const char* bkr_world_name(bkr_u8 world, int lang) {
    if (world >= BKR_WORLD_COUNT) {
        world = BKR_WORLD_NONE;
    }
    return bkr_world_names[lang == BKR_LANG_EN ? 1 : 0][world];
}

const char* bkr_hash_word(bkr_u32 index) {
    return bkr_hash_words[index & 31u];
}
```

- [ ] **Passo 4: Ver passar**

**Rodar:**

```bash
make -C core test
```

**Esperado:**

```text
ok   item_names
ok   world_names
ok   hash_words
11 testes, 10003 verificações, 0 falhas
```

- [ ] **Passo 5: Commit**

**Rodar:**

```bash
git add core/tests/test_names.c core/src/bkr_names.c
git commit -q -m "adiciona nomes de itens, mundos e palavras do hash"
```

### Task 11: Opções, código de compartilhamento e impressão digital

**Arquivos:**
- Criar: `core/tests/test_settings.c`, `core/include/bkr/bkr_settings.h`, `core/src/bkr_settings.c`

`BKR_GENERATOR_VERSION` entra na impressão digital: subir a versão muda todos os seeds e todos os nomes de save, o que impede carregar um save com outro placement.

- [ ] **Passo 1: Escrever os testes**

Arquivo `core/tests/test_settings.c` (criar):

```c
/* test_settings.c — Opções, semente, código de compartilhamento e impressão digital. */
#include "bkr_test.h"
#include "bkr/bkr_settings.h"

void test_seed_sanitize(void) {
    char out[BKR_SEED_MAX + 1];
    BKR_CHECK_EQ(bkr_seed_sanitize("banjo kazooie!", out), 13u);
    BKR_CHECK(bkr_streq(out, "BANJO_KAZOOIE"));
    BKR_CHECK_EQ(bkr_seed_sanitize("@@@", out), 0u);
    BKR_CHECK(bkr_streq(out, ""));
    BKR_CHECK_EQ(bkr_seed_sanitize("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123", out), BKR_SEED_MAX);
    BKR_CHECK(bkr_streq(out, "ABCDEFGHIJKLMNOPQRSTUVWX"));
}

void test_settings_roundtrip(void) {
    BkrSettings s;
    BkrSettings back;
    bkr_u32 bits;
    int ok_count = 0;
    for (bits = 0; bits < 64u; bits++) {
        if (bkr_settings_unpack(bits, &s)) {
            ok_count++;
            BKR_CHECK_EQ(bkr_settings_pack(&s), bits);
            BKR_CHECK(bkr_settings_unpack(bkr_settings_pack(&s), &back));
            BKR_CHECK_EQ(back.shuffle, s.shuffle);
        }
    }
    /* 2 lógicas x 3 embaralhamentos x 2 escopos x 2 modos de movimento x 2 acessos. */
    BKR_CHECK_EQ(ok_count, 48);
    BKR_CHECK(!bkr_settings_unpack(64u, &s));
    bkr_settings_default(&s);
    BKR_CHECK(bkr_settings_valid(&s));
    BKR_CHECK_EQ(bkr_settings_pack(&s), 20u);
}

void test_share_code(void) {
    BkrSeedSpec spec;
    BkrSeedSpec back;
    char code[BKR_SHARE_CODE_MAX];
    bkr_settings_default(&spec.settings);
    bkr_seed_sanitize("Banjo", spec.seed);
    BKR_CHECK(bkr_share_code_format(&spec, code, sizeof(code)));
    BKR_CHECK(bkr_streq(code, "BKR1-GG000M-BANJO"));
    BKR_CHECK(bkr_share_code_parse(code, &back));
    BKR_CHECK(bkr_streq(back.seed, "BANJO"));
    BKR_CHECK_EQ(bkr_settings_pack(&back.settings), bkr_settings_pack(&spec.settings));
    BKR_CHECK(bkr_share_code_parse("bkr1-gg000m-banjo", &back));
    BKR_CHECK(bkr_share_code_parse("BKR1-GGOOOM-BANJO", &back)); /* O lido como 0 */
    BKR_CHECK(!bkr_share_code_parse("BKR2-GG000M-BANJO", &back));
    BKR_CHECK(!bkr_share_code_parse("BKR1-000000-BANJO", &back));
    BKR_CHECK(!bkr_share_code_parse("BKR1-GG000M-", &back));
    BKR_CHECK(!bkr_share_code_parse("BKR1-GG000MBANJO", &back));
    BKR_CHECK(!bkr_share_code_parse("BKR1", &back));
}

void test_fingerprint_changes(void) {
    BkrSeedSpec a;
    BkrSeedSpec b;
    bkr_settings_default(&a.settings);
    bkr_settings_default(&b.settings);
    bkr_seed_sanitize("BANJO", a.seed);
    bkr_seed_sanitize("BANJO", b.seed);
    BKR_CHECK_EQ(bkr_seed_fingerprint(&a), bkr_seed_fingerprint(&b));
    b.settings.scope = BKR_SCOPE_WORLD;
    BKR_CHECK(bkr_seed_fingerprint(&a) != bkr_seed_fingerprint(&b));
    bkr_settings_default(&b.settings);
    bkr_seed_sanitize("BANJO2", b.seed);
    BKR_CHECK(bkr_seed_fingerprint(&a) != bkr_seed_fingerprint(&b));
    /* Valor-ouro da semente BANJO com as opções padrão (vira o nome do save). */
    BKR_CHECK_EQ(bkr_seed_fingerprint(&a), 0x623A1517u);
}
```

- [ ] **Passo 2: Ver falhar**

**Rodar:**

```bash
make -C core test
```

**Esperado (falha):**

```text
bkr/bkr_settings.h
```

- [ ] **Passo 3: Implementar**

Arquivo `core/include/bkr/bkr_settings.h` (criar):

```c
/*
 * bkr_settings.h — Opções do randomizer, código de compartilhamento e
 * impressão digital do seed.
 *
 * Código de compartilhamento: "BKR1-XXXXXX-SEMENTE"
 *   - XXXXXX: 30 bits em base32 Crockford (24 bits de opções + 6 de checagem);
 *   - SEMENTE: [A-Z0-9_], 1..24 caracteres.
 */
#ifndef BKR_SETTINGS_H
#define BKR_SETTINGS_H

#include "bkr_types.h"

/* Versão do gerador. Incremente SEMPRE que o algoritmo de geração mudar. */
#define BKR_GENERATOR_VERSION 1u

enum { BKR_LOGIC_GLITCHLESS = 0, BKR_LOGIC_NONE = 1 };
enum { BKR_SHUFFLE_VANILLA = 0, BKR_SHUFFLE_BY_TYPE = 1, BKR_SHUFFLE_MIXED = 2 };
enum { BKR_SCOPE_GAME = 0, BKR_SCOPE_WORLD = 1 };
enum { BKR_MOVES_VANILLA = 0, BKR_MOVES_MOLEHILLS = 1 };
enum { BKR_ACCESS_ALL = 0, BKR_ACCESS_BEATABLE = 1 };

typedef struct BkrSettings {
    bkr_u8 logic;    /* BKR_LOGIC_* */
    bkr_u8 shuffle;  /* BKR_SHUFFLE_* (jiggies, favos e tokens) */
    bkr_u8 scope;    /* BKR_SCOPE_* */
    bkr_u8 moves;    /* BKR_MOVES_* */
    bkr_u8 access;   /* BKR_ACCESS_* */
    bkr_u8 pad[3];
} BkrSettings;

#define BKR_SEED_MAX 24u
#define BKR_SHARE_CODE_MAX 40u

typedef struct BkrSeedSpec {
    BkrSettings settings;
    char seed[BKR_SEED_MAX + 1];
} BkrSeedSpec;

void bkr_settings_default(BkrSettings* s);
int bkr_settings_valid(const BkrSettings* s);
bkr_u32 bkr_settings_pack(const BkrSettings* s);
int bkr_settings_unpack(bkr_u32 bits, BkrSettings* out);

/* Normaliza texto livre para semente: maiúsculas, espaço vira '_', descarta
 * o resto, corta em BKR_SEED_MAX. Retorna o tamanho (0 = inválida). */
bkr_u32 bkr_seed_sanitize(const char* in, char* out);

/* Formata/lê o código de compartilhamento. Retornam 1 em sucesso. */
int bkr_share_code_format(const BkrSeedSpec* spec, char* out, bkr_u32 cap);
int bkr_share_code_parse(const char* code, BkrSeedSpec* out);

/* Impressão digital: muda com versão do gerador, dados de lógica, opções e semente. */
bkr_u32 bkr_seed_fingerprint(const BkrSeedSpec* spec);

#endif
```

Arquivo `core/src/bkr_settings.c` (criar):

```c
/* bkr_settings.c — Opções, código de compartilhamento e impressão digital. */
#include "bkr/bkr_settings.h"
#include "bkr/bkr_hash.h"
#include "bkr/bkr_text.h"
#include "bkr/bkr_world.h"

static const char bkr_base32[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

void bkr_settings_default(BkrSettings* s) {
    s->logic = BKR_LOGIC_GLITCHLESS;
    s->shuffle = BKR_SHUFFLE_MIXED;
    s->scope = BKR_SCOPE_GAME;
    s->moves = BKR_MOVES_MOLEHILLS;
    s->access = BKR_ACCESS_ALL;
    s->pad[0] = s->pad[1] = s->pad[2] = 0;
}

int bkr_settings_valid(const BkrSettings* s) {
    return s->logic <= BKR_LOGIC_NONE && s->shuffle <= BKR_SHUFFLE_MIXED &&
           s->scope <= BKR_SCOPE_WORLD && s->moves <= BKR_MOVES_MOLEHILLS &&
           s->access <= BKR_ACCESS_BEATABLE;
}

bkr_u32 bkr_settings_pack(const BkrSettings* s) {
    return ((bkr_u32)s->logic) | ((bkr_u32)s->shuffle << 1) | ((bkr_u32)s->scope << 3) |
           ((bkr_u32)s->moves << 4) | ((bkr_u32)s->access << 5);
}

int bkr_settings_unpack(bkr_u32 bits, BkrSettings* out) {
    if ((bits >> 6) != 0) {
        return 0;
    }
    out->logic = (bkr_u8)(bits & 1u);
    out->shuffle = (bkr_u8)((bits >> 1) & 3u);
    out->scope = (bkr_u8)((bits >> 3) & 1u);
    out->moves = (bkr_u8)((bits >> 4) & 1u);
    out->access = (bkr_u8)((bits >> 5) & 1u);
    out->pad[0] = out->pad[1] = out->pad[2] = 0;
    return bkr_settings_valid(out);
}

bkr_u32 bkr_seed_sanitize(const char* in, char* out) {
    bkr_u32 n = 0;
    while (*in != '\0' && n < BKR_SEED_MAX) {
        char c = *in;
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        }
        if (c == ' ') {
            c = '_';
        }
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
            out[n] = c;
            n++;
        }
        in++;
    }
    out[n] = '\0';
    return n;
}

static bkr_u32 bkr_settings_check(bkr_u32 bits) {
    return bkr_fnv1a_u32(BKR_FNV_OFFSET, bits) & 0x3Fu;
}

static int bkr_base32_value(char c) {
    int i;
    if (c >= 'a' && c <= 'z') {
        c = (char)(c - 'a' + 'A');
    }
    if (c == 'O') {
        c = '0';
    }
    if (c == 'I' || c == 'L') {
        c = '1';
    }
    for (i = 0; i < 32; i++) {
        if (bkr_base32[i] == c) {
            return i;
        }
    }
    return -1;
}

static void bkr_settings_block(const BkrSettings* s, char* out6) {
    bkr_u32 bits = bkr_settings_pack(s);
    bkr_u32 value = (bits & 0xFFFFFFu) | (bkr_settings_check(bits) << 24);
    int i;
    for (i = 5; i >= 0; i--) {
        out6[i] = bkr_base32[value & 31u];
        value >>= 5;
    }
    out6[6] = '\0';
}

int bkr_share_code_format(const BkrSeedSpec* spec, char* out, bkr_u32 cap) {
    BkrText t;
    char block[7];
    if (!bkr_settings_valid(&spec->settings) || spec->seed[0] == '\0') {
        return 0;
    }
    bkr_settings_block(&spec->settings, block);
    bkr_text_init(&t, out, cap);
    bkr_text_put(&t, "BKR1-");
    bkr_text_put(&t, block);
    bkr_text_putc(&t, '-');
    bkr_text_put(&t, spec->seed);
    return !t.overflow;
}

int bkr_share_code_parse(const char* code, BkrSeedSpec* out) {
    static const char prefix[] = "BKR1-";
    bkr_u32 value = 0;
    bkr_u32 bits;
    int i;
    for (i = 0; prefix[i] != '\0'; i++) {
        char c = code[i];
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        }
        if (c != prefix[i]) {
            return 0;
        }
    }
    code += 5;
    for (i = 0; i < 6; i++) {
        int v = bkr_base32_value(code[i]);
        if (v < 0) {
            return 0;
        }
        value = (value << 5) | (bkr_u32)v;
    }
    if (code[6] != '-') {
        return 0;
    }
    bits = value & 0xFFFFFFu;
    if ((value >> 24) != bkr_settings_check(bits)) {
        return 0;
    }
    if (!bkr_settings_unpack(bits, &out->settings)) {
        return 0;
    }
    return bkr_seed_sanitize(code + 7, out->seed) > 0;
}

bkr_u32 bkr_seed_fingerprint(const BkrSeedSpec* spec) {
    char block[7];
    bkr_u32 h = BKR_FNV_OFFSET;
    bkr_settings_block(&spec->settings, block);
    h = bkr_fnv1a_str(h, "BKR|");
    h = bkr_fnv1a_u32(h, BKR_GENERATOR_VERSION);
    h = bkr_fnv1a_u32(h, BKR_LOGIC_DATA_HASH);
    h = bkr_fnv1a_str(h, block);
    h = bkr_fnv1a_str(h, "|");
    h = bkr_fnv1a_str(h, spec->seed);
    return h;
}
```

- [ ] **Passo 4: Ver passar**

**Rodar:**

```bash
make -C core test
```

**Esperado:**

```text
ok   seed_sanitize
ok   settings_roundtrip
ok   share_code
ok   fingerprint_changes
15 testes, 10173 verificações, 0 falhas
```

`623A1517` (conferido em `fingerprint_changes`) é o segundo valor-ouro: a impressão digital do seed `BANJO` com as opções padrão.

- [ ] **Passo 5: Commit**

**Rodar:**

```bash
git add core/tests/test_settings.c core/include/bkr/bkr_settings.h core/src/bkr_settings.c
git commit -q -m "adiciona opções, código de compartilhamento e impressão digital"
```

### Task 12: Estado lógico, avaliador de regras e busca

**Arquivos:**
- Criar: `core/tests/test_logic.c`, `core/include/bkr/bkr_logic.h`, `core/src/bkr_logic.c`

O avaliador interpreta o bytecode RPN com uma pilha de booleanos dentro de um único `bkr_u32`. A busca varre as regiões alcançadas até o ponto fixo: abre saídas, dispara eventos, soma grupos de notas e, no modo COLLECT, coleta o item de cada localização alcançada.

- [ ] **Passo 1: Escrever os testes**

Arquivo `core/tests/test_logic.c` (criar):

```c
/* test_logic.c — Estado lógico, avaliador de regras RPN e busca por ponto fixo. */
#include "bkr_test.h"
#include "bkr/bkr_logic.h"
#include "bkr/bkr_rules.h"
#include "bkr/bkr_ids.h"

static BkrSearch t_search;
static BkrState t_state;
static BkrState t_copy;
static BkrCosts t_costs;
static bkr_u8 t_placement[BKR_LOCATION_COUNT];

void test_state_items(void) {
    bkr_state_clear(&t_state);
    bkr_state_add_item(&t_state, BKR_ITEM_JIGGY);
    bkr_state_add_item(&t_state, BKR_ITEM_JIGGY);
    bkr_state_add_item(&t_state, BKR_ITEM_TOKEN);
    bkr_state_add_item(&t_state, BKR_ITEM_HONEYCOMB);
    bkr_state_add_item(&t_state, BKR_ITEM_MOVE(BKR_ABILITY_FLIGHT));
    BKR_CHECK_EQ(t_state.jiggies, 2u);
    BKR_CHECK_EQ(t_state.tokens, 1u);
    BKR_CHECK_EQ(t_state.honeycombs, 1u);
    BKR_CHECK_EQ(t_state.moves, 1u << BKR_ABILITY_FLIGHT);
    bkr_state_copy(&t_copy, &t_state);
    bkr_state_remove_item(&t_state, BKR_ITEM_MOVE(BKR_ABILITY_FLIGHT));
    bkr_state_remove_item(&t_state, BKR_ITEM_TOKEN);
    bkr_state_remove_item(&t_state, BKR_ITEM_TOKEN); /* não fica negativo */
    BKR_CHECK_EQ(t_state.moves, 0u);
    BKR_CHECK_EQ(t_state.tokens, 0u);
    BKR_CHECK_EQ(t_copy.tokens, 1u);
    BKR_CHECK_EQ(t_copy.moves, 1u << BKR_ABILITY_FLIGHT);
    bkr_state_base(&t_state);
    BKR_CHECK_EQ(t_state.moves, BKR_STARTING_MOVES);
    BKR_CHECK(t_state.moves & (1u << BKR_ABILITY_CLIMB));
    BKR_CHECK(!(t_state.moves & (1u << BKR_ABILITY_EGGS)));
}

void test_rule_eval_handmade(void) {
    static const bkr_u16 eggs_and_flip[] = { BKR_OP_HAS, BKR_ABILITY_EGGS, BKR_OP_HAS, BKR_ABILITY_FLAP_FLIP, BKR_OP_AND, BKR_OP_END };
    static const bkr_u16 not_talon_or_tokens[] = { BKR_OP_HAS, BKR_ABILITY_TALON_TROT, BKR_OP_NOT, BKR_OP_TOKENS, 10, BKR_OP_OR, BKR_OP_END };
    static const bkr_u16 world_ccw[] = { BKR_OP_WORLD_COST, 8, BKR_OP_END };
    static const bkr_u16 door_180[] = { BKR_OP_NOTE_DOOR, 1, BKR_OP_END };
    static const bkr_u16 bee[] = { BKR_OP_TRANSFORM_COST, BKR_TRANSFORM_BEE, BKR_OP_END };
    static const bkr_u16 empty_rule[] = { BKR_OP_END };
    static const bkr_u16 bad_opcode[] = { 99, BKR_OP_END };
    bkr_costs_vanilla(&t_costs);
    bkr_state_clear(&t_state);
    BKR_CHECK(bkr_rule_eval_code(empty_rule, &t_state, &t_costs));
    BKR_CHECK(!bkr_rule_eval_code(bad_opcode, &t_state, &t_costs));
    BKR_CHECK(!bkr_rule_eval_code(eggs_and_flip, &t_state, &t_costs));
    bkr_state_add_item(&t_state, BKR_ITEM_MOVE(BKR_ABILITY_EGGS));
    BKR_CHECK(!bkr_rule_eval_code(eggs_and_flip, &t_state, &t_costs));
    bkr_state_add_item(&t_state, BKR_ITEM_MOVE(BKR_ABILITY_FLAP_FLIP));
    BKR_CHECK(bkr_rule_eval_code(eggs_and_flip, &t_state, &t_costs));
    BKR_CHECK(bkr_rule_eval_code(not_talon_or_tokens, &t_state, &t_costs));
    bkr_state_add_item(&t_state, BKR_ITEM_MOVE(BKR_ABILITY_TALON_TROT));
    BKR_CHECK(!bkr_rule_eval_code(not_talon_or_tokens, &t_state, &t_costs));
    t_state.tokens = 10;
    BKR_CHECK(bkr_rule_eval_code(not_talon_or_tokens, &t_state, &t_costs));
    /* Slot 8 = CCW: 1+2+5+7+8+9+10+12+15 = 69 jiggies acumulados. */
    t_state.jiggies = 68;
    BKR_CHECK(!bkr_rule_eval_code(world_ccw, &t_state, &t_costs));
    t_state.jiggies = 69;
    BKR_CHECK(bkr_rule_eval_code(world_ccw, &t_state, &t_costs));
    t_state.notes = 179;
    BKR_CHECK(!bkr_rule_eval_code(door_180, &t_state, &t_costs));
    t_state.notes = 180;
    BKR_CHECK(bkr_rule_eval_code(door_180, &t_state, &t_costs));
    /* Abelha = 5+10+15+20+25 = 75 tokens acumulados. */
    t_state.tokens = 74;
    BKR_CHECK(!bkr_rule_eval_code(bee, &t_state, &t_costs));
    t_state.tokens = 75;
    BKR_CHECK(bkr_rule_eval_code(bee, &t_state, &t_costs));
}

void test_vanilla_placement_beats_game(void) {
    bkr_u32 i;
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        t_placement[i] = bkr_locations[i].vanilla_item;
    }
    bkr_costs_vanilla(&t_costs);
    bkr_state_base(&t_state);
    bkr_search_init(&t_search, &t_state);
    bkr_search_sweep(&t_search, &t_costs, t_placement, BKR_SWEEP_COLLECT);
    BKR_CHECK(BKR_BIT_GET(t_search.st.events, BKR_GOAL_EVENT));
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        BKR_CHECK(BKR_BIT_GET(t_search.loc_done, i));
    }
    BKR_CHECK_EQ(t_search.st.jiggies, 100u);
    BKR_CHECK_EQ(t_search.st.tokens, 116u);
    BKR_CHECK_EQ(t_search.st.honeycombs, 24u);
}

void test_all_items_reach_everything(void) {
    bkr_u32 i;
    bkr_costs_vanilla(&t_costs);
    bkr_state_base(&t_state);
    t_state.moves = 0xFFFFFu;
    t_state.jiggies = 100;
    t_state.tokens = 116;
    bkr_search_init(&t_search, &t_state);
    bkr_search_sweep(&t_search, &t_costs, BKR_NULL, BKR_SWEEP_COLLECT);
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        if (!BKR_BIT_GET(t_search.loc_done, i)) {
            printf("  inalcançável com tudo: %s\n", bkr_location_names[i]);
        }
        BKR_CHECK(BKR_BIT_GET(t_search.loc_done, i));
    }
    BKR_CHECK(BKR_BIT_GET(t_search.st.events, BKR_GOAL_EVENT));
    BKR_CHECK_EQ(t_search.st.notes, 900u);
}

void test_logic_actually_gates(void) {
    bkr_u32 i;
    bkr_u32 reached = 0;
    bkr_costs_vanilla(&t_costs);
    bkr_state_base(&t_state);
    bkr_search_init(&t_search, &t_state);
    bkr_search_sweep(&t_search, &t_costs, BKR_NULL, BKR_SWEEP_COLLECT);
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        reached += BKR_BIT_GET(t_search.loc_done, i);
    }
    /* Sem itens: só Spiral Mountain e o começo do covil. */
    BKR_CHECK(reached > 0u && reached < 40u);
    BKR_CHECK(!BKR_BIT_GET(t_search.st.events, BKR_GOAL_EVENT));
}
```

- [ ] **Passo 2: Ver falhar**

**Rodar:**

```bash
make -C core test
```

**Esperado (falha):**

```text
bkr/bkr_logic.h
```

- [ ] **Passo 3: Implementar**

Arquivo `core/include/bkr/bkr_logic.h` (criar):

```c
/*
 * bkr_logic.h — Estado lógico, avaliação de regras e busca por ponto fixo.
 */
#ifndef BKR_LOGIC_H
#define BKR_LOGIC_H

#include "bkr_types.h"
#include "bkr_world.h"

/* Custos dependentes de configuração (v1 usa os valores vanilla). */
typedef struct BkrCosts {
    bkr_u16 note_door[12];                 /* notas por porta */
    bkr_u16 world_cum[10];                 /* jiggies acumulados por slot de quebra-cabeça */
    bkr_u16 transform_cum[8];              /* tokens acumulados por transformation_e */
} BkrCosts;

typedef struct BkrState {
    bkr_u32 moves;                         /* bit = ability_e */
    bkr_u32 jiggies;
    bkr_u32 tokens;
    bkr_u32 honeycombs;
    bkr_u32 notes;
    bkr_u32 events[BKR_EVENT_WORDS];
    bkr_u32 regions[BKR_REGION_WORDS];
} BkrState;

typedef struct BkrSearch {
    BkrState st;
    bkr_u32 loc_done[BKR_LOC_WORDS];       /* localização alcançada nesta busca */
    bkr_u32 group_done[BKR_GROUP_WORDS];   /* grupo de notas já somado */
    bkr_u32 evdef_done[BKR_EVDEF_WORDS];   /* definição de evento já satisfeita */
} BkrSearch;

enum { BKR_SWEEP_COLLECT = 0, BKR_SWEEP_MARK = 1 };

void bkr_costs_vanilla(BkrCosts* c);

void bkr_state_clear(BkrState* st);
void bkr_state_copy(BkrState* dst, const BkrState* src);
void bkr_state_add_item(BkrState* st, bkr_u8 item);
void bkr_state_remove_item(BkrState* st, bkr_u8 item);
/* Estado inicial: movimentos de Spiral Mountain (BKR_STARTING_MOVES). */
void bkr_state_base(BkrState* st);

int bkr_rule_eval_code(const bkr_u16* code, const BkrState* st, const BkrCosts* costs);
int bkr_rule_eval(bkr_u16 rule, const BkrState* st, const BkrCosts* costs);

/* Começa uma busca a partir de base, com a região inicial alcançada. */
void bkr_search_init(BkrSearch* s, const BkrState* base);
/* Varre até o ponto fixo. Em BKR_SWEEP_COLLECT, o item de cada localização
 * alcançada (placement[i] != BKR_ITEM_NONE) entra no estado; em
 * BKR_SWEEP_MARK, só marca loc_done. placement pode ser BKR_NULL. */
void bkr_search_sweep(BkrSearch* s, const BkrCosts* costs, const bkr_u8* placement, int mode);

#endif
```

Arquivo `core/src/bkr_logic.c` (criar):

```c
/* bkr_logic.c — Estado, avaliação de regras (RPN) e busca por ponto fixo. */
#include "bkr/bkr_logic.h"
#include "bkr/bkr_rules.h"
#include "bkr/bkr_ids.h"

void bkr_costs_vanilla(BkrCosts* c) {
    static const bkr_u16 doors[12] = { 50, 180, 260, 350, 450, 640, 765, 810, 828, 846, 864, 882 };
    static const bkr_u16 puzzle[10] = { 1, 2, 5, 7, 8, 9, 10, 12, 15, 25 };
    bkr_u32 i;
    bkr_u32 acc = 0;
    for (i = 0; i < 12; i++) {
        c->note_door[i] = doors[i];
    }
    for (i = 0; i < 10; i++) {
        acc += puzzle[i];
        c->world_cum[i] = (bkr_u16)acc;
    }
    for (i = 0; i < 8; i++) {
        c->transform_cum[i] = 0xFFFF;
    }
    /* Ordem vanilla dos mundos: MM(5) BGS(10) FP(15) MMM(20) CCW(25). */
    c->transform_cum[BKR_TRANSFORM_TERMITE] = 5;
    c->transform_cum[BKR_TRANSFORM_CROC] = 15;
    c->transform_cum[BKR_TRANSFORM_WALRUS] = 30;
    c->transform_cum[BKR_TRANSFORM_PUMPKIN] = 50;
    c->transform_cum[BKR_TRANSFORM_BEE] = 75;
}

void bkr_state_clear(BkrState* st) {
    st->moves = 0;
    st->jiggies = 0;
    st->tokens = 0;
    st->honeycombs = 0;
    st->notes = 0;
    bkr_words_clear(st->events, BKR_EVENT_WORDS);
    bkr_words_clear(st->regions, BKR_REGION_WORDS);
}

void bkr_state_copy(BkrState* dst, const BkrState* src) {
    dst->moves = src->moves;
    dst->jiggies = src->jiggies;
    dst->tokens = src->tokens;
    dst->honeycombs = src->honeycombs;
    dst->notes = src->notes;
    bkr_words_copy(dst->events, src->events, BKR_EVENT_WORDS);
    bkr_words_copy(dst->regions, src->regions, BKR_REGION_WORDS);
}

void bkr_state_add_item(BkrState* st, bkr_u8 item) {
    if (BKR_ITEM_IS_MOVE(item)) {
        st->moves |= 1u << BKR_ITEM_ABILITY(item);
        return;
    }
    switch (item) {
        case BKR_ITEM_JIGGY:
            st->jiggies++;
            break;
        case BKR_ITEM_TOKEN:
            st->tokens++;
            break;
        case BKR_ITEM_HONEYCOMB:
            st->honeycombs++;
            break;
        default:
            break;
    }
}

void bkr_state_remove_item(BkrState* st, bkr_u8 item) {
    if (BKR_ITEM_IS_MOVE(item)) {
        st->moves &= ~(1u << BKR_ITEM_ABILITY(item));
        return;
    }
    switch (item) {
        case BKR_ITEM_JIGGY:
            if (st->jiggies > 0) {
                st->jiggies--;
            }
            break;
        case BKR_ITEM_TOKEN:
            if (st->tokens > 0) {
                st->tokens--;
            }
            break;
        case BKR_ITEM_HONEYCOMB:
            if (st->honeycombs > 0) {
                st->honeycombs--;
            }
            break;
        default:
            break;
    }
}

void bkr_state_base(BkrState* st) {
    bkr_state_clear(st);
    st->moves = BKR_STARTING_MOVES;
}

/* Pilha de booleanos em um único inteiro: sem arrays na pilha do MIPS. */
#define BKR_PUSH(v) do { stack = (stack << 1) | ((v) ? 1u : 0u); depth++; } while (0)
#define BKR_POP(dst) do { (dst) = stack & 1u; stack >>= 1; depth--; } while (0)

int bkr_rule_eval_code(const bkr_u16* code, const BkrState* st, const BkrCosts* costs) {
    bkr_u32 stack = 0;
    int depth = 0;
    for (;;) {
        bkr_u16 op = *code++;
        bkr_u32 a;
        bkr_u32 b;
        switch (op) {
            case BKR_OP_END:
                return depth > 0 ? (int)(stack & 1u) : 1;
            case BKR_OP_TRUE:
                BKR_PUSH(1);
                break;
            case BKR_OP_FALSE:
                BKR_PUSH(0);
                break;
            case BKR_OP_HAS:
                a = *code++;
                BKR_PUSH((st->moves >> a) & 1u);
                break;
            case BKR_OP_EVENT:
                a = *code++;
                BKR_PUSH(BKR_BIT_GET(st->events, a));
                break;
            case BKR_OP_REGION:
                a = *code++;
                BKR_PUSH(BKR_BIT_GET(st->regions, a));
                break;
            case BKR_OP_NOTE_DOOR:
                a = *code++;
                BKR_PUSH(st->notes >= costs->note_door[a]);
                break;
            case BKR_OP_WORLD_COST:
                a = *code++;
                BKR_PUSH(st->jiggies >= costs->world_cum[a]);
                break;
            case BKR_OP_TRANSFORM_COST:
                a = *code++;
                BKR_PUSH(st->tokens >= costs->transform_cum[a]);
                break;
            case BKR_OP_TOKENS:
                a = *code++;
                BKR_PUSH(st->tokens >= a);
                break;
            case BKR_OP_JIGGIES:
                a = *code++;
                BKR_PUSH(st->jiggies >= a);
                break;
            case BKR_OP_NOTES:
                a = *code++;
                BKR_PUSH(st->notes >= a);
                break;
            case BKR_OP_AND:
                BKR_POP(a);
                BKR_POP(b);
                BKR_PUSH(a & b);
                break;
            case BKR_OP_OR:
                BKR_POP(a);
                BKR_POP(b);
                BKR_PUSH(a | b);
                break;
            case BKR_OP_NOT:
                BKR_POP(a);
                BKR_PUSH(!a);
                break;
            default:
                return 0; /* opcode inválido: regra falsa (nunca acontece com dados gerados) */
        }
    }
}

int bkr_rule_eval(bkr_u16 rule, const BkrState* st, const BkrCosts* costs) {
    return bkr_rule_eval_code(&bkr_rule_code[rule], st, costs);
}

void bkr_search_init(BkrSearch* s, const BkrState* base) {
    bkr_state_copy(&s->st, base);
    bkr_words_clear(s->loc_done, BKR_LOC_WORDS);
    bkr_words_clear(s->group_done, BKR_GROUP_WORDS);
    bkr_words_clear(s->evdef_done, BKR_EVDEF_WORDS);
    BKR_BIT_SET(s->st.regions, BKR_START_REGION);
}

void bkr_search_sweep(BkrSearch* s, const BkrCosts* costs, const bkr_u8* placement, int mode) {
    int changed;
    do {
        bkr_u32 r;
        changed = 0;
        for (r = 0; r < BKR_REGION_COUNT; r++) {
            const BkrRegionDef* reg = &bkr_regions[r];
            bkr_u32 i;
            bkr_u32 end;
            if (!BKR_BIT_GET(s->st.regions, r)) {
                continue;
            }
            end = (bkr_u32)reg->exit_first + reg->exit_count;
            for (i = reg->exit_first; i < end; i++) {
                const BkrExitDef* e = &bkr_exits[i];
                if (!BKR_BIT_GET(s->st.regions, e->to) && bkr_rule_eval(e->rule, &s->st, costs)) {
                    BKR_BIT_SET(s->st.regions, e->to);
                    changed = 1;
                }
            }
            end = (bkr_u32)reg->event_first + reg->event_count;
            for (i = reg->event_first; i < end; i++) {
                const BkrEventDef* ev = &bkr_event_defs[i];
                if (!BKR_BIT_GET(s->evdef_done, i) && bkr_rule_eval(ev->rule, &s->st, costs)) {
                    BKR_BIT_SET(s->evdef_done, i);
                    if (!BKR_BIT_GET(s->st.events, ev->event)) {
                        BKR_BIT_SET(s->st.events, ev->event);
                        changed = 1;
                    }
                }
            }
            end = (bkr_u32)reg->group_first + reg->group_count;
            for (i = reg->group_first; i < end; i++) {
                const BkrNoteGroupDef* g = &bkr_note_groups[i];
                if (!BKR_BIT_GET(s->group_done, i) && bkr_rule_eval(g->rule, &s->st, costs)) {
                    BKR_BIT_SET(s->group_done, i);
                    s->st.notes += g->count;
                    changed = 1;
                }
            }
            end = (bkr_u32)reg->loc_first + reg->loc_count;
            for (i = reg->loc_first; i < end; i++) {
                if (!BKR_BIT_GET(s->loc_done, i) && bkr_rule_eval(bkr_locations[i].rule, &s->st, costs)) {
                    BKR_BIT_SET(s->loc_done, i);
                    if (mode == BKR_SWEEP_COLLECT && placement != BKR_NULL &&
                        placement[i] != BKR_ITEM_NONE) {
                        bkr_state_add_item(&s->st, placement[i]);
                        changed = 1;
                    }
                }
            }
        }
    } while (changed);
}
```

- [ ] **Passo 4: Ver passar**

**Rodar:**

```bash
make -C core test
```

**Esperado:**

```text
ok   state_items
ok   rule_eval_handmade
ok   vanilla_placement_beats_game
ok   all_items_reach_everything
ok   logic_actually_gates
20 testes, 10704 verificações, 0 falhas
```

Se `all_items_reach_everything` falhar, ele imprime as localizações inalcançáveis: isso indica região sem entrada ou evento sem definição na lógica (Task 6).

- [ ] **Passo 5: Commit**

**Rodar:**

```bash
git add core/tests/test_logic.c core/include/bkr/bkr_logic.h core/src/bkr_logic.c
git commit -q -m "adiciona avaliador de regras e busca por ponto fixo"
```

### Task 13: Geração por preenchimento assumido

**Arquivos:**
- Criar: `core/tests/test_generate.c`, `core/include/bkr/bkr_generate.h`, `core/src/bkr_generate.c`

O algoritmo está em A9. Os testes cobrem: determinismo, valor-ouro, restrições de cada opção, balanço (o multiconjunto de itens é preservado), seeds inválidos e propriedades em 500 seeds por preset (`make -C core test BKR_PROPERTY_SEEDS=2000` para a bateria longa).

- [ ] **Passo 1: Escrever os testes**

Arquivo `core/tests/test_generate.c` (criar):

```c
/* test_generate.c — Geração: sucesso, determinismo, restrições e propriedades. */
#include "bkr_test.h"
#include "bkr/bkr_generate.h"
#include "bkr/bkr_text.h"
#include "bkr/bkr_ids.h"

/* O valor-ouro vem do core/Makefile (PLACEMENT_GOLDEN), fonte única
 * compartilhada com o autoteste MIPS. */
#ifndef BKR_PLACEMENT_GOLDEN
#error "BKR_PLACEMENT_GOLDEN não definido: compile os testes com make -C core test"
#endif
#ifndef BKR_PROPERTY_SEEDS
#define BKR_PROPERTY_SEEDS 100
#endif

static BkrGenResult g_a;
static BkrGenResult g_b;
static bkr_u8 g_vanilla[BKR_LOCATION_COUNT];

static void make_spec(BkrSeedSpec* spec, const char* seed) {
    bkr_settings_default(&spec->settings);
    bkr_seed_sanitize(seed, spec->seed);
}

static int same_placement(const BkrGenResult* a, const BkrGenResult* b) {
    bkr_u32 i;
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        if (a->placement[i] != b->placement[i]) {
            return 0;
        }
    }
    return 1;
}

/* Mesmo multiconjunto de itens do vanilla; localizações travadas intactas. */
static void check_balance(const BkrSettings* s, const BkrGenResult* r) {
    bkr_u32 want[0x40];
    bkr_u32 got[0x40];
    bkr_u32 i;
    for (i = 0; i < 0x40u; i++) {
        want[i] = 0;
        got[i] = 0;
    }
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        want[bkr_locations[i].vanilla_item]++;
        got[r->placement[i]]++;
        BKR_CHECK(r->placement[i] != BKR_ITEM_NONE);
        if (!bkr_location_is_shuffled(s, i)) {
            BKR_CHECK_EQ(r->placement[i], bkr_locations[i].vanilla_item);
        }
    }
    for (i = 0; i < 0x40u; i++) {
        BKR_CHECK_EQ(got[i], want[i]);
    }
}

void test_vanilla_world_verifies(void) {
    BkrSettings s;
    bkr_u32 i;
    bkr_settings_default(&s);
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        g_vanilla[i] = bkr_locations[i].vanilla_item;
    }
    BKR_CHECK(bkr_verify(&s, g_vanilla));
}

void test_generate_default(void) {
    BkrSeedSpec spec;
    make_spec(&spec, "BANJO");
    BKR_CHECK(bkr_generate(&spec, &g_a));
    BKR_CHECK(g_a.ok);
    BKR_CHECK(bkr_verify(&spec.settings, g_a.placement));
    check_balance(&spec.settings, &g_a);
    BKR_CHECK(bkr_generate(&spec, &g_b));
    BKR_CHECK(same_placement(&g_a, &g_b));
    BKR_CHECK_EQ(g_a.fingerprint, g_b.fingerprint);
    BKR_CHECK_EQ(g_a.fingerprint, 0x623A1517u);
    /* Valor-ouro: muda só quando o algoritmo ou os dados de lógica mudam
     * (e então BKR_GENERATOR_VERSION deve subir junto). */
    BKR_CHECK_EQ(bkr_placement_hash(g_a.placement), BKR_PLACEMENT_GOLDEN);
}

void test_generate_rejects_invalid(void) {
    BkrSeedSpec spec;
    make_spec(&spec, "@@@");
    BKR_CHECK(!bkr_generate(&spec, &g_a));
    BKR_CHECK(!g_a.ok);
    make_spec(&spec, "BANJO");
    spec.settings.shuffle = 7;
    BKR_CHECK(!bkr_generate(&spec, &g_a));
}

void test_generate_seed_changes_result(void) {
    BkrSeedSpec spec;
    make_spec(&spec, "BANJO");
    BKR_CHECK(bkr_generate(&spec, &g_a));
    make_spec(&spec, "KAZOOIE");
    BKR_CHECK(bkr_generate(&spec, &g_b));
    BKR_CHECK(!same_placement(&g_a, &g_b));
}

void test_molehills_hold_moves(void) {
    BkrSeedSpec spec;
    bkr_u32 i;
    make_spec(&spec, "MOLES");
    BKR_CHECK(bkr_generate(&spec, &g_a));
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        int is_mole = bkr_locations[i].kind == BKR_LOC_MOLEHILL;
        BKR_CHECK_EQ(is_mole, BKR_ITEM_IS_MOVE(g_a.placement[i]) ? 1 : 0);
    }
}

void test_by_type_keeps_kinds(void) {
    BkrSeedSpec spec;
    bkr_u32 i;
    make_spec(&spec, "TYPES");
    spec.settings.shuffle = BKR_SHUFFLE_BY_TYPE;
    BKR_CHECK(bkr_generate(&spec, &g_a));
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        if (bkr_locations[i].kind != BKR_LOC_MOLEHILL) {
            BKR_CHECK_EQ(g_a.placement[i], bkr_locations[i].vanilla_item);
        }
    }
}

void test_world_scope_keeps_worlds(void) {
    BkrSeedSpec spec;
    bkr_u32 per_world[BKR_WORLD_COUNT][4];
    bkr_u32 w;
    bkr_u32 k;
    bkr_u32 i;
    make_spec(&spec, "WORLDS");
    spec.settings.scope = BKR_SCOPE_WORLD;
    BKR_CHECK(bkr_generate(&spec, &g_a));
    for (w = 0; w < BKR_WORLD_COUNT; w++) {
        for (k = 0; k < 4; k++) {
            per_world[w][k] = 0;
        }
    }
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        bkr_u8 item = g_a.placement[i];
        bkr_u8 van = bkr_locations[i].vanilla_item;
        if (item < 4) {
            per_world[bkr_locations[i].world][item]++;
        }
        if (van < 4) {
            per_world[bkr_locations[i].world][van]--;
        }
    }
    for (w = 0; w < BKR_WORLD_COUNT; w++) {
        for (k = 0; k < 4; k++) {
            BKR_CHECK_EQ(per_world[w][k], 0u);
        }
    }
}

void test_vanilla_settings_reproduce_vanilla(void) {
    BkrSeedSpec spec;
    bkr_u32 i;
    make_spec(&spec, "VANILLA");
    spec.settings.shuffle = BKR_SHUFFLE_VANILLA;
    spec.settings.moves = BKR_MOVES_VANILLA;
    BKR_CHECK(bkr_generate(&spec, &g_a));
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        BKR_CHECK_EQ(g_a.placement[i], bkr_locations[i].vanilla_item);
    }
}

void test_property_many_seeds(void) {
    static const bkr_u8 presets[4][5] = {
        /* logic, shuffle, scope, moves, access */
        { BKR_LOGIC_GLITCHLESS, BKR_SHUFFLE_MIXED, BKR_SCOPE_GAME, BKR_MOVES_MOLEHILLS, BKR_ACCESS_ALL },
        { BKR_LOGIC_GLITCHLESS, BKR_SHUFFLE_BY_TYPE, BKR_SCOPE_GAME, BKR_MOVES_MOLEHILLS, BKR_ACCESS_ALL },
        { BKR_LOGIC_GLITCHLESS, BKR_SHUFFLE_MIXED, BKR_SCOPE_WORLD, BKR_MOVES_MOLEHILLS, BKR_ACCESS_ALL },
        { BKR_LOGIC_GLITCHLESS, BKR_SHUFFLE_MIXED, BKR_SCOPE_GAME, BKR_MOVES_MOLEHILLS, BKR_ACCESS_BEATABLE },
    };
    bkr_u32 p;
    for (p = 0; p < 4u; p++) {
        bkr_u32 n;
        bkr_u32 max_attempts = 0;
        bkr_u32 failures = 0;
        for (n = 0; n < BKR_PROPERTY_SEEDS; n++) {
            BkrSeedSpec spec;
            char seed[16];
            BkrText t;
            bkr_text_init(&t, seed, sizeof(seed));
            bkr_text_put(&t, "P");
            bkr_text_u32(&t, p);
            bkr_text_putc(&t, '_');
            bkr_text_u32(&t, n);
            make_spec(&spec, seed);
            spec.settings.logic = presets[p][0];
            spec.settings.shuffle = presets[p][1];
            spec.settings.scope = presets[p][2];
            spec.settings.moves = presets[p][3];
            spec.settings.access = presets[p][4];
            if (!bkr_generate(&spec, &g_a) || !bkr_verify(&spec.settings, g_a.placement)) {
                failures++;
                continue;
            }
            check_balance(&spec.settings, &g_a);
            if (g_a.attempts > max_attempts) {
                max_attempts = g_a.attempts;
            }
        }
        printf("  preset %u: %u seeds, falhas=%u, tentativas máx=%u\n", p, (bkr_u32)BKR_PROPERTY_SEEDS, failures,
               max_attempts);
        BKR_CHECK_EQ(failures, 0u);
    }
}
```

- [ ] **Passo 2: Ver falhar**

**Rodar:**

```bash
make -C core test
```

**Esperado (falha):**

```text
bkr/bkr_generate.h
```

- [ ] **Passo 3: Implementar**

Arquivo `core/include/bkr/bkr_generate.h` (criar):

```c
/*
 * bkr_generate.h — Geração do seed: preenchimento assumido (assumed fill,
 * como Fill.py::fill_restrictive do OoT Randomizer) + verificação.
 */
#ifndef BKR_GENERATE_H
#define BKR_GENERATE_H

#include "bkr_types.h"
#include "bkr_world.h"
#include "bkr_settings.h"
#include "bkr_logic.h"

#define BKR_MAX_ATTEMPTS 40u

typedef struct BkrGenResult {
    bkr_u8 placement[BKR_LOCATION_COUNT];  /* item em cada localização */
    bkr_u32 fingerprint;
    bkr_u32 attempts;
    int ok;
} BkrGenResult;

/* 1 se a localização entra no embaralhamento com estas opções. */
int bkr_location_is_shuffled(const BkrSettings* s, bkr_u32 loc);
/* Gera o seed. Retorna 1 em sucesso (out->ok). Determinístico. */
int bkr_generate(const BkrSeedSpec* spec, BkrGenResult* out);
/* Verifica um placement completo: vencível e, se pedido, 100% alcançável. */
int bkr_verify(const BkrSettings* s, const bkr_u8* placement);
/* Hash do placement (comparação host x MIPS). */
bkr_u32 bkr_placement_hash(const bkr_u8* placement);

#endif
```

Arquivo `core/src/bkr_generate.c` (criar):

```c
/*
 * bkr_generate.c — Geração determinística do seed.
 *
 * Algoritmo (adaptado de OoT-Randomizer, Fill.py::distribute_items_restrictive
 * e fill_restrictive):
 *  1. localizações embaralhadas formam o pool com seus itens vanilla;
 *  2. itens de progressão (movimentos, jiggies, tokens) são colocados com
 *     preenchimento assumido: para cada item, assume-se que todos os itens
 *     ainda não colocados já foram coletados, varre-se o mundo e o item vai
 *     para uma localização vazia alcançável sem ele;
 *  3. o resto (favos) é colocado ao acaso;
 *  4. o resultado é verificado (vencível e, se pedido, 100% alcançável).
 * Toda memória de trabalho é estática (pilha do jogo é pequena).
 */
#include "bkr/bkr_generate.h"
#include "bkr/bkr_rng.h"
#include "bkr/bkr_hash.h"
#include "bkr/bkr_ids.h"

typedef struct BkrItemInst {
    bkr_u8 item;
    bkr_u8 world;        /* mundo de origem (escopo "por mundo") */
    bkr_u8 origin_kind;  /* tipo da localização de origem (modo "por tipo") */
    bkr_u8 pad;
} BkrItemInst;

static BkrItemInst g_pool[BKR_LOCATION_COUNT];
static bkr_u16 g_prog[BKR_LOCATION_COUNT];
static bkr_u16 g_rest[BKR_LOCATION_COUNT];
static bkr_u16 g_cand[BKR_LOCATION_COUNT];
static BkrSearch g_search;
static BkrState g_assumed;
static BkrState g_base;
static BkrCosts g_costs;

int bkr_location_is_shuffled(const BkrSettings* s, bkr_u32 loc) {
    if (bkr_locations[loc].kind == BKR_LOC_MOLEHILL) {
        return s->moves != BKR_MOVES_VANILLA;
    }
    return s->shuffle != BKR_SHUFFLE_VANILLA;
}

static int bkr_is_progression(bkr_u8 item) {
    return BKR_ITEM_IS_MOVE(item) || item == BKR_ITEM_JIGGY || item == BKR_ITEM_TOKEN;
}

static int bkr_allowed(const BkrSettings* s, const BkrItemInst* it, const BkrLocDef* loc) {
    int is_move = BKR_ITEM_IS_MOVE(it->item);
    if (loc->kind == BKR_LOC_MOLEHILL) {
        if (!is_move) {
            return 0;
        }
    } else {
        if (is_move) {
            return 0;
        }
        if (s->shuffle == BKR_SHUFFLE_BY_TYPE && it->origin_kind != loc->kind) {
            return 0;
        }
    }
    if (s->scope == BKR_SCOPE_WORLD && it->world != loc->world) {
        return 0;
    }
    return 1;
}

int bkr_verify(const BkrSettings* s, const bkr_u8* placement) {
    bkr_u32 i;
    bkr_costs_vanilla(&g_costs);
    bkr_state_base(&g_base);
    bkr_search_init(&g_search, &g_base);
    bkr_search_sweep(&g_search, &g_costs, placement, BKR_SWEEP_COLLECT);
    if (!BKR_BIT_GET(g_search.st.events, BKR_GOAL_EVENT)) {
        return 0;
    }
    if (s->access == BKR_ACCESS_ALL) {
        for (i = 0; i < BKR_LOCATION_COUNT; i++) {
            if (!BKR_BIT_GET(g_search.loc_done, i)) {
                return 0;
            }
        }
    }
    return 1;
}

/* Uma tentativa de geração. Retorna 1 se produziu um placement válido. */
static int bkr_attempt(const BkrSettings* s, BkrRng* rng, bkr_u8* placement) {
    bkr_u32 pool_n = 0;
    bkr_u32 prog_n = 0;
    bkr_u32 rest_n = 0;
    bkr_u32 moves_n = 0;
    bkr_u32 i;
    bkr_u32 k;

    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        const BkrLocDef* loc = &bkr_locations[i];
        if (bkr_location_is_shuffled(s, i)) {
            g_pool[pool_n].item = loc->vanilla_item;
            g_pool[pool_n].world = loc->world;
            g_pool[pool_n].origin_kind = loc->kind;
            g_pool[pool_n].pad = 0;
            pool_n++;
            placement[i] = BKR_ITEM_NONE;
        } else {
            placement[i] = loc->vanilla_item;
        }
    }
    for (i = 0; i < pool_n; i++) {
        if (bkr_is_progression(g_pool[i].item)) {
            g_prog[prog_n++] = (bkr_u16)i;
        } else {
            g_rest[rest_n++] = (bkr_u16)i;
        }
    }
    bkr_rng_shuffle_u16(rng, g_prog, prog_n);
    bkr_rng_shuffle_u16(rng, g_rest, rest_n);

    /* Movimentos primeiro (mais restritos), preservando a ordem sorteada. */
    for (i = 0; i < prog_n; i++) {
        if (BKR_ITEM_IS_MOVE(g_pool[g_prog[i]].item)) {
            bkr_u16 v = g_prog[i];
            for (k = i; k > moves_n; k--) {
                g_prog[k] = g_prog[k - 1];
            }
            g_prog[moves_n++] = v;
        }
    }

    bkr_state_base(&g_assumed);
    for (i = 0; i < prog_n; i++) {
        bkr_state_add_item(&g_assumed, g_pool[g_prog[i]].item);
    }

    for (i = 0; i < prog_n; i++) {
        const BkrItemInst* it = &g_pool[g_prog[i]];
        bkr_u32 cand_n = 0;
        bkr_state_remove_item(&g_assumed, it->item);
        if (s->logic != BKR_LOGIC_NONE) {
            bkr_search_init(&g_search, &g_assumed);
            bkr_search_sweep(&g_search, &g_costs, placement, BKR_SWEEP_COLLECT);
        }
        for (k = 0; k < BKR_LOCATION_COUNT; k++) {
            if (placement[k] != BKR_ITEM_NONE || !bkr_location_is_shuffled(s, k)) {
                continue;
            }
            if (!bkr_allowed(s, it, &bkr_locations[k])) {
                continue;
            }
            if (s->logic != BKR_LOGIC_NONE && !BKR_BIT_GET(g_search.loc_done, k)) {
                continue;
            }
            g_cand[cand_n++] = (bkr_u16)k;
        }
        if (cand_n == 0) {
            return 0;
        }
        placement[g_cand[bkr_rng_below(rng, cand_n)]] = it->item;
    }

    for (i = 0; i < rest_n; i++) {
        const BkrItemInst* it = &g_pool[g_rest[i]];
        bkr_u32 cand_n = 0;
        for (k = 0; k < BKR_LOCATION_COUNT; k++) {
            if (placement[k] == BKR_ITEM_NONE && bkr_location_is_shuffled(s, k) &&
                bkr_allowed(s, it, &bkr_locations[k])) {
                g_cand[cand_n++] = (bkr_u16)k;
            }
        }
        if (cand_n == 0) {
            return 0;
        }
        placement[g_cand[bkr_rng_below(rng, cand_n)]] = it->item;
    }

    if (s->logic == BKR_LOGIC_NONE) {
        return 1;
    }
    return bkr_verify(s, placement);
}

int bkr_generate(const BkrSeedSpec* spec, BkrGenResult* out) {
    BkrRng rng;
    bkr_u32 attempt;
    out->ok = 0;
    out->attempts = 0;
    out->fingerprint = bkr_seed_fingerprint(spec);
    if (!bkr_settings_valid(&spec->settings) || spec->seed[0] == '\0') {
        return 0;
    }
    bkr_costs_vanilla(&g_costs);
    bkr_rng_seed(&rng, out->fingerprint);
    for (attempt = 1; attempt <= BKR_MAX_ATTEMPTS; attempt++) {
        out->attempts = attempt;
        if (bkr_attempt(&spec->settings, &rng, out->placement)) {
            out->ok = 1;
            return 1;
        }
    }
    return 0;
}

bkr_u32 bkr_placement_hash(const bkr_u8* placement) {
    return bkr_fnv1a_bytes(BKR_FNV_OFFSET, placement, BKR_LOCATION_COUNT);
}
```

- [ ] **Passo 4: Ver passar** (leva uns 20 segundos por causa das 2000 gerações)

**Rodar:**

```bash
make -C core test
```

**Esperado:**

```text
ok   vanilla_world_verifies
ok   generate_default
ok   generate_rejects_invalid
ok   generate_seed_changes_result
ok   molehills_hold_moves
ok   by_type_keeps_kinds
ok   world_scope_keeps_worlds
ok   vanilla_settings_reproduce_vanilla
  preset 0: 500 seeds, falhas=0, tentativas máx=9
  preset 1: 500 seeds, falhas=0, tentativas máx=9
  preset 2: 500 seeds, falhas=0, tentativas máx=13
  preset 3: 500 seeds, falhas=0, tentativas máx=7
ok   property_many_seeds
29 testes, 637826 verificações, 0 falhas
```

`31098721` (em `generate_default`, vindo de `PLACEMENT_GOLDEN` no `core/Makefile`) é o terceiro valor-ouro: o hash do placement do seed `BANJO`.

- [ ] **Passo 5: Commit**

**Rodar:**

```bash
git add core/tests/test_generate.c core/include/bkr/bkr_generate.h core/src/bkr_generate.c
git commit -q -m "adiciona geração por preenchimento assumido"
```

### Task 14: Spoiler

**Arquivos:**
- Criar: `core/tests/test_spoiler.c`, `core/include/bkr/bkr_spoiler.h`, `core/src/bkr_spoiler.c`

Esferas como no OoT Randomizer: a cada rodada, tudo o que ficou alcançável com os itens coletados até ali. O spoiler só existe no PC; o mod não compila este arquivo.

- [ ] **Passo 1: Escrever os testes**

Arquivo `core/tests/test_spoiler.c` (criar):

```c
/* test_spoiler.c — Spoiler em texto: cabeçalho, esferas e lista por mundo. */
#include "bkr_test.h"
#include "bkr/bkr_spoiler.h"
#include "bkr/bkr_ids.h"

static BkrGenResult s_result;
static char s_spoiler[96 * 1024];

static int contains(const char* text, const char* needle) {
    bkr_u32 n = bkr_strlen(needle);
    for (; *text != '\0'; text++) {
        bkr_u32 i = 0;
        while (i < n && text[i] == needle[i]) {
            i++;
        }
        if (i == n) {
            return 1;
        }
    }
    return 0;
}

void test_spoiler_text(void) {
    BkrSeedSpec spec;
    BkrText t;
    bkr_u32 spheres;
    bkr_settings_default(&spec.settings);
    bkr_seed_sanitize("BANJO", spec.seed);
    BKR_CHECK(bkr_generate(&spec, &s_result));
    bkr_text_init(&t, s_spoiler, sizeof(s_spoiler));
    spheres = bkr_spoiler_write(&spec, &s_result, BKR_LANG_PT_BR, &t);
    BKR_CHECK(spheres > 3u);
    BKR_CHECK(!t.overflow);
    BKR_CHECK(t.len > 1000u);
    BKR_CHECK(contains(s_spoiler, "Codigo: BKR1-GG000M-BANJO"));
    BKR_CHECK(contains(s_spoiler, "Impressao digital: 623A1517"));
    BKR_CHECK(contains(s_spoiler, "Esfera 1:"));
    BKR_CHECK(contains(s_spoiler, "== Localizacoes por mundo =="));
    BKR_CHECK(contains(s_spoiler, "Covil de Gruntilda:"));
    bkr_text_init(&t, s_spoiler, sizeof(s_spoiler));
    BKR_CHECK_EQ(bkr_spoiler_write(&spec, &s_result, BKR_LANG_EN, &t), spheres);
    BKR_CHECK(contains(s_spoiler, "Code: BKR1-GG000M-BANJO"));
    BKR_CHECK(contains(s_spoiler, "== Playthrough spheres =="));
}
```

- [ ] **Passo 2: Ver falhar**

**Rodar:**

```bash
make -C core test
```

**Esperado (falha):**

```text
bkr/bkr_spoiler.h
```

- [ ] **Passo 3: Implementar**

Arquivo `core/include/bkr/bkr_spoiler.h` (criar):

```c
/*
 * bkr_spoiler.h — Spoiler em texto: esferas de coleta (playthrough) e
 * lista de localizações por mundo. Usado pela CLI no host.
 */
#ifndef BKR_SPOILER_H
#define BKR_SPOILER_H

#include "bkr_text.h"
#include "bkr_generate.h"

/* Escreve o spoiler completo. Retorna o número de esferas. */
bkr_u32 bkr_spoiler_write(const BkrSeedSpec* spec, const BkrGenResult* res, int lang, BkrText* out);

#endif
```

Arquivo `core/src/bkr_spoiler.c` (criar):

```c
/* bkr_spoiler.c — Spoiler: cabeçalho, esferas de coleta e lista por mundo. */
#include "bkr/bkr_spoiler.h"
#include "bkr/bkr_ids.h"

static BkrSearch g_sp_search;
static BkrState g_sp_base;
static BkrCosts g_sp_costs;
static bkr_u32 g_sp_collected[BKR_LOC_WORDS];
static bkr_u16 g_sp_new[BKR_LOCATION_COUNT];

static const char* bkr_opt(int lang, const char* pt, const char* en) {
    return lang == BKR_LANG_EN ? en : pt;
}

static void bkr_spoiler_header(const BkrSeedSpec* spec, const BkrGenResult* res, int lang, BkrText* t) {
    char code[BKR_SHARE_CODE_MAX];
    const BkrSettings* s = &spec->settings;
    bkr_u32 i;
    bkr_share_code_format(spec, code, sizeof(code));
    bkr_text_put(t, "Banjo-Kazooie Randomizer - Spoiler\n");
    bkr_text_put(t, bkr_opt(lang, "Codigo: ", "Code: "));
    bkr_text_put(t, code);
    bkr_text_put(t, "\nHash: ");
    for (i = 0; i < 5; i++) {
        bkr_text_put(t, bkr_hash_word(res->fingerprint >> (i * 5)));
        bkr_text_putc(t, i < 4 ? ' ' : '\n');
    }
    bkr_text_put(t, bkr_opt(lang, "Impressao digital: ", "Fingerprint: "));
    bkr_text_hex(t, res->fingerprint, 8);
    bkr_text_put(t, bkr_opt(lang, "\nTentativas: ", "\nAttempts: "));
    bkr_text_u32(t, res->attempts);
    bkr_text_put(t, bkr_opt(lang, "\nOpcoes: logica=", "\nSettings: logic="));
    bkr_text_put(t, s->logic == BKR_LOGIC_NONE ? "none" : "glitchless");
    bkr_text_put(t, " shuffle=");
    bkr_text_put(t, s->shuffle == BKR_SHUFFLE_VANILLA ? "vanilla" : (s->shuffle == BKR_SHUFFLE_BY_TYPE ? "type" : "mixed"));
    bkr_text_put(t, " scope=");
    bkr_text_put(t, s->scope == BKR_SCOPE_WORLD ? "world" : "game");
    bkr_text_put(t, " moves=");
    bkr_text_put(t, s->moves == BKR_MOVES_MOLEHILLS ? "molehills" : "vanilla");
    bkr_text_put(t, " access=");
    bkr_text_put(t, s->access == BKR_ACCESS_BEATABLE ? "beatable" : "all");
    bkr_text_putc(t, '\n');
}

static void bkr_spoiler_line(bkr_u32 loc, const bkr_u8* placement, int lang, BkrText* t) {
    bkr_text_put(t, "  ");
    bkr_text_put(t, bkr_location_names[loc]);
    bkr_text_put(t, ": ");
    bkr_text_put(t, bkr_item_name(placement[loc], lang));
    bkr_text_putc(t, '\n');
}

bkr_u32 bkr_spoiler_write(const BkrSeedSpec* spec, const BkrGenResult* res, int lang, BkrText* out) {
    bkr_u32 sphere = 0;
    bkr_u32 i;
    bkr_u32 w;
    bkr_spoiler_header(spec, res, lang, out);
    bkr_text_put(out, bkr_opt(lang, "\n== Esferas de coleta ==\n", "\n== Playthrough spheres ==\n"));
    bkr_costs_vanilla(&g_sp_costs);
    bkr_state_base(&g_sp_base);
    bkr_search_init(&g_sp_search, &g_sp_base);
    bkr_words_clear(g_sp_collected, BKR_LOC_WORDS);
    for (;;) {
        bkr_u32 new_n = 0;
        bkr_search_sweep(&g_sp_search, &g_sp_costs, res->placement, BKR_SWEEP_MARK);
        for (i = 0; i < BKR_LOCATION_COUNT; i++) {
            if (BKR_BIT_GET(g_sp_search.loc_done, i) && !BKR_BIT_GET(g_sp_collected, i)) {
                g_sp_new[new_n++] = (bkr_u16)i;
            }
        }
        if (new_n == 0) {
            break;
        }
        sphere++;
        {
            bkr_u32 jiggies = 0;
            bkr_u32 tokens = 0;
            bkr_u32 combs = 0;
            for (i = 0; i < new_n; i++) {
                bkr_u8 item = res->placement[g_sp_new[i]];
                jiggies += (item == BKR_ITEM_JIGGY);
                tokens += (item == BKR_ITEM_TOKEN);
                combs += (item == BKR_ITEM_HONEYCOMB);
            }
            bkr_text_put(out, bkr_opt(lang, "Esfera ", "Sphere "));
            bkr_text_u32(out, sphere);
            bkr_text_put(out, ": +");
            bkr_text_u32(out, jiggies);
            bkr_text_put(out, " jiggies, +");
            bkr_text_u32(out, tokens);
            bkr_text_put(out, " tokens, +");
            bkr_text_u32(out, combs);
            bkr_text_put(out, bkr_opt(lang, " favos\n", " honeycombs\n"));
        }
        for (i = 0; i < new_n; i++) {
            bkr_u32 loc = g_sp_new[i];
            BKR_BIT_SET(g_sp_collected, loc);
            bkr_state_add_item(&g_sp_search.st, res->placement[loc]);
            if (BKR_ITEM_IS_MOVE(res->placement[loc])) {
                bkr_spoiler_line(loc, res->placement, lang, out);
            }
        }
    }
    bkr_text_put(out, bkr_opt(lang, "\n== Localizacoes por mundo ==\n", "\n== Locations by world ==\n"));
    for (w = 1; w < BKR_WORLD_COUNT; w++) {
        int header = 0;
        for (i = 0; i < BKR_LOCATION_COUNT; i++) {
            if (bkr_locations[i].world != w) {
                continue;
            }
            if (!header) {
                bkr_text_put(out, bkr_world_name((bkr_u8)w, lang));
                bkr_text_put(out, ":\n");
                header = 1;
            }
            bkr_spoiler_line(i, res->placement, lang, out);
        }
    }
    return sphere;
}
```

- [ ] **Passo 4: Ver passar**

**Rodar:**

```bash
make -C core test
```

**Esperado:**

```text
ok   spoiler_text
30 testes, 637838 verificações, 0 falhas
```

- [ ] **Passo 5: Commit**

**Rodar:**

```bash
git add core/tests/test_spoiler.c core/include/bkr/bkr_spoiler.h core/src/bkr_spoiler.c
git commit -q -m "adiciona spoiler com esferas de coleta"
```

### Task 15: CLI `bkrgen`

**Arquivos:**
- Criar: `core/cli/bkrgen.c`

- [ ] **Passo 1: Implementar**

Arquivo `core/cli/bkrgen.c` (criar):

```c
/*
 * bkrgen.c — CLI do gerador (host). Usa o MESMO núcleo do mod, então o seed
 * gerado aqui é idêntico ao gerado no jogo.
 *
 *   bkrgen --seed BANJO [--logic glitchless|none] [--shuffle vanilla|type|mixed]
 *          [--scope game|world] [--moves vanilla|molehills] [--access all|beatable]
 *          [--code BKR1-XXXXXX-SEED] [--lang pt|en] [--spoiler arquivo.txt]
 */
#include <stdio.h>
#include <string.h>
#include "bkr/bkr_generate.h"
#include "bkr/bkr_spoiler.h"
#include "bkr/bkr_ids.h"

static BkrGenResult g_result;
static char g_spoiler[256 * 1024];

static int pick(const char* value, const char* const* names, int count) {
    int i;
    for (i = 0; i < count; i++) {
        if (strcmp(value, names[i]) == 0) {
            return i;
        }
    }
    return -1;
}

static int usage(void) {
    fprintf(stderr,
            "uso: bkrgen --seed TEXTO [--logic glitchless|none] [--shuffle vanilla|type|mixed]\n"
            "            [--scope game|world] [--moves vanilla|molehills] [--access all|beatable]\n"
            "            [--code BKR1-XXXXXX-SEED] [--lang pt|en] [--spoiler arquivo.txt]\n");
    return 2;
}

int main(int argc, char** argv) {
    static const char* const logic[] = { "glitchless", "none" };
    static const char* const shuffle[] = { "vanilla", "type", "mixed" };
    static const char* const scope[] = { "game", "world" };
    static const char* const moves[] = { "vanilla", "molehills" };
    static const char* const access[] = { "all", "beatable" };
    BkrSeedSpec spec;
    const char* spoiler_path = NULL;
    const char* code = NULL;
    char share[BKR_SHARE_CODE_MAX];
    int lang = BKR_LANG_PT_BR;
    int i;
    int v;

    bkr_settings_default(&spec.settings);
    spec.seed[0] = '\0';
    for (i = 1; i < argc; i++) {
        const char* opt = argv[i];
        const char* val = (i + 1 < argc) ? argv[i + 1] : NULL;
        if (val == NULL) {
            return usage();
        }
        i++;
        if (strcmp(opt, "--seed") == 0) {
            bkr_seed_sanitize(val, spec.seed);
        } else if (strcmp(opt, "--code") == 0) {
            code = val;
        } else if (strcmp(opt, "--spoiler") == 0) {
            spoiler_path = val;
        } else if (strcmp(opt, "--lang") == 0) {
            lang = strcmp(val, "en") == 0 ? BKR_LANG_EN : BKR_LANG_PT_BR;
        } else if (strcmp(opt, "--logic") == 0 && (v = pick(val, logic, 2)) >= 0) {
            spec.settings.logic = (bkr_u8)v;
        } else if (strcmp(opt, "--shuffle") == 0 && (v = pick(val, shuffle, 3)) >= 0) {
            spec.settings.shuffle = (bkr_u8)v;
        } else if (strcmp(opt, "--scope") == 0 && (v = pick(val, scope, 2)) >= 0) {
            spec.settings.scope = (bkr_u8)v;
        } else if (strcmp(opt, "--moves") == 0 && (v = pick(val, moves, 2)) >= 0) {
            spec.settings.moves = (bkr_u8)v;
        } else if (strcmp(opt, "--access") == 0 && (v = pick(val, access, 2)) >= 0) {
            spec.settings.access = (bkr_u8)v;
        } else {
            return usage();
        }
    }
    if (code != NULL && !bkr_share_code_parse(code, &spec)) {
        fprintf(stderr, "código inválido: %s\n", code);
        return 2;
    }
    if (spec.seed[0] == '\0') {
        return usage();
    }
    if (!bkr_generate(&spec, &g_result)) {
        fprintf(stderr, "falha ao gerar após %u tentativas\n", g_result.attempts);
        return 1;
    }
    bkr_share_code_format(&spec, share, sizeof(share));
    printf("codigo=%s\n", share);
    printf("fingerprint=%08X\n", g_result.fingerprint);
    printf("save=bkr_%08X\n", g_result.fingerprint);
    printf("placement_hash=%08X\n", bkr_placement_hash(g_result.placement));
    printf("tentativas=%u\n", g_result.attempts);
    printf("hash=%s %s %s %s %s\n", bkr_hash_word(g_result.fingerprint), bkr_hash_word(g_result.fingerprint >> 5),
           bkr_hash_word(g_result.fingerprint >> 10), bkr_hash_word(g_result.fingerprint >> 15),
           bkr_hash_word(g_result.fingerprint >> 20));
    if (spoiler_path != NULL) {
        BkrText t;
        FILE* f = fopen(spoiler_path, "wb");
        if (f == NULL) {
            fprintf(stderr, "não consegui abrir %s\n", spoiler_path);
            return 1;
        }
        bkr_text_init(&t, g_spoiler, sizeof(g_spoiler));
        bkr_spoiler_write(&spec, &g_result, lang, &t);
        fwrite(g_spoiler, 1, t.len, f);
        fclose(f);
        printf("spoiler=%s\n", spoiler_path);
    }
    return 0;
}
```

- [ ] **Passo 2: Gerar o seed padrão e o spoiler**

**Rodar:**

```bash
make -C core cli
./core/build/bkrgen --seed banjo --spoiler spoiler.txt
head -16 spoiler.txt
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
Banjo-Kazooie Randomizer - Spoiler
Codigo: BKR1-GG000M-BANJO
Opcoes: logica=glitchless shuffle=mixed scope=game moves=molehills access=all
Esfera 1: +3 jiggies, +4 tokens, +0 favos
  MM Molehill - Beak Buster: Trote de Garras
```

- [ ] **Passo 3: Código de compartilhamento, inglês e erro de uso**

**Rodar:**

```bash
./core/build/bkrgen --code bkr1-gg000m-banjo --lang en --spoiler spoiler_en.txt
grep -c "Sphere" spoiler_en.txt
```

**Esperado:**

```text
codigo=BKR1-GG000M-BANJO
placement_hash=31098721
10
```

**Rodar:**

```bash
./core/build/bkrgen --shuffle bagunca --seed X
```

**Esperado (falha):**

```text
uso: bkrgen --seed TEXTO
```

- [ ] **Passo 4: Commit** (`spoiler*.txt` está no `.gitignore`)

**Rodar:**

```bash
git add core/cli/bkrgen.c
git commit -q -m "adiciona CLI bkrgen"
```

### Fase 3 — Portabilidade MIPS

### Task 16: Núcleo em MIPS sem símbolos externos

**Arquivos:** nenhum novo (o alvo `mips-check` já está no `core/Makefile` da Task 7).

O mod é linkado com `--unresolved-symbols=ignore-all`: uma chamada escondida a `memset` ou `memcpy` (que o compilador gera para zerar ou copiar structs grandes) passaria no link e só quebraria no jogo. Esta checagem compila o núcleo com as flags do mod, junta os objetos e exige que `llvm-nm -u` não mostre nada.

- [ ] **Passo 1: Rodar a checagem**

**Rodar:**

```bash
make -C core mips-check
```

**Esperado:**

```text
mips-check: ok (nenhum símbolo externo)
```

Se aparecer `memset`, `memcpy` ou `__udivdi3`, procure no núcleo uma struct atribuída inteira, um array local inicializado ou uma divisão de 64 bits, e reescreva sem isso.

### Task 17: Autoteste do núcleo MIPS no emulador

**Arquivos:**
- Criar: `core/tests/mips_selftest.c`, `core/tests/mips_selftest.ld`, `tools/mips_selftest.py`

Compila o núcleo em MIPS big-endian com as mesmas flags do mod, linka uma imagem plana e executa `bkr_mips_selftest()` no Unicorn. O hash do placement precisa ser igual ao do PC (`31098721`), e o pico de pilha precisa ficar abaixo da metade da pilha da thread principal do jogo. É a prova de que o seed gerado no jogo é o mesmo da CLI.

- [ ] **Passo 1: Ponto de entrada e script de link**

Arquivo `core/tests/mips_selftest.c` (criar):

```c
/*
 * mips_selftest.c — Ponto de entrada para rodar o núcleo compilado em MIPS
 * dentro de um emulador (tools/mips_selftest.py). Gera o seed "BANJO" com as
 * opções padrão e devolve o hash do placement em $v0.
 */
#include "bkr/bkr_generate.h"

static BkrGenResult g_selftest_result;
static BkrSeedSpec g_selftest_spec;

bkr_u32 bkr_mips_selftest(void) {
    bkr_settings_default(&g_selftest_spec.settings);
    bkr_seed_sanitize("BANJO", g_selftest_spec.seed);
    if (!bkr_generate(&g_selftest_spec, &g_selftest_result)) {
        return 0;
    }
    return bkr_placement_hash(g_selftest_result.placement);
}
```

Arquivo `core/tests/mips_selftest.ld` (criar):

```ld
/* Script de link do autoteste MIPS: imagem plana a partir de 0x00400000 (KUSEG: o Unicorn traduz KSEG0). */
ENTRY(bkr_mips_selftest)
SECTIONS {
    . = 0x00400000;
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) *(COMMON) }
    /DISCARD/ : { *(.MIPS.abiflags) *(.reginfo) *(.pdr) *(.comment) *(.gnu.attributes) }
}
```

- [ ] **Passo 2: Executor no Unicorn**

Arquivo `tools/mips_selftest.py` (criar):

```python
#!/usr/bin/env python3
"""Executa o núcleo compilado em MIPS (big-endian, 32 bits) num emulador
Unicorn e compara o hash do placement com o do host. Prova que o seed gerado
dentro do jogo é idêntico ao da CLI, sem precisar abrir o jogo.

Pré-requisito:  pip install unicorn pyelftools
Uso:            python3 tools/mips_selftest.py core/build/mips/selftest.elf --expect 31098721
"""
import argparse
import sys
import time

from elftools.elf.elffile import ELFFile
from unicorn import Uc, UcError, UC_ARCH_MIPS, UC_MODE_MIPS32, UC_MODE_BIG_ENDIAN
from unicorn.mips_const import UC_MIPS_REG_29, UC_MIPS_REG_31, UC_MIPS_REG_2

STACK_TOP = 0x00300000
STACK_SIZE = 0x17F0          # mesmo tamanho da pilha da thread principal do jogo
RETURN_MAGIC = 0x00200000    # endereço de parada (jr $ra cai aqui)
PAGE = 0x1000


def align_down(value):
    return value & ~(PAGE - 1)


def align_up(value):
    return (value + PAGE - 1) & ~(PAGE - 1)


def load(uc, path):
    """Mapeia a união dos segmentos PT_LOAD (podem dividir páginas) e copia os dados."""
    with open(path, "rb") as f:
        elf = ELFFile(f)
        entry = elf.header["e_entry"]
        segments = [s for s in elf.iter_segments() if s["p_type"] == "PT_LOAD"]
        start = min(align_down(s["p_vaddr"]) for s in segments)
        end = max(align_up(s["p_vaddr"] + s["p_memsz"]) for s in segments)
        uc.mem_map(start, end - start)
        for seg in segments:
            uc.mem_write(seg["p_vaddr"], seg.data())
    return entry


def main(argv=None):
    parser = argparse.ArgumentParser(description="Autoteste do núcleo MIPS")
    parser.add_argument("elf")
    parser.add_argument("--expect", required=True, help="hash esperado em hexadecimal")
    args = parser.parse_args(argv)
    uc = Uc(UC_ARCH_MIPS, UC_MODE_MIPS32 + UC_MODE_BIG_ENDIAN)
    entry = load(uc, args.elf)
    uc.mem_map(align_down(STACK_TOP - 0x10000), 0x10000)
    uc.mem_map(RETURN_MAGIC, PAGE)
    uc.reg_write(UC_MIPS_REG_29, STACK_TOP - 16)
    uc.reg_write(UC_MIPS_REG_31, RETURN_MAGIC)
    # "Pinta" a pilha com um padrão para medir o uso máximo depois.
    canary = b"\xDE\xAD\xBE\xEF" * (0x10000 // 4)
    uc.mem_write(STACK_TOP - 0x10000, canary)
    started = time.time()
    try:
        uc.emu_start(entry, RETURN_MAGIC)
    except UcError as exc:
        print(f"ERRO no emulador: {exc}", file=sys.stderr)
        return 1
    elapsed = time.time() - started
    value = uc.reg_read(UC_MIPS_REG_2)
    expected = int(args.expect, 16)
    stack = bytes(uc.mem_read(STACK_TOP - 0x10000, 0x10000))
    first_dirty = next((i for i in range(0, 0x10000, 4) if stack[i:i + 4] != canary[:4]), 0x10000)
    used = 0x10000 - first_dirty
    print(f"hash MIPS={value:08X} esperado={expected:08X} tempo={elapsed:.2f}s pilha={used} bytes")
    if used >= STACK_SIZE // 2:
        print(f"ERRO: o núcleo usou {used} bytes de pilha; o limite de projeto é {STACK_SIZE // 2}", file=sys.stderr)
        return 1
    return 0 if value == expected else 1


if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Passo 3: Rodar**

**Rodar:**

```bash
make -C core mips-selftest
```

**Esperado:**

```text
hash MIPS=31098721 esperado=31098721 tempo=…s pilha=268 bytes
```

- [ ] **Passo 4: Commit**

**Rodar:**

```bash
git add core/tests/mips_selftest.c core/tests/mips_selftest.ld tools/mips_selftest.py
git commit -q -m "adiciona autoteste do núcleo MIPS no Unicorn"
```

### Fase 4 — Mod

### Task 18: Checagem de conflitos de patch com o base

**Arquivos:**
- Criar: `tools/tests/test_check_patch_conflicts.py`, `tools/check_patch_conflicts.py`

Se o mod usar `RECOMP_PATCH` numa função que o Banjo: Recompiled já substitui, o mod nem carrega; se patchear uma função com patch de instrução do base, descarta a correção do base. A checagem lê `../BanjoRecomp/patches/*.c` e `../BanjoRecomp/banjo.us.rev0.toml` e compara com os `RECOMP_PATCH` de `src/`.

- [ ] **Passo 1: Escrever os testes**

Arquivo `tools/tests/test_check_patch_conflicts.py` (criar):

```python
"""Testes da checagem de conflitos de RECOMP_PATCH com o Banjo: Recompiled.

Rodar:  python3 -m unittest discover -s tools/tests -v
"""
import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import check_patch_conflicts as cpc  # noqa: E402


class ConflictCheckTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        root = Path(self.tmp.name)
        self.base = root / "BanjoRecomp"
        self.mod = root / "src"
        (self.base / "patches").mkdir(parents=True)
        self.mod.mkdir()
        (self.base / "patches" / "a.c").write_text(
            "RECOMP_PATCH void gameFile_load(s32 gamenum) {}\n"
            "RECOMP_PATCH Actor *chfoo_spawn(ActorMarker *m) { return 0; }\n", encoding="utf-8")
        (self.base / "banjo.us.rev0.toml").write_text(
            "[[patches.instruction]]\nfunc = \"func_8038AC04\"\nvram = 0x8038AC0C\nvalue = 0x3C0E8000\n",
            encoding="utf-8")

    def tearDown(self):
        self.tmp.cleanup()

    def run_check(self, mod_source):
        """Roda a checagem e guarda a saída em self.output (sem poluir o log dos testes)."""
        (self.mod / "b.c").write_text(mod_source, encoding="utf-8")
        buffer = io.StringIO()
        with contextlib.redirect_stdout(buffer), contextlib.redirect_stderr(buffer):
            code = cpc.main(["--banjorecomp", str(self.base), "--mod", str(self.mod)])
        self.output = buffer.getvalue()
        return code

    def test_regex_finds_pointer_returns(self):
        self.assertEqual(cpc.patched_functions(self.base / "patches"), {"gameFile_load", "chfoo_spawn"})

    def test_detects_patch_conflict(self):
        self.assertEqual(self.run_check("RECOMP_PATCH void gameFile_load(s32 gamenum) {}\n"), 1)
        self.assertIn("gameFile_load", self.output)

    def test_detects_instruction_patch_conflict(self):
        self.assertEqual(self.run_check("RECOMP_PATCH void func_8038AC04(void) {}\n"), 1)
        self.assertIn("func_8038AC04", self.output)

    def test_hooks_and_other_patches_are_fine(self):
        source = ("RECOMP_HOOK(\"gameFile_load\") void on_load(s32 gamenum) {}\n"
                  "RECOMP_PATCH s32 item_adjustByDiffWithHud(s32 item, s32 diff) { return 0; }\n")
        self.assertEqual(self.run_check(source), 0)
        self.assertIn("ok: nenhum conflito", self.output)

    def test_missing_banjorecomp_folder(self):
        buffer = io.StringIO()
        with contextlib.redirect_stdout(buffer), contextlib.redirect_stderr(buffer):
            code = cpc.main(["--banjorecomp", str(self.base / "nao_existe"), "--mod", str(self.mod)])
        self.assertEqual(code, 2)


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Passo 2: Ver falhar**

**Rodar:**

```bash
python3 -m unittest discover -s tools/tests -v
```

**Esperado (falha):**

```text
ModuleNotFoundError: No module named 'check_patch_conflicts'
```

- [ ] **Passo 3: Implementar**

Arquivo `tools/check_patch_conflicts.py` (criar):

```python
#!/usr/bin/env python3
"""Falha se o mod usar RECOMP_PATCH em uma função que o Banjo: Recompiled já
altera: funções com RECOMP_PATCH em BanjoRecomp/patches/*.c (nesse caso o mod
nem carrega) ou com patch de instrução em BanjoRecomp/banjo.us.rev0.toml (o
patch do mod descartaria a correção do base). Hooks (RECOMP_HOOK e
RECOMP_HOOK_RETURN) em funções alteradas são permitidos pelo runtime e não
entram na checagem.

Uso:
    python3 tools/check_patch_conflicts.py --banjorecomp ../BanjoRecomp --mod src
"""
import argparse
import re
import sys
from pathlib import Path

PATCH_RE = re.compile(r"RECOMP_(?:FORCE_)?PATCH\s+[^;{(]*?\b([A-Za-z_][A-Za-z0-9_]*)\s*\(")
INSTRUCTION_BLOCK_RE = re.compile(r"\[\[patches\.instruction\]\]\s*func\s*=\s*\"([A-Za-z_][A-Za-z0-9_]*)\"")


def patched_functions(folder):
    """Nomes de funções marcadas com RECOMP_PATCH em todos os .c da pasta."""
    names = set()
    for path in sorted(Path(folder).rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="replace")
        names.update(PATCH_RE.findall(text))
    return names


def instruction_patched_functions(toml_path):
    """Funções com [[patches.instruction]] no TOML de recompilação do base."""
    if not toml_path.is_file():
        return set()
    return set(INSTRUCTION_BLOCK_RE.findall(toml_path.read_text(encoding="utf-8", errors="replace")))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--banjorecomp", required=True, type=Path, help="checkout do BanjoRecomp")
    parser.add_argument("--mod", required=True, type=Path, help="pasta src/ do mod")
    args = parser.parse_args(argv)
    base_dir = args.banjorecomp / "patches"
    if not base_dir.is_dir():
        print(f"ERRO: {base_dir} não existe", file=sys.stderr)
        return 2
    base = patched_functions(base_dir)
    instr = instruction_patched_functions(args.banjorecomp / "banjo.us.rev0.toml")
    ours = patched_functions(args.mod)
    print(f"patches do Banjo: Recompiled: {len(base)} funções + {len(instr)} patches de instrução | "
          f"patches do mod: {len(ours)} -> {sorted(ours)}")
    conflicts = sorted((base | instr) & ours)
    if conflicts:
        print("ERRO: o mod substitui funções já alteradas pelo Banjo: Recompiled:")
        for name in conflicts:
            print(f"  - {name} (use RECOMP_HOOK/RECOMP_HOOK_RETURN em vez de RECOMP_PATCH)")
        return 1
    print("ok: nenhum conflito de RECOMP_PATCH")
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Passo 4: Ver passar**

**Rodar:**

```bash
python3 -m unittest discover -s tools/tests -v
```

**Esperado:**

```text
Ran 26 tests
OK
```

- [ ] **Passo 5: Commit**

**Rodar:**

```bash
git add tools/tests/test_check_patch_conflicts.py tools/check_patch_conflicts.py
git commit -q -m "adiciona checagem de conflitos de RECOMP_PATCH com o base"
```

### Task 19: Build do mod, manifesto e inicialização

**Arquivos:**
- Substituir: `Makefile`, `mod.toml`
- Criar: `src/bkr_mod.h`, `src/mod_init.c`

O `Makefile` parte do template e muda o seguinte: compila também o núcleo (sem o spoiler) com `core/include` e `core/generated` no caminho de includes; troca `-fno-builtin-memset` por `-fno-builtin`; acrescenta `-Wno-unknown-warning-option`; cria pastas com `mkdir -p` (shell POSIX, também no Windows pelo Git Bash); e ganha os alvos `nrm`, `logic`, `check` e `package`. O `mod.toml` declara id, versão, `minimum_recomp_version = "1.0.2"` e as opções que aparecem no menu de mods. `mod_init.c` roda no evento `recomp_on_init` (A10, item 6).

- [ ] **Passo 1: Makefile do mod**

Arquivo `Makefile` (substituir):

```makefile
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
```

- [ ] **Passo 2: Manifesto e opções**

Arquivo `mod.toml` (substituir):

```toml
# Manifesto do mod Banjo-Kazooie Randomizer para Banjo: Recompiled.

[manifest]
id = "ph_bk_randomizer"
version = "0.1.0"
display_name = "Banjo-Kazooie Randomizer"
description = """
Randomizer com lógica para Banjo: Recompiled. Embaralha jiggies, favos vazios, \
tokens do Mumbo e os movimentos ensinados pelo Bottles, garantindo que o seed \
seja vencível. O seed é gerado dentro do jogo; o mesmo código gera o spoiler \
no PC (ferramenta bkrgen).

Cada seed usa um arquivo de save próprio; seus saves normais não são tocados.
Mudanças nas opções valem ao iniciar o jogo."""
short_description = "Randomizer com lógica: jiggies, favos, tokens e movimentos."
authors = [ "PH" ]
game_id = "bk"
minimum_recomp_version = "1.0.2"
dependencies = [ ]
optional_dependencies = [ ]
native_libraries = [ ]

[inputs]
elf_path = "build/mod.elf"
mod_filename = "ph_bk_randomizer"
func_reference_syms_file = "BanjoRecompSyms/bk.us.rev0.syms.toml"
data_reference_syms_files = [ "BanjoRecompSyms/bk.us.rev0.datasyms.toml" ]
additional_files = [ ]

[[manifest.config_options]]
id = "enabled"
name = "Randomizer"
description = "Liga o randomizer. Desligado, o jogo e os saves são os vanilla."
type = "Enum"
options = [ "Desligado", "Ligado" ]
default = "Ligado"

[[manifest.config_options]]
id = "seed"
name = "Semente"
description = "Qualquer texto (letras, números, espaço e _). A mesma semente com as mesmas opções gera o mesmo jogo."
type = "String"
default = "BANJO"

[[manifest.config_options]]
id = "share_code"
name = "Código de compartilhamento"
description = "Opcional. Cole um código BKR1-XXXXXX-SEMENTE para reproduzir o seed de outra pessoa; ele substitui as opções abaixo."
type = "String"
default = ""

[[manifest.config_options]]
id = "logic"
name = "Lógica"
description = "Glitchless garante que tudo é alcançável sem glitches. Sem lógica pode gerar seeds impossíveis."
type = "Enum"
options = [ "Glitchless", "Sem lógica" ]
default = "Glitchless"

[[manifest.config_options]]
id = "shuffle"
name = "Coletáveis"
description = "Vanilla: não mexe. Por tipo: jiggies entre jiggies, favos entre favos, tokens entre tokens. Misturado: tudo entre si."
type = "Enum"
options = [ "Vanilla", "Por tipo", "Misturado" ]
default = "Misturado"

[[manifest.config_options]]
id = "scope"
name = "Escopo"
description = "Jogo inteiro: itens podem ir para qualquer mundo. Por mundo: cada item fica no mundo de origem."
type = "Enum"
options = [ "Jogo inteiro", "Por mundo" ]
default = "Jogo inteiro"

[[manifest.config_options]]
id = "moves"
name = "Movimentos do Bottles"
description = "Entre molehills: os 9 movimentos dos mundos trocam de molehill. Os de Spiral Mountain continuam vanilla."
type = "Enum"
options = [ "Vanilla", "Entre molehills" ]
default = "Entre molehills"

[[manifest.config_options]]
id = "access"
name = "Acessibilidade"
description = "Todas as localizações: 100% alcançável. Apenas vencível: só garante derrotar Gruntilda."
type = "Enum"
options = [ "Todas as localizações", "Apenas vencível" ]
default = "Todas as localizações"

[[manifest.config_options]]
id = "language"
name = "Idioma dos avisos"
description = "Idioma dos avisos do randomizer na tela."
type = "Enum"
options = [ "Português (Brasil)", "English" ]
default = "Português (Brasil)"

[[manifest.config_options]]
id = "toasts"
name = "Avisos de item"
description = "Mostra na tela qual item foi recebido quando ele difere do coletável vanilla."
type = "Enum"
options = [ "Desligado", "Ligado" ]
default = "Ligado"

[[manifest.config_options]]
id = "debug_log"
name = "Log de depuração"
description = "Escreve código, impressão digital e hash do placement no console (abra o jogo com --show-console)."
type = "Enum"
options = [ "Desligado", "Ligado" ]
default = "Desligado"
```

- [ ] **Passo 3: Declarações compartilhadas e inicialização**

As funções do jogo usadas pelo mod existem no decomp mas não nos headers públicos dele; as assinaturas foram copiadas das definições. `moleTable` é um dado do overlay core2 e o RecompModTool resolve o símbolo pelos arquivos do BanjoRecompSyms.

Arquivo `src/bkr_mod.h` (criar):

```c
/*
 * bkr_mod.h — Declarações compartilhadas do mod (lado MIPS).
 * As assinaturas do jogo foram copiadas das definições do decomp
 * (gitlab.com/banjo.decomp/banjo-kazooie @ 351ca158) porque os headers
 * públicos do decomp não as declaram.
 */
#ifndef BKR_MOD_H
#define BKR_MOD_H

#include "modding.h"
#include "recomputils.h"
#include "recompconfig.h"
#include "functions.h"

#include "bkr/bkr_generate.h"
#include "bkr/bkr_ids.h"

/* ---- Funções do jogo (definidas no decomp, ausentes nos headers) ---- */
u32 jiggyscore_isCollected(enum jiggy_e jiggy_id);
bool honeycombscore_get(enum honeycomb_e indx);
s32 honeycombscore_get_level_total(enum level_e level_id);
bool mumboscore_get(enum mumbotoken_e indx);
s32 item_adjustByDiff(enum item_e item, s32 diff, s32 no_hud);
s32 item_adjustByDiffWithHud(enum item_e item, s32 diff);      /* patch em mod_collect.c */
void item_adjustByDiffWithoutHud(enum item_e item, s32 diff);  /* patch em mod_collect.c */
s32 item_getCount(enum item_e item);
void item_inc(enum item_e item);
void item_set(s32 item, s32 val);
int ability_hasLearned(enum ability_e ability);
enum map_e map_get(void);
s32 getGameMode(void);
void gcpausemenu_80314AC8(int arg0);
void func_80347958(void);
void chmole_setStaticCamera(Actor *this);

/* moleTable (src/core2/ch/mole.c): 10 entradas de 6 bytes no overlay core2. */
typedef struct BkrMoleDescription {
    s16 teach_text_id;
    s16 refresher_text_id;
    s8 camera_node;
    s8 ability;
} BkrMoleDescription;
extern BkrMoleDescription moleTable[10];

/* Exportado pelo Banjo: Recompiled (patches/mumbo_token_patches.c). */
RECOMP_IMPORT("*", void bkrecomp_set_mumbo_token_fixes_enabled(int val));

/* ---- Estado do mod ---- */
typedef struct BkrModState {
    int enabled;          /* opção "Randomizer" ligada */
    int active;           /* seed gerado e save trocado */
    int error;            /* opção ligada mas configuração inválida */
    int lang;             /* BKR_LANG_* */
    int toasts;           /* mostrar avisos de item */
    int debug;            /* registrar no console (--show-console) */
    BkrSeedSpec spec;
    BkrGenResult result;
    char share_code[BKR_SHARE_CODE_MAX];
} BkrModState;

extern BkrModState g_bkr;

/* mod_init.c */
int bkr_rando_active(void);

/* mod_collect.c */
void bkr_suppress_clear(void);

/* mod_grant.c */
void bkr_grant_item(bkr_u8 item);
bkr_u32 bkr_count_received(bkr_u8 item);

/* mod_mole.c */
void bkr_mole_apply(const bkr_u8* placement);
void bkr_mole_tick(void);

/* mod_ui.c */
void bkr_ui_tick(void);
void bkr_ui_toast_item(bkr_u8 item);
void bkr_ui_toast_text(const char* text);

#endif
```

Arquivo `src/mod_init.c` (criar):

```c
/*
 * mod_init.c — Inicialização: lê as opções do mod, gera o seed com o núcleo
 * portátil, troca o arquivo de save e aplica os patches de dados.
 * Roda no evento recomp_on_init (dummy_func_8025AFB0, logo após carregar o
 * core2 e antes de qualquer leitura de save).
 */
#include "bkr_mod.h"
#include "bkr/bkr_text.h"

BkrModState g_bkr;

int bkr_rando_active(void) {
    return g_bkr.active;
}

static void bkr_read_seed_option(const char* key, char* out, bkr_u32 cap) {
    char* text = recomp_get_config_string(key);
    bkr_u32 i = 0;
    out[0] = '\0';
    if (text == NULL) {
        return;
    }
    while (text[i] != '\0' && i + 1 < cap) {
        out[i] = text[i];
        i++;
    }
    out[i] = '\0';
    recomp_free_config_string(text);
}

static void bkr_read_config(void) {
    static char raw_seed[64];
    static char raw_code[64];
    BkrSettings* s = &g_bkr.spec.settings;
    g_bkr.enabled = recomp_get_config_u32("enabled") == 1;
    g_bkr.lang = recomp_get_config_u32("language") == 1 ? BKR_LANG_EN : BKR_LANG_PT_BR;
    g_bkr.toasts = recomp_get_config_u32("toasts") == 1;
    g_bkr.debug = recomp_get_config_u32("debug_log") == 1;
    s->logic = (bkr_u8)recomp_get_config_u32("logic");
    s->shuffle = (bkr_u8)recomp_get_config_u32("shuffle");
    s->scope = (bkr_u8)recomp_get_config_u32("scope");
    s->moves = (bkr_u8)recomp_get_config_u32("moves");
    s->access = (bkr_u8)recomp_get_config_u32("access");
    s->pad[0] = s->pad[1] = s->pad[2] = 0;
    bkr_read_seed_option("seed", raw_seed, sizeof(raw_seed));
    bkr_seed_sanitize(raw_seed, g_bkr.spec.seed);
    bkr_read_seed_option("share_code", raw_code, sizeof(raw_code));
    if (raw_code[0] != '\0' && !bkr_share_code_parse(raw_code, &g_bkr.spec)) {
        g_bkr.error = 1;
    }
}

static void bkr_save_name(bkr_u32 fingerprint, char* out, bkr_u32 cap) {
    BkrText t;
    bkr_text_init(&t, out, cap);
    bkr_text_put(&t, "bkr_");
    bkr_text_hex(&t, fingerprint, 8);
}

RECOMP_CALLBACK("*", recomp_on_init) void bkr_on_init(void) {
    char save_name[16];
    g_bkr.active = 0;
    g_bkr.error = 0;
    bkr_read_config();
    if (!g_bkr.enabled) {
        return;
    }
    if (g_bkr.error) {
        recomp_printf("[bkr] codigo de compartilhamento invalido; randomizer desligado\n");
        return;
    }
    if (g_bkr.spec.seed[0] == '\0' || !bkr_generate(&g_bkr.spec, &g_bkr.result)) {
        g_bkr.error = 1;
        recomp_printf("[bkr] seed invalido ou geracao falhou; randomizer desligado\n");
        return;
    }
    bkr_share_code_format(&g_bkr.spec, g_bkr.share_code, sizeof(g_bkr.share_code));
    /* IDs únicos para os tokens que o jogo original duplicava (0x3D e 0x5E). */
    bkrecomp_set_mumbo_token_fixes_enabled(1);
    bkr_save_name(g_bkr.result.fingerprint, save_name, sizeof(save_name));
    recomp_change_save_file(save_name);
    bkr_mole_apply(g_bkr.result.placement);
    g_bkr.active = 1;
    if (g_bkr.debug) {
        recomp_printf("[bkr] codigo=%s fingerprint=%08X placement_hash=%08X tentativas=%u save=%s\n",
                      g_bkr.share_code, g_bkr.result.fingerprint,
                      bkr_placement_hash(g_bkr.result.placement), g_bkr.result.attempts, save_name);
    }
}
```

- [ ] **Passo 4: Compilar**

**Rodar:**

```bash
make
```

**Esperado:**

```text
-o build/mod.elf
```

O link usa `--unresolved-symbols=ignore-all` (do template): funções do jogo e funções das próximas Tasks ficam para o RecompModTool resolver. Por isso o `make nrm` só passa na Task 22, quando todos os arquivos existirem.

- [ ] **Passo 5: Commit**

**Rodar:**

```bash
git add Makefile mod.toml src/bkr_mod.h src/mod_init.c
git commit -q -m "adiciona build do mod, manifesto com opções e inicialização"
```

### Task 20: Coleta, supressão e entrega de itens

**Arquivos:**
- Criar: `src/mod_collect.c`, `src/mod_grant.c`

O desenho está em A10 (itens 1 a 4). Pontos que exigem cuidado:
- o hook só age se a localização ainda não foi coletada (`!jiggyscore_isCollected(id)` etc.), porque roda antes da função original;
- a entrega acontece antes de armar a supressão, então nunca é suprimida, e o item entregue nunca é o mesmo contador da supressão;
- `honeycombscore_get_total` mantém o corpo original quando o randomizer está desligado;
- `RECOMP_HOOK_RETURN("saveData_load")` recalcula a vida porque `saveData_load` lê os favos antes dos tokens (`src/core2/code_B5040.c:353-366`).

- [ ] **Passo 1: Hooks de coleta e supressão**

Arquivo `src/mod_collect.c` (criar):

```c
/*
 * mod_collect.c — Detecta a coleta de jiggies, favos vazios e tokens e troca
 * o item recebido pelo item sorteado para a localização.
 *
 * Fluxo vanilla, tudo dentro da mesma chamada (src/core2/ba/marker.c e src/core2/code_59A80.c):
 *   jiggy:  jiggyscore_setCollected(id,1) -> item_adjustByDiffWithoutHud(ITEM_26_JIGGY_TOTAL, 1)
 *   favo:   honeycombscore_set(id,1)      -> ... -> item_inc(ITEM_13_EMPTY_HONEYCOMB)
 *   token:  mumboscore_set(id,1)          -> ... -> item_inc(ITEM_1C_MUMBO_TOKEN)
 * O hook roda antes da função marcada. Ele entrega o item sorteado na hora (o bit
 * vanilla de "localização checada" e o item ficam consistentes no mesmo quadro) e
 * arma a supressão do próximo incremento do contador vanilla, que vem logo depois.
 */
#include "bkr_mod.h"

typedef struct BkrSuppress {
    int armed;
    s32 item;
} BkrSuppress;

static BkrSuppress g_suppress;

void bkr_suppress_clear(void) {
    g_suppress.armed = 0;
}

static int bkr_suppress_take(s32 item, s32 diff) {
    if (g_suppress.armed && g_suppress.item == item && diff == 1) {
        g_suppress.armed = 0;
        return 1;
    }
    return 0;
}

/* Corpo original: item_adjustByDiff(item, diff, 0) (src/core2/code_BEF20.c). */
RECOMP_PATCH s32 item_adjustByDiffWithHud(enum item_e item, s32 diff) {
    if (bkr_suppress_take(item, diff)) {
        return item_getCount(item);
    }
    return item_adjustByDiff(item, diff, 0);
}

/* Corpo original: item_adjustByDiff(item, diff, 1) (src/core2/code_BEF20.c). */
RECOMP_PATCH void item_adjustByDiffWithoutHud(enum item_e item, s32 diff) {
    if (bkr_suppress_take(item, diff)) {
        return;
    }
    item_adjustByDiff(item, diff, 1);
}

static void bkr_location_checked(bkr_u16 loc, s32 vanilla_counter_item) {
    bkr_u8 item;
    if (!bkr_rando_active() || loc == BKR_LOC_INVALID || getGameMode() != GAME_MODE_3_NORMAL) {
        return;
    }
    item = g_bkr.result.placement[loc];
    if (item == bkr_locations[loc].vanilla_item) {
        return; /* o fluxo vanilla já entrega o item certo */
    }
    /* Entrega antes de armar a supressão: a entrega nunca é suprimida. */
    bkr_grant_item(item);
    g_suppress.armed = 1;
    g_suppress.item = vanilla_counter_item;
}

RECOMP_HOOK("jiggyscore_setCollected") void bkr_on_jiggy_set(s32 index, s32 value) {
    if (value && index > 0 && index <= 100 && !jiggyscore_isCollected(index)) {
        bkr_location_checked(bkr_loc_by_jiggy[index], ITEM_26_JIGGY_TOTAL);
    }
}

RECOMP_HOOK("honeycombscore_set") void bkr_on_honeycomb_set(s32 index, s32 value) {
    if (value && index > 0 && index <= 24 && !honeycombscore_get(index)) {
        bkr_location_checked(bkr_loc_by_honeycomb[index], ITEM_13_EMPTY_HONEYCOMB);
    }
}

RECOMP_HOOK("mumboscore_set") void bkr_on_token_set(s32 index, s32 value) {
    if (value && index > 0 && index < 128 && !mumboscore_get(index)) {
        bkr_location_checked(bkr_loc_by_token[index], ITEM_1C_MUMBO_TOKEN);
    }
}
```

- [ ] **Passo 2: Entrega e favos derivados**

Arquivo `src/mod_grant.c` (criar):

```c
/*
 * mod_grant.c — Entrega de itens e estado derivado.
 *
 * Princípio do estado derivado: nada extra é salvo. Jiggies para gastar (ITEM_26),
 * tokens (ITEM_1C) e movimentos já são persistidos pelo jogo; favos recebidos são
 * recontados a partir dos bits vanilla de "localização checada" + placement.
 * Por isso a entrega acontece no mesmo quadro em que o jogo marca a localização:
 * não existe janela em que o bit esteja salvo e o item não.
 */
#include "bkr_mod.h"

void bkr_grant_item(bkr_u8 item) {
    if (BKR_ITEM_IS_MOVE(item)) {
        ability_unlock((enum ability_e)BKR_ITEM_ABILITY(item));
    } else if (item == BKR_ITEM_JIGGY) {
        item_adjustByDiffWithoutHud(ITEM_26_JIGGY_TOTAL, 1);
    } else if (item == BKR_ITEM_TOKEN) {
        item_inc(ITEM_1C_MUMBO_TOKEN);
    } else if (item == BKR_ITEM_HONEYCOMB) {
        item_inc(ITEM_13_EMPTY_HONEYCOMB);
        /* Igual ao vanilla (marker.c): trava a pausa até a animação de vida
         * extra, que depois chama gcpausemenu_80314AC8(1). */
        if (!(item_getCount(ITEM_13_EMPTY_HONEYCOMB) < 6)) {
            gcpausemenu_80314AC8(0);
        }
    } else {
        return;
    }
    bkr_ui_toast_item(item);
}

static int bkr_location_checked_in_save(bkr_u32 loc) {
    const BkrLocDef* d = &bkr_locations[loc];
    bkr_u8 item = g_bkr.result.placement[loc];
    switch (d->kind) {
        case BKR_LOC_JIGGY:
            return jiggyscore_isCollected(d->game_id) != 0;
        case BKR_LOC_HONEYCOMB:
            return honeycombscore_get(d->game_id) != 0;
        case BKR_LOC_TOKEN:
            return mumboscore_get(d->game_id) != 0;
        case BKR_LOC_MOLEHILL:
            /* Modo "entre molehills": molehill checado <=> movimento sorteado aprendido. */
            return BKR_ITEM_IS_MOVE(item) && ability_hasLearned((enum ability_e)BKR_ITEM_ABILITY(item));
        default:
            return 0;
    }
}

bkr_u32 bkr_count_received(bkr_u8 item) {
    bkr_u32 i;
    bkr_u32 count = 0;
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        if (g_bkr.result.placement[i] == item && bkr_location_checked_in_save(i)) {
            count++;
        }
    }
    return count;
}

/* Corpo original (src/core2/honeycombscore.c) mantido para o modo desligado.
 * Usado por func_8034789C (vida máxima, a cada carga de save e troca de mapa)
 * e pelo total de favos do menu de pausa. */
RECOMP_PATCH s32 honeycombscore_get_total(void) {
    int i;
    s32 total;
    if (bkr_rando_active()) {
        return (s32)bkr_count_received(BKR_ITEM_HONEYCOMB);
    }
    for (i = 0, total = 0; i < 0xC; i++) {
        total += honeycombscore_get_level_total(i);
    }
    return total;
}

/* saveData_load carrega os favos ANTES dos tokens, então a vida calculada
 * no meio do carregamento ficaria errada; recalcula no fim. */
RECOMP_HOOK_RETURN("saveData_load") void bkr_after_save_load(void) {
    if (bkr_rando_active()) {
        func_80347958();
    }
}
```

- [ ] **Passo 3: Compilar**

**Rodar:**

```bash
make
```

**Esperado:**

```text
-o build/mod.elf
```

- [ ] **Passo 4: Commit**

**Rodar:**

```bash
git add src/mod_collect.c src/mod_grant.c
git commit -q -m "adiciona hooks de coleta, supressão e entrega de itens"
```

### Task 21: Movimentos nos molehills

**Arquivos:**
- Criar: `src/mod_mole.c`

Cada molehill passa a ensinar o movimento sorteado com o texto desse movimento e o nó de câmera do próprio molehill. Os brindes embutidos no diálogo (50 ovos, 25 penas vermelhas, 5 penas douradas) acompanham o texto ensinado. A tabela é conferida a cada quadro e reaplicada se algo a restaurar.

- [ ] **Passo 1: Implementar**

Arquivo `src/mod_mole.c` (criar):

```c
/*
 * mod_mole.c — Movimentos embaralhados entre os 9 molehills dos mundos.
 *
 * Em vez de reescrever o ator do Bottles, reescrevemos a tabela moleTable
 * (overlay core2, residente): cada molehill passa a ensinar o movimento
 * sorteado com o texto de ensino/lembrete DESSE movimento, mantendo o nó de
 * câmera do próprio molehill. Toda a lógica vanilla (aprender vs. lembrete,
 * ovos/penas grátis embutidos no diálogo, plataformas de salto/voo) continua.
 */
#include "bkr_mod.h"

BKR_STATIC_ASSERT(mole_desc_size, sizeof(BkrMoleDescription) == 6);

static BkrMoleDescription g_mole_vanilla[9];
static BkrMoleDescription g_mole_wanted[9];
static int g_mole_ready;

static void bkr_mole_copy(BkrMoleDescription* dst, const BkrMoleDescription* src) {
    dst->teach_text_id = src->teach_text_id;
    dst->refresher_text_id = src->refresher_text_id;
    dst->camera_node = src->camera_node;
    dst->ability = src->ability;
}

static int bkr_mole_same(const BkrMoleDescription* a, const BkrMoleDescription* b) {
    return a->teach_text_id == b->teach_text_id && a->refresher_text_id == b->refresher_text_id &&
           a->camera_node == b->camera_node && a->ability == b->ability;
}

void bkr_mole_apply(const bkr_u8* placement) {
    int i;
    if (!g_mole_ready) {
        for (i = 0; i < 9; i++) {
            bkr_mole_copy(&g_mole_vanilla[i], &moleTable[i]);
        }
    }
    for (i = 0; i < 9; i++) {
        bkr_u8 item = placement[bkr_loc_by_mole[i]];
        int src = i;
        int j;
        if (BKR_ITEM_IS_MOVE(item)) {
            for (j = 0; j < 9; j++) {
                if (g_mole_vanilla[j].ability == BKR_ITEM_ABILITY(item)) {
                    src = j;
                }
            }
        }
        bkr_mole_copy(&g_mole_wanted[i], &g_mole_vanilla[src]);
        g_mole_wanted[i].camera_node = g_mole_vanilla[i].camera_node;
        bkr_mole_copy(&moleTable[i], &g_mole_wanted[i]);
    }
    g_mole_ready = 1;
}

/* Chamada a cada quadro: se algo restaurar a tabela (por exemplo, uma recarga do
 * overlay core2), reaplica. Custa 9 comparações. */
void bkr_mole_tick(void) {
    int i;
    if (!g_mole_ready) {
        return;
    }
    for (i = 0; i < 9; i++) {
        if (!bkr_mole_same(&moleTable[i], &g_mole_wanted[i])) {
            bkr_mole_copy(&moleTable[i], &g_mole_wanted[i]);
        }
    }
}

/*
 * Corpo original: src/core2/ch/mole.c. Os casos 1–4 apontam a câmera para as
 * botas (BGS, nó 0x11) e os tênis (GV, nó 0x29). Quando esses textos são
 * ensinados em outro molehill, o nó não existe no mapa: pulamos a câmera e a
 * flag de nível, mantendo o restante do comportamento.
 */
RECOMP_PATCH void chmole_additionalAbilityLearnActions(ActorMarker *marker, enum asset_e arg1, s32 arg2) {
    Actor *actor = marker_getActor(marker);
    int mole_index = actor->actorTypeSpecificField - 9;
    int boots_here = !bkr_rando_active() || mole_index == 7;
    int trainers_here = !bkr_rando_active() || mole_index == 8;
    switch (arg2) {
        case 1: /* Botas pernaltas */
            if (boots_here) {
                timed_setStaticCameraToNode(0.0f, 0x11);
                levelSpecificFlags_set(LEVEL_FLAG_1A_UNKNOWN, TRUE);
            }
            break;
        case 2:
            if (boots_here) {
                levelSpecificFlags_set(LEVEL_FLAG_1A_UNKNOWN, FALSE);
            }
            chmole_setStaticCamera(actor);
            break;
        case 3: /* Tênis turbo */
            if (trainers_here) {
                timed_setStaticCameraToNode(0.0f, 0x29);
                levelSpecificFlags_set(LEVEL_FLAG_1A_UNKNOWN, TRUE);
            }
            break;
        case 4:
            if (trainers_here) {
                levelSpecificFlags_set(LEVEL_FLAG_1A_UNKNOWN, FALSE);
            }
            chmole_setStaticCamera(actor);
            break;
        case 5: /* Disparo de ovos */
            item_adjustByDiffWithHud(ITEM_D_EGGS, 50);
            break;
        case 6: /* Voo */
            item_adjustByDiffWithHud(ITEM_F_RED_FEATHER, 25);
            break;
        case 7: /* Asa maravilha */
            item_adjustByDiffWithHud(ITEM_10_GOLD_FEATHER, 5);
            break;
        case 8: /* Recarga de vida */
            item_set(ITEM_14_HEALTH, item_getCount(ITEM_15_HEALTH_TOTAL));
            break;
        case 0xff:
            chmole_setStaticCamera(actor);
            break;
    }
}
```

- [ ] **Passo 2: Compilar**

**Rodar:**

```bash
make
```

**Esperado:**

```text
-o build/mod.elf
```

- [ ] **Passo 3: Commit**

**Rodar:**

```bash
git add src/mod_mole.c
git commit -q -m "adiciona movimentos sorteados nos molehills"
```

### Task 22: Interface, pulso por quadro e pacote `.nrm`

**Arquivos:**
- Criar: `src/mod_ui.c`, `src/mod_tick.c`

- [ ] **Passo 1: Interface (RecompUI)**

Arquivo `src/mod_ui.c` (criar):

```c
/*
 * mod_ui.c — Avisos de item recebido e código do seed na seleção de arquivo,
 * via RecompUI. Regras da API (RecompFrontend/recompui/src/core/ui_context.cpp):
 * toda criação/alteração de elemento ocorre entre recompui_open_context e
 * recompui_close_context; abrir duas vezes é erro fatal.
 */
#include "bkr_mod.h"
#include "recompui.h"
#include "bkr/bkr_text.h"

#define BKR_TOAST_FRAMES 180

static RecompuiContext g_ctx;
static RecompuiResource g_toast;
static RecompuiResource g_code;
static int g_ready;
static int g_toast_frames;
static int g_code_visible;
static char g_line[96];

static void bkr_ui_create(void) {
    static const RecompuiColor panel = { 0, 0, 0, 200 };
    static const RecompuiColor text = { 255, 230, 120, 255 };
    RecompuiResource root;
    g_ctx = recompui_create_context();
    recompui_set_context_captures_input(g_ctx, 0);
    recompui_set_context_captures_mouse(g_ctx, 0);
    recompui_open_context(g_ctx);
    root = recompui_context_root(g_ctx);
    recompui_set_position(root, POSITION_ABSOLUTE);
    recompui_set_top(root, 0, UNIT_DP);
    recompui_set_left(root, 0, UNIT_DP);
    recompui_set_right(root, 0, UNIT_DP);
    recompui_set_bottom(root, 0, UNIT_DP);
    recompui_set_display(root, DISPLAY_FLEX);
    recompui_set_flex_direction(root, FLEX_DIRECTION_COLUMN);
    recompui_set_justify_content(root, JUSTIFY_CONTENT_SPACE_BETWEEN);
    recompui_set_align_items(root, ALIGN_ITEMS_CENTER);
    recompui_set_padding(root, 32.0f, UNIT_DP);

    g_code = recompui_create_label(g_ctx, root, " ", LABELSTYLE_NORMAL);
    recompui_set_background_color(g_code, &panel);
    recompui_set_color(g_code, &text);
    recompui_set_padding(g_code, 12.0f, UNIT_DP);
    recompui_set_border_radius(g_code, 12.0f, UNIT_DP);
    recompui_set_display(g_code, DISPLAY_NONE);

    g_toast = recompui_create_label(g_ctx, root, " ", LABELSTYLE_LARGE);
    recompui_set_background_color(g_toast, &panel);
    recompui_set_color(g_toast, &text);
    recompui_set_padding(g_toast, 16.0f, UNIT_DP);
    recompui_set_border_radius(g_toast, 16.0f, UNIT_DP);
    recompui_set_display(g_toast, DISPLAY_NONE);
    recompui_close_context(g_ctx);
    recompui_show_context(g_ctx);
    g_ready = 1;
}

void bkr_ui_toast_text(const char* text) {
    if (!g_ready || !g_bkr.toasts) {
        return;
    }
    recompui_open_context(g_ctx);
    recompui_set_text(g_toast, text);
    recompui_set_display(g_toast, DISPLAY_BLOCK);
    recompui_close_context(g_ctx);
    g_toast_frames = BKR_TOAST_FRAMES;
}

void bkr_ui_toast_item(bkr_u8 item) {
    BkrText t;
    bkr_text_init(&t, g_line, sizeof(g_line));
    bkr_text_put(&t, g_bkr.lang == BKR_LANG_EN ? "You got: " : "Voce recebeu: ");
    bkr_text_put(&t, bkr_item_name(item, g_bkr.lang));
    bkr_ui_toast_text(g_line);
}

static void bkr_ui_update_code(int want_visible) {
    BkrText t;
    bkr_u32 i;
    if (want_visible == g_code_visible) {
        return;
    }
    recompui_open_context(g_ctx);
    if (want_visible) {
        bkr_text_init(&t, g_line, sizeof(g_line));
        if (g_bkr.active) {
            bkr_text_put(&t, g_bkr.share_code);
            bkr_text_put(&t, "  |  ");
            for (i = 0; i < 5; i++) {
                bkr_text_put(&t, bkr_hash_word(g_bkr.result.fingerprint >> (i * 5)));
                if (i < 4) {
                    bkr_text_putc(&t, ' ');
                }
            }
        } else {
            bkr_text_put(&t, g_bkr.lang == BKR_LANG_EN ? "Randomizer: invalid seed or code (see mod options)"
                                                        : "Randomizer: seed ou codigo invalido (veja as opcoes do mod)");
        }
        recompui_set_text(g_code, g_line);
        recompui_set_display(g_code, DISPLAY_BLOCK);
    } else {
        recompui_set_display(g_code, DISPLAY_NONE);
    }
    recompui_close_context(g_ctx);
    g_code_visible = want_visible;
}

void bkr_ui_tick(void) {
    if (!g_bkr.enabled) {
        return;
    }
    if (!g_ready) {
        bkr_ui_create();
    }
    bkr_ui_update_code(map_get() == MAP_91_FILE_SELECT);
    if (g_toast_frames > 0) {
        g_toast_frames--;
        if (g_toast_frames == 0) {
            recompui_open_context(g_ctx);
            recompui_set_display(g_toast, DISPLAY_NONE);
            recompui_close_context(g_ctx);
        }
    }
}
```

- [ ] **Passo 2: Pulso por quadro**

Arquivo `src/mod_tick.c` (criar):

```c
/*
 * mod_tick.c — Pulso por quadro: mainLoop (src/core1/code_0.c) roda uma vez
 * por quadro e não é alterado pelo Banjo: Recompiled.
 */
#include "bkr_mod.h"

RECOMP_HOOK("mainLoop") void bkr_on_main_loop(void) {
    /* Segurança: a supressão é de uso imediato; nunca sobrevive a um quadro. */
    bkr_suppress_clear();
    if (bkr_rando_active()) {
        bkr_mole_tick();
    }
    bkr_ui_tick();
}
```

- [ ] **Passo 3: Gerar o `.nrm`** (o RecompModTool valida os símbolos do jogo e os alvos de hook e patch)

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

Se o RecompModTool reclamar de uma função "marked as a patch but not existing in the original ROM", o nome não existe no BanjoRecompSyms `6820055`: confira a grafia no decomp `351ca15`.

- [ ] **Passo 4: Verificação completa**

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

- [ ] **Passo 5: Commit**

**Rodar:**

```bash
git add src/mod_ui.c src/mod_tick.c
git commit -q -m "adiciona interface e pulso por quadro do mod"
```

### Fase 5 — QA no jogo

### Task 23: Checklist de QA e rodada no jogo

**Arquivos:**
- Criar: `docs/qa-checklist.md`
- Criar depois da rodada: `docs/qa/0.1.0.md` (cópia do checklist com os resultados)

Esta é a única Task que precisa de você jogando. O Claude Code prepara tudo, guia item por item e registra o resultado; você instala, joga e informa o que viu.

- [ ] **Passo 1: Checklist**

Arquivo `docs/qa-checklist.md` (criar):

```markdown
# Checklist de QA no jogo

Os testes automáticos cobrem o gerador, a lógica e a equivalência PC/MIPS. Esta lista cobre o que só o jogo
mostra: ganchos, entrega de itens, UI e saves. Rode-a em cada versão antes de publicar e anote o resultado
(ok / falhou + observação) numa cópia deste arquivo em `docs/qa/<versão>.md`.

## Preparação
1. `make nrm` e instale `build/ph_bk_randomizer.nrm` (Mods → Install Mods).
2. Opções do mod: padrão, com **Log de depuração = Ligado**.
3. Abra o jogo com o console visível: no Windows, `BanjoRecompiled.exe --show-console`; no Linux e no macOS,
   rode o executável a partir de um terminal.
4. Gere o spoiler de referência: `core/build/bkrgen --seed BANJO --spoiler spoiler.txt`.

## A. Inicialização
- [ ] A1. O console mostra `[bkr] codigo=BKR1-GG000M-BANJO fingerprint=623A1517 placement_hash=31098721 tentativas=1 save=bkr_623A1517`, igual à saída do `bkrgen`.
- [ ] A2. A seleção de arquivo mostra `BKR1-GG000M-BANJO  |  Blubber Cheato Grunty Tiptup Bottles`.
- [ ] A3. Depois do primeiro save existe `saves/ph_bk_randomizer/bkr_623A1517.bin` na pasta de dados do jogo
      (Windows `%LOCALAPPDATA%\BanjoRecompiled`, Linux `~/.config/BanjoRecompiled`,
      macOS `~/Library/Application Support/BanjoRecompiled`).
- [ ] A4. Código de compartilhamento inválido (ex.: `BKR1-000000-X`): a seleção de arquivo mostra o aviso de
      seed inválido, o console mostra a falha e o jogo usa os saves normais.
- [ ] A5. Randomizer desligado: nenhum texto do mod na tela e os saves normais aparecem.

## B. Coleta e entrega
Use o spoiler para escolher as localizações (seção "Localizacoes por mundo").
- [ ] B1. Jiggy cujo item é um token: Banjo dança, o aviso "Voce recebeu: Token do Mumbo" aparece, o contador
      de tokens sobe 1 e os jiggies para gastar (pausa) não mudam.
- [ ] B2. Token cujo item é um jiggy: aviso de Jiggy; os jiggies para gastar sobem 1.
- [ ] B3. Favo cujo item é um jiggy ou token: o contador de favos não muda; o item sorteado chega.
- [ ] B4. Localização cujo item é igual ao original: nenhum aviso; comportamento vanilla.
- [ ] B5. Receber o 6º favo pela entrega (de uma localização que não é favo): toca a animação de vida extra, a vida
      máxima sobe 1 e, depois dela, o menu de pausa abre normalmente.
- [ ] B6. Coletar uma localização trocada e, logo depois, salvar e sair pelo menu de pausa: ao carregar, o item
      recebido continua lá (a entrega acontece no mesmo quadro da coleta).
- [ ] B7. Coletar uma localização trocada durante um pulo, sob dano ou logo antes de uma transição de mapa: o item
      chega, o contador fica certo depois da troca de mapa e o jogo não trava.

## C. Molehills
- [ ] C1. Os molehills de MM ensinam os movimentos do spoiler, com o texto do movimento ensinado.
- [ ] C2. Câmera: o Bottles aparece enquadrado; o texto das botas ou dos tênis ensinado fora de BGS/GV não
      leva a câmera para outro lugar.
- [ ] C3. Falar de novo com o mesmo molehill mostra o lembrete do movimento ensinado.
- [ ] C4. Ensinar Disparo de Ovos dá +50 ovos; Voo, +25 penas vermelhas; Asa Maravilha, +5 penas douradas.
- [ ] C5. Os pads de salto de mola e de voo aparecem nos mundos depois de aprender esses movimentos.
- [ ] C6. Voltar à seleção de arquivo e carregar de novo: o molehill continua ensinando o movimento sorteado.

## D. Persistência
- [ ] D1. Salvar, voltar à seleção de arquivo e carregar: jiggies para gastar, tokens, movimentos e localizações
      coletadas continuam iguais.
- [ ] D2. A vida máxima depois de carregar é 5 + favos recebidos ÷ 6 (no máximo 8; 16 com vida dupla).
- [ ] D3. Localizações coletadas não reaparecem e não entregam o item de novo.
- [ ] D4. Fechar o jogo, reabrir com outro seed e depois voltar ao primeiro: cada seed usa o próprio save.

## E. Gastos
- [ ] E1. Preencher o quadro de MM com jiggies recebidos abre o mundo.
- [ ] E2. Pagar uma transformação do Mumbo com tokens recebidos funciona e desconta o valor certo.

## F. Progressão
- [ ] F1. Seguir as esferas do spoiler até a esfera 4 sem precisar de nada fora da lógica.
- [ ] F2. (Antes da 1.0) Uma partida completa até derrotar Gruntilda em um seed com as opções padrão.

## G. Desempenho e estabilidade
- [ ] G1. A geração no início do jogo não causa atraso perceptível (no PC ela leva ~10 ms).
- [ ] G2. 30 minutos de jogo sem travar, sem avisos repetidos no console.

## Quando algo falha
Anote: código do seed, versão do mod e do jogo, passo exato, saída do console e o save usado. Investigue com
`superpowers:systematic-debugging` antes de mudar código. Bug do Banjo: Recompiled vai para `docs/` como nota
para o dono do projeto avaliar; não abra issue upstream a partir de uma sessão com IA.
```

- [ ] **Passo 2: Commit do checklist**

**Rodar:**

```bash
git add docs/qa-checklist.md
git commit -q -m "adiciona checklist de QA no jogo"
```

- [ ] **Passo 3: Preparar a rodada**

**Rodar:**

```bash
make nrm
make -C core cli
./core/build/bkrgen --seed BANJO --spoiler spoiler.txt
```

**Esperado:**

```text
placement_hash=31098721
spoiler=spoiler.txt
```

Instale `build/ph_bk_randomizer.nrm` pelo menu **Mods → Install Mods** do Banjo: Recompiled, deixe as opções no padrão com **Log de depuração = Ligado** e abra o jogo com o console (Windows: `BanjoRecompiled.exe --show-console`; Linux e macOS: pelo terminal).

- [ ] **Passo 4: Rodar o checklist com o jogador**

Siga `docs/qa-checklist.md` na ordem (A → G), um item por vez: diga ao jogador o que fazer, onde (use o `spoiler.txt` para achar localizações trocadas) e o que observar; anote "ok" ou "falhou + observação". Um item falhou? Pare a rodada, colete o que a seção "Quando algo falha" pede e investigue com superpowers:systematic-debugging antes de mudar código. Depois de corrigir: `make check`, `make nrm`, reinstale e repita os itens afetados.

- [ ] **Passo 5: Registrar o resultado** (só depois da rodada)

Copie `docs/qa-checklist.md` para `docs/qa/0.1.0.md`, marque cada item com o resultado, acrescente no topo a versão do jogo, o sistema e a data, e faça o commit:

```bash
git add docs/qa/0.1.0.md
git commit -q -m "registra a rodada de QA da 0.1.0"
```

### Fase 6 — Empacotamento e publicação

### Task 24: Pacote da Thunderstore, integração e publicação

**Arquivos:**
- Criar: `tools/tests/test_package_thunderstore.py`, `tools/package_thunderstore.py`
- Modificar: `CHANGELOG.md` (data da versão)

- [ ] **Passo 1: Escrever os testes**

Arquivo `tools/tests/test_package_thunderstore.py` (criar):

```python
"""Testes do empacotamento para a Thunderstore (tools/package_thunderstore.py).

Rodar:  python3 -m unittest discover -s tools/tests -v
"""
import json
import struct
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import package_thunderstore as pt  # noqa: E402


class IconTest(unittest.TestCase):
    def test_icon_is_256_png(self):
        png = pt.make_icon()
        self.assertEqual(png[:8], b"\x89PNG\r\n\x1a\n")
        width, height, depth, color = struct.unpack(">IIBB", png[16:26])
        self.assertEqual((width, height, depth, color), (256, 256, 8, 2))
        self.assertEqual(png[-12:], b"\x00\x00\x00\x00IEND\xaeB`\x82")

    def test_icon_is_deterministic(self):
        self.assertEqual(pt.make_icon(), pt.make_icon())


class ManifestTest(unittest.TestCase):
    def test_version_from_mod_toml(self):
        text = '[inputs]\nversion = "9.9.9"\n[manifest]\nid = "x"\nversion = "1.2.3"\n'
        self.assertEqual(pt.mod_version(text), "1.2.3")
        self.assertEqual(pt.mod_version((ROOT / "mod.toml").read_text(encoding="utf-8")), "0.1.0")

    def test_manifest_fields(self):
        manifest = pt.build_manifest("0.1.0", "https://example.com/bk")
        self.assertEqual(manifest["name"], "BK_Randomizer")
        self.assertEqual(manifest["dependencies"], [])
        self.assertLessEqual(len(manifest["description"]), 250)
        with self.assertRaises(pt.PackageError):
            pt.build_manifest("0.1.0", "github.com/sem-esquema")


class PackageTest(unittest.TestCase):
    def test_package_layout_and_determinism(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            nrm = tmp / "ph_bk_randomizer.nrm"
            nrm.write_bytes(b"conteudo de teste")
            args = (nrm, "https://example.com/bk", ROOT / "README.md", ROOT / "CHANGELOG.md", ROOT / "mod.toml")
            first = pt.write_package(tmp / "a", *args)
            second = pt.write_package(tmp / "b", *args)
            self.assertEqual(first.name, "BK_Randomizer-0.1.0.zip")
            self.assertEqual(first.read_bytes(), second.read_bytes())
            with zipfile.ZipFile(first) as zf:
                self.assertEqual(zf.namelist(), ["ph_bk_randomizer.nrm", "icon.png", "manifest.json", "README.md",
                                                 "CHANGELOG.md"])
                self.assertEqual(json.loads(zf.read("manifest.json"))["version_number"], "0.1.0")

    def test_missing_nrm_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(pt.PackageError):
                pt.write_package(Path(tmp), Path(tmp) / "nao_existe.nrm", "https://example.com/bk",
                                 ROOT / "README.md", ROOT / "CHANGELOG.md", ROOT / "mod.toml")


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Passo 2: Ver falhar**

**Rodar:**

```bash
python3 -m unittest discover -s tools/tests -v
```

**Esperado (falha):**

```text
ModuleNotFoundError: No module named 'package_thunderstore'
```

- [ ] **Passo 3: Implementar** (o ícone 256×256 é gerado pelo próprio script: uma peça de quebra-cabeça com "?", arte original)

Arquivo `tools/package_thunderstore.py` (criar):

```python
#!/usr/bin/env python3
"""Monta o pacote do mod para a Thunderstore (comunidade Banjo: Recompiled).

Layout igual ao dos pacotes publicados nessa comunidade: na raiz do .zip ficam o
.nrm, icon.png (256x256), manifest.json, README.md e CHANGELOG.md. O ícone é
gerado aqui mesmo (arte original e simples: peça de quebra-cabeça com "?"), para
que o pacote não dependa de nenhum arquivo binário no repositório. O .zip é
determinístico: as mesmas entradas geram os mesmos bytes.

Uso:
    python3 tools/package_thunderstore.py --nrm build/ph_bk_randomizer.nrm \
        --website https://github.com/SEU_USUARIO/bk-randomizer --out dist
"""
import argparse
import json
import re
import struct
import sys
import zipfile
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PACKAGE_NAME = "BK_Randomizer"
DESCRIPTION = "Randomizer com lógica para Banjo: Recompiled: jiggies, favos, tokens e movimentos do Bottles."
ZIP_DATE = (2026, 1, 1, 0, 0, 0)
ICON_SIZE = 256

# "?" em 5x7; cada célula vira um bloco de 12x12 pixels.
QUESTION_MARK = [
    ".###.",
    "#...#",
    "....#",
    "...#.",
    "..#..",
    ".....",
    "..#..",
]


class PackageError(Exception):
    """Erro de empacotamento com mensagem legível."""


def mod_version(mod_toml_text):
    """Lê ``version`` da seção [manifest] do mod.toml (sem depender de tomllib)."""
    in_manifest = False
    for line in mod_toml_text.splitlines():
        stripped = line.strip()
        if stripped.startswith("["):
            in_manifest = stripped == "[manifest]"
            continue
        match = re.match(r'version\s*=\s*"(\d+\.\d+\.\d+)"', stripped)
        if in_manifest and match:
            return match.group(1)
    raise PackageError("version não encontrada na seção [manifest] do mod.toml")


def _icon_pixel(x, y):
    """Cor RGB do pixel (x, y) do ícone."""
    t = y / (ICON_SIZE - 1)
    background = (int(43 - 27 * t), int(27 + 0 * t), int(90 - 27 * t))
    # Peça: quadrado com cantos arredondados e dois "encaixes" redondos (topo e direita).
    left, top, right, bottom, radius = 60, 76, 188, 204, 18
    cx = min(max(x, left + radius), right - radius)
    cy = min(max(y, top + radius), bottom - radius)
    in_square = (x - cx) ** 2 + (y - cy) ** 2 <= radius ** 2 and left <= x <= right and top <= y <= bottom
    in_knob_top = (x - 124) ** 2 + (y - 70) ** 2 <= 24 ** 2
    in_knob_right = (x - 194) ** 2 + (y - 140) ** 2 <= 24 ** 2
    in_piece = in_square or in_knob_top or in_knob_right
    if not in_piece:
        return background
    # Borda escura de ~6 px: algum vizinho a 6 px está fora da peça.
    edge = False
    for dx, dy in ((6, 0), (-6, 0), (0, 6), (0, -6)):
        nx, ny = x + dx, y + dy
        ncx = min(max(nx, left + radius), right - radius)
        ncy = min(max(ny, top + radius), bottom - radius)
        n_square = (nx - ncx) ** 2 + (ny - ncy) ** 2 <= radius ** 2 and left <= nx <= right and top <= ny <= bottom
        n_piece = (n_square or (nx - 124) ** 2 + (ny - 70) ** 2 <= 24 ** 2
                   or (nx - 194) ** 2 + (ny - 140) ** 2 <= 24 ** 2)
        if not n_piece:
            edge = True
    if edge:
        return (168, 114, 26)
    gx, gy = (x - 94) // 12, (y - 98) // 12
    if 0 <= gx < 5 and 0 <= gy < 7 and x >= 94 and y >= 98 and QUESTION_MARK[gy][gx] == "#":
        return (43, 27, 90)
    return (242, 179, 42)


def make_icon():
    """PNG 256x256 RGB, gerado sem bibliotecas externas."""
    rows = []
    for y in range(ICON_SIZE):
        row = bytearray([0])  # filtro "None"
        for x in range(ICON_SIZE):
            row.extend(_icon_pixel(x, y))
        rows.append(bytes(row))
    raw = b"".join(rows)

    def chunk(kind, data):
        body = kind + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    header = struct.pack(">IIBBBBB", ICON_SIZE, ICON_SIZE, 8, 2, 0, 0, 0)
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(raw, 9))
            + chunk(b"IEND", b""))


def build_manifest(version, website):
    """manifest.json da Thunderstore."""
    if not re.match(r"^https?://", website):
        raise PackageError(f"--website precisa ser uma URL http(s): {website!r}")
    if len(DESCRIPTION) > 250:
        raise PackageError("a descrição da Thunderstore tem limite de 250 caracteres")
    return {
        "name": PACKAGE_NAME,
        "version_number": version,
        "website_url": website,
        "description": DESCRIPTION,
        "dependencies": [],
    }


def write_package(out_dir, nrm_path, website, readme, changelog, mod_toml):
    """Grava dist/BK_Randomizer-<versão>.zip e devolve o caminho."""
    nrm_path = Path(nrm_path)
    if not nrm_path.is_file():
        raise PackageError(f"{nrm_path} não existe; rode make nrm antes")
    version = mod_version(Path(mod_toml).read_text(encoding="utf-8"))
    manifest = build_manifest(version, website)
    entries = [
        (nrm_path.name, nrm_path.read_bytes()),
        ("icon.png", make_icon()),
        ("manifest.json", (json.dumps(manifest, indent=4, ensure_ascii=False) + "\n").encode("utf-8")),
        ("README.md", Path(readme).read_bytes()),
        ("CHANGELOG.md", Path(changelog).read_bytes()),
    ]
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path = out_dir / f"{PACKAGE_NAME}-{version}.zip"
    with zipfile.ZipFile(out_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for name, data in entries:
            info = zipfile.ZipInfo(name, date_time=ZIP_DATE)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            zf.writestr(info, data)
    return out_path


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--nrm", required=True, type=Path, help="arquivo .nrm gerado pelo make nrm")
    parser.add_argument("--website", required=True, help="URL pública do repositório do mod")
    parser.add_argument("--out", default=ROOT / "dist", type=Path, help="pasta de saída")
    args = parser.parse_args(argv)
    try:
        path = write_package(args.out, args.nrm, args.website, ROOT / "README.md", ROOT / "CHANGELOG.md",
                             ROOT / "mod.toml")
    except PackageError as exc:
        print(f"ERRO: {exc}", file=sys.stderr)
        return 1
    with zipfile.ZipFile(path) as zf:
        names = zf.namelist()
    print(f"pacote: {path.as_posix()}")
    print("conteúdo: " + ", ".join(names))
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Passo 4: Ver passar**

**Rodar:**

```bash
python3 -m unittest discover -s tools/tests -v
```

**Esperado:**

```text
Ran 32 tests
OK
```

- [ ] **Passo 5: Datar a versão no CHANGELOG e fazer o commit**

**Rodar:**

```bash
python3 -c "import datetime, pathlib; p = pathlib.Path('CHANGELOG.md'); p.write_text(p.read_text(encoding='utf-8').replace('## 0.1.0 — não publicada', '## 0.1.0 — ' + datetime.date.today().isoformat()), encoding='utf-8', newline='\n')"
grep -c "## 0.1.0 — 20" CHANGELOG.md
git add tools/tests/test_package_thunderstore.py tools/package_thunderstore.py CHANGELOG.md
git commit -q -m "adiciona empacotamento da Thunderstore e data a versão 0.1.0"
```

**Esperado:**

```text
1
```

- [ ] **Passo 6: Verificação final e pacote**

Troque `SEU_USUARIO` pelo seu usuário do GitHub (a URL vai no `manifest.json`).

**Rodar:**

```bash
make check
make package WEBSITE=https://github.com/SEU_USUARIO/bk-randomizer
```

**Esperado:**

```text
Ran 32 tests
30 testes, 637838 verificações, 0 falhas
mips-check: ok (nenhum símbolo externo)
hash MIPS=31098721 esperado=31098721 tempo=…s pilha=268 bytes
ok: nenhum conflito de RECOMP_PATCH
pacote: dist/BK_Randomizer-0.1.0.zip
conteúdo: ph_bk_randomizer.nrm, icon.png, manifest.json, README.md, CHANGELOG.md
```

- [ ] **Passo 7: Integrar o branch e marcar a versão** (é a opção "merge local" de superpowers:finishing-a-development-branch)

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

- [ ] **Passo 8: Publicar** (manual; o Claude Code prepara os textos e você confirma cada envio)

1. **GitHub:** crie o repositório `bk-randomizer` (público), depois:

```bash
git remote add origin https://github.com/SEU_USUARIO/bk-randomizer.git
git push -u origin main --tags
```

   Crie a release `v0.1.0` e anexe `build/ph_bk_randomizer.nrm` e `dist/BK_Randomizer-0.1.0.zip`. Na descrição, inclua o código de exemplo `BKR1-GG000M-BANJO`, as limitações do README e a frase sobre o uso de IA.
2. **Thunderstore:** entre em thunderstore.io, abra a comunidade **Banjo: Recompiled** (`thunderstore.io/c/banjo-recompiled`), escolha **Upload** e envie `dist/BK_Randomizer-0.1.0.zip`. O pacote segue o layout dos pacotes publicados nessa comunidade (`.nrm`, `icon.png`, `manifest.json` e `README.md` na raiz).
3. Não divulgue o mod nos canais oficiais do Banjo: Recompiled como se fosse parte do projeto; ele é independente (A3).

## Autorrevisão e checklist final

**Cobertura da especificação**

| Requisito (Parte A) | Onde é implementado | Onde é verificado |
|---|---|---|
| Lógica importada e corrigida (A8, A13) | Tasks 5–6 | testes Python; `all_items_reach_everything` (Task 12) |
| Geração determinística e vencível (A6, A9) | Tasks 9, 11–13 | `generate_default`, propriedades (Task 13), autoteste MIPS (Task 17) |
| Código de compartilhamento, hash e impressão digital (A12) | Task 11 | `share_code`, `fingerprint_changes`, referência Python (Task 9) |
| Spoiler e CLI (A12) | Tasks 14–15 | `spoiler_text`, saída da CLI |
| Entrega atômica e supressão (A6, A10) | Task 20 | QA B1–B7 (Task 23) |
| Favos derivados e vida (A10) | Task 20 | QA B5, D2 |
| Molehills (A10) | Task 21 | QA C1–C6 |
| Save por seed (A11) | Task 19 | QA A3, D1–D4 |
| Interface (A14) | Task 22 | QA A2, A4, B1 |
| Sem conflito com o base, política de IA (A3, A10) | Tasks 3, 18 | `make check` |
| Publicação (A1) | Task 24 | pacote validado pelos testes da Task 24 |

**Antes de chamar a v0.1.0 de pronta**
- [ ] `make check` verde (32 testes Python; 30 testes C com 0 falhas; `mips-check`; autoteste MIPS com `31098721`; nenhum conflito).
- [ ] `make nrm` gera `build/ph_bk_randomizer.nrm`.
- [ ] Checklist de QA completo e registrado em `docs/qa/0.1.0.md`, sem item "falhou" em aberto.
- [ ] CHANGELOG datado; tag `v0.1.0`; branch integrado em `main`.
- [ ] README declara o uso de IA; nada foi enviado aos repositórios do BanjoRecomp.

## Apêndices

### Referência rápida de comandos

| Comando | Faz |
|---|---|
| `make check` | tudo o que é automático (Python, núcleo, MIPS, conflitos) |
| `make nrm` | compila o mod e gera o `.nrm` |
| `make logic` | regenera as tabelas a partir de `logic/*.json` |
| `make -C core test TEST=nome` | roda só os testes cujo nome contém o trecho |
| `make -C core test BKR_PROPERTY_SEEDS=2000` | bateria longa de propriedades |
| `make -C core cli && ./core/build/bkrgen --seed X --spoiler s.txt` | seed e spoiler no PC |
| `make package WEBSITE=URL` | pacote da Thunderstore em `dist/` |

### Solução de problemas

| Sintoma | Causa provável | O que fazer |
|---|---|---|
| `clang: command not found` ou `ld.lld: command not found` | LLVM fora do `PATH` | reabra o terminal; no Windows confira `C:\Program Files\LLVM\bin` no `PATH` |
| erro de alvo `mips` desconhecido, ou código MIPS errado | clang da Apple ou LLVM 19.1.0 do Windows | use o LLVM 18 (Task 1) |
| `Failed to run "zip"` no `make nrm` | falta o utilitário `zip` (Linux/macOS) | instale `zip` |
| `python3: command not found` no Windows | o Python do Windows se chama `python` | troque nos comandos; os Makefiles já escolhem sozinhos |
| `ModuleNotFoundError: No module named 'unicorn'` | autoteste MIPS sem dependências | `python3 -m pip install --user unicorn pyelftools` |
| hash do `lighthouse_raw.json` diferente | Lighthouse em outro commit, ou CRLF | refaça a Task 4; confira o `.gitattributes` |
| `0x5C82A342`, `623A1517` ou `31098721` diferentes | mudança em `logic/`, no compilador ou no gerador | se não foi de propósito, é bug: `git diff` e superpowers:systematic-debugging |
| o mod não aparece ou não carrega no jogo | Banjo: Recompiled abaixo de 1.0.2, ou conflito de patch | atualize o jogo; rode `make check` |
| a seleção de arquivo mostra "seed ou codigo invalido" | código de compartilhamento mal digitado ou semente vazia | corrija nas opções do mod |
| a pausa parou de abrir | contador de pausa desequilibrado (QA B5) | troque de mapa (zera o contador) e registre o caso com o seed |

### Glossário
- **Localização (check):** lugar do jogo que entrega um item (um jiggy, um favo, um token, um molehill).
- **Placement:** o item sorteado para cada localização.
- **Preenchimento assumido (assumed fill):** coloca cada item de progressão onde ele é alcançável supondo que todos os outros ainda não colocados já foram coletados.
- **Esfera:** rodada de coleta no spoiler; tudo o que fica alcançável com os itens das esferas anteriores.
- **Hook / patch:** código do mod que roda antes (ou depois) de uma função do jogo / que substitui a função.
- **Estado derivado:** valor recalculado a partir do seed e dos dados que o jogo já salva, em vez de ser salvo à parte.
- **Valor-ouro:** resultado fixo esperado de um cálculo determinístico, usado para detectar mudanças.

### Fontes
- Banjo: Recompiled — <https://github.com/BanjoRecomp/BanjoRecomp> (README, BUILDING.md, CONTRIBUTING.md, `patches/`, releases)
- BKRecompModTemplate — <https://github.com/BanjoRecomp/BKRecompModTemplate>
- BanjoRecompSyms — <https://github.com/BanjoRecomp/BanjoRecompSyms>
- Documentação do sistema de mods (linkada pelo template) — <https://hackmd.io/fMDiGEJ9TBSjomuZZOgzNg>
- N64Recomp e RecompModTool — <https://github.com/N64Recomp/N64Recomp> (release `mod-tool-release`)
- N64ModernRuntime — <https://github.com/N64Recomp/N64ModernRuntime>
- RecompFrontend — <https://github.com/N64Recomp/RecompFrontend>
- Decomp de Banjo-Kazooie — <https://gitlab.com/banjo.decomp/banjo-kazooie>
- OoT Randomizer — <https://github.com/OoTRandomizer/OoT-Randomizer>
- Lighthouse (HarbourMasters) — <https://github.com/HarbourMasters/Lighthouse>
- Archipelago-BK — <https://github.com/RecompRando/archipelago-bk>
- MMRecompRando — <https://github.com/RecompRando/MMRecompRando>
- Comunidade Banjo: Recompiled na Thunderstore — <https://thunderstore.io/c/banjo-recompiled/>
- xoshiro/xoroshiro (Blackman e Vigna) — <https://prng.di.unimi.it/>
