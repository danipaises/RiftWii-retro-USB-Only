# v0.1.0-alpha.4 — biblioteca e filtros (experimental)

Repositório: https://github.com/danipaises/RiftWii-retro-USB-Only

## Correção encontrada

Na alpha.3, HomeRows filtrava entradas por `is_emulator(app.path)`. Uma ROM usa o mesmo executável de seu emulador, portanto o filtro também removia todas as ROMs. O scanner podia encontrar os arquivos sem que aparecessem nos jogos. A alpha.4 classifica o item pelo caminho da ROM (`retro_rom`), sem confundir o jogo com o executável. Testes cobrem a passagem scanner → catálogo → filtro com esse compartilhamento de DOL.

## Três mudanças

1. **Homebrew separado:** Todos os jogos, favoritos e recentes de jogos listam Wii, GameCube e ROMs presentes. Aplicativos e emuladores ficam em Homebrew. A busca dentro de Homebrew procura só aplicativos; a busca dos jogos respeita o filtro escolhido.
2. **Todos os jogos configurável:** na mesma tela de seleção da foto, role até **Jogos de** (Games from). Marque quais plataformas entram em Todos os jogos. Pode deixar apenas GameCube, Wii e retrô juntos ou outra combinação. A seleção é salva; desmarcar um console não apaga jogos nem histórico. A primeira linha da configuração ativa todos os consoles novamente.
3. **Cada console na lista:** Wii, GameCube, NES, SNES, GB, GBC, GBA, N64, Master System, Mega Drive, Game Gear, SG-1000 e Sega CD têm opções próprias, inclusive com contagem zero. Isso não cria títulos fictícios. Um atalho de console mostra seus jogos independentemente das exclusões feitas em Todos os jogos. Homebrew, favoritos e recentes têm contagens correspondentes às suas próprias listas.

A captura do usuário na alpha.3 mostrava 32 itens = 11 Wii + 6 GC + 15 homebrews. Com esses mesmos arquivos e sem ROMs reconhecidas, Todos os jogos passa a contar 17, enquanto Homebrew permanece separado. O número real aumenta quando as ROMs são encontradas.

## Instalação

Faça backup. Para uma instalação existente use o Update, confira os destinos e copie os arquivos para a raiz do USB com o Wii desligado. Para uma instalação nova, use o Kit completo. O caminho deve ser `apps/riftwii/boot.dol`, com meta.xml, icon.png, fonte e plugins correspondentes. Não misture somente o DOL novo com plugins antigos. Não é necessário desinstalar emuladores, apagar configurações ou formatar.

As ROMs continuam em `retro/<plataforma>` (ex.: `retro/nes/Jogo.nes`, `retro/snes/Jogo.sfc`, `retro/gba/Jogo.gba`), com subpastas e os diretórios nativos já suportados. Uma opção de console com zero indica que nenhum título está no catálogo dessa plataforma: confira caminho, formato e logs. O filtro corrigido não comprova a leitura das ROMs específicas do seu USB.

Os emuladores e o fluxo IOS58/autoboot da alpha.3 permanecem iguais. Esta versão não altera IOS246, cIOS ou NAND. Não inclui ROMs ou BIOS. Não escreve nem remove saves para configurar filtros. O atualizador Linux continua experimental; o atualizador no Wii permanece desativado. Confira SHA256SUMS, manifesto, fontes e licenças nos arquivos do Release. A alpha.3 continua preservada em seu próprio Release.

## Testes e limites

Os testes automatizados verificam ROMs que compartilham o DOL do emulador, separação de aplicativos, filtros, seleção exclusiva GC, múltiplas plataformas, seleção vazia, persistência, scanner, argumentos e atualização/rollback. O workflow exige testes, traduções, compilação Wii/canal, plugins e validação do Kit antes da publicação.

A nova interface e o conteúdo do USB do usuário ainda exigem validação visual em Dolphin e teste no Wii físico. Não declarar correção definitiva de USB/autoboot/retorno com base nesses testes de catálogo. Capas Shelf/Carousel/3D e resumo de adicionados/removidos continuam pendentes, como na alpha.3.

No Wii: abra a tela de filtros, confira que aplicativos não contam como jogos; entre em NES/SNES/GBA e veja a contagem; em Jogos de deixe somente GameCube, volte a Todos os jogos e reinicie para verificar persistência. Reative todos; teste favoritos, recentes, busca, execução e retorno. Se um console continuar zerado, envie `riftwii/session.log` e exemplos de caminhos/extensões (sem ROMs). Para falhas no lançamento, preserve também `riftwii/boot.log`.
