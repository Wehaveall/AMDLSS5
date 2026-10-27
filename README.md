# AMDLSS5

A mod that runs the NVIDIA DLSS 5 neural network ("DLSS Neural Rendering") on AMD Radeon RDNA 4 cards, on top of the
game's own upscaler (FSR 3.1 or newer; in some games their FSR 2 or XeSS). **F10** toggles the network; **End** opens
the mod menu.

This repository holds the source code of the helper and of the installer. Binaries only ship in the release package.

## Requirements

- AMD Radeon **RX 9070 / RX 9070 XT** (RDNA 4). Other cards are not supported.
- Windows 10/11 64-bit, a recent AMD Adrenalin driver.
- A **DirectX 12** game with **FSR 3.1 or newer**, or one of the games listed below with its own setup. RE Engine (Capcom)
  games use REFramework, which the installer includes.

## Tested

Tested **only** on a **Radeon RX 9070 XT**, in these games:

- Resident Evil Requiem
- Onimusha: Way of the Sword (demo)
- Cyberpunk 2077
- Monster Hunter Wilds
- The Witcher 3: Wild Hunt (DirectX 12): in the installer select `bin\x64_dx12\witcher3.exe`
- Shadow of the Tomb Raider: choose **XeSS** as the upscaler in the game settings (the mod turns it into FSR)
- Red Dead Redemption 2: set the API to **DirectX 12** and choose **AMD FSR 2** in the game settings (the mod upgrades it)
- Stellar Blade: choose **AMD FSR 3.1** in the game settings
- Final Fantasy XVI (demo): turn off dynamic resolution in the game's graphics settings

Other DirectX 12 games with FSR 3.1 or newer and the RX 9070 (non-XT) may work, but were not tested. The installer shows
every game it finds, grouped as tested, not tested yet and not supported; games with anti-cheat are marked and cannot be
installed.

## How to install

1. Download `AMDLSS5-<version>.zip` from the releases page and extract **the whole folder** (the `payload` folder stays
   next to the installer).
2. Run `AMDLSS5-Instalador.exe`, pick the game in the library (or select its folder) and click **Install**.
3. In game, keep the upscaler named above on. The network starts **off**: press **F10** to turn it on and off.
4. To remove: run the installer, pick the game and click **Uninstall** (the game folder goes back as it was).

## Features

- The DLSS 5 neural network on AMD RDNA 4 cards.
- Image filters applied before the game HUD: adaptive sharpening, luma sharpening, local contrast, levels, tone and
  color, white balance (Kelvin), face skin, color by range (HSL), ambient occlusion and indirect lighting. All of them
  start off.
- Frame generation x2 for games without frame generation of their own (Red Dead Redemption 2, Shadow of the Tomb
  Raider). In games that already offer it, turn it on in the game's own menu.
- In-game menu in English, Portuguese and Spanish, with "Restore defaults".
- Installer with a game library (all stores), backup and a complete uninstall.

## What's new in 1.6.0

- Shadow of the Tomb Raider, Red Dead Redemption 2, Stellar Blade and Final Fantasy XVI (demo) support.
- Frame generation x2 for games without their own (Red Dead Redemption 2 and Shadow of the Tomb Raider).
- New image filters, including ambient occlusion and indirect lighting.
- New installer with a game library.
- Option to hide the compact counter; "Restore defaults" in the menu.
- Greater stability and bug fixes.

## What's new in 1.5.2

- New DLSS 5 network.
- The Witcher 3 (DirectX 12) support.
- Improved image stability and detail.
- Bug fixes.

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
- Third-party components in the release package (including the FidelityFX files added for games that do not ship them)
  are listed with their licenses in `THIRD_PARTY_NOTICES.txt`.

## License

- Code in this repository: © the AMDLSS5 author. An open-source license has not been chosen yet; until then, all
  rights reserved (you may read it, build it and use it to play).
- Daniel's files, NVIDIA's files, REFramework and the other third-party components follow their own terms.
