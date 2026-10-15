# AMDLSS5

A mod that runs the NVIDIA DLSS 5 neural network ("DLSS Neural Rendering") on AMD Radeon RDNA 4 cards, on top of the
game's own FSR 3.1. **F10** toggles the network; **End** opens the mod menu.

This repository holds the source code of the helper and of the installer. Binaries only ship in the release package.

## Requirements

- AMD Radeon **RX 9070 / RX 9070 XT** (RDNA 4). Other cards are not supported.
- Windows 10/11 64-bit, a recent AMD Adrenalin driver.
- A **DirectX 12** game with **FSR 3.1**. RE Engine (Capcom) games use REFramework, which the installer includes.

## Tested

Tested **only** on a **Radeon RX 9070 XT**, in these games:

- Resident Evil Requiem
- Onimusha: Way of the Sword (demo)
- Cyberpunk 2077
- Monster Hunter Wilds

Other DirectX 12 games with FSR 3.1 and the RX 9070 (non-XT) may work, but were not tested.

## How to install

1. Download `AMDLSS5-<version>.zip` from the releases page and extract **the whole folder** (the `payload` folder stays
   next to the installer).
2. Run `AMDLSS5-Instalador.exe`, select the game folder and click **Install**.
3. In game, keep FSR 3.1 on. The network starts **off**: press **F10** to turn it on and off.
4. To remove: run the installer, pick the game and click **Uninstall** (the game folder goes back as it was).

## Features

- The DLSS 5 neural network on AMD RDNA 4 cards.
- In-game menu in English, Portuguese and Spanish.
- Installer with game detection, backup and a complete uninstall.

## What's new in 1.3.0

- Better network performance.
- Improved image quality.

## What's new in 1.2.1

- Network bug fix.

## What's new in 1.2.0

- Image quality, stability and performance improvements.
- Bug fixes.

**Warning (AMD Adrenalin):** before playing, turn off the Adrenalin metrics overlay and recording/instant replay and set
performance tuning to **Default**; reboot after changing it. With them on, the GPU can hang while the game loads.

## How to build

Visual Studio 2022, CMake, the Windows SDK and the headers from AMD's HIP SDK. Helper: `Build-Helper.ps1`; installer:
`instalador\Build-Instalador.ps1`.

## Credits

- **Daniel (danielblnc)** — [DLSS-NR-on-AMD](https://github.com/danielblnc/DLSS-NR-on-AMD). This mod is built on his
  work. Still trying to contact him for permission.
- **NVIDIA** — DLSS and the DLSS Neural Rendering network. NVIDIA and DLSS are trademarks of NVIDIA Corporation. This
  project is not affiliated with, sponsored or endorsed by NVIDIA or AMD.
- **praydog** — [REFramework](https://github.com/praydog/REFramework) (MIT license).
- AMD FidelityFX / FSR 3.1 — comes with the game; nothing from AMD is redistributed here.

## License

- Code in this repository: © the AMDLSS5 author. An open-source license has not been chosen yet; until then, all
  rights reserved (you may read it, build it and use it to play).
- Daniel's files, NVIDIA's files and REFramework follow their own terms and are not in this repository.
