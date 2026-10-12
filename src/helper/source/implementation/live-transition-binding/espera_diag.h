#pragma once

#include "../live-transition-fence-proof/fence_proof.h"
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <string>

namespace nresperadiag {
struct Vigia {
    HANDLE evento{};
    std::atomic<long long> sinal_qpc{0};
    std::atomic<unsigned> seq{0};
};
inline DWORD WINAPI vigiar(void* p) {
    auto* v = static_cast<Vigia*>(p);
    for (;;) {
        if (WaitForSingleObject(v->evento, INFINITE) != WAIT_OBJECT_0) return 0;
        LARGE_INTEGER t{}; QueryPerformanceCounter(&t);
        v->sinal_qpc.store(t.QuadPart, std::memory_order_relaxed);
        v->seq.fetch_add(1, std::memory_order_release);
    }
}
inline unsigned long long ft(const FILETIME& f) { return (static_cast<unsigned long long>(f.dwHighDateTime) << 32) | f.dwLowDateTime; }
inline unsigned long long cpu_thread_100ns(unsigned id) {
    if (!id) return 0;
    HANDLE h = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, id); if (!h) return 0;
    FILETIME c{}, e{}, k{}, u{}; unsigned long long r = 0;
    if (GetThreadTimes(h, &c, &e, &k, &u)) r = ft(k) + ft(u);
    CloseHandle(h); return r;
}
inline unsigned long long cpu_processo_100ns() {
    FILETIME c{}, e{}, k{}, u{};
    return GetProcessTimes(GetCurrentProcess(), &c, &e, &k, &u) ? ft(k) + ft(u) : 0;
}

class Diag {
public:
    static constexpr unsigned faixas = 400;
    void iniciar() noexcept {
        LARGE_INTEGER f{}; QueryPerformanceFrequency(&f); freq_ = f.QuadPart > 0 ? f.QuadPart : 1;
        vigia_.evento = CreateEventW(nullptr, FALSE, FALSE, nullptr);

        if (vigia_.evento) { HANDLE t = CreateThread(nullptr, 64 * 1024, vigiar, &vigia_, 0, nullptr); if (t) { vigia_ok_ = true; CloseHandle(t); } }
        cpu_proc0_ = cpu_processo_100ns();
    }

    void comeca(nrfence::OwnedFence* f) noexcept {
        resolver(true);
        LARGE_INTEGER t{}; QueryPerformanceCounter(&t); inicio_ = t.QuadPart; em_espera_ = true;
        armado_ = false;
        if (vigia_ok_ && f) { seq0_ = vigia_.seq.load(std::memory_order_acquire); armado_ = f->arm(vigia_.evento); if (!armado_) ++s_.sem_armar; }
    }

    void termina(unsigned voltas, unsigned render, unsigned worker) noexcept {
        ++s_.quadros; render_ = render; if (worker) worker_ = worker;
        if (!em_espera_) return;
        em_espera_ = false;
        LARGE_INTEGER t{}; QueryPerformanceCounter(&t);
        const double ms = 1000.0 * double(t.QuadPart - inicio_) / double(freq_);
        ++s_.esperas; s_.voltas += voltas; s_.ms += ms; if (ms > s_.ms_max) s_.ms_max = ms;
        unsigned b = unsigned(ms / 0.05); if (b >= faixas) b = faixas - 1; ++s_.hist[b];
        if (armado_) { pend_fim_ = t.QuadPart; pend_seq_ = seq0_ + 1; pendente_ = true; resolver(false); }
    }
    std::string linha(unsigned long pid, int modo) {
        resolver(false);
        const unsigned long long proc = cpu_processo_100ns(), ren = cpu_thread_100ns(render_), wor = cpu_thread_100ns(worker_);
        const double q = s_.quadros ? double(s_.quadros) : 1.0;
        auto d = [&](unsigned long long agora, unsigned long long& antes) { const double ms = (antes && agora >= antes) ? double(agora - antes) / 1e4 : -1.0; antes = agora; return ms; };
        const double cp = d(proc, cpu_proc0_), cr = d(ren, cpu_ren0_), cw = d(wor, cpu_wor0_);
        char t[640];
        std::snprintf(t, sizeof t,
            "ESPERA pid=%lu modo=%d quadros=%u esperas=%u voltas=%u espera_ms=%.3f espera_ms_quadro=%.4f espera_ms_media=%.4f p50=%.3f p95=%.3f max=%.3f "
            "sobra_n=%u sobra_ms=%.3f sobra_ms_quadro=%.4f sobra_ms_media=%.4f sobra_max=%.3f sobra_vigia_atrasada=%u sobra_sem_sinal=%u sem_armar=%u "
            "cpu_proc_ms_quadro=%.4f cpu_render_ms_quadro=%.4f cpu_worker_ms_quadro=%.4f\n",
            pid, modo, s_.quadros, s_.esperas, s_.voltas, s_.ms, s_.ms / q, s_.esperas ? s_.ms / s_.esperas : 0.0, pct(0.5), pct(0.95), s_.ms_max,
            s_.sobra_n, s_.sobra_ms, s_.sobra_ms / q, s_.sobra_n ? s_.sobra_ms / s_.sobra_n : 0.0, s_.sobra_max, s_.atrasada, s_.sem_sinal, s_.sem_armar,
            cp < 0 ? -1.0 : cp / q, cr < 0 ? -1.0 : cr / q, cw < 0 ? -1.0 : cw / q);
        s_ = {};
        return t;
    }
private:
    struct Stats {
        unsigned quadros{}, esperas{}, voltas{}, hist[faixas]{};
        double ms{}, ms_max{};
        unsigned sobra_n{}, atrasada{}, sem_sinal{}, sem_armar{};
        double sobra_ms{}, sobra_max{};
    } s_{};
    Vigia vigia_{};
    bool vigia_ok_{}, em_espera_{}, armado_{}, pendente_{};
    long long freq_{1}, inicio_{}, pend_fim_{};
    unsigned seq0_{}, pend_seq_{}, render_{}, worker_{};
    unsigned long long cpu_proc0_{}, cpu_ren0_{}, cpu_wor0_{};

    void resolver(bool forcar) noexcept {
        if (!pendente_) return;
        if (static_cast<int>(vigia_.seq.load(std::memory_order_acquire) - pend_seq_) >= 0) {
            const double ms = 1000.0 * double(pend_fim_ - vigia_.sinal_qpc.load(std::memory_order_relaxed)) / double(freq_);
            if (ms < 0) ++s_.atrasada; else { ++s_.sobra_n; s_.sobra_ms += ms; if (ms > s_.sobra_max) s_.sobra_max = ms; }
            pendente_ = false;
        } else if (forcar) { ++s_.sem_sinal; pendente_ = false; }
    }
    double pct(double p) const noexcept {
        if (!s_.esperas) return 0.0;
        const unsigned alvo = unsigned(p * s_.esperas + 0.5); unsigned acc = 0;
        for (unsigned i = 0; i < faixas; ++i) { acc += s_.hist[i]; if (acc >= (alvo ? alvo : 1)) return (i + 0.5) * 0.05; }
        return faixas * 0.05;
    }
};
}
