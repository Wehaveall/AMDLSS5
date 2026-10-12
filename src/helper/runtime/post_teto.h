#pragma once
#include "nr_publico.h"

#include <atomic>
#include <cmath>
#include <cstdint>
#include <windows.h>
#include "ini_jogo.h"
namespace nrposteto {

inline bool automatico(){
    static const bool v=NRCFG_INT(L"DlssNrOnAmd",L"NeuralPostAuto",0,nrinijogo::caminho())==1;
    return v;
}
inline constexpr double piso=100.0/3.0;
inline constexpr double alvo_quadros=7.0,subir_quadros=5.5,folga_ms=12.0;
inline constexpr std::uint64_t intervalo_ms=2000;

inline constexpr std::uint64_t subir_apos_descida_ms=10000;

inline double degrau_abaixo(double p){
    if(!(p>0))return piso;
    if(p>=100.0)return 100.0;
    const double d=std::floor(p/5.0+1e-9)*5.0;
    return d<piso?piso:d;
}

inline double degrau_acima(double p){
    if(p>=100.0)return 100.0;
    const double d=std::floor(p/5.0+1e-9)*5.0+5.0;
    return d>100.0?100.0:d;
}

inline constexpr double inicio_px=1920.0*1080.0;
inline constexpr unsigned janelas_boas=3;

struct Estado { double teto=100.0; std::uint64_t ultima_ms=0,ultima_descida_ms=0,ultima_janela_ms=0; bool trocou=false,desceu=false;
                unsigned boas=0; double px_provado=0.0; };
struct Entrada {
    double pedido=100.0;
    double quadro_ms=0.0;
    double job_ms=0.0;
    std::uint64_t agora_ms=0;
    std::uint64_t saida_px=0;
    double teto_vram=100.0;
};

inline double pct_para_px(double px,std::uint64_t saida_px){
    if(!saida_px||!(px>0))return piso;
    const double p=100.0*std::sqrt(px/double(saida_px));
    return p>=100.0?100.0:degrau_abaixo(p+1e-6);
}

inline double entrar_post(Estado& e,std::uint64_t saida_px){
    const double px=e.px_provado>inicio_px?e.px_provado:inicio_px;
    const double limite=saida_px?pct_para_px(px,saida_px):50.0;
    if(e.teto>limite)e.teto=limite;
    e.boas=0;return e.teto;
}

inline double passo_latencia(Estado& e,const Entrada& x,bool pode_subir=true){

    const double lim=x.teto_vram>0&&x.teto_vram<x.pedido?x.teto_vram:x.pedido;
    const double efetivo=lim<e.teto?lim:e.teto;
    if(!(x.quadro_ms>0)||!(x.job_ms>0)||!(efetivo>0))return e.teto;
    if(e.trocou&&x.agora_ms-e.ultima_ms<intervalo_ms)return e.teto;
    const double L=(x.job_ms+folga_ms)/x.quadro_ms;
    if(L>alvo_quadros&&efetivo>piso+1e-6){
        const double orcamento=alvo_quadros*x.quadro_ms-folga_ms;
        double novo=orcamento>0?degrau_abaixo(efetivo*std::sqrt(orcamento/x.job_ms)):piso;
        if(novo>=efetivo)novo=degrau_abaixo(efetivo-5.0);
        e.teto=novo;e.ultima_ms=e.ultima_descida_ms=e.ultima_janela_ms=x.agora_ms;e.trocou=e.desceu=true;e.boas=0;return e.teto;
    }
    if(!pode_subir)return e.teto;
    if(L>alvo_quadros){e.boas=0;return e.teto;}

    if(x.agora_ms-e.ultima_janela_ms<intervalo_ms)return e.teto;
    e.ultima_janela_ms=x.agora_ms;
    if(x.saida_px){const double px=double(x.saida_px)*(efetivo/100.0)*(efetivo/100.0);if(px>e.px_provado)e.px_provado=px;}

    if(e.teto<100.0&&lim>e.teto+1e-6&&(!e.desceu||x.agora_ms-e.ultima_descida_ms>=subir_apos_descida_ms)){
        const double acima=degrau_acima(e.teto);
        const double alvo=acima<lim?acima:lim;
        const double job=x.job_ms*(alvo/efetivo)*(alvo/efetivo);
        if((job+folga_ms)/x.quadro_ms<=subir_quadros){
            if(++e.boas>=janelas_boas){e.teto=alvo>=100.0?100.0:alvo;e.ultima_ms=e.ultima_janela_ms=x.agora_ms;e.trocou=true;e.boas=0;}
        }else e.boas=0;
    }else e.boas=0;
    return e.teto;
}

inline double job_previsto(double job_pre_ms,double px_pre,double px_post){
    return job_pre_ms>0&&px_pre>0&&px_post>0?2.0*job_pre_ms*px_post/px_pre:0.0;
}

inline constexpr double reserva_bytes=1024.0*1024.0*1024.0,fixo_bytes=0.6e9,bytes_por_pixel=600.0;
inline double teto_vram(std::uint64_t folga,std::uint64_t saida_px){
    if(!saida_px||!folga)return 100.0;
    const double pixels=(double(folga)-reserva_bytes-fixo_bytes)/bytes_por_pixel;
    if(pixels<=0)return piso;
    const double p=100.0*std::sqrt(pixels/double(saida_px));
    return p>=100.0?100.0:degrau_abaixo(p);
}

inline constexpr std::uint64_t medir_apos_ms=2000;
inline double teto_vram_medido(std::uint64_t folga,std::uint64_t saida_px,double bpp){
    if(!saida_px||!folga||!(bpp>0))return 100.0;
    const double pixels=(double(folga)-reserva_bytes)/bpp;
    if(pixels<=0)return piso;
    const double p=100.0*std::sqrt(pixels/double(saida_px));
    return p>=100.0?100.0:degrau_abaixo(p);
}

inline constexpr std::uint64_t crescer_bytes=64ull*1024*1024;
struct MedidaVram { std::uint64_t uso_pre{},desde_ms{};unsigned centi{};bool travado{};double px_max{};std::uint64_t uso_descida{};bool desceu{}; };

inline double aplicar_medido(MedidaVram& m,double atual,double medido,std::uint64_t uso){
    if(!(medido>0))return atual;
    if(medido<atual){
        if(m.desceu&&uso<m.uso_descida+crescer_bytes)return atual;
        m.travado=m.desceu=true;m.uso_descida=uso;return medido;
    }
    if(m.travado||medido<=atual)return atual;
    const double acima=degrau_acima(atual);
    return medido<acima?medido:acima;
}

inline double amostra_vram(MedidaVram& m,bool post,std::uint64_t uso,std::uint64_t folga_pre,std::uint64_t saida_px,double efetivo_pct,std::uint64_t agora_ms){
    if(!post){m.uso_pre=uso;m.centi=0;m.desde_ms=agora_ms;m.travado=m.desceu=false;m.px_max=0;m.uso_descida=0;return 0.0;}
    const unsigned c=unsigned(efetivo_pct*100.0+.5);
    if(c!=m.centi){m.centi=c;m.desde_ms=agora_ms;return 0.0;}
    if(agora_ms-m.desde_ms<medir_apos_ms||!(efetivo_pct>0))return 0.0;
    const double px=double(saida_px)*(efetivo_pct/100.0)*(efetivo_pct/100.0);
    if(!(px>0))return 0.0;
    if(px>m.px_max)m.px_max=px;
    if(!m.uso_pre||uso<=m.uso_pre)return 0.0;
    return teto_vram_medido(folga_pre,saida_px,double(uso-m.uso_pre)/m.px_max);
}

inline std::atomic<std::uint64_t> folga_pre{0},uso{0},orcamento{0};
inline std::atomic<unsigned> teto_vram_centi{10000};

inline std::atomic<std::uint64_t> segurados_total{0};

inline MedidaVram medida_vram{};inline bool medida_vram_relatada{};

inline Estado estado_latencia{};
}
