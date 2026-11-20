# AMDLSS5

A mod that brings the NVIDIA DLSS 5 neural network ("DLSS Neural Rendering") to AMD Radeon graphics cards, on top of the
game's own upscaler, now with **Neural Apex frame generation up to 6x**. **F10** turns the network on and off; **End** opens
the mod menu.

**Download the latest version (2.5.0) from the [Releases page](https://github.com/Wehaveall/AMDLSS5/releases/latest).**

**Supported games: see [COMPATIBLE_GAMES.md](COMPATIBLE_GAMES.md)** (the list keeps growing, with the setting each game needs).

**Source code status.** This repository currently holds the 1.6.1 source (original credits kept in the files). The 2.x source
is being cleaned up and will be published here in stages, starting with the 2.5 integration code (MIT). A few parts will stay
closed: NVIDIA's network data, third-party binaries such as the Apex frame generator, and some proprietary engine components.
Every release lists the SHA-256 of its files, so you can always verify exactly what you download.

## What's new in 2.5

- **New neural engine (Vulkan)**, built on **Mochizuki's DLSSNR-AMD runtime** 0.0.8.3 (mochizuki0323, MIT license,
  [source](https://github.com/TheAutomatic/DLSSNR-AMD)). Thanks to Mochizuki's engine together with our own
  optimizations, the network is up to about 60% faster and the game runs at about 10% more FPS than 2.0 (measured in
  Resident Evil Requiem, RX 9070 XT). The old HIP engine is gone.
- **Neural Apex frame generation, 2x up to 6x**, by Apex Compute, in every supported DirectX 12 game and also in
  Baldur's Gate 3 (DirectX 11). Total capped at 1000 FPS; even frame pacing first; input lag shown in the compact menu;
  turning it on or off never needs a game restart.
- **New games**: Baldur's Gate 3, Microsoft Flight Simulator 2024, The Last of Us Part I, Batman: Arkham City (32-bit)
  and Black Myth: Wukong Benchmark Tool (experimental).
- **Works in more games without a profile**: the mod adapts to what each game delivers, and the menu tells you why the
  network is not running.
- **17 menu languages** (installer in 20).
- **RX 9060 XT 16 GB and RX 7800 XT** reported working; RX 7000 (RDNA 3) gets its own code path in the new engine.

Full list: [CHANGELOG-2.5.md](CHANGELOG-2.5.md) and the [2.5.0 release notes](https://github.com/Wehaveall/AMDLSS5/releases/tag/v2.5.0).

> **First use in each game:** the new engine prepares its shaders the first time (and again after a driver update). The
> compact menu shows a yellow **"Preparing neural engine... NN%"** line while it works; this can take a while and the game
> is not frozen. After that it starts almost instantly.

## Requirements

- One of these AMD Radeon cards:
  - **Reported working by users:** RX 9070 XT, RX 9070, RX 9070 GRE, RX 9060 XT 16 GB (RDNA 4) and RX 7800 XT (RDNA 3).
  - **Beta:** RX 7900 XT / XTX, RX 7700 XT and RX 7600 (RDNA 3) should work but have no user report yet; RX 9060 XT 8 GB
    and RX 9060 should work but their video memory may be short (the installer warns). If the network cannot start, the
    game keeps running with standard FSR and the menu shows the reason. Please report your results.
  - RX 6000 (RDNA 2) and older cards, and integrated graphics, are not supported.
- Windows 10/11 64-bit and a recent AMD Adrenalin driver.
- A CPU with **AVX2** (Intel Core 4th generation / AMD Ryzen or newer).
- A **DirectX 12** game with **FSR 3.1, FSR 3, FSR 2 or XeSS**, or one of the DirectX 11 / 32-bit games in
  [COMPATIBLE_GAMES.md](COMPATIBLE_GAMES.md).

## How to install

1. Download `AMDLSS5-2.5.0.zip` from the releases page and extract **the whole folder** (the `payload` folder stays next
   to the installer).
2. Run `AMDLSS5-Instalador.exe`. It finds the games installed on your PC (Steam, Epic, GOG, Ubisoft, EA, Battle.net and
   Xbox) and shows them as a library of covers with their state.
3. Click the game's cover to **Install**, **Update** or **Remove** the mod. An older version is updated and your menu
   choices are kept. A game that is not in the list can be added with **Add a game manually...**.
4. In game, choose the upscaler listed in [COMPATIBLE_GAMES.md](COMPATIBLE_GAMES.md). **F10** turns the network on and off
   (**Ctrl+F10** in Baldur's Gate 3); **End** opens the mod menu.
5. To remove: open the installer, click the game and choose **Remove**. The game folder goes back exactly as it was.

**Recommended:** set the game's own upscaler sharpening (FSR / DLSS / XeSS "Sharpness") to **0 / Off** in every game. The
game's sharpening adds fine noise that the neural pass amplifies (flickering and ghosting around characters); use the mod's
own filters if you want a sharper image.

**Warning (AMD Adrenalin):** before playing, turn off the Adrenalin metrics overlay and recording/instant replay and set
performance tuning to **Default**; reboot after changing it. With them on, the GPU can hang while the game loads.

More details are in `read-me.txt` inside the package.

## Credits

- **Mochizuki (mochizuki0323)** — the [DLSSNR-AMD](https://github.com/TheAutomatic/DLSSNR-AMD) runtime that the 2.5 neural
  engine is built on (MIT license); RDNA 3 machine code by TheAutomatic, building on mauri870's DLSSNR-RDNA3.
- **Apex Compute** — Neural Apex (Apex Neural Rendering), the frame generator included unmodified in the package. All
  rights in Neural Apex remain with Apex Compute.
- **NVIDIA** — DLSS and the DLSS Neural Rendering network. NVIDIA and DLSS are trademarks of NVIDIA Corporation. This
  project is not affiliated with, sponsored or endorsed by NVIDIA, AMD or Apex Compute. The NVIDIA network data is not
  covered by this project's license.
- **praydog** — [REFramework](https://github.com/praydog/REFramework) (MIT license).
- Every third-party component in the release package (AMD FidelityFX, the bundled models and others) is listed with its
  license in `THIRD_PARTY_NOTICES.txt`.

## License

The license of the code in this repository is in `LICENSE`. NVIDIA's data, the bundled third-party models (some are for
non-commercial use only), Neural Apex, REFramework, AMD FidelityFX and the other third-party components keep their own
licenses; see `THIRD_PARTY_NOTICES.txt` in the package.

If you use or share the mod, please credit **AMDLSS5**. Thank you!

## Support the project

AMDLSS5 is free and will stay free. There is no paid version, no early access and no feature locked behind a donation.

If the mod made your games look better and you want to help, you can buy me a coffee. Every donation, small or large,
helps pay for the hardware, the games used for testing and the many hours spent on each release.

[Donate with Stripe](https://buy.stripe.com/7sY9AT5PjbRT4M47u9d7q01)

### Gift a game on Steam

Want AMDLSS5 to support a game you love? You can also gift that game on Steam. The mod has to be tested and tuned in
each game, so having the game is the fastest way to make it compatible.

- **DirectX 12** games are the main target (a few DirectX 11 and 32-bit games work through bridges; Vulkan games are not
  supported yet).
- Before gifting, check [COMPATIBLE_GAMES.md](COMPATIBLE_GAMES.md): the game may already be supported.
- Send the gift to my Steam profile: https://steamcommunity.com/profiles/76561197992454025/
- A gifted game is a donation, not a purchase: compatibility is a goal, not a promise. Some games have anti-cheat or
  engines that block mods, and those cannot be supported.

Not able to donate? That is completely fine. Starring the repository, reporting bugs with logs and sharing the mod
help just as much. Thank you!
