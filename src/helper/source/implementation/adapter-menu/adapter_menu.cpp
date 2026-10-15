#include "../../../runtime/nr_publico.h"
#include "adapter_menu.h"
#include "settings.h"
#include "menu_i18n.h"
#include "../../../runtime/pre_scale_state.h"
#include "../../../runtime/neural_tone_state.h"
#include "../../../runtime/sw32x_state.h"
#include "../../../runtime/v05_state.h"
#include "../../../runtime/rede_cada.h"
#include "../../../runtime/rosto_jogador_state.h"
#include "preset.h"
#include "../compose-controls/realce.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <memory>
#include "../present-runtime/present_state.h"
#include "../present-runtime/print_state.h"
#include "../frame-limiter/frame_limiter.h"
namespace nradapter {
namespace {
std::atomic<DWORD> owner{0};
std::atomic_flag busy=ATOMIC_FLAG_INIT;
struct Admission {
    bool admitted=false;
    Admission() noexcept : admitted(!busy.test_and_set(std::memory_order_acquire)) {}
    ~Admission() { if (admitted) busy.clear(std::memory_order_release); }
};
enum class Error { None, Input, Save, Read, Cache, Exception, Runtime };
struct Context {
    void* base;
    NativeVector* rows;
    std::filesystem::path path;
    nr::SettingsStore store;
    Error error=Error::None;
    unsigned* auto_mask=nullptr;
    bool mask_save_error=false;
    unsigned* style_requested=nullptr;
    unsigned* style_effective=nullptr;
    Error style_error=Error::None;
    bool style_initialised=false;
    unsigned* multipass_requested=nullptr;
    unsigned* multipass_effective=nullptr;
    Error multipass_error=Error::None;
    bool multipass_initialised=false;

    bool multipass_tonecurve=false;
    nrtone::State* tone_state=nullptr;
    Error tone_error=Error::None;
    bool tone_initialised=false;
    bool present_mode=false;
    bool present_save_error=false;
    nrcompose::State* compose_controls=nullptr;
    bool compose_initialised=false;
    Error compose_error=Error::None;
    nrreproject::Controls* reproject_controls=nullptr;
    bool reproject_initialised=false;
    Error reproject_error=Error::None;

    bool preset_personalizado=false;Error preset_error=Error::None;
    Context* next=nullptr;
    Context(const NativeHost& host):base(host.base),rows(host.rows),path(host.selected_ini),store(path),auto_mask(host.auto_mask),style_requested(host.style_requested),style_effective(host.style_effective),multipass_requested(host.multipass_requested),multipass_effective(host.multipass_effective),tone_state(host.tone_state),present_mode(host.present_mode),compose_controls(host.compose_controls),reproject_controls(host.reproject_controls) {}
};

Context* contexts=nullptr;
bool cache(Context& context) noexcept {
    SetLastError(ERROR_SUCCESS);

    const BOOL result=NRCFG_ESCREVER(nullptr,nullptr,nullptr,nullptr);
    const DWORD error=GetLastError();

    if (!result && error!=ERROR_SUCCESS) {
        context.error=Error::Cache;
        char detail[96]{}; NRLOGF(detail,sizeof(detail),"NR menu: INI cache flush error %lu\n",error);
        NRDIAG(OutputDebugStringA(detail);) return false;
    }
    return true;
}
void record_operation(Context& context,const nr::Operation& operation,Error failure) {
    if (!operation.ok) context.error=failure;
    else if (context.store.snapshot().diagnostic.empty()) context.error=Error::None;
    else if (context.error==Error::None || context.error==Error::Input) context.error=failure;
    NRDIAG(if (!operation.ok) OutputDebugStringA(operation.diagnostic.c_str());)
}
bool reload_context(Context& context) {
    if (!cache(context)) return false;
    const auto operation=context.store.reload();
    record_operation(context,operation,Error::Read);

    if(operation.ok){
        wchar_t texto[32]{};NRCFG_STR(L"DlssNrOnAmd",L"NeuralPostPercent",L"",texto,32,context.path.c_str());
        const unsigned chave=nrprescale::post_chave_de(texto);
        nrprescale::post_chave.store(chave?chave:100u);nrprescale::post_menu_escreveu.store(true);

        wchar_t v5[8]{};NRCFG_STR(L"DlssNrOnAmd",L"NeuralV05Tokens",L"0",v5,8,context.path.c_str());
        nrv05::ligado.store(nrv05::valor_de(v5));nrv05::menu_escreveu.store(true);
        if(!chave){auto pedido=context.store.snapshot().requested;
            if(!pedido.pre_upscale){pedido.pre_upscale=true;record_operation(context,context.store.request(pedido),Error::Save);}}
    }
    const bool flushed=cache(context);
    return operation.ok && flushed;
}
void output(char* out,std::size_t capacity,const char* text) noexcept {
    if (!out || !capacity) return;
    const auto length=std::strlen(text);
    const auto count=length<capacity-1 ? length : capacity-1;
    std::memcpy(out,text,count); out[count]='\0';
}
bool valid_mask(const unsigned* pointer) noexcept {
    return reinterpret_cast<std::uintptr_t>(pointer)%alignof(unsigned)==0 &&
        accessible(pointer,sizeof(unsigned),Access::Read) && accessible(pointer,sizeof(unsigned),Access::Write);
}
bool valid_style(const Context& context) noexcept {
    return valid_mask(context.style_requested) && valid_mask(context.style_effective) && context.style_requested!=context.style_effective;
}
LONG scalar(unsigned* pointer) noexcept { return InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(pointer),0,0); }
bool valid_multipass(const Context& context) noexcept {
    return valid_mask(context.multipass_requested) && valid_mask(context.multipass_effective) && context.multipass_requested!=context.multipass_effective;
}
bool valid_pass_config(unsigned value) noexcept { return !(value & 0xffff0000u) && (value & 255u)>=1 && (value & 255u)<=3 && (value>>8)<=6; }
void initialise_multipass(Context& context) noexcept {
    if(context.multipass_initialised || !context.multipass_requested)return;
    context.multipass_initialised=true;
    if(!valid_multipass(context))return;
    const DWORD attributes=GetFileAttributesW(context.path.c_str());
    if(attributes!=INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {context.multipass_error=Error::Read;return;}
    wchar_t selected[32]{};
    const DWORD count=NRCFG_STR(L"DlssNrOnAmd",L"NeuralPassConfig",L"769",selected,32,context.path.c_str());
    unsigned value=0;
    if(!count || count>4) {context.multipass_error=Error::Input;return;}
    for(DWORD i=0;i<count;++i) {
        if(selected[i]<L'0' || selected[i]>L'9') {context.multipass_error=Error::Input;return;}
        value=value*10u+static_cast<unsigned>(selected[i]-L'0');
    }
    if(!valid_pass_config(value)) {context.multipass_error=Error::Input;return;}

    if((value&255u)>2u){value=(value&0xff00u)|2u;wchar_t v2[16]{};std::swprintf(v2,16,L"%u",value);
        if(!NRCFG_ESCREVER(L"DlssNrOnAmd",L"NeuralPassConfig",v2,context.path.c_str())){context.multipass_error=Error::Save;return;}}
    if(valid_multipass(context))InterlockedExchange(reinterpret_cast<volatile LONG*>(context.multipass_requested),value);
}
void initialise_style(Context& context) noexcept {
    if(context.style_initialised || !context.style_requested)return;
    context.style_initialised=true;
    if(!valid_style(context))return;
    const DWORD attributes=GetFileAttributesW(context.path.c_str());
    if(attributes!=INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {context.style_error=Error::Read;return;}
    wchar_t selected[16]{};
    const DWORD count=NRCFG_STR(L"DlssNrOnAmd",L"NeuralStyle",L"0",selected,16,context.path.c_str());
    if(count!=1 || selected[0]<L'0' || selected[0]>L'2') {context.style_error=Error::Input;return;}
    if(valid_style(context))InterlockedExchange(reinterpret_cast<volatile LONG*>(context.style_requested),selected[0]-L'0');
    nrreproject::grade_estilo.store(unsigned(selected[0]-L'0'));nrreproject::grade_semeado.store(true);
}
void initialise_tone(Context& context) noexcept {
    if(!context.tone_state || context.tone_initialised)return;
    context.tone_initialised=true;
    const DWORD attributes=GetFileAttributesW(context.path.c_str());
    if(attributes!=INVALID_FILE_ATTRIBUTES && (attributes&FILE_ATTRIBUTE_DIRECTORY)){context.tone_error=Error::Read;return;}
    wchar_t text[16]{};
    const DWORD count=NRCFG_STR(L"DlssNrOnAmd",L"NeuralToneConfig",L"0",text,16,context.path.c_str());
    if(count!=1 || text[0]<L'0' || text[0]>L'9'){context.tone_error=Error::Input;return;}
    context.tone_state->requested.store(unsigned(text[0]-L'0'),std::memory_order_release);
    {wchar_t lt[32]{};NRCFG_STR(L"DlssNrOnAmd",L"LocalTone",L"0",lt,32,context.path.c_str());
     nrreproject::grade_tom.store(nrreproject::tom_de_config(unsigned(text[0]-L'0'),float(std::wcstod(lt,nullptr))));}
}
const wchar_t* compose_keys[]={L"NeuralDetailStrength",L"NeuralColorStrength",L"NeuralHighlightGuard"};
unsigned component(unsigned packed,unsigned control){return control==8?nrcompose::detail(packed):control==9?nrcompose::color(packed):nrcompose::guard(packed);}
void initialise_compose(Context& context){
    if(!context.compose_controls||context.compose_initialised)return;
    context.compose_initialised=true;
    nrcompose::State loaded;
    for(unsigned i=0;i<3;++i){
        wchar_t text[64]{};const auto count=NRCFG_STR(L"DlssNrOnAmd",compose_keys[i],i==2?L"2.00":L"1.00",text,64,context.path.c_str());
        unsigned steps=0;
        if(count>=63||!nrcompose::parse(text,i==2?20:0,i==0?40:i==1?80:160,steps)||!nrcompose::set(loaded,i,steps)){
            context.compose_error=Error::Input;return;
        }
    }
    context.compose_controls->requested.store(loaded.requested.load());
}

constexpr unsigned reproject_first_row=13;
constexpr unsigned reproject_last_row=reproject_first_row+nrreproject::ControlCount-1;

struct IniRow{const wchar_t* key;const char* label;const char* help;const wchar_t* values[14];bool restart;};
constexpr IniRow ini_rows[]={
 {L"Scale","Força do realce","Força do resíduo da rede. Vale na hora.",{L"0.005",L"0.010",L"0.015",L"0.015625",L"0.020",L"0.025",L"0.030",L"0.03125",L"0.035",L"0.040",L"0.046875",L"0.050",L"0.0625",L"0.125"},false},
 {L"Temporal","Histórico da rede (reiniciar)","Histórico temporal próprio da rede. Vale ao reabrir o jogo.",{L"0",L"1"},true},
 {L"Tonemap","Tonemap (reiniciar)","Conversão de tom da entrada da rede: -1 automático. Vale ao reabrir o jogo.",{L"-1",L"0",L"1"},true},
 {L"ToneChannels","Canais de tom (reiniciar)","Vale ao reabrir o jogo.",{L"0",L"1",L"2",L"3"},true},
 {L"UseFsrInputs","Entradas do FSR (reiniciar)","CUIDADO: 0 pode desligar o NR. Vale ao reabrir o jogo.",{L"0",L"1"},true},
 {L"UseDepth","Usar profundidade (reiniciar)","CUIDADO: sem profundidade a reprojeção piora. Vale ao reabrir o jogo.",{L"0",L"1"},true},
 {L"Interop","Interop D3D12/HIP (reiniciar)","CUIDADO: 0 pode desligar o NR ou travar. Vale ao reabrir o jogo.",{L"0",L"1"},true},
 {L"HipDevice","Dispositivo HIP (reiniciar)","CUIDADO: -1 escolhe sozinho; outro valor pode desligar o NR. Vale ao reabrir o jogo.",{L"-1",L"0",L"1"},true},
 {L"NeuralGuidedUpscale","Upscale guiado (reiniciar)","Vale ao reabrir o jogo.",{L"0",L"1"},true},
 {L"NeuralPreScalePercent","Rede neural % (reiniciar)","Resolução em que a rede neural roda, em % da resolução interna: menos % = job mais rápido e menos detalhe. Só vale ao fechar e reabrir o jogo.",{L"100",L"90",L"85",L"80",L"75",L"70",L"60",L"50"},true},
 {L"NeuralDumpFrames","Despejos (reiniciar)","Grava amostras de diagnóstico em disco; cada uma pode dar um solavanco. 0 desliga. Vale ao reabrir o jogo.",{L"0",L"20",L"60"},true},
 {L"NeuralDumpFillStages","Despejo do preenchimento (reiniciar)","Diagnóstico. Vale ao reabrir o jogo.",{L"0",L"1"},true},
 {L"NeuralDumpDepth","Despejo da profundidade (reiniciar)","Diagnóstico. Vale ao reabrir o jogo.",{L"0",L"1"},true},
 {L"NeuralStyleDiagnostic","Diagnóstico de estilo (reiniciar)","Diagnóstico; custa FPS. Vale ao reabrir o jogo.",{L"0",L"1"},true},
 {L"NeuralKernelTrace","Trace de kernels (reiniciar)","Diagnóstico; custa FPS. Vale ao reabrir o jogo.",{L"0",L"1"},true},
 {L"NeuralKernelTraceJobs","Jobs do trace (reiniciar)","Diagnóstico. Vale ao reabrir o jogo.",{L"1",L"3",L"10"},true},
 {L"NeuralContractTrace","Trace de contrato (reiniciar)","Diagnóstico; custa FPS. Vale ao reabrir o jogo.",{L"0",L"1"},true},
 {L"NeuralContractTraceCopy","Trace: cópia (reiniciar)","Diagnóstico. Vale ao reabrir o jogo.",{L"0",L"1"},true},
 {L"NeuralContractTraceImmediate","Trace: imediato (reiniciar)","Diagnóstico. Vale ao reabrir o jogo.",{L"0",L"1"},true},
 {L"NeuralContractTraceBytes","Trace: bytes (reiniciar)","Diagnóstico. Vale ao reabrir o jogo.",{L"65536",L"262144",L"1048576"},true},
 {L"NeuralContractTraceFullTensors","Trace: tensores (reiniciar)","Diagnóstico. Vale ao reabrir o jogo.",{L"0",L"1"},true},
 {L"NeuralContractTraceSingle","Trace: único (reiniciar)","Diagnóstico. Vale ao reabrir o jogo.",{L"0",L"1"},true},

 {L"NeuralPreScaleAuto","Rede neural automática","Mantém o tempo da rede igual em qualquer preset do FSR: a rede roda sempre no mesmo tamanho e a % muda sozinha com a resolução interna (entre parênteses, a % agora). 1440p = 100% no Qualidade a 4K; 1270p = 100% no Equilibrado; 1080p = 100% no Desempenho; 720p = 100% no Ultra Desempenho (rede maior custa mais tempo e mais idade no Assíncrono). Vale na hora. Ligada, a Rede neural % (reiniciar) não vale; Desligada, volta a valer.",{L"1440",L"1270",L"1080",L"900",L"720",L"648",L"576",L"504",L"432",L"360",L"0"},false},

 {L"NeuralFrameCap","Limite de FPS","Limita os quadros reais (renderizados) por segundo; com o frame generation a tela mostra o dobro. A placa fica com folga e a rede neural do Assíncrono termina antes: o realce chega mais novo (no Hangar, a 100 FPS, 25% menos atraso). Vale na hora e também com o NR desligado. Deixe o limite do jogo em Variável. Desligado = sem limite.",{L"0",L"60",L"90",L"100",L"110",L"120",L"130",L"144",L"165"},false},
 {L"ToneCurve","Curva da entrada neural (reiniciar)","Legacy preserva o preset atual. Reinhard e ACES mudam a entrada da rede; use uma passagem neural. Vale ao reabrir o jogo.",{L"legacy",L"reinhard",L"aces"},true},
 {L"ToneLift","Elevar sombras da entrada (reiniciar)","Eleva a entrada da rede em 0 a 0,25. Requer curva Reinhard ou ACES. Vale ao reabrir o jogo.",{L"0",L"0.025",L"0.05",L"0.075",L"0.10",L"0.15",L"0.20",L"0.25"},true},
 {L"UseGameExposure","Exposicao do jogo (reiniciar)","1 usa a exposicao do FSR; 0 seleciona a exposicao automatica do mod. Vale ao reabrir o jogo.",{L"1",L"0"},true},

 {L"NeuralRegSwin32X","Kernel swin32","Versão do kernel swin32 (~30% da rede). Daniel = original v0.4.3. A = reescrito, imagem idêntica, rede ~0,2 ms mais rápida no POST 1080. E = reescrito mais rápido, ~0,4 ms, imagem muda de leve (mais visível no 360p). Vale na hora.",{L"0",L"1",L"2"},false},

 {L"NeuralV05Tokens","Rede v0.5 (tokens)","Lê o nível global da rede inteiro, como a v0.5.0 do Daniel (o padrão dele): os tokens de enchimento do último nível entram na atenção. Só muda a imagem quando a grade da rede não é múltipla de 4: rede 360p, 1080p e 4K sim; só a rede de 720p exata (1280x720) não muda. No POST 33% com saída 4K a rede da cena roda a 640x360 (grade 10x6 vira 12x8): MUDA. Custo: pequeno (a parte global da rede roda com até o dobro de tokens a 360p). Vale na hora.",{L"0",L"1"},false},

 {L"NeuralRedeCadaQuadros","Rede a cada","Limita quantas vezes a rede neural roda no Assíncrono: um job novo só começa se já passaram estes quadros do jogo desde o anterior. Com rede menor (POST 810p) isto vira FPS de verdade, em troca de um realce mais velho (mais rastro em movimento). Todo quadro = como sempre (a rede roda assim que termina). Vale na hora.",{L"1",L"2",L"3",L"4",L"6",L"8",L"10"},false},

 {L"NeuralRostoJogador","Rosto do personagem","Segunda avaliação da rede só na cabeça do personagem controlado, num recorte da saída 4K em resolução cheia (a cabeça é achada pela profundidade do jogo). Mais detalhe no rosto; custa alguns ms por job. Fica ligado em close, cutscene e zoom; só sai num corte de câmera de verdade. Vale na hora.",{L"0",L"1"},false},
};
constexpr unsigned ini_count=unsigned(sizeof(ini_rows)/sizeof(*ini_rows));
constexpr unsigned ini_first_row=reproject_last_row+1;

constexpr bool mesma_chave(const wchar_t* a,const wchar_t* b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
constexpr unsigned ini_indice(const wchar_t* chave){for(unsigned i=0;i<ini_count;++i)if(mesma_chave(ini_rows[i].key,chave))return i;return ini_count;}
constexpr unsigned ini_prescale=ini_indice(L"NeuralPreScalePercent");
static_assert(ini_prescale<ini_count);

constexpr unsigned ini_prescale_auto=ini_indice(L"NeuralPreScaleAuto");
static_assert(ini_prescale_auto<ini_count&&ini_prescale_auto>ini_prescale);

constexpr unsigned valor_indice(const wchar_t* const* valores,const wchar_t* chave){for(unsigned i=0;i<14&&valores[i];++i)if(mesma_chave(valores[i],chave))return i;return 0;}
constexpr unsigned ini_prescale_auto_720=valor_indice(ini_rows[ini_prescale_auto].values,L"720");
static_assert(ini_prescale_auto_720>0,"720 precisa estar na lista de NeuralPreScaleAuto");

constexpr unsigned ini_limite=ini_indice(L"NeuralFrameCap");
constexpr unsigned ini_sw32x=ini_indice(L"NeuralRegSwin32X");
static_assert(ini_sw32x<ini_count);
constexpr unsigned ini_v05=ini_indice(L"NeuralV05Tokens");
static_assert(ini_v05<ini_count);
constexpr unsigned ini_rede_cada=ini_indice(L"NeuralRedeCadaQuadros");
static_assert(ini_rede_cada<ini_count);
constexpr unsigned ini_rosto=ini_indice(L"NeuralRostoJogador");
static_assert(ini_rosto<ini_count);
static_assert(ini_limite<ini_count);
static_assert(reproject_first_row+nrreproject::RatioTransfer==51,"linhas 46..51 inicializadas abaixo");
static_assert(reproject_first_row+nrreproject::GainLimit==52&&reproject_first_row+nrreproject::SoftGuard==53,"linhas 52..53 inicializadas abaixo");
static_assert(reproject_first_row+nrreproject::LightGuard==54&&reproject_first_row+nrreproject::DetailGuard==55&&reproject_first_row+nrreproject::LightSmooth==56,"linhas 54..56 inicializadas abaixo");
static_assert(reproject_first_row+nrreproject::HaloReduce==57&&reproject_first_row+nrreproject::DepthAwareRead==58&&reproject_first_row+nrreproject::RatioFloor==59,"linhas 57..59 inicializadas abaixo");
static_assert(reproject_first_row+nrreproject::LateCompose==60,"linha 60 inicializada abaixo");
static_assert(reproject_first_row+nrreproject::HaloNetScale==61&&reproject_first_row+nrreproject::LightGuideDepth==62,"linhas 61..62 inicializadas abaixo");
static_assert(reproject_first_row+nrreproject::HaloExcess==63&&reproject_first_row+nrreproject::EdgeCarry==64,"linhas 63..64 inicializadas abaixo");
static_assert(reproject_first_row+nrreproject::HaloCutMoving==65&&reproject_last_row==65,"linha 65 inicializada abaixo");
constexpr unsigned ini_last_row=ini_first_row+ini_count-1;

constexpr unsigned despejo_row=ini_last_row+1;
constexpr unsigned despejo_quadros=24;

constexpr unsigned idioma_row=despejo_row+1;
constexpr unsigned entrada_row=despejo_row+2;

constexpr unsigned modo_rede_row=despejo_row+3;

constexpr unsigned preset_row=despejo_row+4;

constexpr unsigned bilateral_row=despejo_row+5;
constexpr unsigned bilateral_niveis[]{0u,2u,3u,4u};
static_assert(nrreproject::bilateral_strength_list[2]==.5f&&nrreproject::bilateral_strength_list[3]==.75f&&nrreproject::bilateral_strength_list[4]==1.f);

constexpr unsigned print_row=despejo_row+6;
static_assert(print_row==bilateral_row+1);
constexpr unsigned last_row=print_row;

bool available(unsigned control,const NativeHost& host) noexcept {
    if(control==0)return true;
    if(control==1)return !host.hide_scale;
    if(control==2)return host.auto_mask!=nullptr;
    if(control==3)return host.style_requested!=nullptr;
    if(control==4||control==5)return host.multipass_requested!=nullptr;
    if(control==6)return host.tone_state!=nullptr;
    if(control==7)return host.present_mode;
    if(control>=8&&control<=10)return host.compose_controls!=nullptr;
    if(control==11||control==12)return false;
    return control<=last_row&&host.reproject_controls!=nullptr;
}
void initialise_menu_preferences(const std::filesystem::path& path) noexcept {
    wchar_t text[16]{};
    NRCFG_STR(L"DlssNrOnAmd",L"MenuLanguage",L"pt",text,16,path.c_str());
    nrmenu::Language language=nrmenu::Language::Pt;
    if(nrmenu::parse_language(text,language))nrmenu::set_language(language);
    const UINT block=NRCFG_INT(L"DlssNrOnAmd",L"MenuInputBlock",0,path.c_str());
    nrmenu::set_input_block(block==1?nrmenu::InputBlock::Everything:nrmenu::InputBlock::MenuKeys);
    nrmenu::set_show_help(NRCFG_INT(L"DlssNrOnAmd",L"MenuShowHelp",nrmenu::show_help_default?1:0,path.c_str())!=0);
}

struct IniCache{bool lido{};std::wstring atual[ini_count];std::wstring abertura[ini_count];};
IniCache& ini_cache(){static IniCache c;return c;}
std::wstring ini_ler(const std::wstring& path,const wchar_t* key){wchar_t t[64]{};const auto fallback=mesma_chave(key,L"ToneCurve")?L"legacy":mesma_chave(key,L"ToneLift")?L"0":mesma_chave(key,L"UseGameExposure")?L"1":L"";NRCFG_STR(L"DlssNrOnAmd",key,fallback,t,64,path.c_str());return t;}
void ini_preparar(const std::wstring& path){auto& c=ini_cache();if(c.lido)return;c.lido=true;for(unsigned i=0;i<ini_count;++i)c.atual[i]=c.abertura[i]=ini_ler(path,ini_rows[i].key);}
void ini_texto(unsigned i,char* out,std::size_t capacity) noexcept;
void ini_agir(const std::wstring& path,unsigned i,int delta) noexcept;

bool tonecurve_ativo(const std::wstring& path) noexcept {

    if(nrneuraltone::requested.load(std::memory_order_relaxed))return !nrneuraltone::multipass_ok.load(std::memory_order_relaxed);
    const auto v=ini_ler(path,L"ToneCurve");if(v.empty()||_wcsicmp(v.c_str(),L"legacy")==0)return false;
    if(NRCFG_INT(L"DlssNrOnAmd",L"NeuralToneMultipass",1,path.c_str())==0)return true;
    const auto l=ini_ler(path,L"ToneLift");wchar_t* fim{};const float lift=std::wcstof(l.c_str(),&fim);
    return !(fim!=l.c_str()&&!*fim&&nrneuraltone::identidade(v.c_str(),lift));
}
const wchar_t* reproject_keys[]={L"NeuralReprojectClampStrength",L"NeuralReprojectMotionDivergence",
                                 L"NeuralReprojectDepthRelative",L"NeuralReprojectColorThreshold",
                                 L"NeuralReprojectConfidenceRate",L"NeuralReprojectMotionFactor",
                                 L"NeuralReprojectDeformTolerance",L"NeuralReprojectDetailMotion",
                                 L"NeuralReprojectOcclusionTolerance",
                                 L"NeuralReprojectBilateral",L"NeuralReprojectBilateralDetail",
                                 L"NeuralReprojectBilateralGuide",L"NeuralReprojectBilateralHold",
                                 L"NeuralReprojectBilateralRidge",L"NeuralReprojectPointMotion",
                                 L"NeuralReprojectDualMotion",L"NeuralReprojectDualMotionThreshold",
                                 L"NeuralReprojectResidualFloor",L"NeuralReprojectConfidenceFloor",
                                 L"NeuralReprojectGateSmooth",L"NeuralReprojectDeformBlur",
                                 L"NeuralReprojectMotionReference",L"NeuralReprojectDepthAbsolute",
                                 L"NeuralAsyncMaxAge",L"NeuralReprojectComposeAfterUpscale",L"NeuralReprojectFillMode",L"NeuralReprojectDepthSnap",L"NeuralReprojectMotionRepair",L"NeuralReprojectRawDivergence",L"NeuralReprojectStabilize",L"NeuralGradeExposure",L"NeuralGradeContrast",L"NeuralGradeSaturation",L"NeuralReprojectMotionStrength",L"NeuralReprojectMotionStrengthRef",L"NeuralReprojectJitterComp",L"NeuralReprojectStabilizeStill",L"NeuralReprojectNoiseCancel",L"NeuralReprojectRatioTransfer",L"NeuralGainLimit",L"NeuralHighlightGuardSoft",
                                 L"NeuralHighlightGuardLight",L"NeuralHighlightGuardDetail",L"NeuralLightSmooth",L"NeuralHaloReduce",
                                 L"NeuralReprojectDepthAwareRead",L"NeuralRatioFloor",L"NeuralLateCompose",
                                 L"NeuralHaloNetScale",L"NeuralLightGuideDepth",L"NeuralHaloExcess",L"NeuralReprojectEdgeCarry",
                                 L"NeuralHaloCutMoving"};
static_assert(sizeof(reproject_keys)/sizeof(*reproject_keys)==nrreproject::ControlCount);

bool reproject_aceita_negativo(unsigned control) noexcept { return control==nrreproject::MotionFactor||control==nrreproject::JitterComp||control==nrreproject::GradeExposure||control==nrreproject::GradeContrast||control==nrreproject::GradeSaturation; }

unsigned reproject_decimals(unsigned control) noexcept {
    using namespace nrreproject;
    switch(control){
    case Depth: case BilateralDetail: case ResidualFloor: case MotionReference: return 3;
    case Hysteresis: case BilateralRidge: case DepthAbsolute: return 4;
    case BilateralGuide: case BilateralHold: case PointMotion: case DualMotion:
    case GateSmooth: case DeformBlur: case MaxAge: case ComposeAfter: case FillMode: case DepthSnap: case MotionRepair: case RawDivergence: return 0;
    case Stabilize: return 2;
    case GradeExposure: case GradeContrast: case GradeSaturation: return 2;
    case MotionStrength: return 2;
    case MotionStrengthRef: return 0;
    case JitterComp: return 0;
    case StabilizeStill: return 0;
    case NoiseCancel: return 2;
    case RatioTransfer: return 0;
    case GainLimit: return 2;
    case SoftGuard: return 0;
    case LightGuard: case DetailGuard: case LightSmooth: return 2;
    case HaloReduce: return 0;
    case DepthAwareRead: return 0;
    case RatioFloor: return 4;
    case LateCompose: return 0;
    case HaloNetScale: case LightGuideDepth: return 0;
    case HaloExcess: return 1;
    case EdgeCarry: return 0;
    case HaloCutMoving: return 0;
    default: return 2;
    }
}
void reproject_text(unsigned control,unsigned index,char* out,std::size_t capacity) noexcept {

    if(index==0 && (control==nrreproject::Clamp || control==nrreproject::Divergence ||
                    control==nrreproject::Hysteresis ||
                    control==nrreproject::DeformTolerance ||
                    control==nrreproject::DetailMotion ||
                    control==nrreproject::OcclusionTolerance ||
                    control==nrreproject::BilateralStrength || control==nrreproject::BilateralHold ||
                    control==nrreproject::PointMotion || control==nrreproject::DualMotion ||
                    control==nrreproject::ResidualFloor || control==nrreproject::ConfidenceFloor ||
                    control==nrreproject::GateSmooth || control==nrreproject::DeformBlur ||
                    control==nrreproject::MaxAge)){
        output(out,capacity,"Desligado");return;
    }
    if(control==nrreproject::BilateralGuide){output(out,capacity,index?"Profundidade":"Luminância");return;}
    if(control==nrreproject::ComposeAfter){output(out,capacity,index?"Depois do FSR":"Antes do FSR");return;}
    if(control==nrreproject::FillMode){output(out,capacity,index?"Borda suave":"Média");return;}
    if(control==nrreproject::DepthSnap){output(out,capacity,index==0?"Desligado":index==1?"1 px":"2 px");return;}
    if(control==nrreproject::MotionRepair){output(out,capacity,index==0?"Desligado":index==1?"2 px":index==2?"3 px":"4 px");return;}
    if(control==nrreproject::RawDivergence){output(out,capacity,index==0?"Desligado":index==1?"Fraca":index==2?"Media":"Forte");return;}
    if(control==nrreproject::Stabilize){output(out,capacity,index==0?"Desligado":index==1?"Leve":index==2?"Média":"Forte");return;}
    if(control==nrreproject::MotionStrength){static constexpr const char* t[]={"Desligado","75%","60%","50%","40%","30%"};output(out,capacity,index<6?t[index]:"?");return;}
    if(control==nrreproject::RatioTransfer){output(out,capacity,index?"Ligado":"Desligado");return;}
    if(control==nrreproject::GainLimit){static constexpr const char* t[]={"Desligado","5%","10%","20%"};output(out,capacity,index<4?t[index]:"?");return;}
    if(control==nrreproject::SoftGuard){output(out,capacity,index?"Ligado":"Desligado");return;}

    if(control==nrreproject::LightGuard){static constexpr const char* t[]={"Desligado","1.5","2.0","2.5","3.0","4.0","5.0","6.0","8.0","Sem limite"};
        static_assert(sizeof(t)/sizeof(*t)==nrreproject::counts[nrreproject::LightGuard]);output(out,capacity,index<10?t[index]:"?");return;}
    if(control==nrreproject::DetailGuard){static constexpr const char* t[]={"1.1","1.2","1.3","1.5","1.75","2.0","2.5","3.0","4.0","6.0","8.0","Sem limite"};
        static_assert(sizeof(t)/sizeof(*t)==nrreproject::counts[nrreproject::DetailGuard]);output(out,capacity,index<12?t[index]:"?");return;}
    if(control==nrreproject::LightSmooth){static constexpr const char* t[]={"Desligado","Leve","Média","Forte"};output(out,capacity,index<4?t[index]:"?");return;}
    if(control==nrreproject::HaloReduce){static constexpr const char* t[]={"Desligado","Leve","Médio","Forte"};output(out,capacity,index<4?t[index]:"?");return;}
    if(control==nrreproject::DepthAwareRead){output(out,capacity,index?"Ligado":"Desligado");return;}
    if(control==nrreproject::RatioFloor){static constexpr const char* t[]={"Padrão (1/512)","0,005","0,01"};output(out,capacity,index<3?t[index]:"?");return;}
    if(control==nrreproject::LateCompose){output(out,capacity,index?"Ligado":"Desligado");return;}
    if(control==nrreproject::HaloNetScale||control==nrreproject::LightGuideDepth){output(out,capacity,index?"Ligado":"Desligado");return;}
    if(control==nrreproject::HaloExcess){static constexpr const char* t[]={"Desligado","Leve","Médio","Forte"};output(out,capacity,index<4?t[index]:"?");return;}
    if(control==nrreproject::EdgeCarry){output(out,capacity,index?"Ligado":"Desligado");return;}
    if(control==nrreproject::HaloCutMoving){static constexpr const char* t[]={"Desligado","Forte","Máximo"};output(out,capacity,index<3?t[index]:"?");return;}
    if(control==nrreproject::NoiseCancel){static constexpr const char* t[]={"Desligado","Fraco","Médio","Forte","Máximo"};output(out,capacity,index<5?t[index]:"?");return;}
    if(control==nrreproject::StabilizeStill){static constexpr const char* t[]={"2 px","1 px","4 px","8 px","Sempre"};output(out,capacity,index<5?t[index]:"?");return;}
    if(control==nrreproject::JitterComp){output(out,capacity,index==0?"Desligado":index==1?"Ligado":"Invertido");return;}
    if(control==nrreproject::MotionStrengthRef){static constexpr const char* t[]={"2 px","4 px","8 px","16 px"};output(out,capacity,index<4?t[index]:"?");return;}
    if(control==nrreproject::PointMotion){output(out,capacity,"Ligado");return;}
    if(control==nrreproject::DualThreshold&&index==0){output(out,capacity,"Sem limiar");return;}
    char text[32]{};
    std::snprintf(text,sizeof(text),"%.*f",int(reproject_decimals(control)),
                  double(nrreproject::lists[control][index]));
    output(out,capacity,text);
}
void initialise_reproject(Context& context) noexcept {
    if(!context.reproject_controls||context.reproject_initialised)return;
    context.reproject_initialised=true;
    unsigned chosen[nrreproject::ControlCount]{};
    for(unsigned i=0;i<nrreproject::ControlCount;++i){
        wchar_t text[32]{};
        const auto count=NRCFG_STR(L"DlssNrOnAmd",reproject_keys[i],L"",text,32,context.path.c_str());

        if(!count){chosen[i]=i==nrreproject::LightGuard?nrreproject::nearest(i,nrrealce::luz_padrao):nrreproject::defaults[i];continue;}
        if(count>=31){context.reproject_error=Error::Input;return;}
        wchar_t* end{};
        const double parsed=std::wcstod(text,&end);
        if(end==text||!end||*end||!std::isfinite(parsed)||
           (parsed<0&&!reproject_aceita_negativo(i))){context.reproject_error=Error::Input;return;}
        chosen[i]=nrreproject::nearest(i,float(parsed));

        if(i==nrreproject::LightGuard&&nrrealce::luz_legada(parsed)){
            chosen[i]=nrreproject::nearest(i,nrreproject::guarda_sem_limite);

FILE* f{};if(_wfopen_s(&f,L"nr_realce.log",L"a")==0&&f){
                std::fprintf(f,"pid=%lu REALCE_LUZ_LEGADO NeuralHighlightGuardLight=%.2f (Desligado antigo, saiu da lista) -> Sem limite; o INI so muda quando a linha for trocada no menu\n",
                             GetCurrentProcessId(),parsed);std::fclose(f);}

        }
    }
    for(unsigned i=0;i<nrreproject::ControlCount;++i)nrreproject::set(*context.reproject_controls,i,chosen[i]);

    context.reproject_controls->seeded.store(true,std::memory_order_release);

    context.reproject_controls->menu.store(true,std::memory_order_release);
}

void carregar_grade_do_estilo(Context& context,unsigned estilo) noexcept {
    if(!context.reproject_controls)return;
    float t=nrreproject::grade_tom.load();if(!(t>0.f))t=0.f;if(t>1.f)t=1.f;
    const float ev=estilo==1?-.1f*t:0.f,co=estilo==1?-.25f*t:0.f,sa=estilo==1?-.1f*t:estilo==2?-.15f*t:0.f;
    const unsigned ctl[3]={nrreproject::GradeExposure,nrreproject::GradeContrast,nrreproject::GradeSaturation};const float val[3]={ev,co,sa};
    for(unsigned k=0;k<3;++k){
        const unsigned idx=nrreproject::nearest(ctl[k],val[k]);
        wchar_t text[32]{};std::swprintf(text,32,L"%.2f",double(nrreproject::lists[ctl[k]][idx]));
        if(!NRCFG_ESCREVER(L"DlssNrOnAmd",reproject_keys[ctl[k]],text,context.path.c_str())){context.reproject_error=Error::Save;return;}
        nrreproject::set(*context.reproject_controls,ctl[k],idx);
    }
}

nrpreset::Leitura ler_preset(const Context& context) noexcept {
    nrpreset::Leitura l{};
    auto texto=[&](const wchar_t* chave,wchar_t (&v)[32]){return NRCFG_STR(L"DlssNrOnAmd",chave,L"",v,32,context.path.c_str())>0;};
    auto real=[&](const wchar_t* chave,double& out){wchar_t v[32]{};if(!texto(chave,v))return false;wchar_t* fim{};out=std::wcstod(v,&fim);return fim&&fim!=v&&std::isfinite(out);};
    auto inteiro=[&](const wchar_t* chave,unsigned& out){wchar_t v[32]{};if(!texto(chave,v))return false;unsigned n=0;
        for(const wchar_t* c=v;*c;++c){if(*c<L'0'||*c>L'9'||n>100000u)return false;n=n*10u+unsigned(*c-L'0');}out=n;return true;};
    unsigned estilo{};l.tem_estilo=inteiro(L"NeuralStyle",estilo);l.estilo=int(estilo);
    l.tem_local=real(L"LocalStructure",l.local);l.tem_skin=real(L"SkinStructure",l.skin);
    l.tem_mascara=inteiro(L"UseAutoMask",l.mascara);l.tem_passes=inteiro(L"NeuralPassConfig",l.pass_config);
    l.tem_tom=real(L"LocalTone",l.tom);l.tem_canais=inteiro(L"ToneChannels",l.canais);l.tem_config=inteiro(L"NeuralToneConfig",l.config);
    return l;
}

bool aplicar_preset(Context& context,unsigned preset) noexcept {
    if(preset>2)return false;
    const auto lido=ler_preset(context);
    const unsigned passes=nrpreset::pass_config(lido.tem_passes,lido.pass_config,tonecurve_ativo(context.path));
    wchar_t estilo[2]{wchar_t(L'0'+preset),0},pc[16]{};std::swprintf(pc,16,L"%u",passes);
    const struct {const wchar_t* chave;const wchar_t* valor;} chaves[]{{L"NeuralStyle",estilo},{L"LocalStructure",L"0.880"},{L"SkinStructure",L"1.000"},
        {L"UseAutoMask",L"1"},{L"NeuralPassConfig",pc},{L"LocalTone",L"1.000"},{L"ToneChannels",L"1"},{L"NeuralToneConfig",L"5"}};
    for(const auto& e:chaves)if(!NRCFG_ESCREVER(L"DlssNrOnAmd",e.chave,e.valor,context.path.c_str())){context.preset_error=Error::Save;return false;}
    (void)cache(context);
    if(valid_style(context)){InterlockedExchange(reinterpret_cast<volatile LONG*>(context.style_requested),LONG(preset));
        nrreproject::grade_estilo.store(preset);nrreproject::grade_semeado.store(true);carregar_grade_do_estilo(context,preset);context.style_error=Error::None;}
    if(valid_mask(context.auto_mask)){InterlockedExchange(reinterpret_cast<volatile LONG*>(context.auto_mask),1);context.mask_save_error=false;}
    if(valid_multipass(context)){InterlockedExchange(reinterpret_cast<volatile LONG*>(context.multipass_requested),LONG(passes));context.multipass_error=Error::None;}
    if(context.tone_state){context.tone_state->requested.store(nrpreset::tone_config,std::memory_order_release);context.tone_error=Error::None;
        nrreproject::grade_tom.store(nrreproject::tom_de_config(nrpreset::tone_config,float(nrpreset::local_tone)));}
    auto* bytes=static_cast<char*>(context.base);
    if(bytes&&context.auto_mask==reinterpret_cast<unsigned*>(bytes+0x94AB0)&&accessible(bytes+0x94AA4,8,Access::Write)){
        const float local=float(nrpreset::local_structure),skin=float(nrpreset::skin_structure);LONG l{},s{};std::memcpy(&l,&local,4);std::memcpy(&s,&skin,4);
        InterlockedExchange(reinterpret_cast<volatile LONG*>(bytes+0x94AA4),l);InterlockedExchange(reinterpret_cast<volatile LONG*>(bytes+0x94AA8),s);
    }
    context.preset_error=Error::None;return true;
}

struct PresetCache{bool lido{};unsigned valor{};FILETIME escrita{};std::uint64_t tamanho{};};
PresetCache& preset_cache() noexcept {static PresetCache c;return c;}
void preset_invalidar() noexcept {preset_cache().lido=false;}

bool mudou_no_disco(const std::wstring& path,PresetCache& c) noexcept {
    WIN32_FILE_ATTRIBUTE_DATA d{};
    if(!GetFileAttributesExW(path.c_str(),GetFileExInfoStandard,&d)){c.escrita=FILETIME{};c.tamanho=0;return true;}
    const std::uint64_t tamanho=(std::uint64_t(d.nFileSizeHigh)<<32)|d.nFileSizeLow;
    const bool mudou=d.ftLastWriteTime.dwLowDateTime!=c.escrita.dwLowDateTime||d.ftLastWriteTime.dwHighDateTime!=c.escrita.dwHighDateTime||tamanho!=c.tamanho;
    c.escrita=d.ftLastWriteTime;c.tamanho=tamanho;
    return mudou;
}
unsigned preset_valor(Context& context) noexcept {
    auto& c=preset_cache();
    if(mudou_no_disco(context.path,c))c.lido=false;
    if(c.lido)return c.valor;
    c.valor=context.preset_personalizado?nrpreset::personalizado:nrpreset::detectar(ler_preset(context),tonecurve_ativo(context.path));
    c.lido=true;
    return c.valor;
}

void preset_invalidar_reavaliar(Context& context) noexcept {
    preset_invalidar();
    if(context.preset_personalizado&&nrpreset::detectar(ler_preset(context),tonecurve_ativo(context.path))!=nrpreset::personalizado)
        context.preset_personalizado=false;
}
void ini_texto(unsigned i,char* out,std::size_t capacity) noexcept {
    const auto& c=ini_cache();const std::wstring& v=c.atual[i];
    if(i==ini_prescale_auto){

        const long long chave=v.empty()?0:std::wcstoll(v.c_str(),nullptr,10);
        if(!nrprescale::auto_alvo_de(chave,1)){output(out,capacity,"Desligado");return;}
        char t[48]{};const int pct=int(nrprescale::efetivo.load(std::memory_order_relaxed)+.5);
        if(chave==1)std::snprintf(t,sizeof t,"Ligado (%d%%)",pct);else std::snprintf(t,sizeof t,"%lldp (%d%%)",chave,pct);
        output(out,capacity,t);return;
    }
    if(i==ini_sw32x){
        const int e=nrsw32x::escolha.load(std::memory_order_relaxed);const unsigned d=nrsw32x::disponivel.load(std::memory_order_acquire);
        static constexpr const char* nomes[]{"Daniel","A (exato)","E (rápido)"};
        const int k=e>=0&&e<=2?e:0;
        output(out,capacity,d==0?"indisp.":(d>>k&1u)?nomes[k]:"sem arquivo");return;
    }
    if(i==ini_v05){

        const bool on=nrv05::menu_escreveu.load()?nrv05::ligado.load():nrv05::valor_de(v.c_str());
        output(out,capacity,on?"Ligado":"Desligado");return;
    }
    if(i==ini_rosto){

        const bool on=(nrrj::menu_escreveu.load()||nrrj::ini_lido.load())?nrrj::ligado.load():nrrj::valor_de(v.c_str());
        output(out,capacity,on?"Ligado":"Desligado");return;
    }
    if(i==ini_rede_cada){

        const unsigned n=nrredecada::definido.load()?nrredecada::aceitar(nrredecada::pedido.load()):nrredecada::aceitar(v.empty()?0:std::wcstoll(v.c_str(),nullptr,10));
        char t[24]{};if(n<=1)std::snprintf(t,sizeof t,"Todo quadro");else std::snprintf(t,sizeof t,"%u quadros",n);
        output(out,capacity,t);return;
    }
    if(i==ini_limite){

        const unsigned fps=nrlimit::definido.load()?nrlimit::pedido.load():nrlimit::aceitar(v.empty()?0:std::wcstoll(v.c_str(),nullptr,10));
        char t[24]{};if(fps)std::snprintf(t,sizeof t,"%u FPS",fps);else std::snprintf(t,sizeof t,"Desligado");
        output(out,capacity,t);return;
    }
    if(v.empty()){output(out,capacity,"Padrão");return;}
    char t[32]{};std::size_t n=0;for(wchar_t ch:v){if(n+1>=sizeof(t))break;t[n++]=(ch>=32&&ch<127)?char(ch):'?';}
    if(ini_rows[i].restart&&v!=c.abertura[i]&&n+6<sizeof(t)){std::memcpy(t+n," pend.",6);n+=6;}
    t[n]=0;output(out,capacity,t);
}
void ini_agir(const std::wstring& path,unsigned i,int delta) noexcept {
    auto& c=ini_cache();const auto& row=ini_rows[i];unsigned n=0;while(n<14&&row.values[n])++n;if(!n)return;
    if(i==ini_sw32x){

        const int atual=nrsw32x::escolha.load(std::memory_order_relaxed);
        const int novo=((atual<0||atual>2?0:atual)+(delta<0?2:1))%3;
        const wchar_t texto[]={wchar_t(L'0'+novo),0};
        if(!NRCFG_ESCREVER(L"DlssNrOnAmd",row.key,texto,path.c_str()))return;
        c.atual[i]=texto;nrsw32x::escolha.store(novo,std::memory_order_relaxed);
        return;
    }
    if(i==ini_v05){

        const bool atual=nrv05::menu_escreveu.load()?nrv05::ligado.load():nrv05::valor_de(c.atual[i].c_str());
        const wchar_t* texto=atual?L"0":L"1";
        if(!NRCFG_ESCREVER(L"DlssNrOnAmd",row.key,texto,path.c_str()))return;
        c.atual[i]=texto;nrv05::ligado.store(!atual);nrv05::menu_escreveu.store(true);
        return;
    }
    if(i==ini_rosto){

        const bool atual=(nrrj::menu_escreveu.load()||nrrj::ini_lido.load())?nrrj::ligado.load():nrrj::valor_de(c.atual[i].c_str());
        const wchar_t* texto=atual?L"0":L"1";
        if(!NRCFG_ESCREVER(L"DlssNrOnAmd",row.key,texto,path.c_str()))return;
        c.atual[i]=texto;nrrj::ligado.store(!atual);nrrj::menu_escreveu.store(true);
        return;
    }
    if(i==ini_rede_cada){

        const unsigned atual=nrredecada::definido.load()?nrredecada::aceitar(nrredecada::pedido.load()):nrredecada::aceitar(c.atual[i].empty()?0:std::wcstoll(c.atual[i].c_str(),nullptr,10));
        const unsigned novo=nrredecada::proximo(atual,delta);
        wchar_t texto[8]{};std::swprintf(texto,8,L"%u",novo);
        if(!NRCFG_ESCREVER(L"DlssNrOnAmd",row.key,texto,path.c_str()))return;
        c.atual[i]=texto;nrredecada::pedido.store(novo);nrredecada::definido.store(true);
        return;
    }
    if(i==ini_limite){

        const unsigned atual=nrlimit::definido.load()?nrlimit::pedido.load():nrlimit::aceitar(c.atual[i].empty()?0:std::wcstoll(c.atual[i].c_str(),nullptr,10));
        const unsigned novo=nrlimit::proximo(atual,delta);
        wchar_t texto[16]{};std::swprintf(texto,16,L"%u",novo);
        if(!NRCFG_ESCREVER(L"DlssNrOnAmd",row.key,texto,path.c_str()))return;
        c.atual[i]=texto;nrlimit::pedido.store(novo);nrlimit::definido.store(true);
        return;
    }
    int atual=-1;for(unsigned k=0;k<n;++k)if(c.atual[i]==row.values[k])atual=int(k);

    if(atual<0&&i==ini_prescale_auto)atual=int(ini_prescale_auto_720);
    const int proximo=atual<0?0:(atual+(delta<0?int(n)-1:1))%int(n);
    if(!NRCFG_ESCREVER(L"DlssNrOnAmd",row.key,row.values[proximo],path.c_str()))return;
    c.atual[i]=row.values[proximo];
    if(i==ini_prescale_auto){

        wchar_t alvo[32]{};NRCFG_STR(L"DlssNrOnAmd",L"NeuralPreScaleTargetPixels",L"0",alvo,32,path.c_str());
        nrprescale::auto_alvo.store(nrprescale::auto_alvo_de(std::wcstoll(row.values[proximo],nullptr,10),std::wcstoll(alvo,nullptr,10)));
        nrprescale::auto_menu_escreveu.store(true);
        return;
    }
    if(!row.restart&&std::wcscmp(row.key,L"Scale")==0){
        const double novo=std::wcstod(row.values[proximo],nullptr);
        if(novo>0)nrreproject::escala_desejada.store(float(novo),std::memory_order_relaxed);
    }
}
void read(void* data,unsigned control,char* out,std::size_t capacity) noexcept {
    Admission admission; if (!admission.admitted) { output(out,capacity,"Ocupado"); return; }
    auto& context=*static_cast<Context*>(data);
    try {

        if(control==12){output(out,capacity,"Assíncrono");return;}
        if(control>=ini_first_row && control<=ini_last_row){ini_preparar(context.path);ini_texto(control-ini_first_row,out,capacity);return;}
        if(control==idioma_row){output(out,capacity,nrmenu::language_name(nrmenu::language()));return;}
        if(control==preset_row){
            if(context.preset_error!=Error::None){output(out,capacity,"Erro ao salvar");return;}
            const unsigned p=preset_valor(context);
            static constexpr const char* nomes[]{"Preset A","Preset B","Preset C","Personalizado"};
            output(out,capacity,nomes[p]);return;
        }
        if(control==modo_rede_row){

            const auto state=context.store.snapshot();
            if(state.readiness==nr::Readiness::Failed){output(out,capacity,"Erro no NR");return;}
            const bool ready=state.readiness==nr::Readiness::Ready&&state.effective&&state.effective_generation==state.generation&&*state.effective==state.requested;
            const unsigned indice=nrprescale::modo2_indice(state.requested.pre_upscale,nrprescale::auto_alvo.load(std::memory_order_relaxed),nrprescale::post_chave.load(std::memory_order_relaxed));
            const bool pre=nrprescale::modo2_pre(indice);const unsigned nominal=nrprescale::modo2_linhas(indice);
            const int pct=int(nrprescale::efetivo.load(std::memory_order_relaxed)+.5);
            char t[48]{};

            (void)ready;

            const auto q=nrprescale::unpack_quadro(nrprescale::quadro.load(std::memory_order_acquire));
            const unsigned base=pre?q.render_h:q.saida_h;
            const unsigned linhas=base?unsigned(double(base)*nrprescale::efetivo.load(std::memory_order_relaxed)/100.+.5):0u;
            const char* nome=pre?"PRE":"POST";

            if(linhas){if(linhas+12>=nominal&&linhas<=nominal+12)std::snprintf(t,sizeof t,"%s %up",nome,nominal);
                else std::snprintf(t,sizeof t,"%s %u>%up",nome,nominal,linhas);}
            else if(pre)std::snprintf(t,sizeof t,"PRE %up (%d%%)",nominal,pct);
            else std::snprintf(t,sizeof t,"POST %u (%d%%)",nrprescale::modo2_chave(indice),pct);
            output(out,capacity,t);return;
        }
        if(control==bilateral_row){
            if(!context.reproject_controls){output(out,capacity,"Indisponível");return;}
            if(context.reproject_error!=Error::None){output(out,capacity,context.reproject_error==Error::Save?"Erro ao salvar":"Valor inválido");return;}
            const unsigned i=nrreproject::index_of(*context.reproject_controls,nrreproject::BilateralStrength);
            static constexpr const char* nomes[]{"Desligada","0.25","Leve","Média","Forte"};
            output(out,capacity,i<5?nomes[i]:"?");return;
        }
        if(control==entrada_row){output(out,capacity,nrmenu::input_block()==nrmenu::InputBlock::Everything?nrmenu::input_everything:nrmenu::input_menu_keys);return;}
        if(control==despejo_row){char t[48]{};if(nrreproject::gravar_aviso.load()&&nrreproject::final_cheio.load())std::snprintf(t,sizeof t,"Cheia (%u/%u)",nrreproject::final_total.load(),nrreproject::final_teto_sessao.load());
            else if(nrreproject::despejo_final.load())std::snprintf(t,sizeof t,"Gravando...");
            else std::snprintf(t,sizeof t,"Gravados %u",nrreproject::final_gravados.load());output(out,capacity,t);return;}
        if(control==print_row){char t[48]{};
            if(nrprint::pedido.load()||nrprint::salvando.load())std::snprintf(t,sizeof t,"Salvando...");
            else if(nrprint::ultima_falhou.load())std::snprintf(t,sizeof t,"Erro ao salvar");
            else std::snprintf(t,sizeof t,"Salvo %u",nrprint::salvos.load());
            output(out,capacity,t);return;}
        if(control>=reproject_first_row && control<=reproject_last_row){
            if(!context.reproject_controls){output(out,capacity,"Indisponível");return;}
            if(context.reproject_error!=Error::None){output(out,capacity,context.reproject_error==Error::Save?"Erro ao salvar":"Valor inválido");return;}

            reproject_text(control-13,nrreproject::index_of(*context.reproject_controls,control-13),out,capacity);return;
        }
        if(control>=8 && control<=10){
            if(!context.compose_controls){output(out,capacity,"Indisponível");return;}
            if(context.present_mode&&nrpresentmode::active()){output(out,capacity,"Só PRE/POST");return;}
            if(context.compose_error!=Error::None){output(out,capacity,context.compose_error==Error::Save?"Erro ao salvar":"Valor inválido");return;}
            const auto requested=context.compose_controls->requested.load();
            if(!nrcompose::valid(requested)){output(out,capacity,"Valor inválido");return;}
            char text[32]{};std::snprintf(text,sizeof(text),"%.2f%s",component(requested,control)/20.,requested==context.compose_controls->recorded.load()?"":" pend.");
            output(out,capacity,text);return;
        }
        if(control==7) {
            if(!context.present_mode){output(out,capacity,"Indisponível");return;}
            if(context.present_save_error){output(out,capacity,"Erro ao salvar");return;}
            const auto startup=nrpresentmode::startup.load(),requested=nrpresentmode::requested.load();
            if(!startup){output(out,capacity,"Aguardando");return;}
            if(requested!=(startup==2 ? 1u : 0u)){output(out,capacity,requested ? "Liga ao reabrir" : "Desl. reabrir");return;}
            const auto status=nrpresentmode::status.load();
            if(status==4){output(out,capacity,"Erro no NR");return;}
            if(startup==1){output(out,capacity,"Desligado");return;}
            output(out,capacity,status==1 ? "Ligado: ativo" : status==2 ? "Ligado: inativo" : status==3 ? "Sem suporte" : "Ligado: espera");return;
        }
        if(control==0 && context.present_mode && nrpresentmode::active()){
            output(out,capacity,"Usando Present");return;
        }
        if(control==6) {
            if(!context.tone_state){output(out,capacity,"Indisponível");return;}
            if(context.tone_error!=Error::None){output(out,capacity,context.tone_error==Error::Save ? "Erro ao salvar" : "Valor inválido");return;}
            const auto request=context.tone_state->requested.load(),effective=context.tone_state->effective.load();
            if(request>9){output(out,capacity,"Valor inválido");return;}
            if(effective==nrtone::failed){output(out,capacity,"Erro no NR");return;}
            const char* names[]={"Original","0","0.25","0.5","0.75","1","1.25","1.5","1.75","2"};char text[24]{};
            std::snprintf(text,sizeof(text),"%s%s",names[request],request==effective ? "" : " pend.");output(out,capacity,text);return;
        }
        if(control==4 || control==5) {
            if(!valid_multipass(context)) {output(out,capacity,"Indisponível");return;}
            if(context.multipass_error!=Error::None) {output(out,capacity,context.multipass_error==Error::Save ? "Erro ao salvar" : context.multipass_error==Error::Read ? "Erro de leitura" : "Valor inválido");return;}
            const unsigned requested=scalar(context.multipass_requested),effective=scalar(context.multipass_effective);
            if(!valid_pass_config(requested) || !valid_pass_config(effective)) {output(out,capacity,"Valor inválido");return;}
            static constexpr const char* strengths[]={"0x","0.5x","0.75x","1x","1.25x","1.5x","2x"};
            char text[16]{};
            if(control==4 && context.multipass_tonecurve && (requested&255u)==1u) {output(out,capacity,"Só 1 (Curva)");return;}
            if(control==4)std::snprintf(text,sizeof(text),"%u%s",requested&255u,requested==effective ? "" : " pend.");
            else std::snprintf(text,sizeof(text),"%s%s",strengths[requested>>8],requested==effective ? "" : " pend.");
            output(out,capacity,text);return;
        }
        if(control==3) {
            if(!valid_style(context)) {output(out,capacity,"Indisponível");return;}
            if(context.style_error!=Error::None) {output(out,capacity,context.style_error==Error::Save ? "Erro ao salvar" : context.style_error==Error::Read ? "Erro de leitura" : "Valor inválido");return;}
            const LONG requested=scalar(context.style_requested),effective=scalar(context.style_effective);
            if(requested<0 || requested>2 || effective<0 || effective>2) {output(out,capacity,"Valor inválido");return;}
            static constexpr const char* names[]={"Padrão","Natural","Cinemático"};
            static constexpr const char* pending[]={"Padrão pend.","Natural pend.","Cinemático..."};
            output(out,capacity,requested==effective ? names[requested] : pending[requested]);return;
        }
        if (control==2) {
            if(!valid_mask(context.auto_mask)) {output(out,capacity,"Indisponível");return;}
            if(context.mask_save_error) {output(out,capacity,"Erro ao salvar");return;}
            const auto native=InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(context.auto_mask),0,0);
            output(out,capacity,native>1 || native<0 ? "Valor inválido" : native ? "Ligado" : "Desligado");return;
        }
        if (control>1) { output(out,capacity,"Valor inválido"); return; }
        switch (context.error) {
            case Error::Input: output(out,capacity,"Valor inválido"); return;
            case Error::Save: output(out,capacity,"Erro ao salvar"); return;
            case Error::Read: output(out,capacity,"Erro de leitura"); return;
            case Error::Cache: output(out,capacity,"Erro de cache"); return;
            case Error::Exception: output(out,capacity,"Erro adaptador"); return;
            case Error::Runtime: output(out,capacity,"Erro no NR"); return;
            case Error::None: break;
        }
        const auto state=context.store.snapshot();
        if (state.readiness==nr::Readiness::Failed) { output(out,capacity,"Erro no NR"); return; }
        const bool ready=state.readiness==nr::Readiness::Ready && state.effective &&
            state.effective_generation==state.generation && *state.effective==state.requested;
        const char* status=ready ? "ativo" : "pend.";
        char text[16]{};
        if (control==0) std::snprintf(text,sizeof(text),"%s %s",state.requested.pre_upscale ? "Ligado" : "Desligado",status);
        else std::snprintf(text,sizeof(text),"%u%% %s",state.requested.percent,status);
        output(out,capacity,text);
    } catch (...) { context.error=Error::Exception; output(out,capacity,"Erro adaptador"); }
}
void act(void* data,unsigned control,int delta) noexcept {
    Admission admission; if (!admission.admitted) return;
    auto& context=*static_cast<Context*>(data);
    try {
        if(control==12)return;
        if(control>=ini_first_row&&control<=ini_last_row){if(delta<-1||delta>1)return;ini_preparar(context.path);ini_agir(context.path,control-ini_first_row,delta);return;}
        if(control==preset_row){
            if(delta<-1||delta>1)return;
            if(!cache(context))return;
            const unsigned atual=preset_valor(context);
            const unsigned novo=nrpreset::proximo(atual,delta<0?-1:1);

            if(novo==nrpreset::personalizado){context.preset_personalizado=true;context.preset_error=Error::None;preset_invalidar();return;}
            context.preset_personalizado=false;(void)aplicar_preset(context,novo);(void)cache(context);preset_invalidar();return;
        }
        if(control==modo_rede_row){
            if(delta<-1||delta>1)return;
            if(!cache(context))return;
            const auto state=context.store.snapshot();
            const unsigned atual=nrprescale::modo2_indice(state.requested.pre_upscale,nrprescale::auto_alvo.load(std::memory_order_relaxed),nrprescale::post_chave.load(std::memory_order_relaxed));
            const unsigned novo=nrprescale::modo2_proximo(atual,delta<0?-1:1);
            if(!nrprescale::modo2_pre(novo)){
                wchar_t texto[8]{};std::swprintf(texto,8,L"%u",nrprescale::modo2_chave(novo));
                if(!NRCFG_ESCREVER(L"DlssNrOnAmd",L"NeuralPostPercent",texto,context.path.c_str())){context.error=Error::Save;return;}
                nrprescale::post_chave.store(nrprescale::modo2_chave(novo));nrprescale::post_menu_escreveu.store(true);
                if(!cache(context))return;
            }else{

                const unsigned linhas=nrprescale::modo2_linhas(novo);wchar_t texto[8]{};std::swprintf(texto,8,L"%u",linhas);
                if(!NRCFG_ESCREVER(L"DlssNrOnAmd",L"NeuralPreScaleAuto",texto,context.path.c_str())){context.error=Error::Save;return;}
                nrprescale::auto_alvo.store(nrprescale::auto_alvo_de(linhas,0));nrprescale::auto_menu_escreveu.store(true);
                {auto& c=ini_cache();if(c.lido)c.atual[ini_prescale_auto]=texto;}
            }
            auto pedido=state.requested;pedido.pre_upscale=nrprescale::modo2_pre(novo);
            if(pedido!=state.requested)record_operation(context,context.store.request(pedido),Error::Save);
            (void)cache(context);return;
        }
        if(control==idioma_row){
            if(delta<-1||delta>1)return;
            const unsigned n=nrmenu::language_count,current=unsigned(nrmenu::language());
            const auto next=nrmenu::Language((current+(delta<0?n-1:1))%n);
            wchar_t code[8]{};for(unsigned i=0;i<7&&nrmenu::language_code(next)[i];++i)code[i]=wchar_t(nrmenu::language_code(next)[i]);
            if(!NRCFG_ESCREVER(L"DlssNrOnAmd",L"MenuLanguage",code,context.path.c_str()))return;
            nrmenu::set_language(next);return;
        }
        if(control==entrada_row){
            if(delta<-1||delta>1)return;
            const bool everything=nrmenu::input_block()!=nrmenu::InputBlock::Everything;
            if(!NRCFG_ESCREVER(L"DlssNrOnAmd",L"MenuInputBlock",everything?L"1":L"0",context.path.c_str()))return;
            nrmenu::set_input_block(everything?nrmenu::InputBlock::Everything:nrmenu::InputBlock::MenuKeys);return;
        }
        if(control==bilateral_row){
            if(!context.reproject_controls||delta<-1||delta>1)return;
            const unsigned atual=nrreproject::index_of(*context.reproject_controls,nrreproject::BilateralStrength),n=4;

            unsigned pos=0;for(unsigned k=0;k<n;++k)if(bilateral_niveis[k]<=atual)pos=k;
            const unsigned proximo=bilateral_niveis[(pos+(delta<0?n-1:1))%n];
            wchar_t text[32]{};std::swprintf(text,32,L"%.2f",double(nrreproject::bilateral_strength_list[proximo]));
            if(!NRCFG_ESCREVER(L"DlssNrOnAmd",reproject_keys[nrreproject::BilateralStrength],text,context.path.c_str())){context.reproject_error=Error::Save;return;}
            context.reproject_error=nrreproject::set(*context.reproject_controls,nrreproject::BilateralStrength,proximo)?Error::None:Error::Input;return;
        }
        if(control==despejo_row){if(delta<-1||delta>1)return;(void)nrreproject::pedir_gravacao(despejo_quadros,GetTickCount64());return;}
        if(control==print_row){if(delta<-1||delta>1)return;nrprint::pedir();return;}
        if(control>=reproject_first_row&&control<=reproject_last_row){
            if(!context.reproject_controls||delta<-1||delta>1)return;
            const unsigned which=control-13,total=nrreproject::counts[which];
            const unsigned current=nrreproject::index_of(*context.reproject_controls,which);

            unsigned next=(current+(delta<0?total-1:1))%total;

            if(which==nrreproject::LightGuard&&next==0)next=delta<0?total-1:1;
            wchar_t text[32]{};
            std::swprintf(text,32,L"%.*f",int(reproject_decimals(which)),double(nrreproject::lists[which][next]));
            if(!NRCFG_ESCREVER(L"DlssNrOnAmd",reproject_keys[which],text,context.path.c_str())){context.reproject_error=Error::Save;return;}
            context.reproject_error=nrreproject::set(*context.reproject_controls,which,next)?Error::None:Error::Input;return;
        }
        if(control>=8&&control<=10){
            if(!context.compose_controls||delta<-1||delta>1||(context.present_mode&&nrpresentmode::active()))return;
            const auto packed=context.compose_controls->requested.load();if(!nrcompose::valid(packed))return;
            const unsigned current=component(packed,control),lo=control==10?20u:0u,hi=control==8?40u:control==9?80u:160u;
            const unsigned next=delta<0?(current>lo?current-1:lo):(current<hi?current+1:hi);
            wchar_t text[32]{};std::swprintf(text,32,L"%.2f",next/20.);
            if(!NRCFG_ESCREVER(L"DlssNrOnAmd",compose_keys[control-8],text,context.path.c_str())){context.compose_error=Error::Save;return;}
            context.compose_error=nrcompose::set(*context.compose_controls,control-8,next)?Error::None:Error::Input;return;
        }
        if(control==7) {
            if(!context.present_mode || delta<-1 || delta>1)return;
            const unsigned next=nrpresentmode::requested.load() ? 0u : 1u;
            if(!NRCFG_ESCREVER(L"DlssNrOnAmd",L"PresentMode",next ? L"1" : L"0",context.path.c_str())){
                context.present_save_error=true;return;
            }
            nrpresentmode::requested.store(next,std::memory_order_release);context.present_save_error=false;return;
        }
        if(control==0 && context.present_mode && nrpresentmode::active())return;
        if(control==6) {
            if(!context.tone_state || delta<-1 || delta>1)return;
            const unsigned request=context.tone_state->requested.load();if(request>9)return;
            const unsigned next=(request+(delta<0 ? 9u : 1u))%10;
            const wchar_t text[]={wchar_t(L'0'+next),0};
            if(!NRCFG_ESCREVER(L"DlssNrOnAmd",L"NeuralToneConfig",text,context.path.c_str())){context.tone_error=Error::Save;return;}
            context.tone_state->requested.store(next,std::memory_order_release);context.tone_error=Error::None;
            {wchar_t lt[32]{};NRCFG_STR(L"DlssNrOnAmd",L"LocalTone",L"0",lt,32,context.path.c_str());
             nrreproject::grade_tom.store(nrreproject::tom_de_config(next,float(std::wcstod(lt,nullptr))));}
            preset_invalidar_reavaliar(context);return;
        }
        if(control==4 || control==5) {
            if(delta<-1 || delta>1 || !valid_multipass(context))return;
            unsigned requested=scalar(context.multipass_requested);
            if(!valid_pass_config(requested) || !valid_pass_config(scalar(context.multipass_effective)))return;
            const unsigned passes=requested&255u,index=requested>>8;

            unsigned next=control==4 ? (requested&0xff00u)|(passes>2u ? (delta<0 ? 2u : 1u) : passes%2u+1u) : passes|(((index+(delta<0 ? 6u : 1u))%7)<<8);

            if(control==4){
                context.multipass_tonecurve=tonecurve_ativo(context.path);
                if(context.multipass_tonecurve){
                    next=(requested&0xff00u)|1u;
                    if(next==requested){context.multipass_error=Error::None;return;}
                }
            }
            wchar_t selected[16]{};std::swprintf(selected,16,L"%u",next);
            if(!NRCFG_ESCREVER(L"DlssNrOnAmd",L"NeuralPassConfig",selected,context.path.c_str())) {context.multipass_error=Error::Save;return;}
            if(!valid_multipass(context))return;

            const auto prior=InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(context.multipass_requested),next,requested);
            context.multipass_error=static_cast<unsigned>(prior)==requested ? Error::None : Error::Input;
            preset_invalidar_reavaliar(context);return;
        }
        if(control==3) {
            if(delta<-1 || delta>1 || !valid_style(context))return;
            const LONG requested=scalar(context.style_requested),effective=scalar(context.style_effective);
            if(requested<0 || requested>2 || effective<0 || effective>2)return;
            const LONG next=(requested+(delta<0 ? 2 : 1))%3;
            const wchar_t selected[]={static_cast<wchar_t>(L'0'+next),0};
            if(!NRCFG_ESCREVER(L"DlssNrOnAmd",L"NeuralStyle",selected,context.path.c_str())) {context.style_error=Error::Save;return;}
            if(!valid_style(context))return;
            InterlockedExchange(reinterpret_cast<volatile LONG*>(context.style_requested),next);
            nrreproject::grade_estilo.store(unsigned(next));nrreproject::grade_semeado.store(true);
            carregar_grade_do_estilo(context,unsigned(next));
            context.style_error=Error::None;
            preset_invalidar_reavaliar(context);return;
        }
        if (control==2) {
            if(delta<-1 || delta>1 || !valid_mask(context.auto_mask))return;
            const auto native=InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(context.auto_mask),0,0);
            if(native<0 || native>1)return;
            const LONG next=native ? 0 : 1;

            if(!NRCFG_ESCREVER(L"DlssNrOnAmd",L"UseAutoMask",next ? L"1" : L"0",context.path.c_str())) {
                context.mask_save_error=true;return;
            }
            if(!valid_mask(context.auto_mask))return;
            InterlockedExchange(reinterpret_cast<volatile LONG*>(context.auto_mask),next);
            context.mask_save_error=false;
            preset_invalidar_reavaliar(context);return;
        }
        if (control>1 || delta<-1 || delta>1) { context.error=Error::Input; return; }
        if (!cache(context)) return;
        const auto key=delta==-1 ? nr::Key::Left : delta==1 ? nr::Key::Right : nr::Key::Enter;
        auto result=context.store.control(control==0 ? nr::Control::PreUpscale : nr::Control::Scale,key);
        record_operation(context,result,Error::Save);
        (void)cache(context);
    } catch (...) { context.error=Error::Exception; }
}
bool label(const nrmenu::NativeRow& row,const char* expected) noexcept {
    const auto length=std::strlen(expected)+1;
    return accessible(row.label,length,Access::Read) && std::memcmp(row.label,expected,length)==0;
}
bool original_shape(const NativeVector& rows,std::size_t count) noexcept {
    if (count!=5 && count!=6) return false;
    if (!label(rows.begin[0],"Enabled") || !label(rows.begin[1],"Mode") ||
        !label(rows.begin[2],"Structure intensity") || !label(rows.begin[3],"Skin structure")) return false;
    if (count==6 && !label(rows.begin[4],"Tone intensity")) return false;
    return label(rows.begin[count-1],"Dump next frame");
}
struct LocalRows {

    nrmenu::NativeRow rows[last_row+1]{};
    void initialise(nrmenu::MenuCallbacks callbacks,bool present_mode=false) noexcept {
        nrmenu::NativeRow pair[2]{};nrmenu::initialise_rows(pair,callbacks);
        nrmenu::move_row(rows[0],pair[0]);nrmenu::move_row(rows[1],pair[1]);
        nrmenu::initialise_row(rows[7],callbacks,7,"Present (reiniciar)","Experimental em SDR: processa a imagem final, incluindo a interface, sem histórico temporal. Reinicie o jogo para aplicar. Quando ativo, substitui PRE/POST.");
        nrmenu::initialise_row(rows[2],callbacks,2,"Máscara automática","Ajusta automaticamente a estrutura geral e da pele. Desligar pode tornar o efeito neural quase invisível.");
        nrmenu::initialise_row(rows[3],callbacks,3,"Estilo neural","Condicionamento neural experimental: Padrão, Natural ou Cinemático. Pendente aguarda o próximo processamento neural.");
        nrmenu::initialise_row(rows[4],callbacks,4,"Passos neurais","Executa a rede 1, 2 ou 3 vezes por processamento. Pendente aguarda uma execução neural concluída. Com Curva da entrada neural diferente de Legado, só 1 passo funciona: esta linha fica em 1 e mostra o motivo.");
        nrmenu::initialise_row(rows[6],callbacks,6,"Tom local",present_mode ? "Original preserva os ajustes nativos. De 0 a 2 controla tom e estrutura diretamente. Present não usa histórico; aplica no próximo processamento." : "Original preserva os ajustes nativos. De 0 a 2 controla tom e estrutura diretamente. Aplica no processamento neural e reinicia o histórico.");
        nrmenu::initialise_row(rows[5],callbacks,5,present_mode ? "Intensidade neural" : "Força dos passos extras",present_mode ? "Em Present, ajusta todos os passos; em PRE/POST, somente os passos extras. Zero desativa Present." : "Multiplica a intensidade dos passos após o primeiro: 0, 0.5, 0.75, 1, 1.25, 1.5 ou 2.");
        nrmenu::initialise_row(rows[8],callbacks,8,"Intensidade dos detalhes","Composição final PRE/POST, incluindo o primeiro passo. De 0 a 2; 1 preserva o padrão. Acima de 1 amplia a transferência limitada de luz. Independente do estilo.");
        nrmenu::initialise_row(rows[9],callbacks,9,"Intensidade das cores","Transferência neural de cores PRE/POST. 0 preserva o matiz original; 1 transfere integralmente; acima de 1 intensifica as cores. De 0 a 4. Não altera o estilo.");
        nrmenu::initialise_row(rows[13],callbacks,13,"Limite de vizinhança","Limita a correção do quadro anterior ao intervalo de cores dos pixels vizinhos do quadro atual. Tira cintilação e rastro; quanto maior, menos correção sobra. Desligado mantém o comportamento de hoje.");
        nrmenu::initialise_row(rows[14],callbacks,14,"Rejeição por movimento","Descarta a correção onde os vetores de movimento discordam dos vizinhos, que é a borda de quem se move — onde o rastro aparece. Menor é mais agressivo, mas a escala mudou quando a medida passou a comparar com a vizinhança: varra a lista inteira em vez de confiar num valor antigo. Desligado mantém o comportamento de hoje.");
        nrmenu::initialise_row(rows[15],callbacks,15,"Tolerância de profundidade","Quanto a profundidade pode diferir para o pixel do quadro anterior ainda valer. Menor descarta mais e reduz rastro; maior preenche mais área. Padrão 0.030.");
        nrmenu::initialise_row(rows[16],callbacks,16,"Tolerância de cor","Quanto a cor atual pode diferir da cor reprojetada antes de a correção ser descartada. Maior mantém a correção em rostos e roupas que se deformam; menor evita rastro. Padrão 0.15.");
        nrmenu::initialise_row(rows[18],callbacks,18,"Escala do movimento","Multiplica o vetor de movimento usado para seguir a correção até o quadro atual. Se o rastro cresce quando você aumenta os passos neurais, a escala ou o sinal estão errados: experimente -1.00, 0.50 e 2.00. 1.00 é o comportamento de hoje.");
        nrmenu::initialise_row(rows[19],callbacks,19,"Tolerância a deformação","Descarta a correção onde o campo de movimento estica ou cisalha, que é cabelo, tecido e membros articulados — não parede nem chão. Em pixels de deformação acumulada; menor descarta mais. Medido em 22/09 como PIOR que não usar: acrescenta borda própria na silhueta. Aqui para ser testado, não recomendado. Desligado mantém o comportamento de hoje.");
        nrmenu::initialise_row(rows[20],callbacks,20,"Detalhe em movimento","Mantém o tom e a cor da correção sempre, e desvanece só o detalhe fino onde a cena se move — que é a parte que vira borda dobrada quando chega fora de registro. Em pixels por quadro: menor descarta detalhe mais cedo. Desligado mantém o comportamento de hoje.");
        nrmenu::initialise_row(rows[21],callbacks,21,"Teste de desoclusão","Segue o vetor até o quadro anterior e volta pelo vetor encontrado lá: numa correspondência válida a volta cai no pixel de origem. Onde a personagem desocupou, não cai — e não cai na região inteira, ao contrário da rejeição por movimento, que só pega a orla e deixa o miolo em filamentos. Em pixels de erro de fechamento; menor descarta mais. Desligado mantém o comportamento de hoje.");
        nrmenu::initialise_row(rows[17],callbacks,17,"Suavizar rejeição","Limita o quanto a confiança pode mudar de um quadro para o outro. Tira a cintilação que a rejeição por movimento cria, ao custo de o rastro demorar um ou dois quadros a mais para sumir. Menor suaviza mais. Desligado mantém o comportamento de hoje.");

        nrmenu::initialise_row(rows[22],callbacks,22,"Transferência do realce","Recalcula o realce da rede com a cor do quadro atual, numa grade local, em vez de arrastar o realce velho para onde ela estava. Reduz rastro e aura. 1 = inteira. Desligado mantém o comportamento antigo.");
        nrmenu::initialise_row(rows[23],callbacks,23,"Tolerância do realce","Quanto o realce arrastado pode se afastar do modelo recalculado. Menor corta mais rastro e suaviza um pouco as bordas; maior devolve borda e deixa mais rastro. Recomendado 0.020.");
        nrmenu::initialise_row(rows[24],callbacks,24,"Separação da grade","Como a grade separa a personagem do fundo. Luminância separa por brilho; Profundidade separa pela distância, mesmo com a mesma cor: cabelo escuro contra fundo escuro, onde aparece a aura.");
        nrmenu::initialise_row(rows[25],callbacks,25,"Retenção do modelo","Quando a rede atrasa mais de 16 quadros (Balanceado ou Qualidade a 4K), mantém o modelo da última grade em vez de tirar o efeito do quadro: tira o liga e desliga. Em quadros. Desligado mantém o comportamento antigo.");
        nrmenu::initialise_row(rows[26],callbacks,26,"Regularização","Estabilidade do ajuste da grade. Maior deixa o modelo mais simples e estável; menor segue mais de perto o realce da rede. Padrão 0.0010.");
        nrmenu::initialise_row(rows[27],callbacks,27,"Movimento pontual","Usa o vetor de movimento exato do pixel, sem interpolar os vizinhos. Desligado mantém o comportamento antigo.");
        nrmenu::initialise_row(rows[28],callbacks,28,"Movimento duplo","Na desoclusão, busca o realce pelo vetor de quem cobria o fundo, onde o mesmo fundo estava visível. O número é o raio da busca. Desligado mantém o comportamento antigo.");
        nrmenu::initialise_row(rows[29],callbacks,29,"Limiar do mov. duplo","Em pixels: o movimento duplo só entra onde o salto do vetor passa deste valor, para não confundir desoclusão com giro de câmera. Sem limiar = sempre.");
        nrmenu::initialise_row(rows[30],callbacks,30,"Piso do realce","Atenua o realce arrastado abaixo desta magnitude. Desligado mantém o comportamento antigo.");
        nrmenu::initialise_row(rows[31],callbacks,31,"Piso da confiança","Menor confiança que as rejeições podem publicar. Desligado mantém o comportamento antigo.");
        nrmenu::initialise_row(rows[32],callbacks,32,"Suavizar portão","Raio, em pixels, da suavização espacial das rejeições. Desligado mantém o comportamento antigo.");
        nrmenu::initialise_row(rows[33],callbacks,33,"Suavizar deformação","Raio, em pixels, da suavização da medida de deformação. Desligado mantém o comportamento antigo.");
        nrmenu::initialise_row(rows[34],callbacks,34,"Referência de movimento","Movimento, em fração da tela por quadro, em que o limite de vizinhança chega à força total. Padrão 0.010.");
        nrmenu::initialise_row(rows[35],callbacks,35,"Tolerância absoluta","Diferença de profundidade sempre aceita no teste do quadro anterior, somada à tolerância relativa. Padrão 0.0001.");
        nrmenu::initialise_row(rows[36],callbacks,36,"Idade máxima","Encurta o rastro do modo assíncrono: quando o resultado da rede ficaria mais velho que este número de quadros, o quadro espera o resultado novo. Menor = menos rastro e menos FPS. Desligado = sem espera.");
        nrmenu::initialise_row(rows[37],callbacks,37,"Compor o realce","Onde o realce do modo assíncrono é somado. Antes do FSR é o comportamento de até hoje. Depois do FSR amplia o realce e o soma na imagem final: o realce atrasado não entra no histórico do FSR, que é o que deixa cópias atrás da personagem. Só vale no Assíncrono; muda no quadro seguinte.");
        nrmenu::initialise_row(rows[38],callbacks,38,"Preencher o buraco","Como preencher a parede que ela acabou de descobrir, onde o realce antigo não vale. Média é o comportamento de até hoje e deixa o contorno dela atrás. Borda suave mistura a transição em volta do buraco e tira esse contorno. Só vale no Assíncrono com preenchimento ligado; muda no quadro seguinte.");
        nrmenu::initialise_row(rows[39],callbacks,39,"Borda em movimento","Remove as cópias do personagem pintadas na parede atrás dele: rejeita o realce antigo onde a borda dele passou em algum quadro intermediário. Desligado é o comportamento de até hoje; 2 px pega mais. Muda no quadro seguinte.");
        nrmenu::initialise_row(rows[40],callbacks,40,"Reparar movimento","Remove as cópias do contorno atrás do personagem: o jogo desenha o movimento dele 2-3 px além da silhueta, e a parede colada nele herda esse movimento. Perto das bordas, usa o movimento da própria superfície. Desligado é o comportamento de até hoje; muda no quadro seguinte.");
        nrmenu::initialise_row(rows[41],callbacks,41,"Divergência bruta","Remove as cópias do contorno atrás do personagem quando a câmera o acompanha: descarta o realce antigo onde o pixel tem movimento longo mas chega num lugar que estava parado. Forte age em movimento lento. Desligado é o comportamento de até hoje; muda no quadro seguinte.");
        nrmenu::initialise_row(rows[42],callbacks,42,"Estabilizar realce","Tira o flicker do cenário no Assíncrono: mistura o realce de cada quadro com o do quadro anterior, limitado ao que existe em volta agora (não cria rastro). Forte estabiliza mais e muda mais devagar. Desligado é o comportamento de até hoje; muda no quadro seguinte.");
        nrmenu::initialise_row(rows[43],callbacks,43,"Exposição (EV)","Exposição (EV) da imagem realçada pela rede. Positivo clareia, negativo escurece. Ao trocar o estilo, recebe o valor da NVIDIA (Natural -0,10 EV). Só no Assíncrono, no PRE e no POST; muda no quadro seguinte.");
        nrmenu::initialise_row(rows[44],callbacks,44,"Contraste","Contraste da imagem realçada pela rede. Positivo aumenta, negativo suaviza. Ao trocar o estilo, recebe o valor da NVIDIA (Natural -25%). Só no Assíncrono, no PRE e no POST; muda no quadro seguinte.");
        nrmenu::initialise_row(rows[45],callbacks,45,"Saturação","Saturação das cores da imagem realçada pela rede. Positivo aviva, negativo tira cor. Ao trocar o estilo, recebe o valor da NVIDIA (Natural -10%, Cinemático -15%). Só no Assíncrono, no PRE e no POST; muda no quadro seguinte.");
        nrmenu::initialise_row(rows[46],callbacks,46,"Força em movimento","Deixa usar a Força do realce máxima da NVIDIA (0,03125): parado o realce sai inteiro; onde a imagem anda ele cai suavemente para esta fração, que é onde aparecem o flicker e o rastro. 50% em movimento equivale a 0,0156. Desligado = força inteira sempre. Só no Assíncrono; muda no quadro seguinte.");
        nrmenu::initialise_row(rows[51],callbacks,51,"Transferir por razão","Tira o flicker que a Proteção de realces acima de 1,2 causa no Assíncrono: aplica ao quadro atual a mesma razão de brilho que a rede calculou no quadro dela, em vez de somar o realce antigo sobre um pixel que mudou. Permite Proteção 2 e Detalhes 1,5 sem cintilar. Muda no quadro seguinte.");
        nrmenu::initialise_row(rows[52],callbacks,52,"Limitar variação do ganho","Tira o flicker em movimento da Proteção de realces alta no Assíncrono: o ganho de cada ponto só pode mudar esta porcentagem por quadro em relação ao quadro anterior, seguindo o movimento. Onde a cena acabou de aparecer (ou fora da tela) o ganho novo passa direto. Menor tira mais flicker e demora mais para seguir o realce novo. Só na composição antes do FSR; muda no quadro seguinte.");
        nrmenu::initialise_row(rows[53],callbacks,53,"Proteção suave","Troca o corte seco da Proteção de realces por uma curva: o ganho se aproxima do limite devagar em vez de bater nele e soltar. Mesmo limite máximo; um pouco menos de realce perto dele. Só no Assíncrono; muda no quadro seguinte.");

        nrmenu::initialise_row(rows[54],callbacks,54,"Realce da luz","Até quanto a rede pode clarear ou escurecer a luz larga da cena. Maior = mais contraste e noção de profundidade, como o mod do Daniel; Sem limite = sem teto. Pode piscar mais em movimento. No PRE limita só a luz larga; no POST, que tem um teto só, ela muda esse teto junto com o Realce do detalhe (3,0 e 1,3 = o teto de hoje; Sem limite nos dois = sem teto). Vale na hora.");
        nrmenu::initialise_row(rows[55],callbacks,55,"Realce do detalhe","Até quanto a rede pode reforçar o detalhe fino (bordas, texturas, rugas). 1,3 = padrão, sem flicker; maior = detalhe mais forte e mais contraste, como o mod do Daniel; Sem limite = sem teto. Acima de 2 pode piscar em movimento. Vale com qualquer Realce da luz: no PRE limita só o detalhe; no POST muda o teto único junto com o Realce da luz. Vale na hora.");
        nrmenu::initialise_row(rows[56],callbacks,56,"Suavizar iluminação","Com a Proteção da iluminação ligada: mistura a iluminação de cada quadro com a do quadro anterior, seguindo o movimento, e tira o salto de luz na troca do resultado da rede. Onde a imagem do jogo mudou passa a luz nova na hora. Forte suaviza mais e segue mais devagar o que a rede muda sozinha. Muda no quadro seguinte.");
        nrmenu::initialise_row(rows[57],callbacks,57,"Reduzir halo","Tira a aura clara em volta dos personagens, sobretudo em cima da cabeça: a rede clareia uma faixa fina do fundo colada na silhueta, e a composição aumenta isso no fundo escuro. Aqui o clareamento dessa faixa (até 4, 6 ou 8 px do contorno, pela profundidade) é reduzido pela metade (Leve), 3/4 (Médio) ou todo (Forte). Não mexe no personagem nem no escurecimento. Só no Assíncrono, antes do FSR; muda no quadro seguinte.");
        nrmenu::initialise_row(rows[58],callbacks,58,"Realce pela profundidade","Tira o realce do cabelo que vaza para o fundo na silhueta (rastro e halo colados na borda): ao buscar o realce do resultado anterior, só mistura pontos da mesma superfície deste pixel, pela profundidade. Vale também para o Detalhe em movimento. Desligado = a leitura de hoje. Só no Assíncrono; muda no quadro seguinte.");
        nrmenu::initialise_row(rows[59],callbacks,59,"Piso da razão","Tira flicker nas sombras: a composição multiplica o brilho pela razão entre a imagem com e sem o realce, e nos pontos muito escuros essa razão salta com qualquer mudança do realce. Um piso maior deixa o realce quase somado nos escuros, sem mudar os meios-tons; pode tirar um pouco de realce das sombras. Padrão = o de hoje. Muda no quadro seguinte.");
        nrmenu::initialise_row(rows[60],callbacks,60,"Composição tardia","Tira ~1 quadro de atraso do realce no Assíncrono: enquanto a rede ainda calcula o resultado novo, o quadro prepara as duas composições (a de agora e a do resultado novo) e a placa escolhe na hora de desenhar a que já estiver pronta. Menos rastro atrás de quem se move; o halo pode subir um pouco (use junto com Reduzir halo). Se o estado das duas composições divergir ela se desliga sozinha até reabrir o jogo. Desligado = o de hoje. Vale a partir do próximo resultado da rede.");
        nrmenu::initialise_row(rows[61],callbacks,61,"Reduzir halo pela rede","Com a Rede neural abaixo de 100%, a aura da rede fica mais larga na tela (a 50%, o dobro): Ligado alarga a faixa do Reduzir halo na mesma proporção (Médio: 6 px viram 12 px a 50%; no máximo 16 px). Com a rede a 100% não muda nada. Só vale com o Reduzir halo ligado; muda no quadro seguinte.");
        nrmenu::initialise_row(rows[63],callbacks,63,"Halo em movimento","Tira a aura clara em volta de quem se move (cabeça, ombros), sem tocar em nada parado: onde o fundo encosta num personagem ou objeto que anda, a iluminação que a rede deu a esse fundo só pode passar um pouco da do fundo vizinho da mesma parede, e nunca fica abaixo dela. Câmera e objeto parados: nenhum ponto muda. Leve, Médio ou Forte = quanto do excesso sai. Só com a Proteção da iluminação ligada; muda no quadro seguinte.");
        nrmenu::initialise_row(rows[64],callbacks,64,"Borda acompanha o movimento","Tira a faixa clara e a cópia do contorno que ficam para trás de quem se move no Assíncrono: a faixa fina do fundo colada na silhueta passa a andar junto com o personagem, em vez de ficar presa na parede. Parado não muda nada. Vale para os quadros capturados daqui em diante (menos de 1 segundo).");
        nrmenu::initialise_row(rows[65],callbacks,65,"Cortar ao redor em movimento","Modo agressivo contra halo, ghost e flicker em volta de quem se move no Assíncrono: tira o realce da rede do fundo em volta do personagem (a faixa colada nele e o rastro deixado para trás) e põe ali o realce do fundo vizinho, nunca mais escuro que ele. Nada parado muda: câmera e objeto parados ficam exatamente iguais. Forte mantém a borda que a Borda acompanha o movimento já pôs no lugar; Máximo corta também essa. A faixa colada só com a Proteção da iluminação ligada. Muda no quadro seguinte.");
        nrmenu::initialise_row(rows[62],callbacks,62,"Iluminação pela profundidade","Com a Proteção da iluminação ligada: tira a aura larga e fraca em volta da cabeça e dos ombros. A iluminação que a rede dá ao personagem deixa de vazar para a parede atrás dele, porque a suavização da iluminação só mistura pontos à mesma distância da câmera. Desligado = o de hoje. Muda no quadro seguinte.");
        nrmenu::initialise_row(rows[50],callbacks,50,"Cancelar ruído","Tira o flicker parado das áreas claras no Assíncrono: a rede limpa o ruído de cada quadro, e o realce de um quadro somado a outro devolve esse ruído. Nos pontos parados em que a cor só mudou no nível de ruído, mostra a saída limpa da rede. Mais forte aceita diferenças maiores como ruído. Muda no quadro seguinte.");
        nrmenu::initialise_row(rows[49],callbacks,49,"Estabilizar só parado","Limita o Estabilizar realce aos pontos parados: acima deste movimento, em pixels por quadro, o realce passa direto (sem rastro). Sempre = estabiliza tudo, como antes. Muda no quadro seguinte.");
        nrmenu::initialise_row(rows[48],callbacks,48,"Compensar jitter","Tira o flicker parado do Assíncrono: o realce de cada resultado foi calculado sobre um quadro com um jitter do FSR e é somado em quadros com outro jitter, desalinhado até 1 px. Ligado desloca o realce pela diferença; se piorar, teste Invertido. Muda no quadro seguinte.");
        nrmenu::initialise_row(rows[47],callbacks,47,"Movimento da força mínima","Quanto a imagem precisa andar, em pixels por quadro, para a força chegar à fração de movimento. Menor corta mais cedo (menos flicker ao andar devagar); maior mantém mais força em movimento lento. Muda no quadro seguinte.");
        for(unsigned r=0;r<ini_count;++r)nrmenu::initialise_row(rows[ini_first_row+r],callbacks,ini_first_row+r,ini_rows[r].label,ini_rows[r].help);
        nrmenu::initialise_row(rows[despejo_row],callbacks,despejo_row,"Gravar 24 quadros","Grava 24 quadros seguidos da imagem antes e depois do FSR (cerca de 1,8 GB na pasta do jogo), sem travar o jogo, para analisar ghost e flicker. Para ghost: acione e ande ou gire a câmera logo em seguida. Para flicker: fique parado. Não muda a imagem.");
        nrmenu::initialise_row(rows[print_row],callbacks,print_row,"Tirar print","Salva a imagem da tela como PNG em nr_prints, na pasta do jogo, sem este menu e sem o texto do mod (o HUD do jogo fica). Atalho: F11, sem abrir o menu. Não trava o jogo.");
        nrmenu::initialise_row(rows[idioma_row],callbacks,idioma_row,nrmenu::language_label,nrmenu::language_help);
        nrmenu::initialise_row(rows[entrada_row],callbacks,entrada_row,nrmenu::input_label,nrmenu::input_help);
        nrmenu::initialise_row(rows[bilateral_row],callbacks,bilateral_row,"Transferência bilateral","Recalcula o realce da rede com a cor do quadro atual, numa grade local, em vez de arrastar o realce velho para onde a pessoa estava: menos rastro e menos flicker no Assíncrono (no Hangar, na cadência do jogo, cerca de 37% menos rastro e 40% menos flicker com Forte). Funciona junto com a Composição tardia. Leve = 0,50, Média = 0,75, Forte = 1,00 (NeuralReprojectBilateral). Desligada = o de hoje. Vale na hora.");
        nrmenu::initialise_row(rows[preset_row],callbacks,preset_row,"Preset","Preset do modelo, como os Model A/B/C do mod da RenoDX: A = estilo Padrão, B = Natural, C = Cinemático, todos com estrutura 0,88, pele 1,00, máscara automática, tom local 1,00 e 2 passos neurais. Estilo, estrutura, pele, máscara, tom e passos valem na hora; LocalTone e ToneChannels do INI (reiniciar). Personalizado não mexe em nada e aparece quando os valores não batem com nenhum preset. Com Curva da entrada neural diferente de Legado, os passos ficam como já estavam (nunca 2, que quebraria o próximo processamento).");
        nrmenu::initialise_row(rows[modo_rede_row],callbacks,modo_rede_row,"Modo da rede","Onde a rede neural roda. PRE: antes do FSR, na resolução interna (o padrão). POST: depois do FSR, sobre a imagem já reconstruída, em 33, 50, 75 ou 100% da resolução de saída: mais detalhe de cenário, rosto e textura, com a rede mais cara (a 1440p, POST 75% leva cerca de 3 vezes o tempo do PRE). Quando a rede não cabe na janela do Assíncrono ou na memória de vídeo livre, a % é limitada sozinha: entre parênteses, a % em que a rede rodou no último processamento. Vale na hora.");
        nrmenu::initialise_row(rows[10],callbacks,10,"Proteção de realces","Limite da proporção de luminosidade PRE/POST, de 1 a 8. Padrão 2. Detalhes 1, cores 1 e proteção 2 preservam exatamente a composição anterior.");
    }
    ~LocalRows() { for(auto& row:rows)nrmenu::destroy_row(row); }
};
bool same_bridge(const nrmenu::NativeRow& row,const nrmenu::NativeRow& probe) noexcept {

    return row.label==probe.label && row.getter==row.getter_storage && row.action==row.action_storage &&
           std::memcmp(row.getter_storage,probe.getter_storage,sizeof(void*))==0 &&
           std::memcmp(row.action_storage,probe.action_storage,sizeof(void*))==0;
}
bool same_context(const nrmenu::NativeRow& row,const NativeHost& host) noexcept {

    nrmenu::MenuCallbacks getter{},action{};
    std::memcpy(&getter,row.getter_storage+8,sizeof(getter));
    std::memcpy(&action,row.action_storage+8,sizeof(action));
    if (getter.read!=read || getter.act!=act || action.read!=read || action.act!=act || getter.context!=action.context) return false;
    for (auto context=contexts;context;context=context->next) {
        if (context==getter.context) return context->base==host.base && context->rows==host.rows && context->path==host.selected_ini && context->auto_mask==host.auto_mask && context->style_requested==host.style_requested && context->style_effective==host.style_effective && context->multipass_requested==host.multipass_requested && context->multipass_effective==host.multipass_effective && context->tone_state==host.tone_state && context->present_mode==host.present_mode && context->compose_controls==host.compose_controls && context->reproject_controls==host.reproject_controls;
    }
    return false;
}
AppendResult append_menu_admitted(const NativeHost& host) noexcept {
    const auto thread=GetCurrentThreadId();
    try {
        std::size_t count=0;
        if (!host.base || !host.grow || !accessible(reinterpret_cast<void*>(host.grow),1,Access::Execute) ||
            !valid_vector(host.rows,count) || !valid_path(host.selected_ini)) return AppendResult::InvalidHost;
        if (!host.ready) return AppendResult::NotReady;
        if(host.auto_mask && !valid_mask(host.auto_mask))return AppendResult::InvalidHost;
        if(host.compose_controls && (reinterpret_cast<std::uintptr_t>(host.compose_controls)%alignof(nrcompose::State) ||
            !accessible(host.compose_controls,sizeof(nrcompose::State),Access::Read) || !accessible(host.compose_controls,sizeof(nrcompose::State),Access::Write)))return AppendResult::InvalidHost;
        if(host.reproject_controls && (reinterpret_cast<std::uintptr_t>(host.reproject_controls)%alignof(nrreproject::Controls) ||
            !accessible(host.reproject_controls,sizeof(nrreproject::Controls),Access::Read) || !accessible(host.reproject_controls,sizeof(nrreproject::Controls),Access::Write)))return AppendResult::InvalidHost;
        if(host.tone_state && (reinterpret_cast<std::uintptr_t>(host.tone_state)%alignof(nrtone::State) ||
            !accessible(host.tone_state,sizeof(nrtone::State),Access::Read) || !accessible(host.tone_state,sizeof(nrtone::State),Access::Write)))return AppendResult::InvalidHost;
        LocalRows probe; probe.initialise({nullptr,nullptr,nullptr},host.present_mode);
        if((host.style_requested || host.style_effective) && (!valid_mask(host.style_requested) || !valid_mask(host.style_effective) || host.style_requested==host.style_effective))return AppendResult::InvalidHost;
        if((host.multipass_requested || host.multipass_effective) && (!valid_mask(host.multipass_requested) || !valid_mask(host.multipass_effective) || host.multipass_requested==host.multipass_effective))return AppendResult::InvalidHost;
        unsigned indices[last_row+1]={0,1,2,3,4,5,6,7,8,9,10,13,14,15,16,17,18,19,20,21};
        unsigned added=host.hide_scale ? 1u : 2u;
        if(host.auto_mask)indices[added++]=2;
        if(host.style_requested)indices[added++]=3;
        if(host.multipass_requested) {indices[added++]=4;indices[added++]=5;}
        if(host.tone_state)indices[added++]=6;
        if(host.present_mode&&host.full_menu)indices[added++]=7;
        if(host.compose_controls){indices[added++]=8;indices[added++]=9;indices[added++]=10;}
        if(host.reproject_controls&&host.full_menu){
            for(unsigned i=reproject_first_row;i<=reproject_last_row;++i)indices[added++]=i;
            for(unsigned i=ini_first_row;i<=ini_last_row;++i)indices[added++]=i;
            indices[added++]=despejo_row;
            indices[added++]=idioma_row;indices[added++]=entrada_row;indices[added++]=modo_rede_row;indices[added++]=preset_row;indices[added++]=bilateral_row;
            indices[added++]=print_row;
        }else if(host.reproject_controls){

            added=0;
            for(auto* e=nrmenu::catalog_begin();e!=nrmenu::catalog_end();++e){
                if(e->native||!e->shown)continue;
                for(unsigned i=0;i<=last_row;++i){
                    if(!probe.rows[i].label||std::strcmp(probe.rows[i].label,e->label)!=0)continue;
                    if(available(i,host))indices[added++]=i;
                    break;
                }
            }
        }
        if(count>=added && original_shape(*host.rows,count-added)) {
            bool bridges=true,contexts_match=true;
            for(unsigned i=0;i<added;++i) {
                const auto& row=host.rows->begin[count-added+i];
                const bool bridge=same_bridge(row,probe.rows[indices[i]]);
                bridges=bridges && bridge;
                contexts_match=contexts_match && bridge && same_context(row,host);
            }
            if(bridges)return contexts_match ? AppendResult::AlreadyPresent : AppendResult::InvalidHost;
        }
        if (!original_shape(*host.rows,count)) return AppendResult::InvalidHost;
        DWORD empty=0;
        if (!owner.compare_exchange_strong(empty,thread,std::memory_order_acq_rel) && empty!=thread) return AppendResult::WrongThread;
        Context* context=contexts;
        while (context && (context->base!=host.base || context->rows!=host.rows || context->path!=host.selected_ini || context->auto_mask!=host.auto_mask || context->style_requested!=host.style_requested || context->style_effective!=host.style_effective || context->multipass_requested!=host.multipass_requested || context->multipass_effective!=host.multipass_effective || context->tone_state!=host.tone_state || context->present_mode!=host.present_mode || context->compose_controls!=host.compose_controls || context->reproject_controls!=host.reproject_controls)) context=context->next;
        if (!context) {
            auto created=std::make_unique<Context>(host);
            created->next=contexts; context=created.release(); contexts=context;
        }
        (void)reload_context(*context);
        initialise_menu_preferences(context->path);
        LocalRows local; local.initialise({context,read,act},host.present_mode);
        unsigned appended=0;
        try {
            for (unsigned i=0;i<added;++i) {
                auto& row=local.rows[indices[i]];
                if (host.rows->end!=host.rows->capacity) {
                    nrmenu::move_row(*host.rows->end,row); ++host.rows->end;
                } else {

                    (void)host.grow(host.rows,host.rows->end,&row);
                }
                ++appended;
            }
        } catch (...) {

            while (appended) { --host.rows->end; nrmenu::destroy_row(*host.rows->end); --appended; }
            return AppendResult::Failed;
        }
        initialise_style(*context);
        initialise_multipass(*context);
        initialise_tone(*context);
        initialise_compose(*context);
        initialise_reproject(*context);
        if(context->present_mode)(void)nrpresentmode::initialize(context->path);
        if(host.hide_scale){
            nrmenu::replace_mode_control(host.rows->begin[1],{context,read,act});
        }
        return AppendResult::Appended;
    } catch (...) { return AppendResult::Failed; }
}
template<class Result,class Work> Result guarded_result(Work&& work,Result wrong_thread,Result busy_result,Result failed_result) noexcept {
    const auto current=owner.load(std::memory_order_acquire);
    const auto thread=GetCurrentThreadId();
    if (current && current!=thread) return wrong_thread;
    Admission admission; if (!admission.admitted) return busy_result;

    const auto admitted_owner=owner.load(std::memory_order_acquire);
    if (admitted_owner && admitted_owner!=thread) return wrong_thread;
    try { return work(); } catch (...) { return failed_result; }
}
template<class Work> AppendResult guarded(Work&& work) noexcept {
    return guarded_result<AppendResult>(std::forward<Work>(work),AppendResult::WrongThread,AppendResult::Busy,AppendResult::Failed);
}
template<class Work> GatewayResult gateway(void* base,Work&& work) noexcept {
    Admission admission;if(!admission.admitted)return {GatewayStatus::Busy,{}};
    try {
        Context* found=nullptr;
        for (auto context=contexts;context;context=context->next) {
            if (context->base!=base) continue;
            if (found) return {GatewayStatus::Ambiguous,{}};
            found=context;
        }
        if (!found) return {GatewayStatus::NotReady,{}};
        return work(*found);
    } catch (...) { return {GatewayStatus::Failed,{}}; }
}
}
AppendResult append_menu(const NativeHost& host) noexcept {
    return guarded([&] { return append_menu_admitted(host); });
}
AppendResult append_from_base(void* base) noexcept {
    return guarded([&] {
        NativeHost host;
        if (!resolve_host(base,host)) return AppendResult::InvalidHost;
#ifdef NR_GAME_PRE_ONLY_MENU
        host.hide_scale=true;
#endif
        return append_menu_admitted(host);
    });
}
GatewayResult runtime_snapshot(void* base) noexcept {
    return gateway(base,[](Context& context) -> GatewayResult { return {GatewayStatus::Ok,context.store.snapshot()}; });
}
GatewayResult runtime_reload(void* base) noexcept {
    return gateway(base,[](Context& context) -> GatewayResult {
        if (!reload_context(context)) return {GatewayStatus::Failed,{}};
        return {GatewayStatus::Ok,context.store.snapshot()};
    });
}
GatewayResult runtime_request_scale(void* base,unsigned percent) noexcept {
    return gateway(base,[&](Context& context) -> GatewayResult {
        auto requested=context.store.snapshot().requested;requested.percent=percent;
        if(!context.store.request(requested).ok)return {GatewayStatus::Failed,context.store.snapshot()};
        return {GatewayStatus::Ok,context.store.snapshot()};
    });
}
GatewayResult runtime_acknowledge(void* base,std::uint64_t generation) noexcept {
    return gateway(base,[&](Context& context) -> GatewayResult {

        GatewayResult response{GatewayStatus::Ok,context.store.snapshot()};
        if (!context.store.commit_if_current(generation)) return {GatewayStatus::Stale,{}};
        response.state->effective=response.state->requested;
        response.state->effective_generation=response.state->generation;
        response.state->readiness=nr::Readiness::Ready;
        return response;
    });
}
GatewayResult runtime_fail(void* base,std::uint64_t generation,const std::string& diagnostic) noexcept {
    return gateway(base,[&](Context& context) -> GatewayResult {
        GatewayResult response{GatewayStatus::Ok,context.store.snapshot()};
        auto& before=*response.state;
        if (before.generation!=generation) return {GatewayStatus::Stale,{}};

        const auto detail=(context.error==Error::Save || context.error==Error::Read) && !before.diagnostic.empty() ?
            before.diagnostic+"; "+diagnostic : diagnostic;
        before.diagnostic=detail;
        before.readiness=nr::Readiness::Failed;
        if (!context.store.fail_if_current(generation,detail)) return {GatewayStatus::Stale,{}};
        if (context.error==Error::None || context.error==Error::Input) context.error=Error::Runtime;
        return response;
    });
}
}
