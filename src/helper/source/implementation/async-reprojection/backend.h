#pragma once
#include "../active-scheduling/active_scheduling.h"
#include <atomic>
#include <cstdint>
#include <memory>
namespace nrreproject {
enum class Result { Composed, Skipped, Failed, Unsupported };

inline std::atomic<bool> cadeia_direta{true};

struct LateRequest {
    void* residual_prev{};int job_prev{};float exposure_prev{};long long capture_qpc_prev{};
    void* late_source{};int job_late{};float exposure_late{};long long capture_qpc_late{};
};

struct LateCounters {
    std::uint64_t armed{},recorded{},resolved{},executed_late{},refused{},b_failed{},inconsistent{};
    double gain_frames{},gain_ms{};
    std::uint64_t refused_reason[8]{};
    bool desligada{};
};

class Backend {
public:
    Backend(); ~Backend();
    Backend(const Backend&)=delete;Backend& operator=(const Backend&)=delete;
    bool begin(const NrCaptureRecord&);
    void accepted(int job) noexcept;
    Result compose(void* command,void* destination,std::uint32_t d3d_state,
                   void* residual,int job,float exposure,std::uint32_t tone_word);

    void late_arm(int job) noexcept;

    Result compose_late(void* command,void* destination,std::uint32_t d3d_state,
                        const LateRequest& request,std::uint32_t tone_word);
    LateCounters late_counters() const noexcept;

    Result compose_post(void* command,void* output,std::uint32_t ffx_state);
    bool post_pending() const noexcept;

    void refuse_post(const char* reason) noexcept;
    void invalidate() noexcept;

    void invalidate_suave() noexcept;
    void reset() noexcept;

    struct Counters {
        std::uint64_t composed{},skipped{},unsupported{},reallocations{},limit_refusals{};
        unsigned retired_generations{};
        std::uint64_t post_composed{},post_refused{};

        std::uint64_t post_lost{};
    };
    Counters counters() const noexcept;

    void rj_sem_job(bool v) noexcept;
private: struct Impl;std::unique_ptr<Impl> p_;

    void rj_recorte(void* command,void* color,unsigned vista,bool post) noexcept;
    void rj_aceito(int job) noexcept;
    void rj_compor(void* command,void* destination,unsigned formato,int job,int age,float exposure,unsigned tone) noexcept;
    void rj_segurar(void* command,void* destination,int job,float exposure,unsigned tone) noexcept;
};

bool despejar_final(void* command,void* input,std::uint32_t estado_entrada,void* output,std::uint32_t estado_saida,
                    unsigned modo,std::uint64_t quadro) noexcept;

void despejo_final_tick() noexcept;

struct FinalReservaInfo {
    unsigned modo{},lotes{},voo{},falhas{};
    unsigned long long alocacoes_chamadora{},alocacoes_fundo{},lancamentos{},prontas{},descartadas{};
};
FinalReservaInfo final_reserva_info() noexcept;
void preparar_final(void* command,void* input,void* output,unsigned modo) noexcept;
}
