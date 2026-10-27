#pragma once
#include <atomic>
#include "../../../runtime/proxy_rva.h"
#include <cstdint>
#include <cmath>
#include <chrono>
namespace nrreproject {

inline std::atomic<int> prof_invertida_jogo{-1};

inline std::atomic<unsigned> cor_bits_adaptada{0};
inline std::atomic<bool> cor_ffx_srgb{false};

inline constexpr std::uint32_t rva_tom_ini=nrproxy::rva_tom_ini,rva_tom_ef=nrproxy::rva_tom_ef;
inline std::atomic<int> tom_pedido{-1},tom_ini{-2},tom_ef{-2};

inline long long agora_ms() noexcept {return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
inline std::atomic<long long> fg_ultimo_ms{-1000000};
inline void fg_marca() noexcept {fg_ultimo_ms.store(agora_ms(),std::memory_order_relaxed);}
inline bool fg_ativo() noexcept {return agora_ms()-fg_ultimo_ms.load(std::memory_order_relaxed)<500;}

enum Control : unsigned { Clamp, Divergence, Depth, Color, MotionFactor, DetailMotion, PointMotion, MotionReference, DepthAbsolute, MaxAge,
                          ComposeAfter, FillMode, RawDivergence, Stabilize, GradeExposure, GradeContrast, GradeSaturation, MotionStrength, MotionStrengthRef, JitterComp,
                          StabilizeStill, GainLimit, SoftGuard, LightGuard, DetailGuard, LightSmooth, HaloReduce, RatioFloor, LateCompose, HaloNetScale,
                          LightGuideDepth, HaloExcess, HaloCutMoving, ControlCount };
inline constexpr float clamp_list[]={0.f,.25f,.5f,.75f,1.f};

inline constexpr float divergence_list[]={0.f,4.f,2.f,1.f,.5f,.25f};
inline constexpr float depth_list[]={.1f,.03f,.01f,.003f,.001f};
inline constexpr float color_list[]={.15f,.5f,1.f,1.5f,2.5f,4.f};

inline constexpr float motion_factor_list[]={1.f,-1.f,.5f,-.5f,2.f,-2.f};

inline constexpr float detail_motion_list[]={0.f,16.f,12.f,8.f,5.f,3.f};

inline constexpr float point_motion_list[]={0.f,1.f};
inline constexpr float motion_reference_list[]={.01f,.005f,.02f,.05f};
inline constexpr float depth_absolute_list[]={.0001f,0.f,.001f,.01f};

inline constexpr float max_age_list[]={0.f,6.f,5.f,4.f,3.f,2.f,1.f};

inline constexpr float compose_after_list[]={0.f,1.f};

inline constexpr float fill_mode_list[]={0.f,2.f};

inline constexpr float raw_divergence_list[]={0.f,20.f,8.f,4.f};

inline constexpr float stabilize_list[]={0.f,.5f,.33f,.2f};

inline constexpr float grade_exposure_list[]={0.f,-.3f,-.2f,-.1f,.1f,.2f,.3f};
inline constexpr float grade_contrast_list[]={0.f,-.5f,-.25f,-.1f,.1f,.25f,.5f};
inline constexpr float grade_saturation_list[]={0.f,-.3f,-.15f,-.1f,.1f,.15f,.3f,.5f};

inline constexpr float motion_strength_list[]={1.f,.75f,.6f,.5f,.4f,.3f};

inline constexpr float motion_strength_ref_list[]={2.f,4.f,8.f,16.f};

inline constexpr float jitter_comp_list[]={0.f,1.f,-1.f};

inline constexpr float stabilize_still_list[]={2.f,1.f,4.f,8.f,0.f};

inline constexpr float gain_limit_list[]={0.f,.05f,.10f,.20f};

inline constexpr float soft_guard_list[]={0.f,1.f};

inline constexpr float guarda_sem_limite=1000.f;
inline constexpr float light_guard_list[]={0.f,1.5f,2.f,2.5f,3.f,4.f,5.f,6.f,8.f,guarda_sem_limite};

inline constexpr float detail_guard_list[]={1.1f,1.2f,1.3f,1.5f,1.75f,2.f,2.5f,3.f,4.f,6.f,8.f,guarda_sem_limite};

inline constexpr float ln_sem_limite=128.f;
inline float ln_protecao(float v){return v>=guarda_sem_limite ? ln_sem_limite : std::log(v);}

inline constexpr float light_smooth_list[]={0.f,.3f,.15f,.08f};

inline constexpr float halo_reduce_list[]={0.f,1.f,2.f,3.f};

inline constexpr float ratio_floor_list[]={1.f/512.f,.005f,.01f};

inline constexpr float late_compose_list[]={0.f,1.f};

inline constexpr float halo_net_scale_list[]={0.f,1.f};

inline unsigned halo_raio_rede(unsigned r,double rede_pct){
    if(!(rede_pct>0.)||rede_pct>=100.)return r;
    const double v=double(r)*100./rede_pct;return v>=16. ? 16u : unsigned(v+.5);
}

inline constexpr float light_guide_depth_list[]={0.f,1.f};

inline constexpr float halo_excess_list[]={0.f,.5f,1.f,1.5f};

inline unsigned halo_excesso_raio(unsigned r100,double rede_pct){
    if(!(rede_pct>0.)||rede_pct>=100.)return r100>24u?24u:r100;
    const double v=double(r100)*100./rede_pct;return v>=24. ? 24u : unsigned(v+.5);
}

inline constexpr float halo_cut_moving_list[]={0.f,1.f,2.f};

inline unsigned corte_raio_compose(unsigned nivel,double rede_pct){
    if(nivel==0u||nivel>2u)return 0u;
    const double r=nivel==1u?16.:24.;const double v=(rede_pct>0.&&rede_pct<100.)?r*100./rede_pct:r;
    const unsigned u=v>=30. ? 30u : unsigned(v+.5);return 2u*((u+1u)/2u)>30u?30u:2u*((u+1u)/2u);
}
inline constexpr const float* lists[ControlCount]={clamp_list,divergence_list,depth_list,color_list,motion_factor_list,detail_motion_list,point_motion_list,motion_reference_list,depth_absolute_list,max_age_list,compose_after_list,fill_mode_list,
    raw_divergence_list,stabilize_list,grade_exposure_list,grade_contrast_list,grade_saturation_list,motion_strength_list,motion_strength_ref_list,jitter_comp_list,stabilize_still_list,gain_limit_list,soft_guard_list,light_guard_list,
    detail_guard_list,light_smooth_list,halo_reduce_list,ratio_floor_list,late_compose_list,halo_net_scale_list,light_guide_depth_list,halo_excess_list,halo_cut_moving_list};
inline constexpr unsigned counts[ControlCount]={5,6,5,6,6,6,2,4,4,7,2,2,
    4,4,7,7,8,6,4,3,5,4,2,10,
    12,4,4,3,2,2,2,4,3};

inline constexpr unsigned defaults[ControlCount]={0,0,1,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,3,1,0,0,0,0,0,
    1,0,0,0,0,0,0,0,0};
struct Controls {
    std::atomic<unsigned> index[ControlCount];

    std::atomic<bool> seeded{false};

    std::atomic<bool> menu{false};
    Controls() noexcept { for(unsigned i=0;i<ControlCount;++i)index[i].store(defaults[i],std::memory_order_relaxed); }
};
inline Controls controls;

inline std::atomic<float> jitter_atual_x{0.f},jitter_atual_y{0.f};

inline std::atomic<float> near_atual{0.f};
inline std::atomic<bool> jitter_valido{false};
inline std::atomic<unsigned> despejo_pedido{0};
inline std::atomic<unsigned> despejo_gravados{0};

inline std::atomic<unsigned> despejo_final{0};

struct FinalExtras {
    void *motion{},*depth{},*exposure{};
    std::uint32_t motion_state{},depth_state{},exposure_state{};
    float jitter_x{},jitter_y{},mv_x{},mv_y{};
    std::uint32_t render_w{},render_h{},output_w{},output_h{};

    float camera_near{},camera_far{},camera_fov{};
    float expo_rede{-1.f};
    int rj_on{-1};float rj_nivel{},rj_ganho{},rj_cx{},rj_cy{},rj_lado{},rj_dx{},rj_dy{};int rj_rx{},rj_ry{},rj_rl{},rj_composto{},rj_job{};int rj_idade{-1};

    int rjn_on[7]{-1,-1,-1,-1,-1,-1,-1};float rjn_nivel[7]{},rjn_ganho[7]{},rjn_cx[7]{},rjn_cy[7]{},rjn_lado[7]{};int rjn_rx[7]{},rjn_ry[7]{},rjn_rl[7]{},rjn_composto[7]{},rjn_job[7]{};
    bool valido{};
};
inline FinalExtras final_extras{};

inline std::atomic<unsigned> final_gravados{0};
inline std::atomic<int> idade_composta{-1};

inline std::atomic<bool> gravar_aviso{false},gravar_f9{false},final_cheio{false};

inline std::atomic<bool> gravar_tela{false};
inline bool gravar_tela_de(bool presente,double v,bool f9) noexcept {return presente?v==1.:f9;}
inline std::atomic<unsigned> final_pedido{24},final_total{0};

inline constexpr unsigned final_teto_padrao=96,final_teto_max=1024,gravar_quadros_padrao=24,gravar_quadros_max=600;
inline std::atomic<unsigned> gravar_quadros{gravar_quadros_padrao},final_teto_sessao{final_teto_padrao};
inline unsigned gravar_quadros_de(bool presente,double v) noexcept {
    if(!presente||!std::isfinite(v))return gravar_quadros_padrao;
    v=std::round(v);return v<1.?1u:v>double(gravar_quadros_max)?gravar_quadros_max:unsigned(v);
}
inline unsigned gravar_teto_de(unsigned quadros,bool presente,double v) noexcept {
    if(presente&&std::isfinite(v)){v=std::round(v);const double lo=double(quadros);return v<lo?quadros:v>double(final_teto_max)?final_teto_max:unsigned(v);}
    const unsigned t=4u*quadros;return t>final_teto_padrao?(t<final_teto_max?t:final_teto_max):final_teto_padrao;
}
inline std::atomic<std::uint64_t> aviso_cheio_ate_ms{0};

inline bool pedir_gravacao(unsigned n,std::uint64_t agora_ms) noexcept {
 (void)n;(void)agora_ms;return false;
}

inline std::atomic<bool> final_preparo_pedido{false};
inline bool final_quer_preparo() noexcept {return final_preparo_pedido.load(std::memory_order_relaxed);}

inline std::atomic<void*> base_recurso{nullptr};
inline std::atomic<unsigned> base_fatia{0};

inline std::atomic<void*> diag_cena_recurso{nullptr},diag_warp_recurso{nullptr},diag_realce_recurso{nullptr},diag_rostos_recurso{nullptr};
inline std::atomic<float> escala_desejada{0.f};
inline std::atomic<float> escala_nativa{0.f};
inline float multiplicador_escala(){const float d=escala_desejada.load(std::memory_order_relaxed),n=escala_nativa.load(std::memory_order_relaxed);
    if(!(d>0.f)||!(n>0.f)||!std::isfinite(d)||!std::isfinite(n))return 1.f;const float m=d/n;return m<=16.f?m:16.f;}

inline std::atomic<unsigned> grade_estilo{0};
inline std::atomic<float> grade_tom{1.f};
inline std::atomic<bool> grade_semeado{false};

inline float tom_de_config(unsigned n,float local_tone){return n==0?local_tone:float(n-1)*.25f;}

inline void somar_grade_do_estilo(unsigned estilo,float tom,float& ev,float& contraste,float& saturacao){
    float t=std::isfinite(tom)?tom:1.f;if(!(t>0.f))t=0.f;if(t>1.f)t=1.f;
    const float sev=estilo==1?-.1f*t:0.f,sco=estilo==1?-.25f*t:0.f,ssa=estilo==1?-.1f*t:estilo==2?-.15f*t:0.f;
    ev+=sev;contraste+=sco;saturacao=(1.f+saturacao)*(1.f+ssa)-1.f;}
inline unsigned index_of(const Controls& s,unsigned control) {
    if(control>=ControlCount)return 0;
    const unsigned i=s.index[control].load(std::memory_order_acquire);
    return i<counts[control] ? i : defaults[control];
}
inline float value(const Controls& s,unsigned control) {
    return control<ControlCount ? lists[control][index_of(s,control)] : 0.f;
}
inline bool set(Controls& s,unsigned control,unsigned index) {
    if(control>=ControlCount || index>=counts[control])return false;
    s.index[control].store(index,std::memory_order_release);return true;
}

inline unsigned nearest(unsigned control,float requested) {
    if(control>=ControlCount || !std::isfinite(requested))return 0;
    unsigned best=0;float distance=std::fabs(lists[control][0]-requested);
    for(unsigned i=1;i<counts[control];++i) {
        const float candidate=std::fabs(lists[control][i]-requested);
        if(candidate<distance) {best=i;distance=candidate;}
    }
    return best;
}

inline bool seed(Controls& s,const float (&values)[ControlCount]) {
    if(s.seeded.exchange(true,std::memory_order_acq_rel))return false;
    for(unsigned i=0;i<ControlCount;++i)set(s,i,nearest(i,values[i]));
    return true;
}
}
