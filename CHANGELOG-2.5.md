# AMDLSS5 2.5 — Changelog (since 2.2.0)

Version 2.2 was not released on GitHub; its changes are summarized at the end. Supported games: [COMPATIBLE_GAMES.md](COMPATIBLE_GAMES.md).

> **First use in each game:** the new engine prepares its shaders the first time (and again after a driver update). The compact menu shows a yellow "Preparing neural engine... NN%" line while it works; the game is not frozen. After that it starts almost instantly.

## New neural network engine
- **The neural network now runs on a new engine** (Vulkan), built on Mochizuki's DLSSNR-AMD runtime 0.0.8.3 (mochizuki0323, MIT licence; source: https://github.com/TheAutomatic/DLSSNR-AMD). Thanks to Mochizuki's engine together with our own optimizations, the network is up to about 60% faster, measured in Resident Evil Requiem, and the game runs at about 10% more FPS compared with version 2.0. On a Radeon RX 9070 XT the network took about 2 ms per cycle at 1440x810. The old HIP engine is no longer in the package: if the new engine cannot start on a PC, the game keeps running with its standard upscaler.
- **Steadier network in game**: the game now waits for the network job instead of competing with it for the graphics card, so the network time stays constant instead of jumping between two values during a session.
- Some steps of the scene network are fused in the new engine (less work per network cycle).
- **Less video memory**: the old engine and its own copies of the capture buffers, the scene history and the network weights are gone (about 0.5 GB less on our bench).
- **Changing the network resolution in the menu** (810, 864, 1080...) keeps the new engine running.

## Image
- **New installs start with the network's tone intensity at 0.3** (was 1.0). Your own value is kept when you update.

## Neural Apex frame generation (2x to 6x)
- **Neural Apex, the frame generator by Apex Compute, is now in the package and goes into every supported DirectX 12 game.** Turn on "Frame Generation" in the mod menu (End) and pick the multiplier, from 2x to 6x. It works together with the neural network, also in games that have no frame generation of their own.
- **The total on screen is capped at 1000 frames per second.** When the game's rate times the multiplier would pass it, the mod lowers the multiplier and the menu shows the multiplier in use and why; when not even 2x fits, the generator pauses ("FPS too high") and comes back when the rate drops.
- Even frame pacing is preferred over a higher frame count, and the game is held just before presenting to keep input lag low. While it runs, the compact menu shows the input lag in cyan.
- **Never two generators at once**: when the game's own frame generator is on, the mod's lines are greyed out with the reason, in every language.
- **Turning the generator on or off never needs a game restart.** It waits until the game is stable (after start-up or loading), then turns on with a short pause; the compact menu shows how long is left and, if the safety watchdog pauses it, the reason.
- Neural Apex and the neural network now work together in games on the newer D3D12 runtime (PRAGMATA, Monster Hunter Wilds, Onimusha).
- Changing the window or output size no longer closes and reopens Neural Apex.
- The old frame generator of version 1.6 is no longer offered in the menu.

## New games
- **Baldur's Gate 3** (DirectX 11): the mod runs through a bridge that takes the place of the game's `libxess_dx11.dll`; start the DirectX 11 version and choose FSR 2.2 in the game. The installer keeps a verified copy of the game's original file and uninstalling puts it back. **Neural Apex frame generation now works in Baldur's Gate 3 (DirectX 11)**: the mod presents the game through its own DirectX 12 swap chain, so the same Apex as the DirectX 12 games runs here (2x–6x, switch it on and off from the menu without restarting). Exclusive fullscreen becomes borderless. **In Baldur's Gate 3 the network key is Ctrl+F10** (F10 alone stays with the game); End opens the menu.
- **Microsoft Flight Simulator 2024**: runs with the mod's general settings (no game profile needed); choose FSR and turn the game's own frame generation off to use Neural Apex.
- **The Last of Us Part I** (added in 2.2).
- **Batman: Arkham City** (32-bit, no upscaler of its own): the mod runs through a small bridge process and brings the neural network, the menu and frame generation. Uninstalling puts the game folder back exactly as it was.
- **Other 32-bit Direct3D 9 games** (experimental): the installer can install the same bridge and says clearly that only Batman was tried; 32-bit games that do not use Direct3D 9 are refused.
- **Black Myth: Wukong (Benchmark Tool)**: the network runs with the game's FSR 3 through the mod's own FSR 3 entry.

## Works in more games without a profile
- The mod adapts to what the game delivers instead of refusing to run: games that send their own pre-exposure (Unreal Engine) are accepted; when the game gives no usable exposure, the mod measures it; when the upscaler gives no depth, the mod can take it from the game's own depth buffer (XeSS games).
- Own FSR 3 entry (FSR 3.0, 3.1.0 and 3.1.1+ layouts) and own FSR 2 entry, so the game's native FSR is enough.
- **The menu tells you why the network is not running** (about 85 reasons, for example "the game sent no depth").
- Exposure is handled the same way in every engine: the mod uses what the game declares on each frame.

## Menu and languages
- **17 languages in the in-game menu** (new: Ukrainian, Traditional Chinese, Indonesian and Vietnamese). The installer is also in Hindi, Arabic (right to left) and Bengali (20 languages); the in-game menu stays in English in these three for now.
- **The structure sliders go from 0 to 1**, and 1.0 is the maximum: above 1 the network gave less fine detail and a wider haze of light. An older value above 1 now counts as 1.0.
- Network resolution from 540p to 1440p, applied live.
- Menu changes apply between network cycles, so changing options while the game runs no longer hangs the GPU.

## Characters
- Grand Theft Auto V Enhanced: no trailing behind moving characters with one neural pass, and a smaller bright halo around characters against dark backgrounds.

## Graphics cards
- **Reported working by users**: AMD Radeon RX 9070 XT, RX 9070, RX 9070 GRE, RX 9060 XT 16 GB and **RX 7800 XT**.
- **Better RDNA 3 support**: the new engine brings its own RDNA 3 machine code (gfx110x). The **RX 7900 XT and RX 7900 XTX should work**, but there is no user report yet (the same for the RX 7700 XT and RX 7600); the installer shows a short notice. If the network fails, the game keeps running with standard FSR.
- RX 9060 XT 8 GB and RX 9060: should work, no report yet; their video memory may not be enough (the installer warns).
- RX 6000 (RDNA 2) and integrated graphics stay unsupported.

## Installer
- **New installs start in English, with every image filter off and the structure settings at 1.0.** Updating keeps everything you chose in the menu.
- **Updates keep the network resolution you chose on RX 9060 / 9060 XT cards** (the card profile used to put it back to 50% on every update).
- Graphics-card notices follow the user reports above: none for the cards reported working, a short "no report yet" notice for the others.
- **Uninstalling keeps your crash dumps** (`nr_*.dmp`), together with recordings and screenshots: they are moved out of the game folder to `AMDLSS5-gravacoes` on the same drive, never deleted.
- **Uninstalling never deletes through a folder link**: if the mod folder, or one of its folders, is a junction or link to another place, only the link is removed and the files where it points are left alone.
- **Baldur's Gate 3: the game's original DLL is never lost**: if it cannot be put back (for example a file held by an antivirus), the uninstall stops as partial and keeps the original copy; running it again finishes the job.
- **Installing in a protected folder** (run as administrator) now keeps the Script Hook V zip you chose for Grand Theft Auto V and the `--mover-gravacoes` option.
- **The optional REFramework download is pinned** to the exact version in the package and checked by its SHA-256; without that information the download is refused.
- The mod now carries an INI that a hand copy of an older version left next to the game's exe (`amdlss5.ini`) into the mod folder, so your menu choices survive a manual update.

## Stability
- A full code review before 2.5 led to fixes in the loader, the network host, the new engine and Neural Apex. The ones you could have met:
  - fixed a crash while loading in games with the Steam overlay, and a crash or hang when closing the game;
  - fixed the keyboard staying blocked after the menu failed to draw;
  - a passing error when presenting a frame no longer becomes a permanent "device removed";
  - if the game unloads and reloads its upscaler, the mod keeps working;
  - a settings file saved as UTF-8 with a byte-order mark no longer resets every setting;
  - video memory is released after an internal failure instead of staying held until the game restarts.

## Recommended game setting
- Set the game's own upscaler sharpening (FSR / DLSS / XeSS "Sharpness") to **0 / Off** in every game. The game's sharpening adds fine noise that the neural pass amplifies (flickering and ghosting around characters); use the mod's own filters for a sharper image. The mod never changes the game's settings.

## Known issues
- **"Neural passes" set to 2** (two network passes): may show some ghosting and a halo around characters, especially in motion. The default single pass is not affected in the same way.
- Some games may show minor artifacts (for example around hair against bright backgrounds). Fixes are planned for the next update.
- Games that output to an 8- or 10-bit display format still get only the FSR image, without the neural network.
- Cyber Engine Tweaks also uses `version.dll`, so the two cannot run together yet; the installer warns you and keeps CET's file in the backup.
- Neural Apex writes its own log files (`nr_fg-*.log`) in `AMDLSS5\anr\dlssg`; uninstalling removes them.

## Included from 2.2 (not released on GitHub)
- The Last of Us Part I support.
- REFramework installed in every RE Engine game except PRAGMATA.
- Script Hook V support for Grand Theft Auto V Enhanced.
- Complete uninstall: the game folder goes back to its original state.
- Four more languages: Turkish, Russian, Dutch and Polish.
