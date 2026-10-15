# Installer (AMDLSS5-Instalador.exe)

Windows program (WinForms, .NET Framework 4) that installs the mod into a game and uninstalls it by restoring the game
folder as it was. Interface in English by default (Portuguese and Spanish with --idioma pt|es). Source in `src/`, no
dependencies.

## Scripts

- `Build-Instalador.ps1 [-Saida <folder>] [-Versao x.y.z]`: builds the installer.
- `Montar-Pacote.ps1`: builds the release package (installer + `payload`) from the mod binaries.
- `Testar-Instalador.ps1 [-Exe <installer>] [-PayloadReal <payload>]`: end-to-end test on fake game folders.
