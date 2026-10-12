#pragma once
#include <string>
#include <cstring>
#include <algorithm>
#include <vector>
namespace nrmenu {
struct DisplayTranslation { const char* source; const char* translated; const char* spanish; };

inline constexpr DisplayTranslation display_translations[]{
 {" active   job "," ativo   tarefa "," activo   tarea "},
 {"   async","   Assíncrono","   Asíncrono"},
 {"   zero-copy","   sem cópia","   sin copia"},
 {"   staged","   cópia intermediária","   copia intermedia"},
 {"Unsupported GPU (","GPU não compatível (","GPU no compatible ("},
 {"): this build runs on Radeon RX 9000 (RDNA4) and RX 7000 (RDNA3) only.","): esta versão funciona apenas nas Radeon RX 9000 (RDNA4) e RX 7000 (RDNA3).","): esta versión solo funciona en Radeon RX 9000 (RDNA4) y RX 7000 (RDNA3)."},
 {"Master switch for the neural rendering pass.","Liga ou desliga o processamento neural.","Activa o desactiva el procesamiento neural."},
 {"Writes dlssnr_in/mv/depth/out<job>.raw next to the DLL for offline analysis.","Salva dlssnr_in/mv/depth/out<job>.raw junto à DLL para análise posterior.","Guarda dlssnr_in/mv/depth/out<job>.raw junto a la DLL para análisis posterior."},
 {"Structure through the semantic character mask. auto follows Structure intensity; 0 disables the skin channel.","Estrutura pela máscara de personagens. Automático segue a intensidade da estrutura; 0 desativa o canal da pele.","Estructura mediante la máscara de personajes. Automático sigue la intensidad de la estructura; 0 desactiva el canal de la piel."},
 {"Broad lighting and colour response. This DLL build gates these channels off; raising it mostly darkens the frame.","Resposta geral de luz e cor. Esta versão limita esses canais; aumentar tende a escurecer o quadro.","Respuesta general de luz y color. Esta versión limita esos canales; aumentarla tiende a oscurecer el cuadro."},
 {"Detail the network adds: ambient occlusion, contact shadows, reflections, subsurface scattering (default 1).","Detalhes adicionados pela rede: oclusão ambiente, sombras de contato, reflexos e dispersão sob a superfície. Padrão: 1.","Detalle que añade la red: oclusión ambiental, sombras de contacto, reflejos y dispersión subsuperficial. Predeterminado: 1."},
 {"FSR is not active: enable FSR (any quality mode) in the game's graphics settings to use DLSS-NR.","O FSR não está ativo. Ative-o em qualquer modo de qualidade nas opções gráficas do jogo para usar o NR.","FSR no está activo. Actívalo en cualquier modo de calidad en las opciones gráficas del juego para usar el NR."},
 {"Weights missing: nvngx_dlssnr.dll was not found next to version.dll. Select your copy with the row above.","Pesos ausentes: nvngx_dlssnr.dll não foi encontrada junto de version.dll. Selecione sua cópia na opção acima.","Faltan los pesos: no se encontró nvngx_dlssnr.dll junto a version.dll. Selecciona tu copia en la opción de arriba."},
 {"Choose the file in the dialog (it opened in a separate window).","Escolha o arquivo na janela de seleção que foi aberta separadamente.","Elige el archivo en la ventana de selección que se abrió aparte."},
 {"Your own copy of nvngx_dlssnr.dll (DLSS 5 Neural Rendering 310.8.0.0, shipped with games that support DLSS 5).","Sua cópia de nvngx_dlssnr.dll (renderização neural DLSS 5, versão 310.8.0.0, distribuída com jogos compatíveis).","Tu copia de nvngx_dlssnr.dll (renderizado neural DLSS 5, versión 310.8.0.0, incluida en juegos compatibles)."},
 {"Select nvngx_dlssnr.dll...","Selecionar nvngx_dlssnr.dll...","Seleccionar nvngx_dlssnr.dll..."},
 {" stale frames)"," quadros anteriores)"," cuadros anteriores)"},
 {"async (residual from an earlier frame)","Assíncrono (efeito de um quadro anterior)","Asíncrono (efecto de un cuadro anterior)"},
 {"async (overlay, game runs free)","Assíncrono (sobreposição; jogo independente)","Asíncrono (superpuesto; juego independiente)"},
 {"auto (follows Structure)","Automático (segue a estrutura)","Automático (sigue la estructura)"},
 {"shared (zero-copy)","Compartilhado (sem cópia)","Compartido (sin copia)"},
 {"yes (this game renders with D3D11: unsupported)","Sim (jogo em D3D11; não compatível)","Sí (juego en D3D11; no compatible)"},
 {" (game folder not writable by this process; settings live in the per-user file)"," (sem permissão de gravação na pasta do jogo; ajustes salvos no arquivo do usuário)"," (sin permiso de escritura en la carpeta del juego; ajustes guardados en el archivo del usuario)"},
 {" (the first is the default; the driver's FSR 4 override adds itself there)"," (o primeiro é o padrão; a substituição por FSR 4 do driver aparece nessa opção)"," (la primera es la predeterminada; la sustitución por FSR 4 del controlador aparece ahí)"},
 {" (the game asks for a specific one)"," (o jogo solicita uma opção específica)"," (el juego solicita una opción específica)"},
 {" (DEVICE REMOVED: the GPU faulted or reset earlier in this session)"," (GPU desconectada: ocorreu uma falha ou reinicialização nesta sessão)"," (GPU desconectada: hubo un fallo o reinicio en esta sesión)"},
 {"FAILED (no dlssnr_on_amd_weights.bin)","FALHA (dlssnr_on_amd_weights.bin ausente)","FALLO (falta dlssnr_on_amd_weights.bin)"},
 {"waiting...","Aguardando...","Esperando..."},
 {"press Enter","pressione Enter","pulsa Enter"},
 {"Press Enter","Pressione Enter","Pulsa Enter"},
 {"End to close","End para fechar","End para cerrar"},
 {"Could not build the weights from that file:","Não foi possível extrair os pesos desse arquivo:","No se pudieron extraer los pesos de ese archivo:"},
 {"DLSS-NR on AMD: weight files are missing.","NR na AMD: arquivos de pesos ausentes.","NR en AMD: faltan los archivos de pesos."},
 {"nvngx_dlssnr.dll was not found in the game folder. To work, this mod needs nvngx_dlssnr.dll (DLSS 5 Neural Rendering 310.8.0.0, shipped with games that support DLSS 5).","nvngx_dlssnr.dll não foi encontrada na pasta do jogo. O mod precisa da versão 310.8.0.0 de renderização neural DLSS 5, distribuída com jogos compatíveis.","No se encontró nvngx_dlssnr.dll en la carpeta del juego. El mod necesita la versión 310.8.0.0 del renderizado neural DLSS 5, incluida en juegos compatibles."},
 {"Press End in the game to select your own copy from another folder, or paste it into the game folder and restart.","Pressione End no jogo para selecionar sua cópia em outra pasta, ou copie-a para a pasta do jogo e reinicie.","Pulsa End en el juego para seleccionar tu copia en otra carpeta, o cópiala en la carpeta del juego y reinicia."},
 {"this is a different build, not a repackaged 310.8.0.0; the mod cannot use it","Esta versão difere da 310.8.0.0; o mod não pode utilizá-la.","Esta versión es distinta de la 310.8.0.0; el mod no puede usarla."},
 {"this DLL build is not one the mod knows (expected DLSS NR 310.8.0.0); checking its tensors instead","DLL desconhecida (esperada: DLSS NR 310.8.0.0); verificando os tensores.","DLL desconocida (se esperaba DLSS NR 310.8.0.0); comprobando los tensores."},
 {"cannot read the DLL","Não foi possível ler a DLL","No se pudo leer la DLL"},
 {"cannot write dlssnr_on_amd_weights.bin","Não foi possível gravar dlssnr_on_amd_weights.bin","No se pudo escribir dlssnr_on_amd_weights.bin"},
 {"resource unreadable","Recurso ilegível","Recurso ilegible"},
 {"WEIGHTS_HT resource not found","Recurso WEIGHTS_HT não encontrado","Recurso WEIGHTS_HT no encontrado"},
 {"LoadLibraryEx failed","Falha em LoadLibraryEx","Fallo en LoadLibraryEx"}
};
inline const std::vector<const DisplayTranslation*>& ordered_display_translations() {

    static const auto ordered=[] {
        std::vector<const DisplayTranslation*> result;
        for(const auto& item:display_translations)result.push_back(&item);
        std::stable_sort(result.begin(),result.end(),[](auto a,auto b){return std::strlen(a->source)>std::strlen(b->source);});
        return result;
    }();
    return ordered;
}

inline std::string localized_text(std::string text,bool spanish=false) {
    for(const auto* item:ordered_display_translations()){
        const char* target=spanish?item->spanish:item->translated;
        std::size_t at=0;
        while((at=text.find(item->source,at))!=std::string::npos){text.replace(at,std::strlen(item->source),target);at+=std::strlen(target);}
    }
    return text;
}
}
