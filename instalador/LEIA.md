# Instalador (AMDLSS5-Instalador.exe)

Programa Windows (WinForms, .NET Framework 4) que instala o mod num jogo e desinstala devolvendo a pasta do jogo como
era. Interface em português, inglês ou espanhol, conforme o idioma do Windows. Fonte em `src/`, sem dependências.

## Scripts

- `Build-Instalador.ps1 [-Saida <pasta>] [-Versao x.y.z]`: compila o instalador.
- `Montar-Pacote.ps1`: monta o pacote de lançamento (instalador + `payload`) a partir dos binários do mod.
- `Testar-Instalador.ps1 [-Exe <instalador>] [-PayloadReal <payload>]`: teste de ponta a ponta em pastas de jogo falsas.

---

# Installer (AMDLSS5-Instalador.exe)

Windows program (WinForms, .NET Framework 4) that installs the mod into a game and uninstalls it by restoring the game
folder as it was. Interface in Portuguese, English or Spanish, following the Windows language. Source in `src/`, no
dependencies.

## Scripts

- `Build-Instalador.ps1 [-Saida <folder>] [-Versao x.y.z]`: builds the installer.
- `Montar-Pacote.ps1`: builds the release package (installer + `payload`) from the mod binaries.
- `Testar-Instalador.ps1 [-Exe <installer>] [-PayloadReal <payload>]`: end-to-end test on fake game folders.
