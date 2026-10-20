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
