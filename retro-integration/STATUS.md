# Estado real — implementação alfa

Base USB-only recuperada do pacote 2610-699, fontes originais bb84fb1a e
base oficial 21172fe07123e8facb2f488332f5c0b174989997. O relato do usuário
confirma a base no Wii, não esta integração nova.

Implementado: catálogo USB limitado a 512 entradas, 11 plataformas com IDs
separados, extensões por plataforma, scanner vazio seguro, adaptadores GX e
Wii64 glN64/Rice, validação de caminho e emulador, arte local e retorno pelo
stub existente preferindo o canal RiftWii. Catálogo aparece em Aplicativos /
Homebrew, com prefixo de console e busca existente. Capas são PNGs em
retro/covers/<plataforma>/<nome completo da ROM>.png. Ausência usa fallback.
Atualização do catálogo pelo botão de atualizar existente. Diretórios não
são recursivos nesta alfa. Escolha N64: retro/n64/emulator.txt, gln64 ou rice.
A escolha deve ser criada manualmente; o exemplo não sobrescreve preferências.

Pendentes: tela própria com categorias selecionáveis, favoritos e recentes
retrô completos, escolha N64 na interface, atualização automática incremental,
validação de saves/state e retorno reais. Os binários GX possuem autoload que
procura substrings de nomes: nomes sobrepostos podem selecionar outro jogo.
O launcher bloqueia nomes ambíguos antes da inicialização; comparação exata no emulador ainda pendente.
Not64 não é integrado ao autoboot: o código inspecionado lê pares de opções,
e não foi validado um caminho de ROM único. 32X não habilitado: não houve
confirmação física do núcleo Wii. Sega CD exige as três BIOS regionais por
prudência; validação por jogo/região e hash de BIOS ainda pendente.

GameCube permanece com as limitações da base USB-only: memory cards do
backend interno dependem de SD. Não é alegado GameCube USB-only completo.
Não se removeu o suporte existente nem se substituiu pelo RiftWii oficial.

Fakemote IOS246 não é instalado/alterado. Emuladores podem recarregar IOS58;
compatibilidade Fakemote não é confirmada. Wii Remote, Nunchuk, Classic e GC
constam nas implementações upstream, mas não foram testados neste kit.

Não chamar este pacote de v1.0.0 Complete. Publicação permitida apenas como
alfa experimental após compilação, testes e verificação de todos os binários.
