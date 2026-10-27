#pragma once

#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace nrfgext {
#pragma pack(push, 4)
struct Fg {
    std::uint32_t magia, versao;
    std::uint32_t disponivel, ligado;
    volatile LONG pedido;
    std::uint32_t reservado;
    std::uint64_t geracoes, tick_ms;
    char provedor[24];
};
#pragma pack(pop)
static_assert(sizeof(Fg) == 64, "nrfgext::Fg = 64 bytes, o mesmo da libxess");
inline constexpr std::uint32_t kMagia = 0x4746524Eu;
inline constexpr char rotulo[] = "Geração de quadros";

inline std::atomic<Fg*>& teste() noexcept {static std::atomic<Fg*> t{nullptr};return t;}

inline std::atomic<bool>& teste_dll() noexcept {static std::atomic<bool> t{false};return t;}

inline bool dll_fg_nossa() noexcept {
    if (teste_dll().load(std::memory_order_relaxed)) return true;
    static std::atomic<bool> vista{false};
    if (vista.load(std::memory_order_relaxed)) return true;
    const HMODULE x = GetModuleHandleW(L"libxess.dll"), f = GetModuleHandleW(L"ffx_fsr2_api_x64.dll");
    const bool nossa = (x && GetProcAddress(x, "nrXessFsrFgEstado")) || (f && GetProcAddress(f, "nrFsr2FfxFgEstado"));
    if (nossa) vista.store(true, std::memory_order_relaxed);
    return nossa;
}

inline Fg* memoria() noexcept {
    if (Fg* t = teste().load(std::memory_order_acquire)) return t;
    static std::atomic<Fg*> m{nullptr};
    static std::atomic<std::uint64_t> proxima{0};
    if (Fg* v = m.load(std::memory_order_acquire)) return v;
    const std::uint64_t agora = GetTickCount64();
    if (!dll_fg_nossa()) {
        if (agora < proxima.load(std::memory_order_relaxed)) return nullptr;
        proxima.store(agora + 2000, std::memory_order_relaxed);
    }
    wchar_t nome[64];
    swprintf(nome, 64, L"Local\\NR_FG_%lu", GetCurrentProcessId());
    HANDLE h = OpenFileMappingW(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, nome);
    if (!h) return nullptr;
    auto* v = static_cast<Fg*>(MapViewOfFile(h, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(Fg)));
    CloseHandle(h);
    if (!v) return nullptr;
    Fg* nulo = nullptr;
    if (!m.compare_exchange_strong(nulo, v)) { UnmapViewOfFile(v); return nulo; }
    return v;
}
inline Fg* valida() noexcept {Fg* m = memoria();return m && m->magia == kMagia && m->versao == 1u ? m : nullptr;}

inline bool disponivel() noexcept {const Fg* m = valida();return m && m->disponivel == 1u;}

inline bool ligado() noexcept {
    const Fg* m = valida();if (!m) return false;
    const LONG p = m->pedido;
    return p == 0 || p == 1 ? p == 1 : m->ligado == 1u;
}

inline bool gerando() noexcept {const Fg* m = valida();return m && m->disponivel == 1u && m->ligado == 1u;}

inline bool alternar() noexcept {
    Fg* m = valida();if (!m || m->disponivel != 1u) return false;
    InterlockedExchange(&m->pedido, ligado() ? 0 : 1);
    return true;
}

inline constexpr char rotulo_compacto[] = "FG x2";
struct Vigia {std::uint64_t geracoes = 0, mudou_ms = 0;};
inline bool gerando_agora(Vigia& v, std::uint64_t agora_ms) noexcept {
    const Fg* m = valida();
    if (!m || m->disponivel != 1u || m->ligado != 1u) return false;
    const std::uint64_t g = m->geracoes;
    if (g != v.geracoes) {v.geracoes = g; v.mudou_ms = agora_ms;}
    return v.mudou_ms != 0 && agora_ms >= v.mudou_ms && agora_ms - v.mudou_ms <= 1500u;
}

inline bool visivel(const char* label) noexcept {return !label || std::strcmp(label, rotulo) != 0 || disponivel();}
}
