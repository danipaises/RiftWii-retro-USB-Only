# v0.1.0-alpha.3 — experimental

Repositório exclusivo: https://github.com/danipaises/RiftWii-retro-USB-Only

Esta versão contém correções candidatas para os problemas relatados na alpha.2. **O novo carregamento USB/autoboot e a interface ainda precisam de confirmação no Wii físico. Não é uma versão estável.** A alpha.2 permanece disponível no seu Release, sem substituição.

## Etapa A — implementada em código, confirmação física pendente

- Scanner NES: pastas `usb:/retro/nes` e `usb:/fceugx/roms`, subpastas até seis níveis, extensões sem distinção entre maiúsculas/minúsculas, NES/FDS/UNF/UNIF/ZIP. SNES e Game Boy também aceitam ZIP nas pastas de plataforma. ZIP em `vbagx/roms` não identifica de forma segura GB/GBC/GBA: use as pastas separadas `retro/gb`, `retro/gbc`, `retro/gba`.
- Scanner ignora arquivos vazios, links e arquivos removidos; registra pastas inacessíveis, plataforma, contagem e emulador ausente. Limite atual: 512 entradas somando aplicativos e ROMs.
- GX: diretório e nome da ROM são argumentos separados por NUL, preservando espaços e UTF-8. O DOL deve apresentar o bloco `_arg`; a ROM deve existir e permitir leitura antes da saída do launcher. Nomes ambíguos que o autoload GX por substring confundiria são recusados.
- Handoff retrô: fecha os clientes de armazenamento, reinicia o IOS58 instalado antes de alocar/carregar o DOL, remonta USB e depois libera completamente o driver para o emulador inicializá-lo. Não instala IOS e não escreve no IOS246. O uso de IOS58 no emulador pode limitar Fakemote; compatibilidade do controle continua pendente.
- Diagnóstico no `usb:/riftwii/boot.log`: `Retro scan`, `Retro launch`, `Homebrew argument`, `Retro handoff`, `USB application mount`; falhas `ROM_READ`, `USB_IOS`, `USB_MOUNT`, `ARGS_MISSING` distinguem as etapas.
- Os binários GX permanecem nas versões fixadas da alpha.2; não é alegado que os próprios emuladores passaram a registrar uma falha interna de montagem. O log confirma somente a preparação do launcher.

O código anterior só examinava a raiz da pasta retrô e não aceitava ZIP. Isso explica arquivos nessas condições, mas não permite afirmar a causa exata das suas ROMs NES sem o conteúdo do pendrive. O menu GX sempre oferece DVD/rede e só oferece FAT quando montado: esse sintoma é compatível com falha de armazenamento. O fluxo anterior desmontava FAT sem liberar integralmente o driver; a correção trata esse ciclo de vida, sem acrescentar apenas uma espera.

## Etapa B — implementada; navegação e desempenho no Wii pendentes

`All games` reúne os jogos Wii, GameCube, ROMs e homebrews USB presentes. Os emuladores conhecidos ficam somente na seção Homebrew. O seletor do filtro mostra plataformas com títulos, Homebrew, favoritos, recentes e a visualização original de mods. A busca consulta o catálogo misto, inclusive ao sair do filtro Homebrew. As configurações anteriores de fonte de jogos continuam na visualização de mods; Todos os jogos, favoritos, recentes e a busca usam o catálogo misto completo. Perfis de mods existentes preservam seus tiles adicionais.

Cada ROM tem nome, plataforma, caminho, emulador e identificador SHA-1 do caminho normalizado (identidade, não validação de segurança). Jogos de mesmo nome em plataformas/pastas distintas têm identidades distintas. Mover/renomear a ROM cria outra identidade; o histórico antigo é preservado. A página da ROM permite iniciar e favoritar. Favoritos e recentes agora aceitam essas identidades ao gravar e reler.

Capas PNG locais na visualização padrão Covers e em Channels, carregadas uma por vez por página, com cache de 24 imagens: `retro/covers/<plataforma>/<nome.extensão>.png`, incluindo as subpastas da ROM. Alternativa para qualquer caminho: `retro/covers/<40 caracteres do identificador>.png` sem o prefixo `retro:`. Sem capa, usa o tile com nome/plataforma. As vistas Shelf/Carousel/3D ainda não exibem capas retrô; use Covers/Channels. Nenhuma capa ou título demonstrativo é incluído.

O botão de procurar jogos refaz o scanner de discos e o de ROMs/apps, invalida capas e conserva favoritos/histórico. O toast mostra a contagem de ROMs/aplicativos; o total de Wii/GC chega ao terminar a leitura dos discos. O log registra a contagem anterior e atual. O resumo de adicionados/removidos e os erros de leitura agrupados na interface ainda precisam de melhoria.

## Instalação e atualização da alpha.2

1. Desligue o Wii. Faça uma cópia do pendrive e preserve o ZIP da alpha.2.
2. Para primeira instalação, extraia `RiftWii-Retro-USB-Only-v0.1.0-alpha.3-Kit.zip` diretamente na raiz de um pendrive FAT32. O arquivo principal fica exatamente em `apps/riftwii/boot.dol`, com `meta.xml`, `icon.png`, fonte e plugins correspondentes. Não coloque a pasta do ZIP envolvendo `apps`.
3. Para atualizar sua alpha.2, prefira o ZIP **Update**. Ele contém apenas executáveis, metadados, recursos de fábrica e plugins permitidos; confira os destinos e copie-os após o backup. Não há ROMs, configurações pessoais, BIOS ou saves nesse ZIP. Não apague as pastas antigas.
4. Seus jogos ficam em `retro/<plataforma>`, `wbfs` e `games`. Pastas vazias são permitidas. Não é necessário baixar emuladores separadamente.
5. Inicie pelo Homebrew Channel USB e execute o roteiro `TESTE-NO-WII.md` antes de usar a versão como principal.

O Kit instala um registro inicial de arquivos de fábrica. Não extraia o Kit indiscriminadamente sobre uma instalação personalizada: o extrator ZIP não oferece as proteções do script.

## Etapa C — base Linux experimental; atualizador no Wii continua desativado

`components.json` publica a combinação fixa de componentes, tamanho, SHA-256, condição experimental e ausência de validação física. `approved` significa combinação aceita pelos testes/build/empacotamento desta versão, **não** aprovação em hardware. Nenhuma versão upstream é instalada automaticamente. O workflow lê `retro-integration/VERSION`, gera Kit, Update, fontes e checksums, cria pré-lançamentos e preserva releases existentes.

O script Linux usa somente a biblioteca padrão Python 3, não precisa de Docker:

```
python3 usb_update.py --usb /media/usuario/PENDRIVE
python3 usb_update.py --usb /media/usuario/PENDRIVE --apply --yes
python3 usb_update.py --usb /media/usuario/PENDRIVE --rollback update-IDENTIFICADOR --yes
```

Sem `--apply --yes`, apenas consulta e compara. Opcional: `--manifest https://github.com/danipaises/RiftWii-retro-USB-Only/releases/download/UMA-TAG/components.json` para escolher uma versão. Confirme o volume apresentado antes de aplicar. Ejete com segurança depois.

As versões dos componentes vêm do registro instalado, e o manifesto mostra a combinação disponível. A instalação é da combinação inteira, ainda sem seleção individual. Somente destinos explicitamente permitidos em `apps/` podem ser substituídos. Arquivo existente desconhecido ou diferente do hash de fábrica bloqueia a atualização. Verifica origem HTTPS do próprio fork, hashes, tamanho, espaço, ZIP e conteúdo antes de escrever; grava backup e diário sincronizados antes das substituições atômicas. O rollback recusa sobrescrever alterações pessoais feitas depois da atualização.

A alpha.2 não tem registro de fábrica verificável: o script **recusa adotá-la automaticamente**. Faça o backup e a atualização manual acima; a instalação limpa do Kit alpha.3 prepara o registro para versões futuras. Em interrupção, use o identificador em `riftwii/update-backups` para restaurar. A atomicidade é por arquivo: uma interrupção pode deixar componentes de versões distintas até o rollback; não há garantia de boot automático nesse intervalo. Não aplique com o pendrive sendo usado pelo Wii.

O antigo `tools/make_update_pack.py` foi avaliado: é o formato SD do atualizador upstream, inclui temas/instalador e não gerencia os emuladores. Permanece preservado para o upstream; não serve como instalador USB seguro. Seu verificador de plugins continua reutilizado no build. A consulta/instalação pelo Wii, verificação periódica, opções de download automático e recuperação antes de iniciar o DOL são trabalho posterior. O bloqueio em `wii/online/online.cpp` permanece ativo.

## Compatibilidade e pendências

Inclui FCE Ultra GX 4.0.3, Snes9x GX 5.0.4, VBA GX 3.0.4, Genesis Plus GX em commit fixo, Wii64 1.5.0 glN64/Rice; hashes e commits em `sources/emulators.lock.json`. Fontes correspondentes e licenças acompanham o Kit. Not64 e Sega 32X continuam pendentes. Sega CD depende de BIOS regionais fornecidas legalmente pelo usuário; não são incluídas. Não há suporte prometido a Saturn/Dreamcast.

Validação concluída nesta sessão: 121 testes C/C++ e 14 testes Python aprovados, compilação Wii e canal, traduções de seis idiomas, verificação de plugins e validação do Kit completo (sete DOLs). O Dolphin abriu o menu All games com biblioteca vazia usando SD virtual; essa verificação não exerceu USB físico nem os emuladores. Testes automatizados verificam argumentos, scanner, identidades/persistência, validação do Kit e atualização/rollback em diretórios temporários. Não equivalem a testes físicos de USB, saves, saída dos emuladores, Fakemote, Wii/GC ou sequência de vários jogos. Siga o roteiro e envie o `boot.log` ao relatar problemas. Nenhum WAD, alteração de NAND ou instalador de cIOS acompanha os pacotes.
