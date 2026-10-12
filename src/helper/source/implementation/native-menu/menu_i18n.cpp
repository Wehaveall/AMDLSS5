#include "../../../runtime/nr_publico.h"
#include "menu_i18n.h"
#include "menu_bridge.h"
#include "display_translation.h"
#include <atomic>
#include <cstring>
#include <cwchar>
#include <string_view>
#include <vector>

namespace nrmenu {
namespace {

std::atomic<unsigned> current_language{unsigned(Language::Pt)};
std::atomic<unsigned> current_block{unsigned(InputBlock::MenuKeys)};
std::atomic<bool> current_show_help{show_help_default};

struct Text { const char* key; const char* pt; const char* en; const char* es; };

const Text own_texts[]{
 {"Enabled","Rede neural [F10]","Neural network [F10]","Red neuronal [F10]"},
 {"Mode","Modo","Mode","Modo"},
 {"Structure intensity","Intensidade da estrutura","Structure intensity","Intensidad de la estructura"},
 {"Skin structure","Estrutura da pele","Skin structure","Estructura de la piel"},
 {"Tone intensity","Intensidade do tom","Tone intensity","Intensidad del tono"},
 {"Dump next frame","Capturar próximo quadro","Dump next frame","Capturar el próximo cuadro"},
 {"Select nvngx_dlssnr.dll...","Selecionar nvngx_dlssnr.dll...","Select nvngx_dlssnr.dll...","Seleccionar nvngx_dlssnr.dll..."},
 {language_label,"Idioma / Language","Language / Idioma","Idioma / Language"},
 {input_label,"Entrada do jogo com menu aberto","Game input while open","Entrada del juego con menú abierto"},
 {language_help,language_help,
  "Menu language: English, Português (BR) or Español. Applies at once and is saved.",
  "Idioma del menú: English, Português (BR) o Español. Se aplica al instante y se guarda."},
 {input_help,input_help,
  "What the game stops receiving while the menu is open. Menu keys: arrows, Enter, End and F10 never reach the game; everything else passes (you can still move and test). Everything: keyboard, mouse and XInput controller stay with the menu. End and F10 never reach the game. Saved.",
  "Lo que el juego deja de recibir con el menú abierto. Teclas del menú: flechas, Enter, End y F10 no llegan al juego; el resto pasa (puedes moverte y probar). Todo: teclado, ratón y mando XInput quedan solo para el menú. End y F10 nunca llegan al juego. Se guarda."},
 {"help/Enabled","Liga ou desliga o processamento neural. F10 alterna sem abrir o menu.",
  "Master switch for the neural rendering pass. F10 toggles it without opening the menu.",
  "Activa o desactiva el procesamiento neural. F10 lo alterna sin abrir el menú."},
 {"help/Structure intensity","Detalhes adicionados pela rede: oclusão ambiente, sombras de contato, reflexos e dispersão sob a superfície. Padrão 1.",
  "Detail the network adds: ambient occlusion, contact shadows, reflections, subsurface scattering (default 1).",
  "Detalle que añade la red: oclusión ambiental, sombras de contacto, reflejos y dispersión subsuperficial. Predeterminado: 1."},
 {"help/Skin structure","Estrutura pela máscara semântica de personagens. Automático segue a intensidade da estrutura; 0 desativa o canal da pele.",
  "Structure through the semantic character mask. auto follows Structure intensity; 0 disables the skin channel.",
  "Estructura mediante la máscara semántica de personajes. Automático sigue la intensidad de la estructura; 0 desactiva el canal de la piel."},
 {"help/Tone intensity","Resposta geral de luz e cor. Esta versão limita esses canais; aumentar tende a escurecer o quadro.",
  "Broad lighting and colour response. This DLL build gates these channels off; raising it mostly darkens the frame.",
  "Respuesta general de luz y color. Esta versión limita esos canales; aumentarla tiende a oscurecer el cuadro."},
 {"help/Dump next frame","Salva dlssnr_in/mv/depth/out<job>.raw junto à DLL para análise offline.",
  "Writes dlssnr_in/mv/depth/out<job>.raw next to the DLL for offline analysis.",
  "Guarda dlssnr_in/mv/depth/out<job>.raw junto a la DLL para análisis posterior."},
 {"help/Select nvngx_dlssnr.dll...","Sua cópia de nvngx_dlssnr.dll (renderização neural DLSS 5, versão 310.8.0.0, distribuída com jogos compatíveis).",
  "Your own copy of nvngx_dlssnr.dll (DLSS 5 Neural Rendering 310.8.0.0, shipped with games that support DLSS 5).",
  "Tu copia de nvngx_dlssnr.dll (renderizado neural DLSS 5, versión 310.8.0.0, incluida en juegos compatibles)."},
};

struct HelperText { const char* key; const char* en; const char* es; };
#define T(key,en,es) {key,en,es},
const HelperText helper_texts[]{
#include "menu_text_helper.inc"
};
#undef T

const Text value_texts[]{
 {"Desligado","Desligado","Off","Desactivado"},{"Ligado","Ligado","On","Activado"},
 {"Desligada","Desligada","Off","Desactivada"},
 {"Normal","Normal","Normal","Normal"},
 {"exato","exato","exact","exacto"},{"rápido","rápido","fast","rápido"},{"indisp.","indisp.","n/a","no disp."},{"sem arquivo","sem arquivo","no file","sin archivo"},
 {"Off","Desligado","Off","Desactivado"},{"On","Ligado","On","Activado"},
 {"OFF","Desligado","Off","Desactivado"},{"ON","Ligado","On","Activado"},
 {"off","Desligado","Off","Desactivado"},{"on","Ligado","On","Activado"},
 {"ativo","ativo","active","activo"},{"inativo","inativo","inactive","inactivo"},{"espera","espera","waiting","espera"},

 {"pend.","*","*","*"},{"Cinemático...","Cinemático *","Cinematic *","Cinemático *"},
 {"Padrão","Padrão","Default","Predeterminado"},{"Default","Padrão","Default","Predeterminado"},
 {"Natural","Natural","Natural","Natural"},{"Cinemático","Cinemático","Cinematic","Cinemático"},{"Cinematic","Cinemático","Cinematic","Cinemático"},
 {"Original","Original","Original","Original"},
 {"Personalizado","Personalizado","Custom","Personalizado"},{"Preset","Preset","Preset","Preset"},
 {"Preset A","Preset A","Preset A","Preset A"},{"Preset B","Preset B","Preset B","Preset B"},{"Preset C","Preset C","Preset C","Preset C"},
 {"Indisponível","Indisponível","Unavailable","No disponible"},
 {"Erro ao salvar","Erro ao salvar","Save error","Error al guardar"},
 {"Erro de leitura","Erro de leitura","Read error","Error de lectura"},
 {"Valor inválido","Valor inválido","Invalid value","Valor no válido"},

 {"Só 1 (Curva)","Só 1 (Curva)","Only 1 (Curve)","Solo 1 (Curva)"},
 {"Erro de cache","Erro de cache","Cache error","Error de caché"},
 {"Erro adaptador","Erro adaptador","Adapter error","Error del adaptador"},
 {"Erro no NR","Erro no NR","NR error","Error en NR"},
 {"Ocupado","Ocupado","Busy","Ocupado"},
 {"Aguardando...","Aguardando...","Waiting...","Esperando..."},{"Aguardando","Aguardando","Waiting","Esperando"},
 {"waiting...","Aguardando...","Waiting...","Esperando..."},
 {"Assíncrono","Assíncrono","Async","Asíncrono"},{"Async","Assíncrono","Async","Asíncrono"},{"async","Assíncrono","Async","Asíncrono"},
 {"Só PRE/POST","Só PRE/POST","PRE/POST only","Solo PRE/POST"},
 {"Liga ao reabrir","Liga ao reabrir","On after restart","Activa al reiniciar"},
 {"Desl. reabrir","Desl. reabrir","Off after restart","Desact. al reiniciar"},
 {"Sem suporte","Sem suporte","Unsupported","Sin soporte"},
 {"Usando Present","Usando Present","Using Present","Usando Present"},
 {"Gravando...","Gravando...","Recording...","Grabando..."},{"Gravados","Gravados","Saved","Guardados"},
 {"Salvando...","Salvando...","Saving...","Guardando..."},{"Salvo","Salvo","Saved","Guardado"},
 {"Profundidade","Profundidade","Depth","Profundidad"},{"Luminância","Luminância","Luminance","Luminancia"},
 {"Depois do FSR","Depois do FSR","After FSR","Después de FSR"},{"Antes do FSR","Antes do FSR","Before FSR","Antes de FSR"},
 {"Borda suave","Borda suave","Soft edge","Borde suave"},
 {"Média","Média","Medium","Media"},{"Media","Média","Medium","Media"},{"Médio","Médio","Medium","Medio"},
 {"Fraca","Fraca","Weak","Débil"},{"Fraco","Fraco","Weak","Débil"},{"Forte","Forte","Strong","Fuerte"},
 {"Leve","Leve","Light","Leve"},{"Máximo","Máximo","Maximum","Máximo"},
 {"Sempre","Sempre","Always","Siempre"},{"Invertido","Invertido","Inverted","Invertido"},
 {"Sem limiar","Sem limiar","No threshold","Sin umbral"},
 {"Todo quadro","Todo quadro","Every frame","Cada cuadro"},{"quadros","quadros","frames","cuadros"},
 {"Sem limite","Sem limite","No limit","Sin límite"},
 {"Automático","Automático","Automatic","Automático"},{"Automatic","Automático","Automatic","Automático"},
 {"Auto","Automático","Auto","Automático"},{"auto","Automático","Auto","Automático"},
 {"auto (follows Structure)","Automático","Auto","Automático"},
 {"Before","Antes","Before","Antes"},{"After","Depois","After","Después"},{"Pending","Pendente","Pending","Pendiente"},
 {"Yes","Sim","Yes","Sí"},{"yes","Sim","Yes","Sí"},{"No","Não","No","No"},{"no","Não","No","No"},
 {"None","Nenhum","None","Ninguno"},{"ready","Pronto","Ready","Listo"},{"Ready","Pronto","Ready","Listo"},
 {"press Enter","pressione Enter","press Enter","pulsa Enter"},{"Press Enter","Pressione Enter","Press Enter","Pulsa Enter"},
 {"pressione Enter","pressione Enter","press Enter","pulsa Enter"},
 {"yes (this game renders with D3D11: unsupported)","Sim (jogo em D3D11; não compatível)","yes (this game renders with D3D11: unsupported)","Sí (juego en D3D11; no compatible)"},
 {"FAILED (no dlssnr_on_amd_weights.bin)","FALHA (dlssnr_on_amd_weights.bin ausente)","FAILED (no dlssnr_on_amd_weights.bin)","FALLO (falta dlssnr_on_amd_weights.bin)"},
 {"Select nvngx_dlssnr.dll...","Selecionar nvngx_dlssnr.dll...","Select nvngx_dlssnr.dll...","Seleccionar nvngx_dlssnr.dll..."},
 {input_menu_keys,input_menu_keys,"Menu keys","Teclas del menú"},{input_everything,input_everything,"Everything","Todo"},

 {"legacy","Legado","Legacy","Heredada"},{"reinhard","Reinhard","Reinhard","Reinhard"},{"aces","ACES","ACES","ACES"},
};

struct ValueOverride { const char* label; const char* key; const char* pt; const char* en; const char* es; };
const ValueOverride value_overrides[]{
 {"Preencher o buraco","Média","Média","Average","Promedio"},
};

const char* const neutral_words[]{"p","x","px","FPS","EV","Enter","FSR","NR","PRE","POST","Present","HIP","ms","English",
                                  "Español","Português","BR","FG","D3D","D3D11","DLL","Daniel","A","E","K","4K"};

const char* const group_names[][language_count]{
 {"GENERAL","GERAL","GENERAL"},
 {"NEURAL NETWORK","REDE NEURAL","RED NEURONAL"},
 {"COMPOSITION","COMPOSIÇÃO","COMPOSICIÓN"},
 {"PERFORMANCE","DESEMPENHO","RENDIMIENTO"},
 {"COLOR","COR","COLOR"},
 {"MOTION AND GHOSTING","MOVIMENTO E GHOSTING","MOVIMIENTO Y ESTELA"},
 {"FLICKER AND STABILITY","FLICKER E ESTABILIDADE","PARPADEO Y ESTABILIDAD"},
 {"HALO","HALO","HALO"},
 {"DIAGNOSTICS","DIAGNÓSTICO","DIAGNÓSTICO"},
 {"REPROJECTION (ADVANCED)","REPROJEÇÃO (AVANÇADO)","REPROYECCIÓN (AVANZADO)"},
 {"ON GAME RESTART","AO REABRIR O JOGO","AL REINICIAR EL JUEGO"},
};
static_assert(sizeof(group_names)/sizeof(*group_names)==unsigned(Group::Count));
const char* const ui_texts[][language_count]{

 {"Arrows/Enter: adjust · End: close · F10: network","Setas/Enter: ajustar · End: fechar · F10: rede","Flechas/Enter: ajustar · End: cerrar · F10: red"},
 {"input","entrada","entrada"},
 {"FG active","FG ativo","FG activo"},
 {"FG idle","FG parado","FG inactivo"},
 {"no FG","sem FG","sin FG"},
 {"FPS: sampling...","FPS: medindo...","FPS: midiendo..."},
 {"display n/a","tela indisponível","pantalla no disponible"},
 {"No options available","Sem opções disponíveis","Sin opciones disponibles"},
 {"Neural rendering","Renderização neural","Renderizado neural"},
 {"* pending: applies with the next network result; for (restart) options, after restarting the game.",
  "* pendente: vale no próximo resultado da rede; nas opções (reiniciar), ao reabrir o jogo.",
  "* pendiente: se aplica con el próximo resultado de la red; en las opciones (reiniciar), al reiniciar el juego."},

 {"Network off: ","Rede desligada: ","Red desactivada: "},
};
static_assert(sizeof(ui_texts)/sizeof(*ui_texts)==unsigned(Ui::Count));

const CompatReason compat_reasons[]{

 {"image_verify",true,{"proxy (version.dll) differs from the one this helper pins","proxy (version.dll) diferente do que este helper exige","proxy (version.dll) distinto del que exige este helper"}},
 {"HIP_import_provenance_failed",false,{"another HIP runtime (amdhip64_7.dll) is loaded","outro runtime HIP (amdhip64_7.dll) carregado","otro runtime HIP (amdhip64_7.dll) cargado"}},
 {"invalid_structure_scalar",false,{"INI value out of range","valor fora da faixa no INI","valor fuera de rango en el INI"}},
 {"invalid_auto_mask",false,{"INI value out of range","valor fora da faixa no INI","valor fuera de rango en el INI"}},
 {"runtime_drift_config",false,{"setting changed live (reopen the game)","ajuste mudou ao vivo (reabra o jogo)","ajuste cambiado en vivo (reabre el juego)"}},
 {"runtime_drift_async_job",false,{"native async job out of sequence","job assíncrono nativo fora de sequência","trabajo asíncrono nativo fuera de secuencia"}},
 {"runtime_drift",true,{"device or queue changed","dispositivo ou fila mudou","el dispositivo o la cola cambió"}},
 {"scheduler_drift",true,{"FSR resources changed","recursos do FSR mudaram","los recursos de FSR cambiaron"}},
 {"ffx_geometry_restage_limit",false,{"FSR recreated too often","FSR recriado vezes demais","FSR recreado demasiadas veces"}},
 {"transition_deadline",false,{"state change timed out","troca de estado passou do prazo","el cambio de estado superó el plazo"}},
 {"owned_fence",true,{"GPU fence failed (device removed?)","cerca da GPU falhou (dispositivo removido?)","la barrera de GPU falló (¿dispositivo eliminado?)"}},
 {"recording_observer_install_or_identity_failed",false,{"game command list cannot be observed","lista de comandos do jogo não observável","la lista de comandos del juego no es observable"}},
 {"command_list_switch_with_pending_recording",false,{"FSR command list changed with work pending","lista do FSR trocou com trabalho pendente","la lista de FSR cambió con trabajo pendiente"}},
 {"async_worker_completion_timeout",false,{"GPU did not answer for 5 s","GPU sem resposta por 5 s","GPU sin respuesta durante 5 s"}},
 {"submitted_frame_completion_timeout",false,{"GPU did not answer for 5 s","GPU sem resposta por 5 s","GPU sin respuesta durante 5 s"}},
 {"resource_rotation_capacity_exceeded",false,{"too many rotating FSR resources","recursos do FSR em rodízio demais","demasiados recursos de FSR en rotación"}},
 {"foreign_queue_tracked_recording",false,{"FSR submitted on another queue","FSR enviado em outra fila","FSR enviado en otra cola"}},
 {"queue_overlap",true,{"overlapping submissions","envios sobrepostos","envíos superpuestos"}},
 {"reset_discarded_unclosed_or_unsubmitted_recording",false,{"FSR list discarded before submit","lista do FSR descartada antes do envio","lista de FSR descartada antes del envío"}},
 {"scale_",true,{"working resolution failed","resolução de trabalho falhou","la resolución de trabajo falló"}},

 {"ffx_upscale_type_0x10001",false,{"no FSR upscale descriptor","sem descritor de upscale do FSR","sin descriptor de escalado de FSR"}},
 {"ffx_reset_flag",false,{"camera cut (one frame)","corte de câmera (um quadro)","corte de cámara (un cuadro)"}},
 {"ffx_pre_exposure_1",false,{"game pre-exposure is not 1.0","pré-exposição do jogo diferente de 1,0","preexposición del juego distinta de 1,0"}},
 {"ffx_required_pointers",false,{"FSR input missing","entrada do FSR ausente","falta una entrada de FSR"}},
 {"ffx_geometry_16384",false,{"invalid FSR size","tamanho do FSR inválido","tamaño de FSR no válido"}},
 {"ffx_motion_scale_positive",false,{"motion vector scale not supported","escala do vetor de movimento sem suporte","escala del vector de movimiento no admitida"}},

 {"describe_color_format",false,{"color format not supported","formato de cor sem suporte","formato de color no admitido"}},
 {"describe_color_uav",false,{"color without UAV access","cor sem acesso UAV","color sin acceso UAV"}},
 {"describe_size_equals_render",false,{"resource size differs from the render size","tamanho do recurso diferente da resolução interna","tamaño del recurso distinto de la resolución interna"}},
 {"describe_motion_format",false,{"motion vector format not supported","formato do vetor de movimento sem suporte","formato del vector de movimiento no admitido"}},
 {"describe_depth_format",false,{"depth format not supported","formato da profundidade sem suporte","formato de profundidad no admitido"}},
 {"describe_exposure",false,{"exposure texture not supported","textura de exposição sem suporte","textura de exposición no admitida"}},
 {"describe_motion_scale_equals_render",false,{"motion vector scale is not the render size","escala do vetor de movimento diferente da resolução interna","escala del vector de movimiento distinta de la resolución interna"}},
 {"describe_device_and_direct_list",false,{"resource on another device or list not DIRECT","recurso em outro dispositivo ou lista não DIRECT","recurso en otro dispositivo o lista no DIRECT"}},
 {"describe_resource_layout",false,{"resource layout not supported","layout de recurso sem suporte","diseño de recurso no admitido"}},
 {"environment_read",false,{"depth disabled in the INI (UseDepth)","profundidade desligada no INI (UseDepth)","profundidad desactivada en el INI (UseDepth)"}},
 {"environment_ok",false,{"proxy not ready","proxy não pronto","proxy no listo"}},
 {"scheduler_fsr_only",false,{"FSR frame not admitted","quadro do FSR não admitido","cuadro de FSR no admitido"}},

 {"capture_formato_de_entrada",false,{"Async input format not supported","formato de entrada do Assíncrono sem suporte","formato de entrada del Asíncrono no admitido"}},
 {"capture_alocacao",false,{"history memory budget exceeded","orçamento de memória do histórico excedido","presupuesto de memoria del historial excedido"}},
 {"capture_exposicao",false,{"Async exposure texture not supported","textura de exposição do Assíncrono sem suporte","textura de exposición del Asíncrono no admitida"}},
 {"capture_estado_de_recurso",false,{"unknown resource state","estado de recurso desconhecido","estado de recurso desconocido"}},
 {"capture_sem_device",false,{"Async capture could not get the device","captura do Assíncrono sem dispositivo","captura del Asíncrono sin dispositivo"}},
 {"capture_device_diferente",false,{"resource on another device","recurso em outro dispositivo","recurso en otro dispositivo"}},
 {"compose_unsupported",false,{"composition target not supported","destino da composição sem suporte","destino de composición no admitido"}},

 {"compose_post_reduzido",false,{"POST composition at output size not available","composição do POST na saída indisponível","composición del POST en la salida no disponible"}},
 {"post_historico_reduzido",false,{"POST history reduced to fit the memory budget","histórico do POST reduzido para caber no orçamento de memória","historial del POST reducido para caber en el presupuesto de memoria"}},

 {"overlay_amd",false,{"AMD metrics overlay is on (Ctrl+Shift+O turns it off; F10 forces the network)","overlay de métricas da AMD ligado (Ctrl+Shift+O desliga; F10 força a rede)","overlay de métricas de AMD activo (Ctrl+Shift+O lo desactiva; F10 fuerza la red)"}},
 {"overlay_amd_recriacao",false,{"paused 2 s: the game recreated its render targets with AMD overlay/recording/camera on","pausa de 2 s: o jogo recriou a tela com overlay/gravação/câmera da AMD ligados","pausa de 2 s: el juego recreó la pantalla con overlay/grabación/cámara de AMD activos"}},
};

using G=Group;

const CatalogEntry catalog[]{

 {"Enabled",G::General,true,true},
 {"Select nvngx_dlssnr.dll...",G::General,true,true},
 {language_label,G::General,true,false},
 {input_label,G::General,true,false},
 {"Tirar print",G::General,false,false},
 {"Mode",G::General,false,true},

 {"Preset",G::Neural,false,false},
 {"Structure intensity",G::Neural,true,true},
 {"Skin structure",G::Neural,false,true},
 {"Tone intensity",G::Neural,true,true},
 {"NR antes do FSR",G::Neural,false,false},
 {"Modo da rede",G::Neural,false,false},
 {"Resolução de trabalho",G::Neural,true,false},
 {"Máscara automática",G::Neural,false,false},
 {"Estilo neural",G::Neural,false,false},

 {"Exposição (EV)",G::Neural,true,false},
 {"Contraste",G::Neural,true,false},
 {"Saturação",G::Neural,true,false},
 {"Passos neurais",G::Neural,true,false},
 {"Rede a cada",G::Neural,false,false},
 {"Intensidade neural",G::Neural,true,false},
 {"Força do realce",G::Neural,false,false},

 {"Realce da luz",G::Neural,false,false},
 {"Realce do detalhe",G::Neural,false,false},
 {"Rede v0.5 (tokens)",G::Neural,false,false},
 {"Rosto do personagem",G::Neural,false,false},
 {"Força dos passos extras",G::Neural,true,false},
 {"Tom local",G::Neural,false,false},
 {"Present (reiniciar)",G::Neural,false,false},

 {"Intensidade dos detalhes",G::Composition,false,false},
 {"Intensidade das cores",G::Composition,false,false},
 {"Proteção de realces",G::Composition,false,false},

 {"Rede neural automática",G::Performance,false,false},
 {"Limite de FPS",G::Performance,true,false},
 {"Kernel swin32",G::Performance,false,false},
 {"Rede neural % (reiniciar)",G::Performance,false,false},

 {"Transferência bilateral",G::Motion,false,false},
 {"Divergência bruta",G::Motion,false,false},
 {"Rejeição por movimento",G::Motion,false,false},
 {"Força em movimento",G::Motion,false,false},
 {"Movimento da força mínima",G::Motion,false,false},
 {"Compensar jitter",G::Motion,false,false},
 {"Compor o realce",G::Motion,false,false},
 {"Composição tardia",G::Motion,false,false},
 {"Realce pela profundidade",G::Motion,false,false},
 {"Borda acompanha o movimento",G::Motion,false,false},
 {"Reparar movimento",G::Motion,false,false},
 {"Preencher o buraco",G::Motion,false,false},
 {"Detalhe em movimento",G::Motion,false,false},
 {"Idade máxima",G::Motion,false,false},
 {"Borda em movimento",G::Motion,false,false},

 {"Estabilizar realce",G::Flicker,false,false},
 {"Estabilizar só parado",G::Flicker,false,false},
 {"Cancelar ruído",G::Flicker,false,false},
 {"Transferir por razão",G::Flicker,false,false},
 {"Limitar variação do ganho",G::Flicker,false,false},
 {"Proteção suave",G::Flicker,false,false},
 {"Suavizar iluminação",G::Flicker,false,false},
 {"Piso da razão",G::Flicker,false,false},

 {"Reduzir halo",G::Halo,false,false},
 {"Reduzir halo pela rede",G::Halo,false,false},
 {"Iluminação pela profundidade",G::Halo,false,false},
 {"Halo em movimento",G::Halo,false,false},
 {"Cortar ao redor em movimento",G::Halo,false,false},

 {"Gravar 24 quadros",G::Diagnostics,false,false},
 {"Dump next frame",G::Diagnostics,false,true},
 {"Despejos (reiniciar)",G::Diagnostics,false,false},
 {"Despejo do preenchimento (reiniciar)",G::Diagnostics,false,false},
 {"Despejo da profundidade (reiniciar)",G::Diagnostics,false,false},
 {"Diagnóstico de estilo (reiniciar)",G::Diagnostics,false,false},
 {"Trace de kernels (reiniciar)",G::Diagnostics,false,false},
 {"Jobs do trace (reiniciar)",G::Diagnostics,false,false},
 {"Trace de contrato (reiniciar)",G::Diagnostics,false,false},
 {"Trace: cópia (reiniciar)",G::Diagnostics,false,false},
 {"Trace: imediato (reiniciar)",G::Diagnostics,false,false},
 {"Trace: bytes (reiniciar)",G::Diagnostics,false,false},
 {"Trace: tensores (reiniciar)",G::Diagnostics,false,false},
 {"Trace: único (reiniciar)",G::Diagnostics,false,false},

 {"Limite de vizinhança",G::Reprojection,false,false},
 {"Tolerância de profundidade",G::Reprojection,false,false},
 {"Tolerância de cor",G::Reprojection,false,false},
 {"Suavizar rejeição",G::Reprojection,false,false},
 {"Escala do movimento",G::Reprojection,false,false},
 {"Tolerância a deformação",G::Reprojection,false,false},
 {"Teste de desoclusão",G::Reprojection,false,false},
 {"Transferência do realce",G::Reprojection,false,false},
 {"Tolerância do realce",G::Reprojection,false,false},
 {"Separação da grade",G::Reprojection,false,false},
 {"Retenção do modelo",G::Reprojection,false,false},
 {"Regularização",G::Reprojection,false,false},
 {"Movimento pontual",G::Reprojection,false,false},
 {"Movimento duplo",G::Reprojection,false,false},
 {"Limiar do mov. duplo",G::Reprojection,false,false},
 {"Piso do realce",G::Reprojection,false,false},
 {"Piso da confiança",G::Reprojection,false,false},
 {"Suavizar portão",G::Reprojection,false,false},
 {"Suavizar deformação",G::Reprojection,false,false},
 {"Referência de movimento",G::Reprojection,false,false},
 {"Tolerância absoluta",G::Reprojection,false,false},

 {"Histórico da rede (reiniciar)",G::Restart,false,false},
 {"Tonemap (reiniciar)",G::Restart,false,false},
 {"Canais de tom (reiniciar)",G::Restart,false,false},
 {"Entradas do FSR (reiniciar)",G::Restart,false,false},
 {"Usar profundidade (reiniciar)",G::Restart,false,false},
 {"Interop D3D12/HIP (reiniciar)",G::Restart,false,false},
 {"Dispositivo HIP (reiniciar)",G::Restart,false,false},
 {"Upscale guiado (reiniciar)",G::Restart,false,false},
 {"Curva da entrada neural (reiniciar)",G::Restart,false,false},
 {"Elevar sombras da entrada (reiniciar)",G::Restart,false,false},
 {"Exposicao do jogo (reiniciar)",G::Restart,false,false},
};

bool same(const char* a,const char* b) noexcept { return a&&b&&std::strcmp(a,b)==0; }
const char* pick(const Text& t,Language l) noexcept { return l==Language::En?t.en:l==Language::Es?t.es:t.pt; }
const Text* own(const char* key) noexcept { for(const auto& t:own_texts)if(same(t.key,key))return &t; return nullptr; }
const HelperText* helper(const char* key) noexcept { for(const auto& t:helper_texts)if(same(t.key,key))return &t; return nullptr; }
bool word_byte(unsigned char c) noexcept { return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||c>=0x80; }
struct Key { std::string_view key; const char* pt; const char* en; const char* es; };
const std::vector<Key>& value_keys() {
    static const std::vector<Key> keys=[] {
        std::vector<Key> k;
        for(const auto& t:value_texts)k.push_back({t.key,t.pt,t.en,t.es});
        std::stable_sort(k.begin(),k.end(),[](const Key& a,const Key& b){return a.key.size()>b.key.size();});
        return k;
    }();
    return keys;
}
bool neutral(std::string_view word) noexcept { for(const char* n:neutral_words)if(word==n)return true; return false; }
}

Language language() noexcept { return Language(current_language.load(std::memory_order_relaxed)%language_count); }
void set_language(Language value) noexcept { current_language.store(unsigned(value)%language_count,std::memory_order_relaxed); }
const char* language_code(Language value) noexcept { return value==Language::En?"en":value==Language::Es?"es":"pt"; }
const char* language_name(Language value) noexcept { return value==Language::En?"English":value==Language::Es?"Español":"Português (BR)"; }
bool parse_language(const wchar_t* code,Language& out) noexcept {
    if(!code)return false;
    if(!_wcsicmp(code,L"en")||!_wcsicmp(code,L"en-US")||!_wcsicmp(code,L"english")){out=Language::En;return true;}
    if(!_wcsicmp(code,L"pt")||!_wcsicmp(code,L"pt-BR")||!_wcsicmp(code,L"ptbr")){out=Language::Pt;return true;}
    if(!_wcsicmp(code,L"es")||!_wcsicmp(code,L"es-ES")||!_wcsicmp(code,L"spanish")){out=Language::Es;return true;}
    return false;
}
bool parse_language_name(const char* name,Language& out) noexcept {
    for(unsigned i=0;i<language_count;++i)if(same(name,language_name(Language(i)))){out=Language(i);return true;}
    return false;
}
bool show_help() noexcept { return current_show_help.load(std::memory_order_relaxed); }
void set_show_help(bool value) noexcept { current_show_help.store(value,std::memory_order_relaxed); }
InputBlock input_block() noexcept { return current_block.load(std::memory_order_relaxed)?InputBlock::Everything:InputBlock::MenuKeys; }
void set_input_block(InputBlock value) noexcept { current_block.store(unsigned(value),std::memory_order_relaxed); }

const CatalogEntry* catalog_begin() noexcept { return catalog; }
const CatalogEntry* catalog_end() noexcept { return catalog+sizeof(catalog)/sizeof(*catalog); }
const CatalogEntry* catalog_find(const char* label) noexcept { for(const auto& e:catalog)if(same(e.label,label))return &e; return nullptr; }
int catalog_index(const char* label) noexcept { const auto* e=catalog_find(label); return e?int(e-catalog):-1; }
bool catalog_shown(const char* label) noexcept { const auto* e=catalog_find(label); return !e||e->shown; }
const char* group_name(Group group,Language value) noexcept {
    return unsigned(group)<unsigned(Group::Count)?group_names[unsigned(group)][unsigned(value)%language_count]:"";
}
const char* ui(Ui key,Language value) noexcept {
    return unsigned(key)<unsigned(Ui::Count)?ui_texts[unsigned(key)][unsigned(value)%language_count]:"";
}

std::string label(const char* source,Language value) {
    if(!source)return {};
    if(const auto* t=own(source))return pick(*t,value);
    if(const auto* t=helper(source))return value==Language::En?t->en:value==Language::Es?t->es:source;
    return value==Language::Pt?localized_text(localized_label(source)):source;
}
std::string help(const char* label_source,const char* help_source,Language value) {

    if(same(label_source,"Mode"))return {};
    if(help_source){
        if(const auto* t=own(help_source))return pick(*t,value);
        if(const auto* t=helper(help_source))return value==Language::En?t->en:value==Language::Es?t->es:help_source;
    }
    if(label_source){
        const std::string key=std::string("help/")+label_source;
        if(const auto* t=own(key.c_str()))return pick(*t,value);
    }
    if(value==Language::Pt)return localized_text(localized_help(label_source,help_source));
    if(value==Language::Es)return localized_text(help_source?help_source:"",true);
    return help_source?help_source:"";
}
std::string value(const char* label_source,const std::string& source,Language lang,bool* complete) {
    if(complete)*complete=true;
    for(const auto& o:value_overrides)if(same(o.label,label_source)&&source==o.key)
        return lang==Language::En?o.en:lang==Language::Es?o.es:o.pt;
    std::string out;out.reserve(source.size()+8);
    const auto& keys=value_keys();
    for(std::size_t i=0;i<source.size();){
        const bool boundary=i==0||!word_byte(static_cast<unsigned char>(source[i-1]));
        bool matched=false;
        if(boundary)for(const auto& k:keys){
            if(source.compare(i,k.key.size(),k.key)!=0)continue;
            const std::size_t end=i+k.key.size();
            if(end<source.size()&&word_byte(static_cast<unsigned char>(source[end]))&&word_byte(static_cast<unsigned char>(k.key.back())))continue;
            out+=lang==Language::En?k.en:lang==Language::Es?k.es:k.pt;i=end;matched=true;break;
        }
        if(matched)continue;
        if(word_byte(static_cast<unsigned char>(source[i]))){
            std::size_t end=i;while(end<source.size()&&word_byte(static_cast<unsigned char>(source[end])))++end;
            if(complete&&!neutral(std::string_view(source).substr(i,end-i)))*complete=false;
            out.append(source,i,end-i);i=end;continue;
        }
        out+=source[i++];
    }
    return out;
}
std::string status(const std::string& source,Language value) {

    const auto at=source.find(" active   job ");
    if(at!=std::string::npos){
        std::size_t i=at+std::strlen(" active   job ");
        if(i<source.size()&&source[i]=='-')++i;
        const auto digits=i;while(i<source.size()&&source[i]>='0'&&source[i]<='9')++i;
        if(i>digits&&source.compare(i,2,": ")==0){
            const auto ms_begin=i+2;auto ms_end=ms_begin;
            while(ms_end<source.size()&&((source[ms_end]>='0'&&source[ms_end]<='9')||source[ms_end]=='.'))++ms_end;
            if(ms_end>ms_begin&&source.compare(ms_end,3," ms")==0){
                static const char* const active[]{"active","ativo","activo"};
                static const char* const network_time[]{"Neural network time","Tempo da rede neural","Tiempo de la red neuronal"};
                const unsigned l=unsigned(value)%language_count;
                return source.substr(0,at)+" "+active[l]+" · "+network_time[l]+": "+source.substr(ms_begin,ms_end-ms_begin)+" ms";
            }
        }
    }
    if(value==Language::En)return source;
    if(value==Language::Es)return localized_text(source,true);
    return localized_text(localized_value(source.c_str()));
}
const CompatReason* compat_reason_begin() noexcept { return compat_reasons; }
const CompatReason* compat_reason_end() noexcept { return compat_reasons+sizeof(compat_reasons)/sizeof(*compat_reasons); }
const char* compat_reason(const char* id,Language value) noexcept {
    if(!id||!*id)return nullptr;
    const unsigned l=unsigned(value)%language_count;
    for(const auto& r:compat_reasons)if(!r.prefix&&std::strcmp(r.id,id)==0)return r.text[l];
    for(const auto& r:compat_reasons)if(r.prefix&&std::strncmp(r.id,id,std::strlen(r.id))==0)return r.text[l];
    return nullptr;
}
std::string network_off(const char* id,const char* field,Language value) {
    std::string out=ui(Ui::NetworkOff,value);
    const char* text=compat_reason(id,value);
    out+=text?text:(id?id:"");
    if(field&&*field){out+=" (";out+=field;out+=")";}
    return out;
}
bool label_known(const char* source) noexcept { return own(source)||helper(source); }
bool help_known(const char* label_source,const char* help_source) noexcept {
    if(help_source&&(own(help_source)||helper(help_source)))return true;
    if(!label_source)return false;
    const std::string key=std::string("help/")+label_source;
    return own(key.c_str())!=nullptr;
}
}
