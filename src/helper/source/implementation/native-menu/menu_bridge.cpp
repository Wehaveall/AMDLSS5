#include "menu_bridge.h"
#include <new>
#include <typeinfo>
#include <cstring>

namespace nrmenu {
namespace {
struct CaptureData {
    MenuCallbacks callbacks;
    unsigned control;
};
struct GetterTag {};
struct ActionTag {};
template<class Result, class Argument> struct Vtable {
    void* (*copy)(void*, void*) noexcept;
    void* (*move)(void*, void*) noexcept;
    Result (*invoke)(void*, Argument) noexcept;
    const std::type_info* (*type)(void*) noexcept;
    void (*destroy)(void*, unsigned char) noexcept;
    void* (*capture)(void*) noexcept;
};
using GetterVtable = Vtable<NativeString*, NativeString*>;
using ActionVtable = Vtable<void, int*>;
template<class Table> struct Callable {
    const Table* vtable;
    CaptureData captured;
};
using GetterObject = Callable<GetterVtable>;
using ActionObject = Callable<ActionVtable>;
static_assert(sizeof(GetterObject) <= 56 && sizeof(ActionObject) <= 56);
static_assert(alignof(GetterObject) == 8 && alignof(ActionObject) == 8);
static_assert(offsetof(GetterObject, captured) == 8 && offsetof(ActionObject, captured) == 8);
static_assert(offsetof(GetterVtable, copy) == 0x00 && offsetof(ActionVtable, copy) == 0x00);
static_assert(offsetof(GetterVtable, move) == 0x08 && offsetof(ActionVtable, move) == 0x08);
static_assert(offsetof(GetterVtable, invoke) == 0x10 && offsetof(ActionVtable, invoke) == 0x10);
static_assert(offsetof(GetterVtable, type) == 0x18 && offsetof(ActionVtable, type) == 0x18);
static_assert(offsetof(GetterVtable, destroy) == 0x20 && offsetof(ActionVtable, destroy) == 0x20);
static_assert(offsetof(GetterVtable, capture) == 0x28 && offsetof(ActionVtable, capture) == 0x28);
static_assert(sizeof(GetterVtable) == 48 && sizeof(ActionVtable) == 48);

template<class Object> void* transfer(void* source, void* destination) noexcept {
    if (source == destination) return destination;

    return ::new (destination) Object(*static_cast<Object*>(source));
}
template<class Tag> const std::type_info* type_identity(void*) noexcept { return &typeid(Tag); }
template<class Object> void destroy(void* source, unsigned char) noexcept {

    static_cast<Object*>(source)->captured = {};
}
template<class Object> void* capture(void* source) noexcept {
    return &static_cast<Object*>(source)->captured;
}
NativeString* read(void* source, NativeString* result) noexcept {
    *result = {};
    const auto& data = static_cast<GetterObject*>(source)->captured;
    if (data.callbacks.read) {
        data.callbacks.read(data.callbacks.context, data.control, result->text, sizeof result->text);
    }
    result->text[15] = '\0';
    while (result->size < 15 && result->text[result->size]) ++result->size;
    result->capacity = 15;
    return result;
}
void act(void* source, int* delta) noexcept {
    const auto& data = static_cast<ActionObject*>(source)->captured;
    if (data.callbacks.act && delta) data.callbacks.act(data.callbacks.context, data.control, *delta);
}
const GetterVtable getter_table = {
    transfer<GetterObject>, transfer<GetterObject>, read, type_identity<GetterTag>,
    destroy<GetterObject>, capture<GetterObject>
};
const ActionVtable action_table = {
    transfer<ActionObject>, transfer<ActionObject>, act, type_identity<ActionTag>,
    destroy<ActionObject>, capture<ActionObject>
};
template<class Object> void move_inline(void*& out, void* storage, void*& source) noexcept {
    out = nullptr;
    if (source) {
        auto* object = static_cast<Object*>(source);
        out = object->vtable->move(source, storage);
        object->vtable->destroy(source, 0);
        source = nullptr;
    }
}
template<class Object> void destroy_inline(void*& source) noexcept {
    if (source) {
        auto* object = static_cast<Object*>(source);
        object->vtable->destroy(source, 0);
        source = nullptr;
    }
}
}
void initialise_row(NativeRow& row, MenuCallbacks callbacks, unsigned control, const char* label, const char* help) noexcept {
    row = {}; row.label=label; row.help=help;
    row.getter=::new (row.getter_storage) GetterObject{&getter_table,{callbacks,control}};
    row.action=::new (row.action_storage) ActionObject{&action_table,{callbacks,control}};
}
void initialise_rows(NativeRow (&rows)[2], MenuCallbacks callbacks) noexcept {
    static constexpr const char* labels[] = {"NR antes do FSR", "Resolução de trabalho"};
    static constexpr const char* help[] = {
        "Processa o NR antes de ampliar a imagem. Ativar NR continua sendo o controle geral.",
        "Escala em relação à janela; o modo FSR também se aplica."
    };
    for (unsigned control = 0; control != 2; ++control) {
        initialise_row(rows[control],callbacks,control,labels[control],help[control]);
    }
}
void replace_mode_control(NativeRow& row, MenuCallbacks callbacks) noexcept {
    if(row.getter){auto* object=static_cast<GetterObject*>(row.getter);object->vtable->destroy(row.getter,row.getter!=row.getter_storage);}
    if(row.action){auto* object=static_cast<ActionObject*>(row.action);object->vtable->destroy(row.action,row.action!=row.action_storage);}
    initialise_row(row,callbacks,12,"Mode","O mod roda sempre no modo assíncrono: o realce da rede chega alguns quadros depois e é reprojetado para o quadro atual.");
}
const char* localized_label(const char* label) noexcept {
    if (!label) return "";
    static constexpr const char* names[][2] = {
        {"Enabled", "Ativar NR"}, {"Mode", "Modo"},
        {"Structure intensity", "Intensidade da estrutura"},
        {"Skin structure", "Estrutura da pele"},
        {"Tone intensity", "Intensidade do tom"},
        {"Dump next frame", "Capturar próximo quadro"}
    };
    for (const auto& name : names) if (std::strcmp(label, name[0]) == 0) return name[1];
    return label;
}
const char* localized_value(const char* value) noexcept {
    if (!value) return "";
    static constexpr const char* names[][2] = {
        {"On", "Ligado"}, {"Off", "Desligado"}, {"ON", "Ligado"}, {"OFF", "Desligado"},
        {"Auto", "Automático"}, {"Automatic", "Automático"},
        {"Default", "Padrão"}, {"Cinematic", "Cinemático"},
        {"Async", "Assíncrono"},
        {"async", "Assíncrono"}, {"auto", "Automático"},
        {"on", "Ligado"}, {"off", "Desligado"},
        {"Before", "Antes"}, {"After", "Depois"}, {"Pending", "Pendente"},
        {"Yes", "Sim"}, {"No", "Não"}, {"None", "Nenhum"}
        ,{"yes", "Sim"}, {"no", "Não"}, {"ready", "Pronto"}, {"Ready", "Pronto"}
    };
    for (const auto& name : names) if (std::strcmp(value, name[0]) == 0) return name[1];
    return value;
}
const char* localized_help(const char* label, const char* original_help) noexcept {
    if (label) {
        static constexpr const char* names[][2] = {
            {"Enabled", "Liga ou desliga o processamento neural. F10 alterna sem abrir o menu."},
            {"Structure intensity", "Detalhes adicionados pela rede: oclusão ambiente, sombras de contato, reflexos e dispersão sob a superfície. Padrão 1."},
            {"Skin structure", "Estrutura pela máscara semântica de personagens. Automático segue a intensidade da estrutura; 0 desativa o canal da pele."},
            {"Tone intensity", "Resposta geral de luz e cor. Esta versão limita esses canais; aumentar tende a escurecer o quadro."},
            {"Dump next frame", "Salva dlssnr_in/mv/depth/out<job>.raw junto à DLL para análise offline."}
        };
        for (const auto& name : names) if (std::strcmp(label, name[0]) == 0) return name[1];
    }
    return original_help ? original_help : "";
}
void move_row(NativeRow& destination, NativeRow& source) noexcept {
    if (&destination == &source) return;
    destination.label = source.label;
    destination.help = source.help;
    move_inline<GetterObject>(destination.getter, destination.getter_storage, source.getter);
    move_inline<ActionObject>(destination.action, destination.action_storage, source.action);
    source.label = nullptr;
    source.help = nullptr;
}
void destroy_row(NativeRow& row) noexcept {
    destroy_inline<GetterObject>(row.getter);
    destroy_inline<ActionObject>(row.action);
    row.label = nullptr;
    row.help = nullptr;
}
}
