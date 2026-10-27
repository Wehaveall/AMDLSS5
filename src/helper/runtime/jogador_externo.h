#pragma once

#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstdio>

namespace nrjext {
#pragma pack(push, 4)
struct Cabeca {
    std::uint32_t magic, versao;
    volatile LONG seq;
    std::uint32_t ok;
    std::uint64_t tick_ms;
    std::uint32_t quadro;
    float u, v, alto, dist;
    float mundo[3];
    float larg, alt;
    std::uint32_t estado;
};
#pragma pack(pop)

inline bool ler(float& u, float& v, float& alto, float& dist, bool* fora = nullptr) {
    if (fora) *fora = false;

    static std::atomic<const Cabeca*> mm{nullptr};
    static std::atomic<std::uint64_t> proxima{0};
    const Cabeca* m = mm.load(std::memory_order_acquire);
    if (!m) {
        const std::uint64_t agora = GetTickCount64();
        if (agora < proxima.load(std::memory_order_relaxed)) return false;
        proxima.store(agora + 2000, std::memory_order_relaxed);
        wchar_t nome[96];
        swprintf(nome, 96, L"Local\\NR_JOGADOR_CABECA_%lu", GetCurrentProcessId());
        HANDLE h = OpenFileMappingW(FILE_MAP_READ, FALSE, nome);
        if (!h) return false;
        m = static_cast<const Cabeca*>(MapViewOfFile(h, FILE_MAP_READ, 0, 0, sizeof(Cabeca)));
        CloseHandle(h);
        if (!m) return false;
        const Cabeca* nulo = nullptr;
        if (!mm.compare_exchange_strong(nulo, m, std::memory_order_acq_rel)) { UnmapViewOfFile(m); m = nulo; }
    }
    if (m->magic != 0x4E524A43u || (m->versao != 1u && m->versao != 2u)) return false;
    for (int i = 0; i < 4; ++i) {
        const LONG s0 = m->seq;
        if (s0 & 1) { YieldProcessor(); continue; }
        MemoryBarrier();
        const Cabeca c = *m;
        MemoryBarrier();
        if (m->seq != s0) continue;
        if (GetTickCount64() - c.tick_ms > 250) return false;
        if (!c.ok) { if (fora && c.versao >= 2u && c.estado == 2u) *fora = true; return false; }
        u = c.u; v = c.v; alto = c.alto; dist = c.dist;
        return true;
    }
    return false;
}
}
