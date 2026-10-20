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
