// Textos do instalador em ingles (padrao), portugues (Brasil) e espanhol. 1.3.0: o padrao e sempre ingles;
// --idioma pt|en|es escolhe outro.
// Arquivo em UTF-8: o Build-Instalador.ps1 compila com /codepage:65001.
using System;
using System.Collections.Generic;
using System.Globalization;

namespace Amdlss5.Instalador
{
    public static class T
    {
        public static int Indice = 1;   // 1.3.0: ingles por padrao (antes seguia o idioma do Windows)
        public static string Codigo { get { return Indice == 0 ? "pt" : Indice == 2 ? "es" : "en"; } }

        static int Escolher(string iso)
        {
            iso = (iso ?? "").ToLowerInvariant();
            if (iso == "pt") return 0;
            if (iso == "es") return 2;
            return 1;
        }

        public static void Definir(string iso) { Indice = Escolher(iso); }

        public static string S(string chave)
        {
            string[] t;
            if (!Tab.TryGetValue(chave, out t)) return chave;
            return t[Indice] ?? t[1];
        }

        public static string F(string chave, params object[] args)
        {
            try { return string.Format(S(chave), args); }
            catch (FormatException) { return S(chave) + " " + string.Join(" ", Array.ConvertAll(args, a => Convert.ToString(a))); }
        }

        // chave -> { pt, en, es }
        static readonly Dictionary<string, string[]> Tab = new Dictionary<string, string[]>
        {
            // ---------------------------------------------------------------- janela
            { "titulo", new[] { "AMDLSS5 MOD - Instalador", "AMDLSS5 MOD - Installer", "AMDLSS5 MOD - Instalador" } },
            { "subtitulo", new[] { "Rede neural do DLSS 5 em placas AMD Radeon (jogos DirectX 12 com FSR 3.1)", "DLSS 5 neural network on AMD Radeon cards (DirectX 12 games with FSR 3.1)", "Red neuronal de DLSS 5 en tarjetas AMD Radeon (juegos DirectX 12 con FSR 3.1)" } },
            { "passo1", new[] { "1. Escolha o jogo: a pasta do jogo ou o .exe dele", "1. Choose the game: its folder or its .exe", "1. Elige el juego: su carpeta o su .exe" } },
            { "btn_pasta", new[] { "Selecionar pasta do jogo...", "Select game folder...", "Seleccionar carpeta del juego..." } },
            { "btn_exe", new[] { "Selecionar o .exe...", "Select the .exe...", "Seleccionar el .exe..." } },
            { "dlg_pasta", new[] { "Escolha a pasta do jogo", "Choose the game folder", "Elige la carpeta del juego" } },
            { "dlg_exe", new[] { "Escolha o executável do jogo", "Choose the game executable", "Elige el ejecutable del juego" } },
            { "filtro_exe", new[] { "Executável do jogo (*.exe)|*.exe", "Game executable (*.exe)|*.exe", "Ejecutable del juego (*.exe)|*.exe" } },
            { "passo2", new[] { "2. Jogo detectado", "2. Detected game", "2. Juego detectado" } },
            { "nenhum_jogo", new[] { "Nenhum jogo escolhido ainda.", "No game chosen yet.", "Todavía no se eligió ningún juego." } },
            { "detectando", new[] { "Analisando a pasta...", "Analyzing the folder...", "Analizando la carpeta..." } },
            { "k_jogo", new[] { "Jogo", "Game", "Juego" } },
            { "k_exe", new[] { "Executável", "Executable", "Ejecutable" } },
            { "k_pasta", new[] { "Pasta do mod", "Mod folder", "Carpeta del mod" } },
            { "k_dx12", new[] { "DirectX 12", "DirectX 12", "DirectX 12" } },
            { "k_fsr", new[] { "FSR 3.1", "FSR 3.1", "FSR 3.1" } },
            { "k_re", new[] { "RE Engine", "RE Engine", "RE Engine" } },
            { "k_nvidia", new[] { "Arquivos NVIDIA", "NVIDIA files", "Archivos NVIDIA" } },
            { "k_estado", new[] { "Mod", "Mod", "Mod" } },
            { "v_sim", new[] { "sim", "yes", "sí" } },
            { "v_nao_confirmado", new[] { "não confirmado", "not confirmed", "no confirmado" } },
            { "v_fsr_mono", new[] { "sim ({0}, versão {1})", "yes ({0}, version {1})", "sí ({0}, versión {1})" } },
            { "v_fsr_outro", new[] { "não reconhecido ({0})", "not recognized ({0})", "no reconocido ({0})" } },
            { "v_fsr_nenhum", new[] { "não encontrado", "not found", "no encontrado" } },
            { "v_re_sim", new[] { "sim ({0}): REFramework necessário", "yes ({0}): REFramework required", "sí ({0}): REFramework necesario" } },
            { "v_re_nao", new[] { "não", "no", "no" } },
            { "v_nvidia_pacote", new[] { "incluídos no pacote", "included in the package", "incluidos en el paquete" } },
            { "v_nvidia_falta", new[] { "faltando no pacote: {0} (será pedido)", "missing from the package: {0} (you will be asked)", "faltan en el paquete: {0} (se pedirán)" } },
            { "v_mod_nao", new[] { "não instalado", "not installed", "no instalado" } },
            { "v_mod_sim", new[] { "instalado ({0} instalação(ões) com backup)", "installed ({0} install(s) with backup)", "instalado ({0} instalación(es) con copia)" } },
            { "v_mod_sem_backup", new[] { "presente, mas instalado sem este instalador (sem backup para desfazer)", "present, but installed without this installer (no backup to undo)", "presente, pero instalado sin este instalador (sin copia para deshacer)" } },
            { "v_storage", new[] { " + _storage_", " + _storage_", " + _storage_" } },
            { "chk_ref", new[] { "Instalar REFramework (dinput8.dll; necessário em jogos RE Engine da Capcom)", "Install REFramework (dinput8.dll; required by Capcom RE Engine games)", "Instalar REFramework (dinput8.dll; necesario en juegos RE Engine de Capcom)" } },
            { "chk_baixar", new[] { "Se o REFramework não vier no pacote, permitir baixá-lo do GitHub", "If REFramework is not in the package, allow downloading it from GitHub", "Si REFramework no viene en el paquete, permitir descargarlo de GitHub" } },
            { "btn_instalar", new[] { "Instalar", "Install", "Instalar" } },
            { "btn_desinstalar", new[] { "Desinstalar", "Uninstall", "Desinstalar" } },
            { "btn_abrir", new[] { "Abrir pasta", "Open folder", "Abrir carpeta" } },
            { "btn_logs", new[] { "Coletar logs", "Collect logs", "Recopilar logs" } },
            { "logs_ok", new[] { "Logs salvos em {0} (anexe numa issue do GitHub).", "Logs saved to {0} (attach it to a GitHub issue).", "Logs guardados en {0} (adjuntalo en un issue de GitHub)." } },
            { "logs_nada", new[] { "Nenhum log do mod nesta pasta. Rode o jogo com o mod uma vez e tente de novo.", "No mod logs in this folder. Run the game with the mod once and try again.", "No hay logs del mod en esta carpeta. Ejecuta el juego con el mod una vez e intentalo de nuevo." } },
            { "rodape", new[] { "Pacote {0}  -  o que for trocado vai antes para AMDLSS5_backup; Desinstalar restaura tudo.", "Package {0}  -  anything replaced is saved to AMDLSS5_backup first; Uninstall restores everything.", "Paquete {0}  -  lo reemplazado se guarda antes en AMDLSS5_backup; Desinstalar restaura todo." } },
            { "sem_pacote", new[] { "Pacote do mod não encontrado (pasta payload ao lado do instalador). Só a desinstalação funciona.", "Mod package not found (payload folder next to the installer). Only uninstall works.", "Paquete del mod no encontrado (carpeta payload junto al instalador). Solo funciona desinstalar." } },
            { "pronto", new[] { "Pronto.", "Ready.", "Listo." } },
            { "avisos_titulo", new[] { "Atenção", "Warning", "Atención" } },
            { "avisos_continuar", new[] { "{0}\r\n\r\nInstalar mesmo assim? (tudo pode ser desfeito com Desinstalar)", "{0}\r\n\r\nInstall anyway? (everything can be undone with Uninstall)", "{0}\r\n\r\n¿Instalar de todos modos? (todo se puede deshacer con Desinstalar)" } },
            { "bloqueado", new[] { "Não dá para instalar neste jogo:\r\n\r\n{0}", "Cannot install on this game:\r\n\r\n{0}", "No se puede instalar en este juego:\r\n\r\n{0}" } },
            { "pedir_nvngx", new[] { "O pacote não traz o arquivo da NVIDIA nvngx_dlssnr.dll (a rede neural do DLSS 5).\r\n\r\nNa próxima janela, escolha o seu nvngx_dlssnr.dll. Ele será copiado para a pasta do jogo.", "The package does not include the NVIDIA file nvngx_dlssnr.dll (the DLSS 5 neural network).\r\n\r\nIn the next window, choose your nvngx_dlssnr.dll. It will be copied to the game folder.", "El paquete no incluye el archivo de NVIDIA nvngx_dlssnr.dll (la red neuronal de DLSS 5).\r\n\r\nEn la siguiente ventana, elige tu nvngx_dlssnr.dll. Se copiará a la carpeta del juego." } },
            { "pedir_pesos", new[] { "O pacote não traz os pesos da rede (dlssnr_on_amd_weights.bin). Sem eles o mod não funciona.\r\n\r\nSe você tem o arquivo, escolha-o na próxima janela. Senão, cancele: nada será instalado.", "The package does not include the network weights (dlssnr_on_amd_weights.bin). The mod does not work without them.\r\n\r\nIf you have the file, choose it in the next window. Otherwise cancel: nothing will be installed.", "El paquete no incluye los pesos de la red (dlssnr_on_amd_weights.bin). Sin ellos el mod no funciona.\r\n\r\nSi tienes el archivo, elígelo en la siguiente ventana. Si no, cancela: no se instalará nada." } },
            { "filtro_nvngx", new[] { "nvngx_dlssnr.dll|nvngx_dlssnr.dll|DLL (*.dll)|*.dll", "nvngx_dlssnr.dll|nvngx_dlssnr.dll|DLL (*.dll)|*.dll", "nvngx_dlssnr.dll|nvngx_dlssnr.dll|DLL (*.dll)|*.dll" } },
            { "filtro_pesos", new[] { "dlssnr_on_amd_weights.bin|dlssnr_on_amd_weights.bin|BIN (*.bin)|*.bin", "dlssnr_on_amd_weights.bin|dlssnr_on_amd_weights.bin|BIN (*.bin)|*.bin", "dlssnr_on_amd_weights.bin|dlssnr_on_amd_weights.bin|BIN (*.bin)|*.bin" } },
            { "cancelado", new[] { "Cancelado: nada foi instalado.", "Cancelled: nothing was installed.", "Cancelado: no se instaló nada." } },
            { "nvngx_invalido", new[] { "{0} não é uma DLL x64 válida.", "{0} is not a valid x64 DLL.", "{0} no es una DLL x64 válida." } },
            { "sucesso_titulo", new[] { "Instalado", "Installed", "Instalado" } },
            { "sucesso", new[] { "Mod instalado em:\r\n{0}\r\n\r\nNo jogo: F10 liga/desliga a rede neural, End abre o menu do mod.\r\nUse FSR 3.1 nas opções gráficas do jogo.\r\n\r\nPara desfazer: abra este instalador, escolha o jogo e clique em Desinstalar.", "Mod installed in:\r\n{0}\r\n\r\nIn game: F10 toggles the neural network, End opens the mod menu.\r\nUse FSR 3.1 in the game graphics options.\r\n\r\nTo undo: open this installer, choose the game and click Uninstall.", "Mod instalado en:\r\n{0}\r\n\r\nEn el juego: F10 activa/desactiva la red neuronal, End abre el menú del mod.\r\nUsa FSR 3.1 en las opciones gráficas del juego.\r\n\r\nPara deshacer: abre este instalador, elige el juego y pulsa Desinstalar." } },
            { "confirma_des", new[] { "Desinstalar o mod de\r\n{0}?\r\n\r\nOs arquivos originais do jogo serão restaurados do backup.", "Uninstall the mod from\r\n{0}?\r\n\r\nThe original game files will be restored from the backup.", "¿Desinstalar el mod de\r\n{0}?\r\n\r\nLos archivos originales del juego se restaurarán desde la copia." } },
            { "des_titulo", new[] { "Desinstalado", "Uninstalled", "Desinstalado" } },
            { "des_sucesso", new[] { "Mod removido. A pasta do jogo voltou ao que era antes.", "Mod removed. The game folder is back to how it was.", "Mod eliminado. La carpeta del juego volvió a como estaba." } },
            { "jogo_aberto_titulo", new[] { "Feche o jogo", "Close the game", "Cierra el juego" } },
            { "permissao_titulo", new[] { "Sem permissão", "No permission", "Sin permiso" } },
            { "permissao_admin", new[] { "{0}\r\n\r\nAbrir o instalador de novo como administrador?", "{0}\r\n\r\nOpen the installer again as administrator?", "{0}\r\n\r\n¿Abrir el instalador de nuevo como administrador?" } },
            { "erro_titulo", new[] { "Erro", "Error", "Error" } },
            // ---------------------------------------------------------------- biblioteca (janela padrão)
            { "bib_titulo", new[] { "AMDLSS5 MOD - Biblioteca de jogos", "AMDLSS5 MOD - Game library", "AMDLSS5 MOD - Biblioteca de juegos" } },
            { "bib_procurando", new[] { "Procurando jogos...", "Looking for games...", "Buscando juegos..." } },
            { "bib_nenhum", new[] { "Nenhum jogo encontrado. Use \"Adicionar jogo manualmente\".", "No games found. Use \"Add a game manually\".", "No se encontró ningún juego. Usa \"Agregar un juego manualmente\"." } },
            { "bib_contagem", new[] { "{0} jogo(s) encontrado(s)", "{0} game(s) found", "{0} juego(s) encontrado(s)" } },
            { "bib_manual", new[] { "Adicionar jogo manualmente...", "Add a game manually...", "Agregar un juego manualmente..." } },
            { "bib_idioma", new[] { "Idioma:", "Language:", "Idioma:" } },
            { "bib_escolha", new[] { "Clique num jogo para instalar, atualizar ou remover o mod.", "Click a game to install, update or remove the mod.", "Haz clic en un juego para instalar, actualizar o quitar el mod." } },
            { "bib_remover", new[] { "Remover", "Remove", "Quitar" } },
            { "bib_atualizar", new[] { "Atualizar", "Update", "Actualizar" } },
            { "bib_nao_instalado", new[] { "Não instalado", "Not installed", "No instalado" } },
            { "bib_instalado", new[] { "Instalado", "Installed", "Instalado" } },
            { "bib_instalado_v", new[] { "Instalado v{0}", "Installed v{0}", "Instalado v{0}" } },
            { "bib_atualizar_selo", new[] { "Atualizar", "Update available", "Actualizar" } },
            { "bib_estado_atualizar", new[] { "Instalado: v{0}. O pacote traz a v{1}: clique em Atualizar.", "Installed: v{0}. The package has v{1}: click Update.", "Instalado: v{0}. El paquete trae la v{1}: pulsa Actualizar." } },
            { "bib_estado_atualizar_desconhecida", new[] { "Instalado: versão desconhecida. O pacote traz a v{0}: clique em Atualizar.", "Installed: unknown version. The package has v{0}: click Update.", "Instalado: versión desconocida. El paquete trae la v{0}: pulsa Actualizar." } },
            { "bib_erro_loja", new[] { "Não foi possível ler a biblioteca da {0}.", "Could not read the {0} library.", "No se pudo leer la biblioteca de {0}." } },
            { "bib_erro_jogo", new[] { "Não foi possível verificar {0}.", "Could not check {0}.", "No se pudo comprobar {0}." } },
            { "bib_confirma_nao_testado", new[] { "{0} ainda não foi testado com o mod. Ele parece compatível, mas pode não funcionar ou ter defeitos.{1}\r\n\r\nInstalar mesmo assim? (tudo pode ser desfeito com Remover)", "{0} has not been tested with the mod yet. It looks compatible, but it may not work or may have defects.{1}\r\n\r\nInstall anyway? (everything can be undone with Remove)", "{0} aún no se probó con el mod. Parece compatible, pero puede no funcionar o tener defectos.{1}\r\n\r\n¿Instalar de todos modos? (todo se puede deshacer con Quitar)" } },
            { "bib_confirma_remover", new[] { "Remover o mod de\r\n{0}?\r\n\r\nOs arquivos originais do jogo serão restaurados do backup.", "Remove the mod from\r\n{0}?\r\n\r\nThe original game files will be restored from the backup.", "¿Quitar el mod de\r\n{0}?\r\n\r\nLos archivos originales del juego se restaurarán desde la copia." } },
            { "bib_sucesso", new[] { "Mod instalado em:\r\n{0}\r\n\r\nNo jogo: F10 liga/desliga a rede neural, End abre o menu do mod.\r\nUse FSR 3.1 nas opções gráficas do jogo.\r\n\r\nPara desfazer: abra este instalador, clique no jogo e em Remover.", "Mod installed in:\r\n{0}\r\n\r\nIn game: F10 toggles the neural network, End opens the mod menu.\r\nUse FSR 3.1 in the game graphics options.\r\n\r\nTo undo: open this installer, click the game and then Remove.", "Mod instalado en:\r\n{0}\r\n\r\nEn el juego: F10 activa/desactiva la red neuronal, End abre el menú del mod.\r\nUsa FSR 3.1 en las opciones gráficas del juego.\r\n\r\nPara deshacer: abre este instalador, haz clic en el juego y en Quitar." } },
            { "bib_admin", new[] { "{0}\r\n\r\nFazer esta ação como administrador? O Windows vai pedir a permissão.", "{0}\r\n\r\nDo this as administrator? Windows will ask for permission.", "{0}\r\n\r\n¿Hacer esta acción como administrador? Windows pedirá el permiso." } },
            { "bib_admin_rodando", new[] { "Rodando como administrador...", "Running as administrator...", "Ejecutando como administrador..." } },
            { "bib_admin_codigo", new[] { "Como administrador: código {0}", "As administrator: exit code {0}", "Como administrador: código {0}" } },
            { "bib_admin_falhou", new[] { "A ação como administrador terminou com o código {0}. Veja o log na janela.", "The action as administrator ended with exit code {0}. See the log in the window.", "La acción como administrador terminó con el código {0}. Mira el registro en la ventana." } },
            { "bib_rodape", new[] { "Pacote {0}  -  o que for trocado vai antes para AMDLSS5_backup; Remover restaura tudo.", "Package {0}  -  anything replaced is saved to AMDLSS5_backup first; Remove restores everything.", "Paquete {0}  -  lo reemplazado se guarda antes en AMDLSS5_backup; Quitar restaura todo." } },
            { "aguarde_fechar", new[] { "Aguarde: a instalação/desinstalação ainda está em andamento. Fechar agora deixaria a pasta do jogo pela metade.", "Please wait: the install/uninstall is still running. Closing now would leave the game folder half done.", "Espera: la instalación/desinstalación sigue en curso. Cerrar ahora dejaría la carpeta del juego a medias." } },

            // ---------------------------------------------------------------- detecção
            { "det_nao_exe", new[] { "{0} não é um .exe.", "{0} is not an .exe.", "{0} no es un .exe." } },
            { "det_nao_existe", new[] { "{0} não existe.", "{0} does not exist.", "{0} no existe." } },
            { "det_sem_exe", new[] { "Nenhum executável de jogo x64 encontrado em {0} (procurei até 3 pastas abaixo). Escolha o .exe do jogo.", "No x64 game executable found in {0} (searched 3 folders deep). Choose the game .exe.", "No se encontró ningún ejecutable x64 de juego en {0} (busqué hasta 3 carpetas abajo). Elige el .exe del juego." } },
            { "det_nao_x64", new[] { "{0} não é um executável de 64 bits.", "{0} is not a 64-bit executable.", "{0} no es un ejecutable de 64 bits." } },
            { "det_sem_version", new[] { "{0} não carrega version.dll: o mod não tem como entrar neste jogo.", "{0} does not load version.dll: the mod has no way into this game.", "{0} no carga version.dll: el mod no tiene cómo entrar en este juego." } },
            { "grupo_testado", new[] { "Testado", "Tested", "Probado" } },
            { "grupo_nao_testado", new[] { "Ainda não testado", "Not tested yet", "Aún no probado" } },
            { "grupo_nao_suportado", new[] { "Ainda não suportado", "Not supported yet", "Aún no compatible" } },
            { "motivo_anticheat", new[] { "Jogo online com anticheat: jogos competitivos banem quem usa o mod. O mod não é instalado.", "Online game with anti-cheat: competitive games ban players who use the mod. The mod is not installed.", "Juego en línea con anti-trampas: los juegos competitivos banean a quien usa el mod. El mod no se instala." } },
            { "bib_tarja_anticheat", new[] { "ANTICHEAT — NÃO INSTALAR\nrisco de banimento", "ANTI-CHEAT — DO NOT INSTALL\nrisk of ban", "ANTI-TRAMPAS — NO INSTALAR\nriesgo de baneo" } },
            { "motivo_windowsapps", new[] { "Jogo da loja do Windows (WindowsApps): a pasta é protegida, o mod não é instalado.", "Windows Store game (WindowsApps): the folder is protected, the mod is not installed.", "Juego de la tienda de Windows (WindowsApps): la carpeta está protegida, el mod no se instala." } },
            { "det_sem_fsr", new[] { "FSR 3.1 não encontrado ao lado do exe (amd_fidelityfx_dx12.dll ou amd_fidelityfx_loader/upscaler_dx12.dll). O mod precisa do FSR 3.1 do jogo.", "FSR 3.1 not found next to the exe (amd_fidelityfx_dx12.dll or amd_fidelityfx_loader/upscaler_dx12.dll). The mod needs the game's FSR 3.1.", "FSR 3.1 no encontrado junto al exe (amd_fidelityfx_dx12.dll o amd_fidelityfx_loader/upscaler_dx12.dll). El mod necesita el FSR 3.1 del juego." } },
            { "det_fsr_outro", new[] { "Arquivo FSR não reconhecido ({0}): pode ser FSR 2/3.0, que o mod não suporta.", "Unrecognized FSR file ({0}): it may be FSR 2/3.0, which the mod does not support.", "Archivo FSR no reconocido ({0}): puede ser FSR 2/3.0, que el mod no soporta." } },
            { "det_sem_dx12", new[] { "DirectX 12 não confirmado (o exe não importa d3d12.dll e não há FSR dx12 ao lado).", "DirectX 12 not confirmed (the exe does not import d3d12.dll and there is no FSR dx12 next to it).", "DirectX 12 no confirmado (el exe no importa d3d12.dll y no hay FSR dx12 al lado)." } },
            { "det_version_alheio", new[] { "Já existe um version.dll de outro mod ({0}). Ele será guardado no backup e substituído.", "A version.dll from another mod already exists ({0}). It will be saved to the backup and replaced.", "Ya existe un version.dll de otro mod ({0}). Se guardará en la copia y se reemplazará." } },

            // ---------------------------------------------------------------- pacote
            { "erro_sem_payload", new[] { "Pacote do mod não encontrado: {0}\\pacote.json", "Mod package not found: {0}\\pacote.json", "Paquete del mod no encontrado: {0}\\pacote.json" } },
            { "erro_payload_json", new[] { "pacote.json inválido ({0}): {1}", "invalid pacote.json ({0}): {1}", "pacote.json inválido ({0}): {1}" } },
            { "erro_payload_falta", new[] { "O pacote está incompleto: falta {0}. Baixe o pacote de novo.", "The package is incomplete: {0} is missing. Download the package again.", "El paquete está incompleto: falta {0}. Descarga el paquete de nuevo." } },
            { "erro_sem_nvngx", new[] { "Falta {0} (arquivo da NVIDIA). Escolha o seu nvngx_dlssnr.dll.", "{0} (NVIDIA file) is missing. Choose your nvngx_dlssnr.dll.", "Falta {0} (archivo de NVIDIA). Elige tu nvngx_dlssnr.dll." } },
            { "erro_sem_pesos", new[] { "Falta {0} (pesos da rede neural). Sem ele o mod não funciona; o pacote completo o traz.", "{0} (neural network weights) is missing. The mod does not work without it; the full package includes it.", "Falta {0} (pesos de la red neuronal). Sin él, el mod no funciona; el paquete completo lo incluye." } },
            { "erro_hash", new[] { "O arquivo {0} do pacote está corrompido (hash diferente). Baixe o pacote de novo.", "Package file {0} is corrupted (hash mismatch). Download the package again.", "El archivo {0} del paquete está dañado (hash distinto). Descarga el paquete de nuevo." } },
            { "erro_dll_invalida", new[] { "{0} não é uma DLL x64 válida ({1}).", "{0} is not a valid x64 DLL ({1}).", "{0} no es una DLL x64 válida ({1})." } },
            { "aviso_hash_usuario", new[] { "Aviso: o seu {0} ({1}) não é a versão testada ({2}); vai instalado mesmo assim.", "Warning: your {0} ({1}) is not the tested version ({2}); installing it anyway.", "Aviso: tu {0} ({1}) no es la versión probada ({2}); se instala de todos modos." } },
            { "conferindo_pacote", new[] { "Conferindo o pacote {0} ({1} arquivos)...", "Checking package {0} ({1} files)...", "Comprobando el paquete {0} ({1} archivos)..." } },

            // ---------------------------------------------------------------- REFramework
            { "ref_embutido", new[] { "REFramework do pacote ({0}).", "REFramework from the package ({0}).", "REFramework del paquete ({0})." } },
            { "ref_consultando", new[] { "Consultando a versão mais nova do REFramework: {0}", "Looking up the latest REFramework: {0}", "Consultando la versión más nueva de REFramework: {0}" } },
            { "ref_baixando", new[] { "Baixando REFramework {0} ({1}) de {2}", "Downloading REFramework {0} ({1}) from {2}", "Descargando REFramework {0} ({1}) de {2}" } },
            { "ref_baixado", new[] { "REFramework {0} baixado e conferido (dinput8.dll {1}, PE x64 com DirectInput8Create).", "REFramework {0} downloaded and checked (dinput8.dll {1}, x64 PE with DirectInput8Create).", "REFramework {0} descargado y comprobado (dinput8.dll {1}, PE x64 con DirectInput8Create)." } },
            { "erro_ref_sem", new[] { "O pacote não traz o REFramework e o download está desligado. Marque a opção de baixar do GitHub ou instale o REFramework você mesmo.", "The package has no REFramework and download is off. Tick the option to download it from GitHub, or install REFramework yourself.", "El paquete no trae REFramework y la descarga está desactivada. Marca la opción de descargar de GitHub o instala REFramework tú mismo." } },
            { "erro_ref_rede", new[] { "Não consegui baixar o REFramework (internet?): {0}", "Could not download REFramework (internet?): {0}", "No pude descargar REFramework (¿internet?): {0}" } },
            { "erro_ref_sem_zip", new[] { "O release {0} do REFramework não tem REFramework.zip.", "REFramework release {0} has no REFramework.zip.", "El release {0} de REFramework no tiene REFramework.zip." } },
            { "erro_ref_url", new[] { "Endereço de download inesperado, recusado: {0}", "Unexpected download address, refused: {0}", "Dirección de descarga inesperada, rechazada: {0}" } },
            { "erro_ref_incompleto", new[] { "Download do REFramework incompleto.", "REFramework download incomplete.", "Descarga de REFramework incompleta." } },
            { "erro_ref_sem_dll", new[] { "REFramework.zip sem dinput8.dll.", "REFramework.zip has no dinput8.dll.", "REFramework.zip sin dinput8.dll." } },
            { "erro_ref_invalido", new[] { "dinput8.dll inválido (não é DLL x64 com DirectInput8Create): {0}", "invalid dinput8.dll (not an x64 DLL exporting DirectInput8Create): {0}", "dinput8.dll inválido (no es DLL x64 con DirectInput8Create): {0}" } },
            { "ref_alheio_mantido", new[] { "Já existe um dinput8.dll que não é o REFramework (outro mod): fica o dele, REFramework não instalado.", "A dinput8.dll that is not REFramework already exists (another mod): keeping it, REFramework not installed.", "Ya existe un dinput8.dll que no es REFramework (otro mod): se mantiene, REFramework no instalado." } },
            { "ref_ja", new[] { "REFramework já instalado (mesmo dinput8.dll).", "REFramework already installed (same dinput8.dll).", "REFramework ya instalado (mismo dinput8.dll)." } },
            { "ref_mantido", new[] { "dinput8.dll existente mantido (o seu REFramework ou outro mod).", "Existing dinput8.dll kept (your REFramework or another mod).", "dinput8.dll existente mantenido (tu REFramework u otro mod)." } },
            { "ref_instalado", new[] { "REFramework instalado: {0}", "REFramework installed: {0}", "REFramework instalado: {0}" } },

            // ---------------------------------------------------------------- instalação
            { "erro_permissao", new[] { "Sem permissão de escrita em {0}. Jogos em C:\\Program Files exigem o instalador como administrador.", "No write permission in {0}. Games under C:\\Program Files need the installer to run as administrator.", "Sin permiso de escritura en {0}. Los juegos en C:\\Program Files necesitan el instalador como administrador." } },
            { "erro_escrita", new[] { "Não consegui escrever em {0}: {1}", "Could not write to {0}: {1}", "No pude escribir en {0}: {1}" } },
            { "erro_espaco", new[] { "Espaço livre insuficiente: {0} livres, precisa de ~{1}.", "Not enough free space: {0} free, needs ~{1}.", "Espacio libre insuficiente: {0} libres, necesita ~{1}." } },
            { "erro_jogo_aberto", new[] { "O jogo está aberto ({0}). Feche o jogo e tente de novo.", "The game is running ({0}). Close the game and try again.", "El juego está abierto ({0}). Cierra el juego e inténtalo de nuevo." } },
            { "erro_backup", new[] { "Backup de {0} divergiu.", "Backup of {0} does not match.", "La copia de {0} no coincide." } },
            { "erro_copia", new[] { "{0} ficou diferente depois de copiar (disco?).", "{0} differs after copying (disk?).", "{0} quedó distinto tras copiar (¿disco?)." } },
            { "erro_sem_backup", new[] { "Não há backup deste instalador em {0}\\AMDLSS5_backup: nada para desinstalar.", "There is no backup from this installer in {0}\\AMDLSS5_backup: nothing to uninstall.", "No hay copia de este instalador en {0}\\AMDLSS5_backup: nada que desinstalar." } },
            { "log_cabecalho", new[] { "{0} {1} -> {2}", "{0} {1} -> {2}", "{0} {1} -> {2}" } },
            { "instalando_em", new[] { "Instalando em {0}", "Installing to {0}", "Instalando en {0}" } },
            { "arq_igual", new[] { "  = {0} (já igual)", "  = {0} (already identical)", "  = {0} (ya igual)" } },
            { "arq_trocado", new[] { "  ~ {0} ({1} -> {2}, original no backup)", "  ~ {0} ({1} -> {2}, original in the backup)", "  ~ {0} ({1} -> {2}, original en la copia)" } },
            { "arq_novo", new[] { "  + {0}", "  + {0}", "  + {0}" } },
            { "arq_ffx_mantido", new[] { "  = {0} (versão {1}, mais nova que a do pacote: mantida)", "  = {0} (version {1}, newer than the package's: kept)", "  = {0} (versión {1}, más nueva que la del paquete: se mantiene)" } },
            { "ini_mesclado", new[] { "  ~ {0} (já existia: só {1} chave(s) nova(s) acrescentada(s))", "  ~ {0} (already existed: only {1} new key(s) added)", "  ~ {0} (ya existía: solo {1} clave(s) nueva(s) agregada(s))" } },
            { "ini_novo", new[] { "  + {0}", "  + {0}", "  + {0}" } },
            { "ini_trocado", new[] { "  ~ {0} (configurações desta versão; {1} preferência(s) sua(s) mantida(s): idioma, menu, limite de FPS, filtros)", "  ~ {0} (this version's settings; {1} of your preference(s) kept: language, menu, FPS limit, filters)", "  ~ {0} (ajustes de esta versión; {1} preferencia(s) tuya(s) conservada(s): idioma, menú, límite de FPS, filtros)" } },
            { "mod_cria", new[] { "  . {0} (o mod cria ao abrir o jogo; sai ao desinstalar)", "  . {0} (the mod creates it when the game starts; removed on uninstall)", "  . {0} (el mod lo crea al abrir el juego; se quita al desinstalar)" } },
            { "mod_guardado", new[] { "  = {0} (já existia: cópia guardada; volta ao desinstalar)", "  = {0} (already existed: copy kept; restored on uninstall)", "  = {0} (ya existía: copia guardada; vuelve al desinstalar)" } },
            { "instalado_ok", new[] { "Instalado. Backup e log: {0}", "Installed. Backup and log: {0}", "Instalado. Copia y registro: {0}" } },
            { "consolidado", new[] { "Versão anterior do mod substituída: o backup continua numa pasta só, com os arquivos originais do jogo ({0} pasta(s) juntada(s), {1} liberados). Pasta: {2}", "Previous mod version replaced: the backup stays in a single folder with the original game files ({0} folder(s) merged, {1} freed). Folder: {2}", "Versión anterior del mod reemplazada: la copia queda en una sola carpeta con los archivos originales del juego ({0} carpeta(s) unida(s), {1} liberados). Carpeta: {2}" } },
            { "consolidar_falhou", new[] { "Aviso: não deu para juntar as pastas de backup ({0}); elas ficam separadas e a desinstalação continua funcionando.", "Warning: could not merge the backup folders ({0}); they stay separate and uninstalling still works.", "Aviso: no se pudieron unir las carpetas de copia ({0}); quedan separadas y la desinstalación sigue funcionando." } },
            { "nota_storage", new[] { "Nota: se o jogo criar a pasta _storage_ depois, rode o instalador de novo para copiar o mod para lá.", "Note: if the game creates a _storage_ folder later, run the installer again to copy the mod there.", "Nota: si el juego crea la carpeta _storage_ después, ejecuta el instalador de nuevo para copiar el mod allí." } },
            { "falhou_desfazendo", new[] { "Falhou: {0} -- desfazendo o que já foi feito...", "Failed: {0} -- undoing what was done...", "Falló: {0} -- deshaciendo lo hecho..." } },
            { "desfazer_falhou", new[] { "Não consegui desfazer tudo: {0}", "Could not undo everything: {0}", "No pude deshacer todo: {0}" } },
            { "desfeito_ok", new[] { "Nada ficou no jogo: tudo foi desfeito.", "Nothing was left in the game: everything was undone.", "No quedó nada en el juego: todo se deshizo." } },
            { "desfeito_parcial", new[] { "Parte ficou no jogo: use Desinstalar (o backup está em AMDLSS5_backup).", "Part was left in the game: use Uninstall (the backup is in AMDLSS5_backup).", "Parte quedó en el juego: usa Desinstalar (la copia está en AMDLSS5_backup)." } },

            // ---------------------------------------------------------------- desinstalação
            { "des_sessao", new[] { "Desfazendo a instalação {0}", "Undoing install {0}", "Deshaciendo la instalación {0}" } },
            { "des_sem_acoes", new[] { "  sem acoes.json em {0}: ignorada", "  no acoes.json in {0}: skipped", "  sin acoes.json en {0}: omitida" } },
            { "des_fora", new[] { "  caminho fora da pasta do jogo recusado: {0}", "  path outside the game folder refused: {0}", "  ruta fuera de la carpeta del juego rechazada: {0}" } },
            { "des_sem_backup", new[] { "  falta o backup de {0}", "  backup of {0} is missing", "  falta la copia de {0}" } },
            { "des_mudou_restaura", new[] { "  {0} mudou depois da instalação; volta o original do backup", "  {0} changed after install; restoring the original from the backup", "  {0} cambió tras la instalación; vuelve el original de la copia" } },
            { "des_divergiu", new[] { "  {0} restaurado diferente do original!", "  {0} restored but differs from the original!", "  ¡{0} restaurado distinto del original!" } },
            { "des_restaurado", new[] { "  < {0} (original restaurado)", "  < {0} (original restored)", "  < {0} (original restaurado)" } },
            { "des_removido", new[] { "  - {0}", "  - {0}", "  - {0}" } },
            { "des_falhou", new[] { "  não deu para desfazer {0} ({1}); o resto continua e o backup fica para uma nova desinstalação", "  could not undo {0} ({1}); the rest continues and the backup is kept for another uninstall", "  no se pudo deshacer {0} ({1}); el resto sigue y la copia queda para otra desinstalación" } },
            { "des_mantido_usuario", new[] { "  {0} foi trocado por outro arquivo depois da instalação: mantido", "  {0} was replaced by another file after install: kept", "  {0} fue reemplazado por otro archivo tras la instalación: se mantiene" } },
            { "des_ok", new[] { "Desinstalado: a pasta do jogo voltou ao original. Log: {0}", "Uninstalled: the game folder is back to the original. Log: {0}", "Desinstalado: la carpeta del juego volvió al original. Registro: {0}" } },
            { "des_sobra", new[] { "  Aviso: a pasta {0} não pôde ser apagada ({1}); ela não tem mais nada a desfazer e pode ser apagada à mão.", "  Warning: the folder {0} could not be deleted ({1}); it has nothing left to undo and can be deleted by hand.", "  Aviso: no se pudo borrar la carpeta {0} ({1}); ya no tiene nada que deshacer y se puede borrar a mano." } },
            { "des_parcial", new[] { "Desinstalado em parte; o backup ficou em {0}. Log: {1}", "Partly uninstalled; the backup was kept in {0}. Log: {1}", "Desinstalado en parte; la copia quedó en {0}. Registro: {1}" } },
        };
    }
}
