# v0.1.0-alpha.5 — catálogo e recuperação USB (experimental)

Repositório: https://github.com/danipaises/RiftWii-retro-USB-Only

## Catálogo: defeito reproduzido e corrigido

A alpha.4 tinha um teto global de 512 entradas incluindo aplicativos. O scanner lia NES primeiro. A reprodução com 26 aplicativos, 600 arquivos NES e um arquivo SNES/GBA encontrou exatamente 486 NES e zero SNES/GBA, como o padrão observado na foto do usuário.

A alpha.5 separa o orçamento de cada plataforma e dos aplicativos. O mesmo teste agora encontra 600 NES, um SNES e um GBA. Os testes também excedem a cota NES e verificam que SNES/GBA continuam sendo lidos. As pastas USB e extensões existentes são preservadas.

A biblioteca continua limitada para caber no Wii: até 2048 ROMs e 2 MiB estimados de metadados por console, valendo o primeiro teto atingido. O scanner verifica espaço antes de ampliar a lista e deixa margem para a interface. Não há promessa de biblioteca ilimitada: excessos e falta de memória são registrados em session.log. Não se leem ROMs inteiras nem centenas de capas durante a indexação. O cache de capas continua com 24 imagens. Bibliotecas grandes ainda exigem teste de memória e desempenho no Wii.

## Tela USB: mesmo caminho do botão Exit

O usuário confirmou que apertar Exit na tela “No readable storage” abre o RiftWii funcional. A alpha.5 executa automaticamente o mesmo resultado MENU_EXIT, respeitando o destino já usado por esse botão. Não substitui isso por um salto para outro loader e não marca um USB não montado como montado.

Uma marca na área RAM já reservada ao reinício permite uma saída automática por cadeia de falha. Se o mesmo início voltar sem conseguir montar, a tela de recuperação reaparece para permitir intervenção. Quando o armazenamento funciona, a marca é limpa, habilitando a recuperação num retorno futuro. Não é criada marca na NAND nem no USB inacessível. Os testes verificam a política da marca; sua preservação pelo HBC/canal do usuário exige confirmação física. Outros loaders que sobrescrevam essa RAM não estão validados.

## Lançamento NES/SNES/GBA

O caminho retrô reiniciava IOS58 enquanto a comunicação WPAD podia continuar ativa. Agora encerra os pedidos dos controles antes dessa troca de IOS, conforme o contrato já existente em ios_reload.hpp. Os argumentos continuam sendo diretório e nome da ROM separados por NUL, com verificação do arquivo e do bloco de argumentos do DOL. O log registra o encerramento WPAD, IOS, montagem e argumentos antes da transferência ao emulador.

Essa ordem foi corrigida em código; ainda não comprova que era a única causa da saída imediata observada no NES. Não há confirmação física de autoboot/USB/saves/retorno nesta versão. Se persistir, preservar boot.log e informar se voltou ao menu de canais ou ao RiftWii. Os binários dos emuladores permanecem fixados nas mesmas versões; não são anunciados como recompilados ou corrigidos internamente.

## Instalação

Desligue o Wii, faça backup e copie o Update para a raiz do USB, conferindo arquivos personalizados. Atualize DOL e plugins juntos. Para primeira instalação, use o Kit completo. O executável fica em `apps/riftwii/boot.dol`, com meta.xml, icon.png, fonte e plugins correspondentes. Não é necessário apagar configurações, ROMs, BIOS ou saves, reinstalar canais ou formatar o USB.

Permanecem Todos os jogos sem aplicativos, Homebrew separado, filtros por console e seleção persistente em Jogos de. Não são instalados IOS/cIOS/WADs e o IOS246 não é alterado. O atualizador no Wii permanece bloqueado. O script Linux continua experimental, com verificação, lista de destinos permitidos, backup e rollback. Releases anteriores permanecem preservados.

## Verificações e pendências

O workflow exige testes C/C++, Python, traduções, build Wii/canal, plugins e validação do Kit antes de publicar. A reprodução do limite e a política da saída automática são testes de computador, não de hardware. A nova interface de recuperação, a retenção da marca através de Exit e os emuladores precisam de teste físico; não foram apresentados como testados em Dolphin. Capas Shelf/Carousel/3D e resumo de adicionados/removidos continuam pendentes.
