# AMDLSS5

**PT-BR** | [English](#english)

Mod que roda a rede neural do NVIDIA DLSS 5 ("DLSS Neural Rendering") em placas AMD Radeon RDNA 4, por cima do FSR 3.1
do próprio jogo. **F10** liga/desliga a rede; **End** abre o menu do mod.

Este repositório tem o código-fonte do helper e do instalador. Os binários só vão no pacote de lançamento.

## Requisitos

- AMD Radeon **RX 9070 / RX 9070 XT** (RDNA 4). Outras placas não são suportadas.
- Windows 10/11 64 bits, driver AMD Adrenalin recente.
- Jogo **DirectX 12** com **FSR 3.1**. Jogos com RE Engine (Capcom) usam o REFramework, que o instalador já inclui.

## Testado

Testado **apenas** numa **Radeon RX 9070 XT**, nestes jogos:

- Resident Evil Requiem
- Onimusha: Way of the Sword (demo)
- Cyberpunk 2077
- Monster Hunter Wilds

Outros jogos DirectX 12 com FSR 3.1 e a RX 9070 (sem XT) podem funcionar, mas não foram testados.

## Como instalar

1. Baixe o `AMDLSS5-<versão>.zip` da página de lançamentos e extraia **a pasta inteira** (a pasta `payload` fica ao
   lado do instalador).
2. Abra `AMDLSS5-Instalador.exe`, selecione a pasta do jogo e clique em **Instalar**.
3. No jogo, deixe o FSR 3.1 ligado. A rede começa **desligada**: aperte **F10** para ligar e desligar.
4. Para remover: abra o instalador, escolha o jogo e clique em **Desinstalar** (a pasta do jogo volta como era).

## Funções

- Rede neural do DLSS 5 em placas AMD RDNA 4.
- Menu no jogo em português, inglês e espanhol.
- Instalador com detecção do jogo, backup e desinstalação completa.

## Novidades da 1.3.0

- Melhorias de qualidade de imagem e da rede.

## Novidades da 1.2.1

- Correção de bug na rede.

## Novidades da 1.2.0

- Melhorias de qualidade de imagem, estabilidade e desempenho.
- Correções de bugs.

**Aviso (AMD Adrenalin):** antes de jogar, desligue o overlay de métricas e a gravação/replay instantâneo do Adrenalin
e deixe o ajuste de desempenho em **Padrão**; reinicie o PC depois de mudar. Com eles ligados a placa pode travar no
carregamento do jogo.

## Como compilar

Visual Studio 2022, CMake, Windows SDK e os headers do HIP SDK da AMD. Helper: `Build-Helper.ps1`; instalador:
`instalador\Build-Instalador.ps1`.

## Créditos

- **Daniel (danielblnc)** — [DLSS-NR-on-AMD](https://github.com/danielblnc/DLSS-NR-on-AMD). Este mod é construído
  sobre o trabalho dele. Ainda tentando contato para permissões.
- **NVIDIA** — DLSS e a rede do DLSS Neural Rendering. NVIDIA e DLSS são marcas da NVIDIA Corporation. Este projeto não
  é afiliado, patrocinado nem endossado pela NVIDIA nem pela AMD.
- **praydog** — [REFramework](https://github.com/praydog/REFramework) (licença MIT).
- **YuNet** — detector de rostos do [OpenCV Zoo](https://github.com/opencv/opencv_zoo/tree/main/models/face_detection_yunet) (licença MIT, © 2020 Shiqi Yu).
- AMD FidelityFX / FSR 3.1 — do próprio jogo; nada da AMD é redistribuído aqui.

## Licença

- Código deste repositório: © o autor do AMDLSS5. A licença de código aberto ainda não foi escolhida; até lá, todos os
  direitos reservados (você pode ler, compilar e usar para jogar).
- Os arquivos do Daniel, da NVIDIA e o REFramework seguem os termos de cada um e não estão neste repositório.

---

## English

A mod that runs the NVIDIA DLSS 5 neural network ("DLSS Neural Rendering") on AMD Radeon RDNA 4 cards, on top of the
game's own FSR 3.1. **F10** toggles the network; **End** opens the mod menu.

This repository holds the source code of the helper and of the installer. Binaries only ship in the release package.

### Requirements

- AMD Radeon **RX 9070 / RX 9070 XT** (RDNA 4). Other cards are not supported.
- Windows 10/11 64-bit, a recent AMD Adrenalin driver.
- A **DirectX 12** game with **FSR 3.1**. RE Engine (Capcom) games use REFramework, which the installer includes.

### Tested

Tested **only** on a **Radeon RX 9070 XT**, in these games:

- Resident Evil Requiem
- Onimusha: Way of the Sword (demo)
- Cyberpunk 2077
- Monster Hunter Wilds

Other DirectX 12 games with FSR 3.1 and the RX 9070 (non-XT) may work, but were not tested.

### How to install

1. Download `AMDLSS5-<version>.zip` from the releases page and extract **the whole folder** (the `payload` folder stays
   next to the installer).
2. Run `AMDLSS5-Instalador.exe`, select the game folder and click **Install**.
3. In game, keep FSR 3.1 on. The network starts **off**: press **F10** to turn it on and off.
4. To remove: run the installer, pick the game and click **Uninstall** (the game folder goes back as it was).

### Features

- The DLSS 5 neural network on AMD RDNA 4 cards.
- In-game menu in Portuguese, English and Spanish.
- Installer with game detection, backup and a complete uninstall.

### What's new in 1.3.0

- Image quality and network improvements.

### What's new in 1.2.1

- Network bug fix.

### What's new in 1.2.0

- Image quality, stability and performance improvements.
- Bug fixes.

**Warning (AMD Adrenalin):** before playing, turn off the Adrenalin metrics overlay and recording/instant replay and set
performance tuning to **Default**; reboot after changing it. With them on, the GPU can hang while the game loads.

### How to build

Visual Studio 2022, CMake, the Windows SDK and the headers from AMD's HIP SDK. Helper: `Build-Helper.ps1`; installer:
`instalador\Build-Instalador.ps1`.

### Credits

- **Daniel (danielblnc)** — [DLSS-NR-on-AMD](https://github.com/danielblnc/DLSS-NR-on-AMD). This mod is built on his
  work. Still trying to contact him for permission.
- **NVIDIA** — DLSS and the DLSS Neural Rendering network. NVIDIA and DLSS are trademarks of NVIDIA Corporation. This
  project is not affiliated with, sponsored or endorsed by NVIDIA or AMD.
- **praydog** — [REFramework](https://github.com/praydog/REFramework) (MIT license).
- **YuNet** — face detector from [OpenCV Zoo](https://github.com/opencv/opencv_zoo/tree/main/models/face_detection_yunet) (MIT license, © 2020 Shiqi Yu).
- AMD FidelityFX / FSR 3.1 — comes with the game; nothing from AMD is redistributed here.

### License

- Code in this repository: © the AMDLSS5 author. An open-source license has not been chosen yet; until then, all
  rights reserved (you may read it, build it and use it to play).
- Daniel's files, NVIDIA's files and REFramework follow their own terms and are not in this repository.
