# AMDLSS5

A mod that brings the NVIDIA DLSS 5 neural network ("DLSS Neural Rendering") to AMD Radeon graphics cards, on top of the
game's own upscaler. **F10** turns the network on and off; **End** opens the mod menu.

Version 2.0 runs on its own engine, written from scratch for this project. **Download it from the
[Releases page](https://github.com/Wehaveall/AMDLSS5/releases/latest).**

The source code currently in this repository is the 1.6.1 source, with its original credits kept in the files; the 2.0
source will be published here later.

## Requirements

- One of these AMD Radeon cards:
  - **RX 9070 / RX 9070 XT** (RDNA 4): supported.
  - **RX 9060 / RX 9060 XT** (RDNA 4): should work, not tested yet on real hardware.
  - **RX 7000** (RX 7900 / 7800 / 7700 / 7600, RDNA 3): **ALPHA**. It has never been tested on real hardware and may not
    work or may run slowly. If it fails, the game keeps running with standard FSR.
  - RX 6000 and older cards, and integrated graphics, are not supported.
- Windows 10/11 64-bit and a recent AMD Adrenalin driver.
- A CPU with **AVX2** (Intel Core 4th generation / AMD Ryzen or newer).
- A **DirectX 12** game with **FSR 3.1 or newer**, or one of the games below with its own setup.

## Supported games

Tested on a Radeon RX 9070 XT:

- **Resident Evil Requiem**
- **Cyberpunk 2077**
- **The Witcher 3: Wild Hunt** (DirectX 12): if you add it manually, select `bin\x64_dx12\witcher3.exe`
- **Red Dead Redemption 2**: set the API to **DirectX 12** and choose **AMD FSR 2** in the game settings (the mod upgrades it)
- **Shadow of the Tomb Raider**: choose **XeSS** as the upscaler (the mod turns it into FSR)
- **Final Fantasy XVI**: turn off dynamic resolution in the graphics settings
- **Monster Hunter Wilds**
- **Onimusha: Way of the Sword**
- **PRAGMATA** (new in 2.0)
- **Nioh 3** (new in 2.0): select **XeSS** in the game's Advanced Resolution settings and confirm when the game asks
- **Grand Theft Auto V Enhanced** (new in 2.0): **Story Mode, offline only**. Choose **FSR 4** in the graphics settings.
  The game's BattlEye anti-cheat must be **off** (Rockstar Games Launcher > Settings > Grand Theft Auto V Enhanced >
  disable BattlEye, or the `-nobattleye` launch argument). **Never go online with the mod.**

In every other game: choose **FSR 3.1** (or newer) as the upscaler. Other DirectX 12 games with FSR 3.1 or newer may
work but were not tested. The installer shows every game it finds, grouped as tested, not tested yet and not supported;
games with anti-cheat are marked, and only those with an official offline mode can be installed (with a warning).

## How to install

1. Download `AMDLSS5-2.0.0.zip` from the releases page and extract **the whole folder** (the `payload` folder stays next
   to the installer).
2. Run `AMDLSS5-Instalador.exe`. It finds the games installed on your PC (Steam, Epic, GOG, Ubisoft, EA, Battle.net and
   Xbox) and shows them as a library of covers with their state: *Not installed*, *Installed v<version>* or
   *Update available*.
3. Click the game's cover to **Install**, **Update** or **Remove** the mod. A game that is not in the list can be added
   with **Add a game manually...** (select its `.exe`).
4. In game, choose the upscaler named above. **F10** turns the network on and off; **End** opens the mod menu.
5. To remove: open the installer, click the game and choose **Remove**. The game folder goes back exactly as it was.

More details are in `read-me.txt` inside the package.

**Warning (AMD Adrenalin):** before playing, turn off the Adrenalin metrics overlay and recording/instant replay and set
performance tuning to **Default**; reboot after changing it. With them on, the GPU can hang while the game loads.

## What's new in 2.0

- **New engine**, written for this project: the mod no longer depends on any other mod's files.
- **Performance**: in Resident Evil Requiem (same scene, 60 s, 4K, FSR, RX 9070 XT), 2.0 runs at 101.2 FPS on average
  (1% low 78.5): +64% FPS and +41% on the 1% low compared with other mods, and +11% FPS and +19% on the 1% low compared
  with 1.6.1.
- **New games**: PRAGMATA, Nioh 3 and Grand Theft Auto V Enhanced (offline); Cyberpunk 2077 now runs on the new engine.
- **9 languages** in the menu and the installer (new: Italian, French, German, Chinese (Simplified), Japanese and Korean).
- **New image options** (all start off): color palettes (LUTs, or your own .cube files), local tonemap, light memory and
  ambient light for indirect lighting, "Less AO near strong light" and Clarity "Respect depth".
- **RX 9060 / 9060 XT** should work; **RX 7000 as an alpha**.
- The neural strength now comes tuned: the strength settings are fixed at the best values we found.
- **Installer**: tidier game folder (only `version.dll` next to the game, everything else in one `AMDLSS5` folder), safer
  updates that keep your settings, anti-cheat detection.
- Stability fixes across the board.

The full list is in the release notes of 2.0.

## Credits

- **NVIDIA** — DLSS and the DLSS Neural Rendering network. NVIDIA and DLSS are trademarks of NVIDIA Corporation. This
  project is not affiliated with, sponsored or endorsed by NVIDIA or AMD. The NVIDIA network files are not covered by
  this project's license.
- **praydog** — [REFramework](https://github.com/praydog/REFramework) (MIT license).
- Third-party components in the release package (AMD FidelityFX, the face detector model and others) are listed with
  their licenses in `THIRD_PARTY_NOTICES.txt`.

## License

The license of the code in this repository is in `LICENSE`. NVIDIA's files, the face detector weights (non-commercial use only), REFramework, AMD FidelityFX and
the other third-party components keep their own licenses.

If you use or share the mod, please credit **AMDLSS5**. Thank you!

## Support the project

AMDLSS5 is free and will stay free. There is no paid version, no early access and no feature locked behind a donation.

If the mod made your games look better and you want to help, you can buy me a coffee. Every donation, small or large,
helps pay for the hardware, the games used for testing and the many hours spent on each release.

[Donate with Stripe](https://buy.stripe.com/7sY9AT5PjbRT4M47u9d7q01)

### Gift a game on Steam

Want AMDLSS5 to support a game you love? You can also gift that game on Steam. The mod has to be tested and tuned in
each game, so having the game is the fastest way to make it compatible.

- Only **DirectX 12** games can be supported (the mod works through the game's DirectX 12 upscaler path; DirectX 11
  and Vulkan games cannot be supported).
- Before gifting, check the supported games list above: the game may already be supported.
- Send the gift to my Steam profile: https://steamcommunity.com/profiles/76561197992454025/
- A gifted game is a donation, not a purchase: compatibility is a goal, not a promise. Some games have anti-cheat or
  engines that block mods, and those cannot be supported.

Not able to donate? That is completely fine. Starring the repository, reporting bugs with logs and sharing the mod
help just as much. Thank you!
