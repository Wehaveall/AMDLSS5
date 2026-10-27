#pragma once

#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>

namespace nrestext {
#pragma pack(push, 4)
struct Publicado {
    std::uint32_t magia, versao;
    volatile LONG seq;
    std::uint32_t ok;
    std::uint64_t handle;
    std::uint32_t geracao, largura, altura, pitch;
    std::uint64_t execucao, tick_ms;
    std::uint32_t luid_baixo;
    std::int32_t luid_alto;
};
#pragma pack(pop)
static_assert(sizeof(Publicado) == 64, "nrestext::Publicado = 64 bytes, o mesmo da libxess");

struct Visto {
    HANDLE handle{};
    std::uint32_t geracao{}, largura{}, altura{}, pitch{};
    std::uint64_t execucao{};
    LUID luid{};
};

inline const Publicado* memoria() {
    static std::atomic<const Publicado*> mem{nullptr};
    static std::atomic<std::uint64_t> proxima{0};
    if (const Publicado* v = mem.load(std::memory_order_acquire)) return v;
    const std::uint64_t agora = GetTickCount64();
    if (agora < proxima.load(std::memory_order_relaxed)) return nullptr;
    proxima.store(agora + 2000, std::memory_order_relaxed);
    wchar_t nome[64];
    swprintf(nome, 64, L"Local\\NR_ESTENCIL_%lu", GetCurrentProcessId());
    HANDLE h = OpenFileMappingW(FILE_MAP_READ, FALSE, nome);
    if (!h) return nullptr;
    auto* v = static_cast<const Publicado*>(MapViewOfFile(h, FILE_MAP_READ, 0, 0, sizeof(Publicado)));
    CloseHandle(h);
    if (!v) return nullptr;
    const Publicado* nulo = nullptr;
    if (!mem.compare_exchange_strong(nulo, v, std::memory_order_acq_rel)) { UnmapViewOfFile(v); return nulo; }
    return v;
}
inline bool ler(Visto& v) {
    const Publicado* m = memoria();
    if (!m) return false;
    if (m->magia != 0x5345524Eu || m->versao != 1u) return false;
    for (int i = 0; i < 4; ++i) {
        const LONG s0 = m->seq;
        if (s0 & 1) { YieldProcessor(); continue; }
        MemoryBarrier();
        const Publicado c = *m;
        MemoryBarrier();
        if (m->seq != s0) continue;
        if (!c.ok || !c.handle || GetTickCount64() - c.tick_ms > 250) return false;
        v.handle = reinterpret_cast<HANDLE>(c.handle);
        v.geracao = c.geracao; v.largura = c.largura; v.altura = c.altura; v.pitch = c.pitch;
        v.execucao = c.execucao;
        v.luid.LowPart = c.luid_baixo; v.luid.HighPart = c.luid_alto;
        return true;
    }
    return false;
}

inline const char* valida(const Visto& v, LUID device) noexcept {
    if (device.LowPart != v.luid.LowPart || device.HighPart != v.luid.HighPart) return "outro_device";
    if (v.largura < 16u || v.altura < 16u || v.largura > 16384u || v.altura > 16384u || v.pitch < v.largura) return "tamanho";
    return nullptr;
}

struct Andamento { std::uint64_t exec_ult{}; unsigned repete{}; };
inline bool andou(Andamento& a, std::uint64_t execucao) noexcept {
    if (execucao == a.exec_ult) return ++a.repete <= 2u;
    a.exec_ult = execucao; a.repete = 0; return true;
}

struct Leitor : Andamento {
    Microsoft::WRL::ComPtr<ID3D12Resource> buf;
    std::uint32_t geracao{}, larg{}, alt{}, pitch{};
    std::uint64_t bytes{};
};

inline const char* pronto(Leitor& L, ID3D12Device* d, Microsoft::WRL::ComPtr<ID3D12Resource>& velho, bool& novo) {
    novo = false;
    Visto v{};
    if (!d || !ler(v)) return "sem_estencil";
    if (const char* m = valida(v, d->GetAdapterLuid())) return m;
    if (v.geracao != L.geracao || !L.buf) {
        Microsoft::WRL::ComPtr<ID3D12Resource> b;
        if (FAILED(d->OpenSharedHandle(v.handle, IID_PPV_ARGS(&b))) || !b) return "abrir_handle";
        const auto bd = b->GetDesc();
        if (bd.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER || bd.Width < std::uint64_t(v.pitch) * (v.altura - 1u) + v.largura || (bd.Width & 3u)) return "buffer";
        velho = std::move(L.buf); L.buf = std::move(b); novo = true;
        L.geracao = v.geracao; L.larg = v.largura; L.alt = v.altura; L.pitch = v.pitch; L.bytes = bd.Width; L.exec_ult = 0; L.repete = 0;
    }
    if (!andou(L, v.execucao)) return "estencil_parado";
    return nullptr;
}

struct Bordas { ID3D12Resource* tabela{}; std::uint32_t w{}, h{}; std::uint64_t execucao{}; };
inline Bordas bordas_quadro{};

inline unsigned bits_mascara(bool tem_chave, double chave, bool tem_sinonimo, double sinonimo, const char** aviso = nullptr) noexcept {
    auto bits = [](double x) -> unsigned { return x >= 1. && x <= 255. ? unsigned(x) : 0u; };
    if (aviso) *aviso = nullptr;
    if (tem_chave) {
        if (aviso && tem_sinonimo && bits(sinonimo) != bits(chave)) *aviso = "conflito";
        return bits(chave);
    }
    if (tem_sinonimo) { if (aviso) *aviso = "sinonimo"; return bits(sinonimo); }
    return 0u;
}

enum class ModoHalo : unsigned { Nada, Personagem, SoProfundidade };
inline ModoHalo modo_halo(unsigned bits, float prof, bool estencil_pronto) noexcept {
    if (bits) return estencil_pronto ? ModoHalo::Personagem : ModoHalo::Nada;
    return prof > 0.f ? ModoHalo::SoProfundidade : ModoHalo::Nada;
}
inline bool aloca_mascara(unsigned bits, float prof) noexcept { return bits != 0u || prof > 0.f; }

template <class K>
inline K mascara_k_so_prof(unsigned largura, unsigned altura, unsigned celw, unsigned celh, unsigned fatia, float r1, unsigned cel_s, float ln_frente, float expo_tom) noexcept {
    const float s = float(cel_s);
    return K{largura, altura, celw, celh, 1u, 1u, 4u, 0u, fatia, r1 * 3.f / 7.f, r1, unsigned(std::ceil(r1 / s)) + 1u, unsigned(std::ceil((r1 + 64.f) / s)),
             std::log(1.5f), ln_frente, expo_tom, 0u};
}
}
