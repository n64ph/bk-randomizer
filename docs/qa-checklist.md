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
