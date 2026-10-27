#pragma once

#include <atomic>
#include <cstdint>
namespace nrfsrint {
struct Limiares {
    std::uint32_t janela_ms=4000;
    std::uint32_t taxa_entra_mil=500;
    std::uint32_t quadros_min=30;
    std::uint32_t buraco_quadros=8;
    std::uint32_t reentradas=3;
    std::uint32_t reentradas_ms=8000;
    std::uint32_t seguido_buraco=4;
    std::uint32_t sai_ms=1000;
    std::uint32_t sai_despachos=20;
    std::uint32_t taxa_sai_mil=450;
    std::uint32_t segura_ms=3000;
    std::uint32_t passo_amostra_ms=250;
};

class Detector {
public:
    explicit Detector(const Limiares& l=Limiares{}) noexcept:l_(l){}

    int passo(std::uint64_t agora_ms,std::uint64_t presents,std::uint64_t gerados,std::uint64_t despachos) noexcept {
        std::uint64_t reais=presents>gerados?presents-gerados:0;
        if(reais<reais_)reais=reais_;
        if(despachos<desp_)despachos=desp_;
        reais_=reais;

        if(n_amostras_==0||agora_ms>=amostras_[(cab_+n_amostras_-1)%n_ring].t+l_.passo_amostra_ms){
            if(n_amostras_==n_ring){cab_=(cab_+1)%n_ring;--n_amostras_;}
            amostras_[(cab_+n_amostras_)%n_ring]={agora_ms,reais,despachos};++n_amostras_;
        }
        janela(agora_ms,reais,despachos);
        int r=0;
        if(despachos>desp_){
            const std::uint64_t buraco=tem_desp_?reais-reais_ult_desp_:0;
            if(tem_desp_&&buraco>=l_.buraco_quadros){reent_[reent_cab_]=agora_ms;reent_cab_=(reent_cab_+1)%n_reent;if(reent_n_<n_reent)++reent_n_;}

            if(!tem_desp_||buraco>=l_.seguido_buraco){seg_t0_=agora_ms;seg_r0_=reais?reais-1:0;seg_d0_=desp_;}
            tem_desp_=true;reais_ult_desp_=reais;desp_=despachos;
            if(ativo_&&buraco<l_.seguido_buraco&&agora_ms>=seg_t0_+l_.sai_ms&&agora_ms>=entrou_+l_.segura_ms){
                const std::uint64_t d=despachos-seg_d0_,q=reais>seg_r0_?reais-seg_r0_:0;
                if(d>=l_.sai_despachos&&q&&d*1000u>=q*std::uint64_t(l_.taxa_sai_mil)){ativo_=false;reent_n_=0;r=-1;}
            }
        }
        if(!ativo_&&r==0&&reentradas(agora_ms)>=l_.reentradas&&taxa_valida_&&taxa_mil_<l_.taxa_entra_mil){ativo_=true;entrou_=agora_ms;r=1;}
        return r;
    }
    bool ativo() const noexcept {return ativo_;}
    bool taxa_valida() const noexcept {return taxa_valida_;}
    std::uint32_t taxa_mil() const noexcept {return taxa_mil_;}
    std::uint64_t quadros_janela() const noexcept {return q_janela_;}
    std::uint64_t despachos_janela() const noexcept {return d_janela_;}
    std::uint32_t reentradas(std::uint64_t agora_ms) const noexcept {
        std::uint32_t n=0;
        for(std::uint32_t i=0;i<reent_n_;++i){const std::uint64_t t=reent_[(reent_cab_+n_reent-1-i)%n_reent];if(agora_ms<t+l_.reentradas_ms)++n;}
        return n;
    }
    const Limiares& limiares() const noexcept {return l_;}
private:
    static constexpr std::uint32_t n_ring=64,n_reent=16;
    struct Amostra{std::uint64_t t,reais,desp;};

    void janela(std::uint64_t agora_ms,std::uint64_t reais,std::uint64_t despachos) noexcept {
        taxa_valida_=false;
        for(std::uint32_t i=n_amostras_;i-->0;){
            const auto& a=amostras_[(cab_+i)%n_ring];
            if(agora_ms<a.t+l_.janela_ms)continue;
            q_janela_=reais-a.reais;d_janela_=despachos>=a.desp?despachos-a.desp:0;
            if(q_janela_>=l_.quadros_min&&d_janela_>=1){taxa_valida_=true;taxa_mil_=std::uint32_t((d_janela_*1000u)/q_janela_);}
            return;
        }
    }
    Limiares l_;
    Amostra amostras_[n_ring]{};std::uint32_t cab_{},n_amostras_{};
    std::uint64_t reent_[n_reent]{};std::uint32_t reent_cab_{},reent_n_{};
    std::uint64_t reais_{},desp_{},reais_ult_desp_{};bool tem_desp_{};
    std::uint64_t seg_t0_{},seg_r0_{},seg_d0_{};
    bool ativo_{};std::uint64_t entrou_{};
    bool taxa_valida_{};std::uint32_t taxa_mil_{};std::uint64_t q_janela_{},d_janela_{};
};

class Repetidos {
public:

    std::uint64_t passo(std::uint64_t upscales) noexcept {
        const bool com=upscales>ultimo_;
        if(!com&&anterior_com_){++repetidos_;anterior_com_=false;}else anterior_com_=com;
        if(upscales>ultimo_)ultimo_=upscales;
        return repetidos_;
    }
    std::uint64_t total() const noexcept {return repetidos_;}
private:
    std::uint64_t ultimo_{},repetidos_{};bool anterior_com_{};
};

inline std::atomic<std::uint64_t> presents{0};
inline std::atomic<std::uint64_t> gerados{0};
inline std::atomic<std::uint64_t> upscales{0};
inline std::atomic<bool> ativo{false};
inline std::atomic<int> modo{-1};
inline std::atomic<std::uint32_t> taxa_mil{0};
inline std::atomic<std::uint64_t> repetidos{0};

inline int efetivo(int valor_ini,bool conhecido) noexcept {return valor_ini<=0?0:(valor_ini>=2||!conhecido)?1:0;}
}
