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
