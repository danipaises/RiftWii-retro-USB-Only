# RiftWii Retro USB-Only — alfa em desenvolvimento

Projeto exclusivo: https://github.com/danipaises/RiftWii-retro-USB-Only

A base USB-only anterior foi recuperada com fontes e DOL preservados. Esta
integração retrô é nova, experimental e não foi testada no Wii físico.
Leia [o estado real e as pendências](retro-integration/STATUS.md).

## Instalação e atualização

Faça backup do pendrive. Extraia o kit na raiz de um USB FAT32. Não substitua
suas configurações nem saves. Os templates têm nomes .example diferentes.
Coloque suas ROMs em retro/<plataforma> e abra Aplicativos no RiftWii.
Sem ROMs, a lista permanece vazia e utilizável. Jogos Wii ficam em wbfs;
GameCube mantém a integração e as limitações anteriores.

Não instale WADs, não altere IOS246 nem instale cIOS para testar esta alfa.
No emulador selecione sair para o loader; quando o canal RiftWii não existir,
o retorno usa o fallback HBC/Menu Wii. Não existe promessa de retorno de todos
os emuladores antes do teste físico.

## Compilar sem Docker local

Instale devkitPro/devkitPPC e portlibs descritos em docs/USB_ONLY.md.
`bash tools/build_usb_only.sh` compila a aplicação e os testes. Para o kit,
instale py7zr e execute `python3 retro-integration/package.py` após o build.
Emuladores são integrados a partir dos releases/binários fixados no lock,
não apresentados como recompilados. Fontes correspondentes acompanham o kit.

## Teste físico pendente

Para cada plataforma NES/SNES/GB/GBC/GBA/N64/MS/MD/GG/SG1000/CD, use uma ROM
homebrew com licença redistribuível obtida do próprio autor e registre URL,
licença, hash, desempenho, controles, save, state, saída e retorno. Não foi
selecionada ainda uma amostra legal validada para cada sistema. Sem SD:
RiftWii → jogo → RiftWii → outro jogo, por três ciclos. Teste também Wii e GC.
Antes de atualizar, preserve apps/riftwii e as pastas de saves de cada emulador.

## Licenças e autoria

RiftWii: KakarottoCake e colaboradores, GPL-3.0-or-later. Veja LICENSE,
NOTICE.md e README-UPSTREAM.md. FCE Ultra GX, Snes9x GX, VBA GX: dborth e
respectivos autores; Wii64: Team Wii64; Genesis Plus GX: Eke-Eke e demais
autores. Cada componente mantém sua licença própria, incluída nas fontes.
O kit que inclui Genesis Plus GX não pode ser vendido ou usado comercialmente.
Nenhuma ROM comercial, BIOS proprietária ou instalador de canal é incluído.
