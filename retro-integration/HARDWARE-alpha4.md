# Teste físico — alpha.4 (pendente)

Faça backup do USB, desligue o Wii antes de copiar e mantenha o SD retirado. Não instale WADs, IOS ou cIOS. Esta versão não exige mudar o IOS246. Preserve configurações e saves; não formate.

1. Atualize DOL, fonte e plugins juntos pelo pacote Update, ou use o Kit numa instalação nova. Abra pelo HBC USB. Confira a versão no meta.xml e os checksums.
2. Na tela da foto, Todos os jogos deve excluir aplicativos. Com 11 Wii e 6 GC, são 17 jogos antes de acrescentar ROMs reconhecidas. Homebrew deve conter somente os aplicativos/emuladores.
3. Cada console deve ter uma linha, mesmo com zero. NES, SNES e GBA com arquivos legais nas pastas respectivas devem ter contagem positiva e mostrar as ROMs na biblioteca; os emuladores continuam na seção Homebrew, sem virar jogos duplicados.
4. Role até Jogos de (Games from), marque só GameCube, volte e escolha Todos os jogos. Devem aparecer somente os títulos GC existentes. Reinicie e confira a persistência. A linha inicial da configuração reativa todas as plataformas. Marque uma combinação Wii/SNES/GBA e confira as contagens. Uma seleção vazia deve continuar utilizável, sem criar títulos.
5. Abra o atalho NES enquanto NES estiver desmarcado em Todos os jogos: o atalho deve mostrar suas ROMs. Teste busca no filtro escolhido e busca em Homebrew; aplicativos não devem entrar nas buscas de jogos.
6. Favorite duas ROMs de mesmo nome em plataformas distintas, execute-as e confira favoritos/recentes após reiniciar. Retire um arquivo apenas com o USB ocioso e o Wii desligado; releia a biblioteca. O título removido some, sem apagar o histórico salvo. Recoloque-o e confira sua identidade.
7. Teste ROMs NES/SNES/GBA, saves, save states e a sequência jogo → RiftWii → outro jogo, sem SD. Repita Wii e GC existentes. A correção da biblioteca não comprova USB/autoboot/retorno dentro dos emuladores. Fakemote exige teste individual, sem alterar IOS246.
8. Preserve `riftwii/session.log` ao relatar títulos ausentes e `riftwii/boot.log` ao relatar lançamento. Envie exemplos de caminhos e extensões, sem enviar ROMs/BIOS. Uma categoria zero sozinha não distingue pasta errada de falha de leitura; o log e a estrutura real do USB são necessários.

A interface nova ainda precisa de verificação visual em Dolphin e Wii. Não declarar esses testes como realizados antes de obter evidências. Capas retrô em Shelf/Carousel/3D continuam pendentes; use Covers/Channels.
