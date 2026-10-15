#pragma once
#include "../../../runtime/nr_publico.h"
#include <cstddef>
#include <string>

namespace nrmenu {
enum class Language : unsigned { En=0, Pt=1, Es=2 };
inline constexpr unsigned language_count=3;
Language language() noexcept;
void set_language(Language value) noexcept;
const char* language_code(Language value) noexcept;
const char* language_name(Language value) noexcept;
bool parse_language(const wchar_t* code,Language& out) noexcept;
bool parse_language_name(const char* name,Language& out) noexcept;

inline constexpr char language_label[]="Idioma";
inline constexpr char language_help[]="Idioma do menu: English, Português (BR) ou Español. Vale na hora e fica salvo no INI (MenuLanguage).";
inline constexpr char input_label[]="Entrada do jogo com menu aberto";
inline constexpr char input_help[]="O que o jogo deixa de receber com o menu aberto. Teclas do menu: setas, Enter, End e F10 não chegam ao jogo; o resto passa (dá para andar e testar). Tudo: teclado, mouse e controle XInput ficam só com o menu. End e F10 nunca chegam ao jogo. Salvo no INI (MenuInputBlock).";
inline constexpr char input_menu_keys[]="Teclas do menu";
inline constexpr char input_everything[]="Tudo";

inline constexpr bool show_help_default=true;
bool show_help() noexcept;
void set_show_help(bool value) noexcept;

enum class InputBlock : unsigned { MenuKeys=0, Everything=1 };
InputBlock input_block() noexcept;
void set_input_block(InputBlock value) noexcept;

enum class Group : unsigned { General, Neural, Composition, Performance, Color, Motion, Flicker, Halo, Diagnostics,
                              Reprojection, Restart, Count };
struct CatalogEntry { const char* label; Group group; bool shown; bool native; };
const CatalogEntry* catalog_begin() noexcept;
const CatalogEntry* catalog_end() noexcept;
const CatalogEntry* catalog_find(const char* label) noexcept;
int catalog_index(const char* label) noexcept;
bool catalog_shown(const char* label) noexcept;
const char* group_name(Group group,Language value) noexcept;

std::string label(const char* source,Language value);
std::string help(const char* label_source,const char* help_source,Language value);

std::string value(const char* label_source,const std::string& source,Language value,bool* complete=nullptr);
std::string status(const std::string& source,Language value);
enum class Ui : unsigned { Hints, Input, FgActive, FgIdle, FgNone, FpsCollecting, DisplayUnavailable, Empty,
                           FallbackStatus, PendingLegend, NetworkOff, Count };
const char* ui(Ui key,Language value) noexcept;

struct CompatReason { const char* id; bool prefix; const char* text[language_count]; };
const CompatReason* compat_reason_begin() noexcept;
const CompatReason* compat_reason_end() noexcept;
const char* compat_reason(const char* id,Language value) noexcept;
std::string network_off(const char* id,const char* field,Language value);

bool label_known(const char* source) noexcept;
bool help_known(const char* label_source,const char* help_source) noexcept;
}
