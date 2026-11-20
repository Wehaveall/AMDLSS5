# AMDLSS5 — compatible games

Status for **AMDLSS5 2.5.0**. Tested on an AMD Radeon RX 9070 XT unless noted. The installer finds these games on your PC and
shows them as *tested*. Any other DirectX 12 game with FSR 3.1 / FSR 3 / FSR 2 or XeSS may work as *not tested yet*.

**Neural Apex** = the mod's frame generation (2x up to 6x, capped at 1000 FPS in total). It starts off: turn it on in the mod
menu (**End**). Turn the game's own frame generation off first; the mod never runs two generators at once.

**Recommended in every game:** set the game's upscaler sharpening (FSR / DLSS / XeSS "Sharpness") to **0 / Off**.

| Game | Status | In-game setting | Neural Apex |
|---|---|---|---|
| **Baldur's Gate 3** | **New in 2.5** — tested | Start the **DirectX 11** version, choose **FSR 2.2**. Network key is **Ctrl+F10** (F10 alone stays with the game); **End** opens the menu. Exclusive fullscreen becomes borderless. | Yes (2x–6x) |
| **Microsoft Flight Simulator 2024** | **New in 2.5** — tested (general settings, no game profile) | Choose **FSR** as the upscaler; turn the game's frame generation off to use Neural Apex | Yes |
| **The Last of Us Part I** | **New in 2.5** — tested | Choose **FSR** as the upscaler | Yes |
| **Batman: Arkham City (GOTY)** | **New in 2.5** — tested (32-bit, through a bridge process) | No upscaler needed; the installer sets up the bridge and uninstalling restores the game folder exactly | Frame generation through the bridge |
| Black Myth: Wukong (Benchmark Tool) | New in 2.5 — experimental | Choose **FSR 3** | Not tested |
| Resident Evil Requiem | Tested | Choose **FSR 3.1** | Yes |
| Cyberpunk 2077 | Tested | Choose **FSR 3.1**; the game's frame generation off to use Neural Apex | Yes |
| The Witcher 3: Wild Hunt (next-gen, DirectX 12) | Tested | Run `bin\x64_dx12\witcher3.exe` (DirectX 12); choose FSR | Yes |
| Red Dead Redemption 2 | Tested | API **DirectX 12**, upscaler **AMD FSR 2** (the mod upgrades it) | Yes |
| Shadow of the Tomb Raider | Tested | Choose **XeSS** (the mod takes it over) | Yes |
| Final Fantasy XVI | Tested | Choose FSR; turn **dynamic resolution off** | Should work (not re-tested on 2.5) |
| Monster Hunter Wilds | Tested | Choose FSR | Yes |
| Onimusha: Way of the Sword | Tested | Choose FSR | Yes |
| PRAGMATA | Tested | Choose FSR | Yes |
| Nioh 3 | Tested | Choose **XeSS** in Advanced Resolution settings and confirm | Should work (not re-tested on 2.5) |
| Grand Theft Auto V Enhanced | Tested — **Story Mode, offline only** | Choose FSR. BattlEye **off** (Rockstar Games Launcher > Settings > Grand Theft Auto V Enhanced > disable BattlEye, or `-nobattleye`). **Never go online with the mod.** | Yes |
| Stellar Blade (demo) | Supported in 1.6 — not re-tested on 2.5 | Choose FSR | Not tested |
| Other 32-bit Direct3D 9 games | Experimental | The installer can set up the same bridge as Batman and says clearly that only Batman was tried | — |

Not supported: games with anti-cheat (except official offline modes such as GTA V Story Mode), Vulkan and OpenGL games, and
games that output to an 8- or 10-bit display format (they get only the FSR image). Cyber Engine Tweaks also uses
`version.dll` and cannot run together with the mod yet.

Want a game added? See *Gift a game on Steam* in the [README](README.md#gift-a-game-on-steam). Compatibility is a goal, not a promise.
