#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
#include "rosto_npc_canon.h"

namespace nrrj {
inline constexpr int npc_det_max=7;

struct Parametros {
    int q=4;
    float borda_ln=.08f;
    int area_min=120;
    float d_min=.012f;
    float k_face=1550.f;
    float pos_x=.30f,pos_y=.40f,sig_x=.20f,sig_y=.17f;
    float liga=.55f,desliga=.35f,aceita=.30f;

    float liga_gen=.80f;
    int perde_n=15;
    float escala_max=2600.f;
    int liga_n=2,desliga_n=4;
    float corte_ok=.25f;int corte_seg=6;

    bool corte_multi=false;

    int corte_modo=0;

    int corte_ref_jobs=8;float corte_ref_dist=.75f,corte_ref_tam=1.35f,corte_tend=.60f;int corte_bloco=16;
    float corte_nota_forte=.85f;
    float corte_nota_ref=.70f;
    float corte_nota=.55f;
    float cab_max_4k=760.f;
    bool cinema_livre=false;

    bool persistente=false;int desliga_n_pers=12;

    int recorte_fixo=0;float recorte_fixo_margem=1.2f;int recorte_forcado=0;

    float corte_ref_tam_forte=1.6f,corte_ref_dist_forte=1.0f;
    float alfa_pos=.6f,alfa_tam=.25f;
    int recorte_min=640,recorte_max=1024;
    float zona_morta=.12f;

    bool generico=true;

    float conv_med=.5f;

    int busca_apos=60,busca_passos=6,busca_hits=2;

    float sinal_mov=.5f,sinal_dif=.15f;int sinal_n=6;

    float pos_centro=.50f;bool usa_centro=true;

    float pos_direita=.70f;bool usa_direita=true;

    bool usa_rival=true;float rival_frac=.85f,rival_dist=1.5f,rival_min=.30f,rival_tam=.80f;

    bool npc=false;int npc_min=160,npc_recorte=512;float npc_expande=.8f,npc_nota=.15f,npc_alfa=.2f,npc_troca_fator=1.5f;
    float npc_liga_s=1.f,npc_desliga_s=.25f,npc_fica_s=1.5f;int npc_troca_n=8,npc_segura=3;

    bool perto=false;float perto_cab4k=540.f,perto_max_s=3.f,perto_ref_dist=1.f,perto_ref_tam=1.6f;

    int npc_modo=0;float rosto_nota=.7f,rosto_baixa=.5f,rosto_cab=1.6f,rosto_dy=.15f,rosto_sem_s=.5f,rosto_g_min=200.f;int rosto_ref_jobs=60;
    float rosto_idade_max=.3f,rosto_npc_some_s=.5f;int rosto_volta_n=2;

    float rosto_dela=.35f;

    float npc_segura_s=1.5f;

    float npc_nota_seg=.4f,npc_alfa_c=.5f,npc_alfa_prof=.15f,npc_salto=.25f,npc_tam_seg=.5f;
    float npc_repoe_tam=1.3f,npc_repoe_dist=1.5f;

    float rosto_sem_npc=1.f,rosto_conf=.6f,npc_perde_s=1.5f,npc_espera_s=3.f;int rosto_conf_n=3;

    bool npc2=false;int npc2_conf_n=2;float npc2_ausente_s=1.5f,npc2_espera_s=3.f,npc2_salto=.5f;

    float npc2_alfa=.3f,npc2_pulo=.4f,npc2_centro=.6f,npc2_nota_seg=.35f,npc2_nota_novo=.7f,npc2_segura_s=1.f,npc2_min_s=2.f;

    float npc2_vel_max=0.f,npc2_rapido_s=1.f;

    float npc2_zona=1.15f,npc2_troca_s=.5f;

    float npc2_faixa=.15f,npc2_salto_roi=.25f,npc2_alfa_roi=.5f,npc2_nota_rapida=.75f,npc2_reancora=2.f;

    float npc2_ocl_faixa=.3f,npc2_ocl_frac=.3f,npc2_ocl_max_s=4.f,npc2_vel_ref=.5f;
    float npc2_vel_novo=1.5f,npc2_nota_salto=.6f,npc2_cand_falta_s=.5f,npc2_mv_max=1.5f;

    int npc_max=3;float npc2_cab_min=96.f;int npc2_recorte_min=640;float npc2_dup=.5f;

    int npc_degraus=0;float npc2_segura_peq=0.f,npc2_peq_cab=160.f;

    int npc2_recorte_forcado=0;

    float npc_fracao=0.f;int npc_frac_teto=1024,npc_amplia=0;

    int npc_peito=0;

    int npc_frente=0;

    int npc_roi_conf=0;float npc_roi_veto_s=2.f,npc_roi_veto_raio=.75f;bool npc_roi_veto_segue=true;

    int npc_vel_img=0;

    int npc_atras=0;float npc_atras_tam=.6f,npc_atras_prof=1.45f,npc_atras_cob=.25f;

    int npc_corpo=0;float npc_corpo_dy=.5f,npc_corpo_dx=2.f,npc_corpo_prof=1.3f;

    int npc_camada=0;float npc_camada_vis=.35f,npc_camada_margem=.3f;

    int npc_vida=0;float vida_in_s=.35f;int vida_conf_n=2,vida_roi_n=1;float vida_borda=.25f,vida_re_s=1.f,vida_re_dist=1.f;

    float vida2_in_s=.15f,vida2_cam_zona=1.6f,vida2_nota=.75f;
    int npc_leve=0;float npc_leve_salto=.6f;

    int npc_todos=0;float todos_nota=.8f;int todos_perto=0,todos_perto_l=600,todos_mig_l=681;float todos_recente=.25f,todos_fica=.5f,todos_cab_max=700.f,todos_fora_s=.5f,todos_prof_s=2.f,todos_perde_s=8.f;
    int npc_canon=0;nrcanon::Param canon{};
    float npc2_conf_s=0.f;

    float g_rosto_dist=1.f;

    int giro_rosto_jobs=4;float giro_rosto_dist=2.f;
    int adquire=0;float adq_min=280.f,adq_unico=.8f,adq_corpo=.7f,adq_corpo_frac=.7f,adq_nota1=.85f,adq_cab1=400.f,adq_dist=.5f,adq_esc_min=.71f,adq_esc_max=1.41f,adq_esc_vies=1.33f,giro_ok_ant=.5f;

    int reancora=0,rea_jobs=2;float rea_prox=.9f;

    int caminha_dela=0;float cd_tol=1.25f;

};

struct RostoDet {float x,y,w,h,nota;float lm[10]{};float px=0.f;};

struct Grades {
    int W=0,H=0;
    const float* dq=nullptr;
    const float* mvq=nullptr;
    int w=0,h=0;
    const float* d=nullptr;
    float k=1.f;
    float esc=3.f;
    float saida_h=2160.f;
    double t=-1.0;

    const RostoDet* rostos=nullptr;int n_rostos=0;std::uint64_t rostos_versao=0;float rostos_idade=0.f;double rostos_t=-1.0;
    const RostoDet* roi[npc_det_max]{};int n_roi[npc_det_max]{};std::uint64_t roi_versao[npc_det_max]{};float roi_idade[npc_det_max]{};double roi_t[npc_det_max]{-1.0,-1.0,-1.0,-1.0,-1.0,-1.0,-1.0};
};

struct Resultado {
    bool on=false,corte=false,grande=false,cinema=false,tem_est=false,tem_recorte=false,rec_mov=false;
    float conf=0.f,ok=1.f;
    float est[3]{};
    int recorte[3]{};
    bool rec_f_tem=false;float rec_f[2]{};
    float cab4k=0.f,escala=0.f;int perdido=0,ncand=0;

    bool recontou=false;
    float tend=-1.f;
    bool conv_errada=false,busca=false,adotou=false,duvida=false;float rival=0.f,rival_xy[3]{},nb_melhor=0.f;bool direita=false;int votos=0;unsigned motivo_off=0;float kface=0.f,med_prox=0.f,ok_neg=0.f;int sinal=1,hip=0;

    bool alvo_npc{},troca_alvo{},npc_ativo{};float npc_expande{},npc_bruto{},npc_cab4k{};int npc_celulas{};

    bool perto{},perto_inicio{},perto_fim{};unsigned perto_jobs{},perto_motivo{};float perto_escala{1.f};

    bool rf_novo{},rf_vis{},rf_g{};int rf_n{};float rf_nota{},rf_sem{};

    int rf_motivo{};float rf_npc_nota{},rf_idade_s{};
    bool tem_camada{};float camada[2]{};

    int mig_de{-1},mig_para{-1};
    bool vida_pend{},vida_sai{},vida_pub_novo{},vida_era_pub{},vida_re{},vida_oculto{},vida_oculto_novo{},vida_volta{},vida_camq{};

    bool adquiriu{},escala_conf{},adocao_vetada{},lista_velha{},giro{},giro_rosto{},reancorou{};float cd_dbg[10]{};
    float dg[7]{};int g_motivo{},adq_n{};float g_dmin{-1.f},kf_razao{};
};

namespace det {

inline float mediana(std::vector<float> v){
    if(v.empty())return NAN;
    const size_t n=v.size(),m=n/2;
    std::nth_element(v.begin(),v.begin()+std::ptrdiff_t(m),v.end());
    if(n&1u)return v[m];
    const float a=v[m];const float b=*std::max_element(v.begin(),v.begin()+std::ptrdiff_t(m));
    return .5f*(a+b);
}

inline int arred(double x){return int(std::nearbyint(x));}
inline float clip01(float v){return v<0.f?0.f:(v>1.f?1.f:v);}

inline int npc_lado_fracao(int lado,double pede,float f0,int teto_rede,int rmax,int rede_min=0){
    const int rmin=rede_min>0?256:(std::min)(384,teto_rede),cabe=(std::min)(int(std::ceil(1.5*pede/128.0))*128,rmax);
    if(lado>0&&lado>=cabe&&pede>=.8*f0*lado&&pede<=1.3*f0*lado)return lado;
    const double alvo=pede/f0;
    int L=rede_min>0&&alvo<rede_min-16?int(std::lround(alvo/32.0))*32:int(std::lround(alvo/128.0))*128;
    L=(std::clamp)(L,rmin,(std::max)(rmin,teto_rede));
    return (std::max)(L,cabe);
}

struct Comp {int x0,y0,x1,y1,area;};
inline int rotular(const unsigned char* m,int W,int H,bool oito,std::vector<int>& lab,std::vector<Comp>& comps){
    lab.assign(size_t(W)*H,0);comps.clear();std::vector<int> pilha;
    for(int y=0;y<H;++y)for(int x=0;x<W;++x){
        const size_t i=size_t(y)*W+x;if(!m[i]||lab[i])continue;
        const int id=int(comps.size())+1;Comp c{x,y,x,y,0};pilha.assign(1,int(i));lab[i]=id;
        while(!pilha.empty()){
            const int k=pilha.back();pilha.pop_back();const int kx=k%W,ky=k/W;++c.area;
            c.x0=(std::min)(c.x0,kx);c.y0=(std::min)(c.y0,ky);c.x1=(std::max)(c.x1,kx);c.y1=(std::max)(c.y1,ky);
            for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){
                if(!dx&&!dy)continue;if(!oito&&dx&&dy)continue;
                const int nx=kx+dx,ny=ky+dy;if(nx<0||ny<0||nx>=W||ny>=H)continue;
                const size_t j=size_t(ny)*W+nx;if(!m[j]||lab[j])continue;lab[j]=id;pilha.push_back(int(j));
            }
        }
        comps.push_back(c);
    }
    return int(comps.size());
}

inline void runs_cabeca(const unsigned char* m,int W,int H,double x0,int y0,int nrows,std::vector<float>& larg,std::vector<float>& cent){
    larg.clear();cent.clear();double xc=x0;
    for(int y=y0;y<(std::min)(H,y0+nrows);++y){
        if(y<0)continue;
        const unsigned char* row=m+size_t(y)*W;
        int xi=arred(xc);
        if(xi<0||xi>=W||!row[xi]){
            int j=-1;double melhor=1e30;
            for(int x=0;x<W;++x)if(row[x]){const double dd=std::fabs(x-xc);if(dd<melhor){melhor=dd;j=x;}}
            if(j<0){larg.push_back(0.f);cent.push_back(float(xc));continue;}
            if(std::fabs(j-xc)>3){larg.push_back(0.f);cent.push_back(float(xc));continue;}
            xi=j;
        }
        int a=xi;while(a>0&&row[a-1])--a;
        int b=xi;while(b<W-1&&row[b+1])++b;
        larg.push_back(float(b-a+1));cent.push_back(float((a+b)/2.0));
        if(b-a+1<3*(std::max)(4.f,larg[0]))xc=.7*xc+.3*(a+b)/2.0;
    }
}
inline float maxv(const std::vector<float>& v,size_t a,size_t b){float m=-1e30f;bool tem=false;for(size_t i=a;i<(std::min)(b,v.size());++i){m=(std::max)(m,v[i]);tem=true;}return tem?m:0.f;}

struct Cand {double x,ytop,wh,topo_oc,alt,wexp,d,omb;int area,bx,by,bw,bh;bool fundo;};
}

class Rastreio {
public:
    Parametros P;

    double hip_x[32]{},hip_y[32]{};

    bool tem_prev=false;double prev_x=0,prev_ytop=0,prev_wh=0;
    bool tem_est=false;double est[3]{};
    bool on=false;int cont_on=0,cont_off=0,seg_corte=0,perdido=0;
    unsigned long long cortes_multi_testes=0,cortes_multi_salvos=0;

    double ref_cab[3]{};int ref_idade=-1;unsigned long long cortes_vetados=0;
    bool tem_recorte=false;int recorte[2]{};int rec_lado=0;
    std::vector<float> lq_ant;int lq_W=0,lq_H=0;
    bool tem_escala=false;double escala=0;double mv_g=0;

    double kf=0;int kf_idx=0,sem_n=0,hip_cursor=0;int hip_hits[32]{};
    int sinal=1,sinal_votos=0;

    double npc_ema=0;bool npc_ativo=false;double npc_liga_t=0,npc_desl_t=0,npc_desde=1e9,npc_t_ant=-1.0;
    bool npc_tem=false;double npc_est[3]{};int npc_perdido=0,npc_troca_c=0;unsigned npc_id=0;
    bool npc_tem_rec=false;int npc_rec[2]{};bool npc_alvo_ant=false;unsigned npc_id_ant=0;
    bool perto_seg=false;double perto_seg_t=0,perto_t_ant=-1.0;unsigned perto_jobs=0,perto_motivo=0;

    std::uint64_t rf_versao=0;bool rf_vis=false;double rf_sem_t=0,rf_t_ant=-1.0;float rf_nota=0;int rf_n=0;
    bool rf_g_tem=false;double rf_g_est[3]{};int rf_g_rec[2]{};bool rf_g_tem_rec=false;int rf_g_lado=0;
    int rf_vis_n=0;double rf_baixo_t=0,rf_desde=1e9,npc_sem_t=0;float npc_nota=0;bool rf_alvo_npc=false;float rf_idade_ult=0;double npc_vis[2]{};int npc_falta_n=0;double npc_med[3]{},npc_tam_t=0;

    struct EstadoAlvo {bool tem_prev=false;double prev_x=0,prev_ytop=0,prev_wh=0;bool tem_est=false;double est[3]{};bool on=false;
        int cont_on=0,cont_off=0,seg_corte=0,perdido=0;double ref_cab[3]{};int ref_idade=-1;bool tem_recorte=false;int recorte[2]{};int rec_lado=0;
        bool tem_escala=false;double escala=0;bool perto_seg=false;double perto_seg_t=0;};
    EstadoAlvo grace_salva;bool trk_npc=false;int troca_pend=0,troca_motivo=0;double pend_est[3]{};float pend_nota=0;
    bool cand_tem=false;double cand_est[3]{};int cand_n=0;float cand_nota=0;double npc_ausente_t=0,npc_perd_t=0;

    struct Npc2Slot {
        bool tem=false;double est[3]{};int rec[2]{};bool tem_rec=false;int lado=0;unsigned id=0;float nota=0;
        double conf_t=0,desde=1e9;
        unsigned long long movs=0,presos=0,jobs=0,medidas=0,rejeitadas=0;
        double yu[3]{};bool yu_tem=false;
        bool solto_corte=false;
        bool salto_id=false;unsigned long long saltos_id=0,roi_dono=0;int mig_n=0;double est_ini[3]{};bool esmaece=false;double fora_t=0,prof_ruim_t=0;
        nrcanon::Trilha cn;double cn_rec[2]{};int cn_lado=0;unsigned long long cn_sem=0;
        double vel=0,rapido_t=0;
        std::uint64_t roi_versao=0;unsigned long long roi_ok=0,roi_falta=0,reancoras=0;
        double prof=0,ocl_t=0;unsigned long long ocl_jobs=0,mv_ruim=0;
        bool visto=false;unsigned long long roi_falta0=0;
        unsigned long long roi_ok0=0;double t_preso=-1.0;
        bool cam_tem=false;double cam[2]{};unsigned long long cam_jobs=0;
        bool vida_pub=false,vida_re=false,vida_oculto=false;int vida_hits=0;unsigned long long vida_ocultos=0;unsigned long long vida_frente=0;
        Resultado saida;
    };
    Npc2Slot n2s[npc_det_max];
    std::uint64_t gy_versao=0;unsigned long long g_rosto_n=0;
    double g_hist[8][3]{};int g_hist_n=0,g_hist_i=0;
    double n2_t_ant=-1.0;std::uint64_t n2_versao=0;unsigned n2_prox_id=0;
    bool n2c_tem=false;double n2c_est[3]{};int n2c_n=0;float n2c_nota=0;double n2c_vel=0,n2c_prof=0,n2c_visto_t=-1,n2c_t0=0,n2c_tl=0;

    double t_corte=-1.0;bool corte_t_pend=false;std::uint64_t adq_versao=0;bool adq_pend=false;double adq_p[3]{};
    double adq_el[8][3]{};int adq_el_n=0;bool adq_job=false,adq_sem=false;int adq_outro_n=0;
    unsigned long long adquiriu_n=0,promovido_n=0,escala_conf_n=0,adocoes_vetadas=0,giros=0;double ok_ant=1.0;
    int giro_desde=999;unsigned long long giro_rosto_n=0;
    int rea_off=0;bool rea_pend=false;double rea_p[3]{};unsigned long long reancorou_n=0;
    double cd_z=0;bool cd_tem=false,cd_dela_ant=false;unsigned long long cd_parou_n=0,cd_valeu_n=0;
    double vida_mem[8][5]{};int vida_mem_n=0;double vida_relogio=0;unsigned long long vida_re_n=0;
    double dbg_prof_ela=0;
    double dir_c[8][6]{};int dir_n=0;unsigned long long diretos_n=0,migrou_n=0,dup_desfeito_n=0,esmaeceu_n=0;
    double veto_roi[8][7]{};int veto_roi_n=0;double n2_relogio=0;unsigned long long roi_conf_soltou_n=0,roi_veto_n=0;
    std::vector<std::array<double,6>> dbg_cs;
    unsigned long long sem_lugar_n=0;

    void reiniciar(){*this=Rastreio{P};}
    explicit Rastreio(const Parametros& p=Parametros{}):P(p){}

    static float mediana_amostra(const float* d,size_t n){
        if(!d||!n)return 0.f;const size_t passo=(std::max<size_t>)(1,n/4096);std::vector<float> v;v.reserve(n/passo+1);
        for(size_t i=0;i<n;i+=passo)v.push_back(d[i]);return det::mediana(v);
    }
    Resultado passo(const Grades& g0){
        Resultado R{};
        const float LIGA=P.generico?P.liga_gen:P.liga;
        const int W=g0.W,H=g0.H,q=P.q;
        if(kf<=0)kf=P.k_face;
        R.kface=float(kf);R.sinal=sinal;R.hip=kf_idx;
        if(corte_t_pend){t_corte=g0.t;corte_t_pend=false;}
        adq_job=false;

        {static const double* sem=[]{static double v[3]{};char b[64]{};size_t n=0;
            if(getenv_s(&n,b,sizeof b,"NR_RJ_SEMENTE")==0&&n>1&&sscanf_s(b,"%lf,%lf,%lf",&v[0],&v[1],&v[2])==3&&v[2]>=32)return (const double*)v;return (const double*)nullptr;}();
         static bool semeado=false;
         if(sem&&!semeado&&g0.dq){semeado=true;const double e4=double(g0.esc)*(2160.0/(std::max)(1.f,g0.saida_h));
             est[0]=sem[0]/e4;est[1]=sem[1]/e4;est[2]=sem[2]/e4;tem_est=true;on=false;tem_prev=true;prev_x=est[0]/P.q;prev_wh=est[2]/1.25/P.q;prev_ytop=(est[1]-.5*est[2])/P.q;
             ref_cab[0]=est[0];ref_cab[1]=est[1];ref_cab[2]=est[2];ref_idade=0;}}
        if(!g0.dq||!g0.mvq||!g0.d||W<8||H<8||g0.w<W||g0.h<H){R.corte=true;reset_corte();R.motivo_off=1;return R;}

        if(P.generico){
            R.med_prox=mediana_amostra(g0.d,size_t(g0.w)*size_t(g0.h));
            if(R.med_prox>P.conv_med){R.conv_errada=true;reset_corte();R.motivo_off=64;return R;}
        }

        std::vector<float> mvs;Grades g=g0;
        if(P.generico&&sinal<0){mvs.assign(g0.mvq,g0.mvq+size_t(W)*H*2);for(auto& v:mvs)v=-v;g.mvq=mvs.data();}
        const double sh=double(g.h)/720.0;
        const double kface=(P.generico?kf:double(P.k_face))*sh;
        const float dmin=P.generico?float(double(P.d_min)*P.k_face/kf):P.d_min;
        std::vector<float> lq(size_t(W)*H);
        for(size_t i=0;i<lq.size();++i)lq[i]=std::log((std::max)(g.dq[i],1e-6f));
        {std::vector<float> mags(size_t(W)*H);for(size_t i=0;i<mags.size();++i)mags[i]=std::hypot(g.mvq[2*i],g.mvq[2*i+1]);mv_g=det::mediana(mags);}

        double perto_dt=0;bool perto_ok=false,perto_expirou=false;

        const bool modo_rosto=P.npc&&P.npc_modo==1;
        if(P.perto||modo_rosto){
            perto_dt=(g.t>=0&&perto_t_ant>=0)?std::clamp(g.t-perto_t_ant,0.0,.5):double(g.k)/60.0;perto_t_ant=g.t;
            perto_ok=on&&tem_est&&(modo_rosto||est[2]*g.esc*(2160.0/(std::max)(1.f,g.saida_h))>=P.perto_cab4k);
            if(perto_ok&&perto_seg&&perto_seg_t+perto_dt>=P.perto_max_s){perto_ok=false;perto_expirou=true;}
        }

        double ok=1.0,okn=1.0,tend=-1.0;
        if(!lq_ant.empty()&&lq_W==W&&lq_H==H){
            size_t bons=0,bons_n=0;
            for(int y=0;y<H;++y)for(int x=0;x<W;++x){
                const size_t i=size_t(y)*W+x;
                const float sx=float(x)+g.k*g.mvq[2*i],sy=float(y)+g.k*g.mvq[2*i+1];
                int xi=int(std::lround(sx)),yi=int(std::lround(sy));xi=std::clamp(xi,0,W-1);yi=std::clamp(yi,0,H-1);
                if(std::fabs(lq_ant[size_t(yi)*W+xi]-lq[i])<.05f)++bons;
                if(P.generico){
                    int xn=int(std::lround(float(x)-g.k*g.mvq[2*i])),yn=int(std::lround(float(y)-g.k*g.mvq[2*i+1]));xn=std::clamp(xn,0,W-1);yn=std::clamp(yn,0,H-1);
                    if(std::fabs(lq_ant[size_t(yn)*W+xn]-lq[i])<.05f)++bons_n;
                }
            }
            ok=double(bons)/double(lq.size());okn=double(bons_n)/double(lq.size());
            if((P.corte_multi||(P.corte_modo&4))&&ok<P.corte_ok&&g.k>1.f){
                for(const float ks:{(std::max)(1.f,std::floor(g.k*.5f)),1.f}){
                    size_t b2=0;
                    for(int y=0;y<H;++y)for(int x=0;x<W;++x){
                        const size_t i=size_t(y)*W+x;
                        int xi=int(std::lround(float(x)+ks*g.mvq[2*i])),yi=int(std::lround(float(y)+ks*g.mvq[2*i+1]));xi=std::clamp(xi,0,W-1);yi=std::clamp(yi,0,H-1);
                        if(std::fabs(lq_ant[size_t(yi)*W+xi]-lq[i])<.05f)++b2;
                    }
                    ok=(std::max)(ok,double(b2)/double(lq.size()));++cortes_multi_testes;
                    if(ok>=P.corte_ok){++cortes_multi_salvos;break;}
                }
            }

            if((P.corte_modo&1)&&ok<P.corte_ok){
                std::vector<float> dd(lq.size());
                for(int y=0;y<H;++y)for(int x=0;x<W;++x){const size_t i=size_t(y)*W+x;
                    const int xi=std::clamp(int(std::lround(float(x)+g.k*g.mvq[2*i])),0,W-1),yi=std::clamp(int(std::lround(float(y)+g.k*g.mvq[2*i+1])),0,H-1);
                    dd[i]=lq[i]-lq_ant[size_t(yi)*W+xi];}
                const int B=(std::max)(2,P.corte_bloco);size_t b=0;std::vector<float> v;
                for(int by=0;by<H;by+=B)for(int bx=0;bx<W;bx+=B){v.clear();
                    for(int y=by;y<(std::min)(H,by+B);++y)for(int x=bx;x<(std::min)(W,bx+B);++x)v.push_back(dd[size_t(y)*W+x]);
                    const float md=det::mediana(v);for(const float u:v)if(std::fabs(u-md)<.05f)++b;}
                tend=double(b)/double(lq.size());
            }

            const bool voto_fraco=P.caminha_dela>=2&&(okn<.6||cd_dela_ant);
            if(P.generico&&mv_g>P.sinal_mov&&!perto_ok&&!modo_rosto){
                if(okn>ok+P.sinal_dif&&!voto_fraco)++sinal_votos;else if(ok>okn+P.sinal_dif)sinal_votos=0;
                if(sinal_votos>=P.sinal_n){sinal=-sinal;sinal_votos=0;reset_corte();R.sinal=sinal;R.ok=float(ok);R.ok_neg=float(okn);R.motivo_off=1;return R;}
            }
        }
        R.ok_neg=float(okn);
        lq_ant=lq;lq_W=W;lq_H=H;
        bool corte=ok<P.corte_ok;
        std::vector<det::Cand> cands;candidatos(g,lq,kface,dmin,cands);

        R.tend=float(tend);
        if(corte&&(P.corte_modo&(1|16))&&ref_idade>=0&&ref_idade<=P.corte_ref_jobs&&!cands.empty()&&
           (((P.corte_modo&1)&&tend>=P.corte_tend)||(P.corte_modo&16))){
            const bool so_forte=!((P.corte_modo&1)&&tend>=P.corte_tend);
            int jv=-1;double nv=-1;std::vector<double> nbv(cands.size(),0.0);
            for(size_t i=0;i<cands.size();++i){(void)pontua(cands[i],W,H,g,kface,false,&nbv[i]);if(nbv[i]>nv){nv=nbv[i];jv=int(i);}}
            if(jv>=0&&nv>=(so_forte?double(P.corte_nota_forte):(std::min)(double(LIGA),double(P.corte_nota)))){
                const auto& c=cands[size_t(jv)];
                bool rival_v=false;
                for(size_t i=0;i<cands.size();++i){if(int(i)==jv)continue;const auto& o=cands[i];
                    const double dist=std::hypot(o.x-c.x,(o.ytop+.5*o.wh)-(c.ytop+.5*c.wh));
                    if(dist>P.rival_dist*(std::max)(o.wh,c.wh)&&o.wh>=P.rival_tam*c.wh&&nbv[i]>=P.rival_min&&nbv[i]>=P.rival_frac*nv)rival_v=true;}
                if(!rival_v&&no_ref(c,g)){corte=false;R.recontou=true;++cortes_vetados;}
            }

            if(corte&&(P.corte_modo&16)){
                for(size_t i=0;i<cands.size();++i)if(nbv[i]>=P.corte_nota_ref&&no_ref(cands[i],g)){corte=false;R.recontou=true;++cortes_vetados;break;}
            }
        }
        bool perto_corte=false;
        if(corte&&perto_ok){corte=false;perto_corte=true;}

        if(corte&&(P.adquire&8)&&on&&tem_est&&tend>=P.corte_tend&&ok_ant<=P.giro_ok_ant){corte=false;R.giro=true;++giros;}
        ok_ant=ok;if(R.giro)giro_desde=0;else if(giro_desde<999)++giro_desde;
        if(corte){tem_prev=false;tem_est=false;seg_corte=(P.corte_modo&2)?0:P.corte_seg;tem_recorte=false;tem_escala=false;
            cd_tem=false;
        }
        if(corte){t_corte=g.t;adq_pend=false;adq_sem=false;adq_outro_n=0;}
        if(modo_rosto)rostos_aplica(g,cands,corte,R);
        int ib=-1;double bs=-1;
        std::vector<double> nb(cands.size(),0.0);
        for(size_t i=0;i<cands.size();++i){const double sc=pontua(cands[i],W,H,g,kface,false,&nb[i]);if(sc>bs){bs=sc;ib=int(i);}}
        bool duvida=false;double rival=0;
        if(P.generico&&P.usa_rival&&ib>=0){
            const auto& cb=cands[size_t(ib)];
            for(size_t i=0;i<cands.size();++i){if(int(i)==ib)continue;const auto& c=cands[i];
                const double dist=std::hypot(c.x-cb.x,(c.ytop+.5*c.wh)-(cb.ytop+.5*cb.wh));
                if(dist<=P.rival_dist*(std::max)(c.wh,cb.wh)||c.wh<P.rival_tam*cb.wh)continue;
                const double ni=nb[i];
                if(ni>rival){rival=ni;R.rival_xy[0]=float(c.x*q);R.rival_xy[1]=float((c.ytop+.5*c.wh)*q);R.rival_xy[2]=float(c.wh*q);}}
            duvida=rival>=P.rival_min&&rival>=P.rival_frac*nb[size_t(ib)];
        }
        R.rival=float(rival);R.duvida=duvida;if(ib>=0)R.nb_melhor=float(nb[size_t(ib)]);
        if(ib>=0){double f[7];(void)pontua(cands[size_t(ib)],W,H,g,kface,false,nullptr,f);for(int i=0;i<7;++i)R.dg[i]=float(f[i]);}
        bool tem_meas=false;double mx=0,mytop=0,mwh=0,md=0;
        double conf=ib>=0?bs:0.0;

        const double LIGA2=((P.corte_modo&2)&&!tem_prev&&ib>=0&&ref_idade>=0&&ref_idade<=P.corte_ref_jobs&&no_ref(cands[size_t(ib)],g))?(std::min)(double(LIGA),double(P.corte_nota)):double(LIGA);
        if(ib>=0&&conf>=(tem_prev?double(P.aceita):LIGA2)){
            double fx,fy,fw;
            if(mede_fino(g,cands[size_t(ib)],fx,fy,fw)&&fw/(std::max)(cands[size_t(ib)].wh*q,1.0)>=.6&&fw/(std::max)(cands[size_t(ib)].wh*q,1.0)<=1.7){
                tem_meas=true;mx=fx;mytop=fy;mwh=fw;md=cands[size_t(ib)].d;}
        }

        bool tem_pred=false;double pred[3]{};
        if(tem_est){
            const int gx=std::clamp(int(est[0])/q,0,W-1),gy=std::clamp(int(est[1])/q,0,H-1);

            double sx=0,sy=0;int n=0;
            for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){const int xx=gx+dx,yy=gy+dy;if(xx<0||yy<0||xx>=W||yy>=H)continue;const size_t i=size_t(yy)*W+xx;sx+=g.mvq[2*i];sy+=g.mvq[2*i+1];++n;}
            double mvx=n?sx/n*q*g.k:0,mvy=n?sy/n*q*g.k:0;
            if((P.caminha_dela>=1)&&cd_tem&&!trk_npc){
                double ex=0,ey=0;int ne=0;const double zl=cd_z/double(P.cd_tol),zh=cd_z*double(P.cd_tol);
                for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){const int xx=gx+dx,yy=gy+dy;if(xx<0||yy<0||xx>=W||yy>=H)continue;const size_t i=size_t(yy)*W+xx;
                    const double z=g.dq[i];if(z>=zl&&z<=zh){ex+=g.mvq[2*i];ey+=g.mvq[2*i+1];++ne;}}
                mvx=ne?ex/ne*q*g.k:0;mvy=ne?ey/ne*q*g.k:0;R.cd_dbg[8]=float(ne);if(!ne)++cd_parou_n;}
            pred[0]=est[0]-mvx;pred[1]=est[1]-mvy;pred[2]=est[2];tem_pred=true;
        }
        double cx=0,cy=0,lado=0,salto=0;bool tem_caixa=false;[[maybe_unused]] bool cd_dela=false;
        if(tem_meas){R.cd_dbg[0]=float(mx);R.cd_dbg[1]=float(mytop+.625*mwh);R.cd_dbg[2]=float(1.25*mwh);}
        if(tem_pred){R.cd_dbg[3]=float(pred[0]);R.cd_dbg[4]=float(pred[1]);}R.cd_dbg[6]=float(conf);
        if(tem_meas){
            lado=1.25*mwh;cx=mx;cy=mytop+.5*lado;tem_caixa=true;
            if((P.caminha_dela>=1)&&cd_tem&&!trk_npc&&tem_pred&&tem_est){
                const double zm=prox_rosto(g,cx,cy,lado),t=lado/(std::max)(est[2],1.0);
                cd_dela=zm>=cd_z/double(P.cd_tol)&&zm<=cd_z*double(P.cd_tol)&&t>=.75&&t<=1.0/.75;
                if(cd_dela&&
                   std::hypot(cx-est[0],cy-est[1])<std::hypot(cx-pred[0],cy-pred[1])){pred[0]=est[0];pred[1]=est[1];R.cd_dbg[9]=1.f;++cd_valeu_n;}}
            if(tem_pred){
                salto=std::hypot(cx-pred[0],cy-pred[1])/(std::max)(pred[2],1.0);
                if(salto>=.6&&conf<LIGA&&perdido<P.perde_n)tem_meas=false;
            }
        }

        bool perto_hold=false;
        if(perto_ok&&tem_pred){
            if(perto_seg&&tem_meas){const double dd=salto,t=lado/(std::max)(pred[2],1.0);
                if(dd>P.perto_ref_dist||t>P.perto_ref_tam||t*P.perto_ref_tam<1.0)tem_meas=false;}
            perto_hold=perto_corte||!tem_meas||(sinal_votos>0&&!modo_rosto);
            if(!tem_meas){const double e=perto_escala(g,pred);R.perto_escala=float(e);
                pred[2]=(std::min)(pred[2]*e,double((std::min)(g.w,g.h)));}
        }

        bool g_rosto=false;
        if(P.npc2&&!modo_rosto&&!tem_meas&&g.rostos&&g.rostos_versao==gy_versao)R.g_motivo=1;
        if(P.npc2&&!modo_rosto&&!tem_meas&&g.rostos&&g.rostos_versao!=gy_versao){
            gy_versao=g.rostos_versao;
            const double idade_s=(g.t>=0&&g.rostos_t>=0)?g.t-g.rostos_t:double(g.rostos_idade)/60.0;
            const double* ref=tem_pred?pred:((ref_idade>=0&&ref_idade<=P.corte_ref_jobs)?ref_cab:nullptr);
            R.g_motivo=!ref?2:corte?3:idade_s>P.rosto_idade_max?4:5;
            if(ref&&!corte&&idade_s<=P.rosto_idade_max){
                const double esc=(std::max)(double(g.esc),1e-3),idade=std::clamp(double(g.rostos_idade),0.0,30.0);double dm=1e30,gdm=1e30;
                for(int i=0;i<g.n_rostos;++i){const auto& f=g.rostos[i];if(!(f.nota>=P.rosto_nota)||!(f.h>0.f))continue;
                    double fx=(f.x+.5*f.w)/esc,fy=(f.y+.5*f.h)/esc;const double fh=f.h/esc;
                    double vx,vy;mv_em(g,q,fx,fy,vx,vy);fx-=vx*idade;fy-=vy*idade;
                    const double l=double(P.rosto_cab)*fh,y=fy-double(P.rosto_dy)*fh,r=l/(std::max)(ref[2],1.0),d=std::hypot(fx-ref[0],y-ref[1])/(std::max)(ref[2],1.0);
                    gdm=(std::min)(gdm,d);
                    if(d<=P.g_rosto_dist&&r>=.6&&r<=1.6&&d<dm){dm=d;cx=fx;cy=y;lado=l;}}
                if(gdm<1e29)R.g_dmin=float(gdm);
                if(dm<1e29){g_rosto=true;tem_meas=true;tem_caixa=true;mwh=lado/1.25;mx=cx;mytop=cy-.5*lado;R.g_motivo=8;
                    salto=tem_pred?std::hypot(cx-pred[0],cy-pred[1])/(std::max)(pred[2],1.0):1.0;conf=(std::max)(conf,double(LIGA));++g_rosto_n;}}
        }

        bool adq=false,escala_rosto=false,adq_lista_nova=false;
        if(P.adquire&&P.npc2&&!modo_rosto&&g.rostos&&g.rostos_versao!=adq_versao){
            adq_versao=g.rostos_versao;adq_el_n=0;
            const double idade_s=(g.t>=0&&g.rostos_t>=0)?g.t-g.rostos_t:double(g.rostos_idade)/60.0;
            const bool velha=t_corte>=0&&g.rostos_t>=0&&g.rostos_t<=t_corte;R.lista_velha=velha;
            if(idade_s<=P.rosto_idade_max){
                const double esc=(std::max)(double(g.esc),1e-3),idade=std::clamp(double(g.rostos_idade),0.0,30.0),e4=g.esc*(2160.0/(std::max)(1.f,g.saida_h));
                double bx=0,by=0,bl=0,bp=-1;float bn=0;double bh=0;
                for(int i=0;i<g.n_rostos;++i){const auto& f=g.rostos[i];if(!(f.nota>=P.rosto_nota)||!(f.h>0.f))continue;
                    double fx=(f.x+.5*f.w)/esc,fy=(f.y+.5*f.h)/esc;const double fh=f.h/esc,l=double(P.rosto_cab)*fh;
                    if(l*e4<P.adq_min)continue;
                    double vx,vy;mv_em(g,q,fx,fy,vx,vy);fx-=vx*idade;fy-=vy*idade;const double y=fy-double(P.rosto_dy)*fh;
                    bool unico=true;for(int j=0;j<g.n_rostos&&unico;++j)if(j!=i&&g.rostos[j].nota>=P.rosto_baixa&&g.rostos[j].h>=P.adq_unico*f.h)unico=false;
                    const double pr=prox_rosto(g,fx,y,l);if(!unico||!(pr>0)||!corpo_abaixo(g,fx,y,l,pr))continue;
                    if(adq_el_n<8){adq_el[adq_el_n][0]=fx;adq_el[adq_el_n][1]=y;adq_el[adq_el_n][2]=l;++adq_el_n;}
                    if(pr>bp){bp=pr;bx=fx;by=y;bl=l;bn=f.nota;bh=fh;}}
                R.adq_n=adq_el_n;(void)bh;adq_lista_nova=true;
                if(bp>0){
                    const double kfr=(bl/1.25)/(bp*sh);R.kf_razao=float(kfr/(std::max)(kf,1e-6));
                    {const float rz=R.kf_razao/(std::max)(P.adq_esc_vies,1e-3f);escala_rosto=(P.adquire&4)&&rz>=P.adq_esc_min&&rz<=P.adq_esc_max;}

                    if((P.adquire&1)&&!tem_est&&!tem_meas&&!corte&&!velha){
                        const bool forte=bn>=P.adq_nota1&&bl*e4>=P.adq_cab1;
                        const bool confirma=adq_pend&&std::hypot(bx-adq_p[0],by-adq_p[1])<=P.adq_dist*(std::max)(bl,1.0)&&bl/(std::max)(adq_p[2],1.0)>=.67&&bl/(std::max)(adq_p[2],1.0)<=1.5;
                        if(forte||confirma){adq=true;adq_pend=false;cx=bx;cy=by;lado=bl;tem_meas=true;tem_caixa=true;mwh=lado/1.25;mx=cx;mytop=cy-.5*lado;
                            salto=1.0;conf=(std::max)(conf,double(LIGA));adq_sem=true;adq_outro_n=0;adq_job=true;++adquiriu_n;R.adquiriu=true;
                            if(P.corte_modo&2)seg_corte=0;}
                        else{adq_pend=true;adq_p[0]=bx;adq_p[1]=by;adq_p[2]=bl;}
                    }else if(tem_est)adq_pend=false;

                    const double rzr=double(R.kf_razao)/(std::max)(double(P.adq_esc_vies),1e-3);
                    const bool cega=!(rzr>=P.adq_esc_min&&rzr<=P.adq_esc_max);
                    if((P.reancora&1)&&tem_est&&!on&&rea_off>=P.rea_jobs&&!corte&&!velha&&!adq&&!(tem_meas&&conf>=LIGA)&&(cega||bp>=double(P.rea_prox)*prox_rosto(g,est[0],est[1],est[2]))){
                        const bool forte=bn>=P.adq_nota1&&bl*e4>=P.adq_cab1;
                        const bool confirma=rea_pend&&std::hypot(bx-rea_p[0],by-rea_p[1])<=P.adq_dist*(std::max)(bl,1.0)&&bl/(std::max)(rea_p[2],1.0)>=.67&&bl/(std::max)(rea_p[2],1.0)<=1.5;
                        if(forte||confirma){adq=true;rea_pend=false;cx=bx;cy=by;lado=bl;tem_meas=true;tem_caixa=true;mwh=lado/1.25;mx=cx;mytop=cy-.5*lado;g_rosto=false;
                            salto=1.0;conf=(std::max)(conf,double(LIGA));adq_sem=true;adq_outro_n=0;adq_job=true;++reancorou_n;R.reancorou=true;
                            if(P.corte_modo&2)seg_corte=0;}
                        else{rea_pend=true;rea_p[0]=bx;rea_p[1]=by;rea_p[2]=bl;}
                    }else rea_pend=false;
                }else{adq_pend=false;rea_pend=false;}
            }
        }else if(P.adquire&&adq_pend){double vx,vy;mv_em(g,q,adq_p[0],adq_p[1],vx,vy);adq_p[0]-=vx*g.k;adq_p[1]-=vy*g.k;}

        if((P.adquire&1)&&adq_sem&&!adq&&tem_est){
            if(tem_meas&&!g_rosto&&salto<.6)adq_sem=false;
            else{int jn=-1;double nn=-1;for(size_t i=0;i<cands.size();++i)if(nb[i]>nn){nn=nb[i];jn=int(i);}
                bool outro=false;
                if(jn>=0&&nn>=LIGA&&!duvida){const auto& c=cands[size_t(jn)];const double cxx=c.x*q,cyy=(c.ytop+.5*c.wh)*q;
                    outro=std::hypot(cxx-est[0],cyy-est[1])>(std::max)(est[2],1.0);
                    double fx,fy,fw;
                    if(outro&&++adq_outro_n>=P.liga_n&&mede_fino(g,c,fx,fy,fw)&&fw/(std::max)(c.wh*q,1.0)>=.6&&fw/(std::max)(c.wh*q,1.0)<=1.7){
                        tem_meas=true;g_rosto=false;tem_pred=false;mx=fx;mytop=fy;mwh=fw;md=c.d;lado=1.25*mwh;cx=mx;cy=mytop+.5*lado;tem_caixa=true;salto=1.0;
                        conf=(std::max)(conf,nn);adq_sem=false;adq_outro_n=0;}}
                if(!outro)adq_outro_n=0;}
        }

        if((P.adquire&16)&&adq_lista_nova&&!tem_meas&&tem_pred&&giro_desde<=P.giro_rosto_jobs&&adq_el_n==1){
            const double l=adq_el[0][2],d=std::hypot(adq_el[0][0]-pred[0],adq_el[0][1]-pred[1])/(std::max)(pred[2],1.0),r=l/(std::max)(pred[2],1.0);
            if(d<=P.giro_rosto_dist&&r>=.6&&r<=1.6){cx=adq_el[0][0];cy=adq_el[0][1];lado=l;g_rosto=true;tem_meas=true;tem_caixa=true;mwh=lado/1.25;mx=cx;mytop=cy-.5*lado;
                salto=d;conf=(std::max)(conf,double(LIGA));++giro_rosto_n;R.giro_rosto=true;}}
        R.cd_dbg[5]=float(salto);R.cd_dbg[7]=tem_meas?1.f:0.f;
        if(tem_meas){
            tem_prev=true;prev_x=mx/q;prev_ytop=mytop/q;prev_wh=mwh/q;perdido=0;
            if(tem_pred&&salto<.6){const double a=P.alfa_pos,at=P.alfa_tam;
                est[0]=pred[0]+a*(cx-pred[0]);est[1]=pred[1]+a*(cy-pred[1]);est[2]=pred[2]+at*(lado-pred[2]);}
            else{est[0]=cx;est[1]=cy;est[2]=lado;}
            tem_est=true;
            if(P.caminha_dela>=1){cd_z=prox_rosto(g,est[0],est[1],est[2]);cd_tem=cd_z>0&&!trk_npc;}
            cd_dela_ant=cd_dela;
        }else{
            cd_dela_ant=false;
            ++perdido;
            if(tem_pred&&(perdido<=P.perde_n||perto_hold)){
                est[0]=pred[0];est[1]=pred[1];est[2]=pred[2];tem_est=true;
                if(tem_prev){prev_x=pred[0]/q;prev_ytop=(pred[1]-.5*pred[2])/q;}
            }else{tem_est=false;tem_prev=false;tem_escala=false;}
        }
        const double esc4k=g.esc*(2160.0/(std::max)(1.f,g.saida_h));
        const double cab4k=tem_est?est[2]*esc4k:0.0;
        const bool grande=!P.persistente&&cab4k>P.cab_max_4k;
        double esc_m=0;bool tem_esc_m=false;
        if(tem_meas&&!g_rosto&&!adq){esc_m=mwh/(std::max)(md,1e-6)/sh;tem_esc_m=true;
            if(tem_escala)escala=.8*escala+.2*esc_m;else{escala=esc_m;tem_escala=true;}}

        const double fcin=P.generico?((P.corte_modo&8)?(std::max)(1.0,kf/P.k_face):kf/P.k_face):1.0;
        const bool cinema=tem_escala&&escala>double(P.escala_max)*fcin;
        if(seg_corte>0)--seg_corte;
        const bool fora_img=P.generico&&tem_est&&(est[0]<0||est[1]<0||est[0]>=g.w||est[1]>=g.h);

        const bool bloqueio=corte||grande||(cinema&&!P.cinema_livre)||seg_corte>0||!tem_est||(fora_img&&!(P.persistente&&on))||(P.generico&&sinal_votos>0&&!perto_hold&&!modo_rosto&&!(P.caminha_dela>=2&&cd_dela&&tem_meas));
        if(bloqueio){on=false;cont_on=0;}
        else if(on&&perto_hold)cont_off=0;
        else if(on){cont_off=(!tem_meas||conf<P.desliga)?cont_off+1:0;if(cont_off>=(P.persistente?P.desliga_n_pers:P.desliga_n)){on=false;cont_on=0;}}
        else{cont_on=(tem_meas&&conf>=LIGA2&&(!duvida||modo_rosto))?cont_on+1:0;
            const bool volta=P.persistente&&tem_meas&&ib>=0&&ref_idade>=0&&ref_idade<=P.corte_ref_jobs&&no_ref(cands[size_t(ib)],g);
            if(cont_on>=(volta?1:P.liga_n)){on=true;cont_off=0;}}
        if((g_rosto||adq)&&!bloqueio&&!on){on=true;cont_off=0;}
        if(P.perto||modo_rosto){
            if(perto_expirou&&!tem_meas){on=false;cont_on=0;}
            const bool seg=perto_hold&&on;
            if(seg){if(!perto_seg){perto_seg=true;perto_seg_t=0;perto_jobs=0;perto_motivo=0;R.perto_inicio=true;}
                perto_seg_t+=perto_dt;++perto_jobs;perto_motivo|=(perto_corte?1u:0u)|(tem_meas?0u:2u)|(sinal_votos>0?4u:0u);}
            else if(perto_seg){perto_seg=false;R.perto_fim=true;}
            R.perto=seg;R.perto_jobs=perto_jobs;R.perto_motivo=perto_motivo;
        }
        if(P.generico){

            if(escala_rosto&&!(on||(tem_meas&&conf>=LIGA))){if(sem_n>0){R.escala_conf=true;++escala_conf_n;}sem_n=0;}else
            if(on||(tem_meas&&conf>=LIGA))sem_n=0;else ++sem_n;
            if(sem_n>=P.busca_apos&&!corte&&P.busca_passos>0){

                const int nh=2*P.busca_passos+1;int idx=(hip_cursor%nh)-P.busca_passos;++hip_cursor;
                if(idx==kf_idx){idx=(hip_cursor%nh)-P.busca_passos;++hip_cursor;}
                {
                    const double ka=double(P.k_face)*std::pow(2.0,.5*idx);
                    std::vector<det::Cand> ca;candidatos(g,lq,ka*sh,float(double(P.d_min)*P.k_face/ka),ca);
                    int jb=-1;double js=-1;for(size_t i=0;i<ca.size();++i){const double sc=pontua(ca[i],W,H,g,ka*sh,true);if(sc>js){js=sc;jb=int(i);}}
                    bool acha=false;double hx=0,hy=0,hl=0;(void)hl;
                    if(jb>=0&&js>=LIGA){double fx,fy,fw;const auto& cb=ca[size_t(jb)];const double cw=(std::max)(cb.wh*q,1.0);
                        acha=mede_fino(g,cb,fx,fy,fw)&&fw/cw>=.6&&fw/cw<=1.7;hx=cb.x/W;hy=(cb.ytop+.6*cb.wh)/H;hl=1.25*fw;
                        const double esc4=g.esc*(2160.0/(std::max)(1.f,g.saida_h));
                        if(acha&&(1.25*fw*esc4>P.cab_max_4k||fw/(std::max)(cb.d,1e-6)/sh>double(P.escala_max)*ka/P.k_face))acha=false;}
                    const size_t ih=size_t(idx+P.busca_passos)&31u;int& hh=hip_hits[ih];
                    if(acha&&hh>0&&std::hypot(hx-hip_x[ih],hy-hip_y[ih])>.15)hh=0;
                    hh=acha?hh+1:0;if(acha){hip_x[ih]=hx;hip_y[ih]=hy;}
                    R.busca=true;R.hip=idx;

                    if(hh>=P.busca_hits&&(P.adquire&4)&&g.rostos){const double esc=(std::max)(double(g.esc),1e-3),ox=hx*W*q,oy=hy*H*q;bool algum=false,bate=false;
                        for(int i=0;i<g.n_rostos;++i){const auto& f=g.rostos[i];if(!(f.nota>=P.rosto_baixa)||!(f.h>0.f))continue;algum=true;
                            if(std::hypot((f.x+.5*f.w)/esc-ox,(f.y+.5*f.h)/esc-oy)<=(std::max)(hl,1.0))bate=true;}
                        if(algum&&!bate){hh=0;R.adocao_vetada=true;++adocoes_vetadas;}}
                    if(hh>=P.busca_hits){kf=ka;kf_idx=idx;sem_n=0;for(auto& x:hip_hits)x=0;tem_escala=false;R.adotou=true;
                        reset_corte();seg_corte=0;}
                }
            }
        }

        bool rec_mov=false;
        if(tem_est){
            int alvo=int(std::ceil(1.5*est[2]*esc4k/128.0))*128;alvo=std::clamp(alvo,P.recorte_min,P.recorte_max);

            if(P.recorte_fixo>0){const double c=P.recorte_fixo_margem*est[2]*esc4k;const int t[3]{P.recorte_fixo,P.recorte_fixo*3/2,P.recorte_fixo*2};
                int k=0;while(k<2&&c>t[k])++k;
                if(rec_lado>0){int ka=0;while(ka<2&&t[ka]<rec_lado)++ka;if(k<ka&&c*1.15>t[k])k=ka;}
                alvo=(std::min)(t[k],P.recorte_max);}
            if(P.recorte_forcado>0)alvo=P.recorte_forcado;
            if(!rec_lado||alvo>rec_lado||alvo<=rec_lado-256||P.recorte_fixo>0)rec_lado=alvo;
            const double lr=rec_lado/esc4k;
            if(!tem_recorte){recorte[0]=det::arred(est[0]);recorte[1]=det::arred(est[1]);tem_recorte=true;rec_mov=true;}
            else{const double dz=P.zona_morta*lr;
                const bool saiu=(std::fabs(est[0]-recorte[0])+est[2]/2>lr/2)||(std::fabs(est[1]-recorte[1])+est[2]/2>lr/2);
                if(saiu||std::fabs(est[0]-recorte[0])>dz||std::fabs(est[1]-recorte[1])>dz){recorte[0]=det::arred(est[0]);recorte[1]=det::arred(est[1]);rec_mov=true;}}
        }else{tem_recorte=false;rec_lado=0;}

        {static const double* cx_ini=[]{static double v[3]{};char b[64]{};size_t n=0;
            if(getenv_s(&n,b,sizeof b,"NR_RJ_CAIXA")==0&&n>1&&sscanf_s(b,"%lf,%lf,%lf",&v[0],&v[1],&v[2])==3&&v[2]>=128)return (const double*)v;return (const double*)nullptr;}();
         if(cx_ini){on=true;corte=false;tem_est=true;est[0]=cx_ini[0]/esc4k;est[1]=cx_ini[1]/esc4k;est[2]=cx_ini[2]/1.5/esc4k;
             rec_lado=int(cx_ini[2]);if(!tem_recorte)rec_mov=true;recorte[0]=det::arred(est[0]);recorte[1]=det::arred(est[1]);tem_recorte=true;}}
        R.on=on;R.corte=corte;R.ok=float(ok);R.grande=grande;R.cinema=cinema;R.conf=float(conf);
        if(P.reancora)rea_off=on?0:(std::min)(rea_off+1,1000000);
        R.tem_est=tem_est;if(tem_est){R.est[0]=float(est[0]);R.est[1]=float(est[1]);R.est[2]=float(est[2]);}
        R.tem_recorte=tem_recorte;if(tem_recorte){R.recorte[0]=recorte[0];R.recorte[1]=recorte[1];R.recorte[2]=int(std::lround(rec_lado*((std::max)(1.f,g.saida_h)/2160.0)));}
        if(tem_est){ref_cab[0]=est[0];ref_cab[1]=est[1];ref_cab[2]=est[2];ref_idade=0;}else if(ref_idade>=0)++ref_idade;
        R.rec_mov=rec_mov;R.cab4k=float(cab4k);R.escala=tem_esc_m?float(esc_m):0.f;R.perdido=perdido;R.ncand=int(cands.size());
        if(!on){unsigned m=0;if(corte)m|=1u;if(grande)m|=2u;if(cinema)m|=4u;if(!tem_est)m|=8u;if(seg_corte>0&&!corte)m|=16u;if(P.generico&&sinal_votos>0)m|=128u;if(duvida)m|=256u;if(fora_img)m|=512u;if(!m)m=32u;R.motivo_off=m;}
        R.kface=float(kf);R.sinal=sinal;R.votos=sinal_votos;if(!R.busca)R.hip=kf_idx;R.direita=P.usa_direita;
        if(P.npc&&P.npc_modo==2)npc_passo(g,cands,nb,ib,corte,esc4k,R);
        else if(P.npc&&P.npc_modo==1)npc_rostos(g,cands,ib,corte,esc4k,R);
        if(P.npc2)npc2_passo(g,cands,ib,corte,esc4k);
        (void)tem_caixa;
        return R;
    }

    bool no_ref(const det::Cand& c,const Grades& g) const {
        if(ref_idade<0)return false;const int q=P.q;
        const double lado_c=1.25*c.wh*q,cx_c=c.x*q,cy_c=c.ytop*q+.5*lado_c;
        const double dist=std::hypot(cx_c-ref_cab[0],cy_c-ref_cab[1])/(std::max)(ref_cab[2],1.0);
        const double tam=lado_c/(std::max)(ref_cab[2],1.0);
        const double cab4k_v=lado_c*g.esc*(2160.0/(std::max)(1.f,g.saida_h));
        const double rt=(P.corte_modo&16)?(std::max)(P.corte_ref_tam,P.corte_ref_tam_forte):P.corte_ref_tam;
        const double rd=(P.corte_modo&16)?(std::max)(P.corte_ref_dist,P.corte_ref_dist_forte):P.corte_ref_dist;
        return (P.persistente||cab4k_v<=P.cab_max_4k)&&dist<=rd&&tam<=rt&&tam>=1.0/rt;
    }
    void reset_corte(){
        cd_tem=false;
        tem_prev=false;tem_est=false;seg_corte=P.corte_seg;tem_recorte=false;tem_escala=false;on=false;cont_on=0;lq_ant.clear();ref_idade=-1;
        npc_tem=false;npc_tem_rec=false;npc_alvo_ant=false;npc_perdido=0;npc_troca_c=0;
        rf_g_tem=false;rf_g_tem_rec=false;rf_vis=false;rf_vis_n=0;rf_alvo_npc=false;
        n2c_tem=false;n2c_n=0;g_hist_n=0;
        corte_t_pend=true;adq_pend=false;adq_el_n=0;adq_sem=false;adq_outro_n=0;
        vida_mem_n=0;for(auto& n:n2s){const bool tinha=n.tem,era=n.vida_pub;n.tem=false;n.tem_rec=false;n.lado=0;n.yu_tem=false;n.prof=0;n.ocl_t=0;n.vida_pub=false;n.vida_oculto=false;n.vida_hits=0;n.saida=Resultado{};n.saida.vida_era_pub=era;n.saida.troca_alvo=tinha;n.saida.rf_motivo=tinha?3:0;n.solto_corte=n.solto_corte||tinha;}
    }

private:

    double perto_escala(const Grades& g,const double* pr) const {
        const int W=g.W,H=g.H,q=P.q;const double cx=pr[0]/q,cy=pr[1]/q,m=.75*pr[2]/q;
        const int x0=(std::max)(0,int(std::floor(cx-m))),x1=(std::min)(W-1,int(std::ceil(cx+m))),y0=(std::max)(0,int(std::floor(cy-m))),y1=(std::min)(H-1,int(std::ceil(cy+m)));
        double s=0;int n=0;
        for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){const double dx=x-cx,dy=y-cy,r2=dx*dx+dy*dy;if(r2<1.0)continue;
            const size_t i=size_t(y)*W+x;const double v=-(double(g.mvq[2*i])*dx+double(g.mvq[2*i+1])*dy)/r2;if(!std::isfinite(v))continue;s+=v;++n;}
        if(n<16)return 1.0;
        return std::clamp(std::pow((std::max)(.5,1.0+s/n),double(g.k)),.7,1.4);
    }

    static void mv_em(const Grades& g,int q,double x,double y,double& mx,double& my){
        const int gx=std::clamp(int(x)/q,0,g.W-1),gy=std::clamp(int(y)/q,0,g.H-1);double sx=0,sy=0;int m=0;
        for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){const int xx=gx+dx,yy=gy+dy;if(xx<0||yy<0||xx>=g.W||yy>=g.H)continue;const size_t i=size_t(yy)*g.W+xx;sx+=g.mvq[2*i];sy+=g.mvq[2*i+1];++m;}
        mx=m?sx/m*q:0.0;my=m?sy/m*q:0.0;
    }

    double prox_rosto(const Grades& g,double x,double y,double l) const {const int q=P.q;const double r=.25*l,yc=y+.1*l;
        const int x0=(std::max)(0,int((x-r)/q)),x1=(std::min)(g.W-1,int((x+r)/q)),y0=(std::max)(0,int((yc-r)/q)),y1=(std::min)(g.H-1,int((yc+r)/q));
        std::vector<float> v;for(int yy=y0;yy<=y1;++yy)for(int xx=x0;xx<=x1;++xx)v.push_back(g.dq[size_t(yy)*g.W+xx]);
        return v.empty()?0.0:double(det::mediana(v));
    }
    bool corpo_abaixo(const Grades& g,double x,double y,double l,double pr) const {const int q=P.q;
        const int x0=(std::max)(0,int((x-.5*l)/q)),x1=(std::min)(g.W-1,int((x+.5*l)/q)),y0=(std::max)(0,int((y+.5*l)/q));
        int n=0,ok=0;for(int yy=y0;yy<g.H;++yy)for(int xx=x0;xx<=x1;++xx){++n;if(g.dq[size_t(yy)*g.W+xx]>=P.adq_corpo*pr)++ok;}
        return n==0||ok>=P.adq_corpo_frac*n;
    }

    void salvar(EstadoAlvo& s) const {
        s.tem_prev=tem_prev;s.prev_x=prev_x;s.prev_ytop=prev_ytop;s.prev_wh=prev_wh;s.tem_est=tem_est;for(int i=0;i<3;++i){s.est[i]=est[i];s.ref_cab[i]=ref_cab[i];}
        s.on=on;s.cont_on=cont_on;s.cont_off=cont_off;s.seg_corte=seg_corte;s.perdido=perdido;s.ref_idade=ref_idade;s.tem_recorte=tem_recorte;
        s.recorte[0]=recorte[0];s.recorte[1]=recorte[1];s.rec_lado=rec_lado;s.tem_escala=tem_escala;s.escala=escala;s.perto_seg=perto_seg;s.perto_seg_t=perto_seg_t;}
    void restaurar(const EstadoAlvo& s){
        tem_prev=s.tem_prev;prev_x=s.prev_x;prev_ytop=s.prev_ytop;prev_wh=s.prev_wh;tem_est=s.tem_est;for(int i=0;i<3;++i){est[i]=s.est[i];ref_cab[i]=s.ref_cab[i];}
        on=s.on;cont_on=s.cont_on;cont_off=s.cont_off;seg_corte=s.seg_corte;perdido=s.perdido;ref_idade=s.ref_idade;tem_recorte=s.tem_recorte;
        recorte[0]=s.recorte[0];recorte[1]=s.recorte[1];rec_lado=s.rec_lado;tem_escala=s.tem_escala;escala=s.escala;perto_seg=s.perto_seg;perto_seg_t=s.perto_seg_t;
        if(tem_est&&tem_prev){prev_x=est[0]/P.q;prev_ytop=(est[1]-.5*est[2])/P.q;}}

    void rostos_aplica(const Grades& g,const std::vector<det::Cand>& cands,bool corte,Resultado& R){
        if(!troca_pend)return;
        if(corte){troca_pend=0;return;}
        const int q=P.q;
        if(troca_pend==1){
            if(!trk_npc)salvar(grace_salva);
            double mx,my;mv_em(g,q,pend_est[0],pend_est[1],mx,my);double e[3]{pend_est[0]-mx*g.k,pend_est[1]-my*g.k,pend_est[2]};
            int jc=-1;double dc=1e30;
            for(int i=0;i<int(cands.size());++i){const auto& c=cands[size_t(i)];const double lado=1.25*c.wh*q,x=c.x*q,y=c.ytop*q+.5*lado,r=lado/(std::max)(e[2],1.0),d=std::hypot(x-e[0],y-e[1]);
                if(d<=.5*e[2]&&r>=.5&&r<=2.0&&d<dc){dc=d;jc=i;}}
            if(jc>=0){const auto& c=cands[size_t(jc)];prev_x=c.x;prev_ytop=c.ytop;prev_wh=c.wh;est[2]=1.25*c.wh*q;est[0]=c.x*q;est[1]=c.ytop*q+.5*est[2];}
            else{est[0]=e[0];est[1]=e[1];est[2]=e[2];prev_wh=e[2]/1.25/q;prev_x=e[0]/q;prev_ytop=(e[1]-.5*e[2])/q;}
            tem_prev=true;tem_est=true;on=true;cont_on=0;cont_off=0;perdido=0;seg_corte=0;for(int i=0;i<3;++i)ref_cab[i]=est[i];ref_idade=0;
            tem_recorte=false;rec_lado=0;tem_escala=false;perto_seg=false;trk_npc=true;++npc_id;cand_tem=false;cand_n=0;
        }else{restaurar(grace_salva);trk_npc=false;}
        R.troca_alvo=true;R.rf_motivo=troca_motivo;R.rf_npc_nota=troca_pend==1?pend_nota:0.f;rf_desde=0;troca_pend=0;npc_ausente_t=0;npc_perd_t=0;rf_vis_n=0;
    }

    void npc_rostos(const Grades& g,const std::vector<det::Cand>& cands,int ib,bool corte,double esc4k,Resultado& R){
        (void)cands;(void)ib;
        const int q=P.q;
        const double dt=(g.t>=0&&rf_t_ant>=0)?std::clamp(g.t-rf_t_ant,0.0,.5):double(g.k)/60.0;rf_t_ant=g.t;
        rf_sem_t+=dt;rf_baixo_t+=dt;rf_desde+=dt;
        auto anda=[&](double* e){double mx,my;mv_em(g,q,e[0],e[1],mx,my);e[0]-=mx*g.k;e[1]-=my*g.k;};
        if(corte){

            if(trk_npc){trk_npc=false;R.troca_alvo=true;R.rf_motivo=5;rf_desde=0;}
            troca_pend=0;cand_tem=false;cand_n=0;rf_g_tem=rf_g_tem_rec=false;rf_vis=false;rf_vis_n=0;
        }else{
            if(trk_npc&&grace_salva.tem_est)anda(grace_salva.est);
            if(cand_tem)anda(cand_est);
            if(rf_g_tem)anda(rf_g_est);

            if(trk_npc){npc_ausente_t+=dt;const bool fora_tela=est[0]<0||est[1]<0||est[0]>=g.w||est[1]>=g.h;npc_perd_t=(tem_est&&on&&perdido==0&&!fora_tela)?0.0:npc_perd_t+dt;
                if(fora_tela){npc_perd_t=(std::max)(npc_perd_t,double(P.npc_perde_s));npc_ausente_t=(std::max)(npc_ausente_t,double(P.npc_perde_s));}}
            const bool tem_g=trk_npc?grace_salva.tem_est:tem_est;const double* ge=trk_npc?grace_salva.est:est;
            const double idade_s=(g.t>=0&&g.rostos_t>=0)?g.t-g.rostos_t:double(g.rostos_idade)/60.0;
            if(g.rostos&&g.rostos_versao!=rf_versao&&idade_s<=P.rosto_idade_max){
                rf_versao=g.rostos_versao;R.rf_novo=true;rf_idade_ult=float(idade_s);
                struct Cab {double x,y,l;float nota;};std::vector<Cab> cs;
                const double idade=std::clamp(double(g.rostos_idade),0.0,30.0),esc=(std::max)(double(g.esc),1e-3);
                for(int i=0;i<g.n_rostos;++i){const auto& f=g.rostos[i];if(!(f.nota>=P.npc_nota_seg)||!(f.h>0.f))continue;
                    double fx=(f.x+.5*f.w)/esc,fy=(f.y+.5*f.h)/esc;const double fh=f.h/esc;
                    double mx,my;mv_em(g,q,fx,fy,mx,my);fx-=mx*idade;fy-=my*idade;
                    cs.push_back({fx,fy-double(P.rosto_dy)*fh,double(P.rosto_cab)*fh,f.nota});}
                rf_n=int(cs.size());
                auto no_centro=[&](const Cab& c,const double* e,double fr){return std::fabs(c.x-e[0])<=fr*e[2]&&std::fabs(c.y-e[1])<=fr*e[2];};
                int gi=-1;
                for(int i=0;i<int(cs.size());++i){const auto& c=cs[size_t(i)];bool dela=false;
                    if(tem_g)dela=no_centro(c,ge,P.rosto_dela);
                    else if(!trk_npc&&c.l*esc4k>=P.rosto_g_min){
                        auto perto_de=[&](const double* e){const double r=c.l/(std::max)(e[2],1.0);return std::hypot(c.x-e[0],c.y-e[1])<=e[2]&&r>=.5&&r<=2.0;};
                        const double hx=c.x/g.w,hy=c.y/g.h;
                        const bool zona=std::fabs(hy-P.pos_y)<=2*P.sig_y&&(std::fabs(hx-P.pos_x)<=P.sig_x||(P.usa_centro&&std::fabs(hx-P.pos_centro)<=P.sig_x)||(P.usa_direita&&std::fabs(hx-P.pos_direita)<=P.sig_x));
                        dela=(rf_g_tem&&perto_de(rf_g_est))||(ref_idade>=0&&ref_idade<=P.rosto_ref_jobs&&perto_de(ref_cab))||zona;}
                    if(dela&&(gi<0||c.nota>cs[size_t(gi)].nota))gi=i;}

                const float nd=gi>=0?cs[size_t(gi)].nota:0.f;rf_nota=nd;
                if(nd>=P.rosto_baixa)rf_baixo_t=0;
                rf_vis_n=nd>=P.rosto_nota?rf_vis_n+1:0;
                rf_vis=rf_baixo_t<P.rosto_sem_s;if(nd>=P.rosto_nota)rf_sem_t=0;

                if(trk_npc&&tem_est){for(const auto& c:cs)if(no_centro(c,est,.5)){npc_ausente_t=0;break;}}

                int im=-1;
                for(int i=0;i<int(cs.size());++i){if(i==gi)continue;const auto& c=cs[size_t(i)];
                    if(c.nota<P.rosto_conf||c.l*esc4k<P.npc_min)continue;
                    if(tem_g&&no_centro(c,ge,P.rosto_dela))continue;
                    if(trk_npc&&tem_est&&no_centro(c,est,.5))continue;
                    if(im<0||c.l>cs[size_t(im)].l)im=i;}
                if(cand_tem){
                    int ic=-1;for(int i=0;i<int(cs.size());++i){const auto& c=cs[size_t(i)];if(i==gi||c.nota<P.rosto_conf||c.l*esc4k<P.npc_min)continue;
                        const double r=c.l/(std::max)(cand_est[2],1.0);if(std::hypot(c.x-cand_est[0],c.y-cand_est[1])<=cand_est[2]&&r>=.5&&r<=2.0&&(ic<0||c.l>cs[size_t(ic)].l))ic=i;}
                    if(ic>=0&&(im<0||cs[size_t(im)].l<1.5*cs[size_t(ic)].l))im=ic;
                    if(im>=0&&im==ic){const auto& c=cs[size_t(im)];cand_est[0]=c.x;cand_est[1]=c.y;cand_est[2]=c.l;cand_nota=c.nota;++cand_n;}
                    else if(im>=0){const auto& c=cs[size_t(im)];cand_est[0]=c.x;cand_est[1]=c.y;cand_est[2]=c.l;cand_nota=c.nota;cand_n=1;}
                    else{cand_tem=false;cand_n=0;}}
                else if(im>=0){const auto& c=cs[size_t(im)];cand_est[0]=c.x;cand_est[1]=c.y;cand_est[2]=c.l;cand_nota=c.nota;cand_n=1;cand_tem=true;}
                R.npc_cab4k=cand_tem?float(cand_est[2]*esc4k):0.f;

                if(!trk_npc&&gi>=0&&nd>=P.rosto_nota&&!tem_est){const auto& c=cs[size_t(gi)];
                    if(!rf_g_tem||std::hypot(c.x-rf_g_est[0],c.y-rf_g_est[1])>rf_g_est[2]){rf_g_est[0]=c.x;rf_g_est[1]=c.y;rf_g_est[2]=c.l;rf_g_tem_rec=false;}
                    else{const double a=P.alfa_pos,at=P.alfa_tam;rf_g_est[0]+=a*(c.x-rf_g_est[0]);rf_g_est[1]+=a*(c.y-rf_g_est[1]);rf_g_est[2]+=at*(c.l-rf_g_est[2]);}
                    rf_g_tem=true;}
                else if(tem_est)rf_g_tem=false;
            }
            if(rf_g_tem&&(trk_npc||tem_est||rf_baixo_t>=P.rosto_sem_s||rf_g_est[0]<0||rf_g_est[1]<0||rf_g_est[0]>=g.w||rf_g_est[1]>=g.h)){rf_g_tem=false;rf_g_tem_rec=false;}

            if(!troca_pend){
                auto para_npc=[&](int m){troca_pend=1;troca_motivo=m;pend_est[0]=cand_est[0];pend_est[1]=cand_est[1];pend_est[2]=cand_est[2];pend_nota=cand_nota;};
                const bool cand_ok=cand_tem&&cand_n>=P.rosto_conf_n&&rf_desde>=P.npc_espera_s;
                if(!trk_npc){if(rf_baixo_t>=P.rosto_sem_npc&&cand_ok)para_npc(1);}
                else if(rf_vis_n>=P.rosto_volta_n){troca_pend=2;troca_motivo=2;}
                else if(npc_ausente_t>=P.npc_perde_s&&npc_perd_t>=P.npc_perde_s){if(cand_ok)para_npc(6);else{troca_pend=2;troca_motivo=4;}}
            }
        }

        auto recorte_zm=[&](const double* e,int* rec,bool& tem_rec,int lado4k){bool mov=false;const double lr=lado4k/esc4k;
            if(!tem_rec){rec[0]=det::arred(e[0]);rec[1]=det::arred(e[1]);tem_rec=true;mov=true;}
            else{const double dz=P.zona_morta*lr;const bool saiu=(std::fabs(e[0]-rec[0])+e[2]/2>lr/2)||(std::fabs(e[1]-rec[1])+e[2]/2>lr/2);
                if(saiu||std::fabs(e[0]-rec[0])>dz||std::fabs(e[1]-rec[1])>dz){rec[0]=det::arred(e[0]);rec[1]=det::arred(e[1]);mov=true;}}
            return mov;};
        auto saida=[&](const double* e,const int* rec,int lado4k,bool mov){
            R.est[0]=float(e[0]);R.est[1]=float(e[1]);R.est[2]=float(e[2]);R.tem_est=true;
            R.recorte[0]=rec[0];R.recorte[1]=rec[1];R.recorte[2]=int(std::lround(lado4k*((std::max)(1.f,g.saida_h)/2160.0)));R.tem_recorte=true;
            R.on=true;R.corte=false;R.cab4k=float(e[2]*esc4k);R.rec_mov=mov;R.motivo_off=0;};

        R.alvo_npc=trk_npc;if(!R.troca_alvo)R.rf_npc_nota=cand_tem?cand_nota:0.f;R.rf_idade_s=rf_idade_ult;
        if(!trk_npc&&rf_g_tem&&!corte){

            const double c4=rf_g_est[2]*esc4k;int lado=int(std::ceil(1.5*c4/128.0))*128;lado=std::clamp(lado,P.recorte_min,P.recorte_max);
            if(P.recorte_fixo>0){const double cm=P.recorte_fixo_margem*c4;const int t3[3]{P.recorte_fixo,P.recorte_fixo*3/2,P.recorte_fixo*2};int k=0;while(k<2&&cm>t3[k])++k;lado=(std::min)(t3[k],P.recorte_max);}
            if(lado!=rf_g_lado){rf_g_lado=lado;rf_g_tem_rec=false;}
            const bool mov=recorte_zm(rf_g_est,rf_g_rec,rf_g_tem_rec,lado);saida(rf_g_est,rf_g_rec,lado,mov);R.rf_g=true;}
        R.rf_vis=rf_vis;R.rf_n=rf_n;R.rf_nota=rf_nota;R.rf_sem=float((std::min)(rf_sem_t,999.0));
    }

    void canon_medir(Npc2Slot& n,const RostoDet& f,double esc,double sx,double sy,double pf){
        if(!(f.px>0.f)){++n.cn_sem;return;}
        float lm[10];for(int i=0;i<5;++i){lm[2*i]=float(f.lm[2*i]/esc-sx);lm[2*i+1]=float(f.lm[2*i+1]/esc-sy);}
        double zx,zy,zs;if(!nrcanon::medida(lm,zx,zy,zs)){++n.cn_sem;return;}
        const double bx=f.x/esc-sx,by=f.y/esc-sy,bw=f.w/esc,bh=f.h/esc;
        if(zx<bx||zy<by||zx>bx+bw||zy>by+bh||zs>2.5*(std::max)(bw,bh)){++n.cn_sem;return;}
        n.cn.medir(zx,zy,zs,n.desde,std::clamp((double(f.px)-6.0)/30.0,.15,1.0),pf,P.canon);
    }
    void npc2_passo(const Grades& g,const std::vector<det::Cand>& cands,int ib,bool corte,double esc4k){
        const int q=P.q;const int K=std::clamp(P.npc_max,1,npc_det_max);
        const double dt=(g.t>=0&&n2_t_ant>=0)?std::clamp(g.t-n2_t_ant,0.0,.5):double(g.k)/60.0;n2_t_ant=g.t;
        if(P.npc_roi_conf>0)n2_relogio+=dt;
        if(P.npc_vida){vida_relogio+=dt;int w=0;
            for(int i=0;i<vida_mem_n;++i){auto* e=vida_mem[i];if(vida_relogio-e[3]>double(P.vida_re_s))continue;double mx,my;mv_em(g,q,e[0],e[1],mx,my);
                e[0]-=mx*g.k;e[1]-=my*g.k;if(w!=i)for(int c=0;c<5;++c)vida_mem[w][c]=e[c];++w;}vida_mem_n=w;}

        auto sobre_ela=[&](double x,double y){if(!tem_est)return false;const double hz=.5*P.npc2_zona*est[2];
            return std::fabs(x-est[0])<=hz&&std::fabs(y-est[1])<=hz;};

        if(tem_est){g_hist[g_hist_i][0]=est[0];g_hist[g_hist_i][1]=est[1];g_hist[g_hist_i][2]=est[2];g_hist_i=(g_hist_i+1)%8;g_hist_n=(std::min)(g_hist_n+1,8);}
        auto onde_ela_esteve=[&](double x,double y){for(int i=0;i<g_hist_n;++i){const double hz=.5*P.npc2_zona*g_hist[i][2];
            if(std::fabs(x-g_hist[i][0])<=hz&&std::fabs(y-g_hist[i][1])<=hz)return true;}return false;};

        auto mv_pessoa=[&](const double* e,double prof,double& mx,double& my){
            const int gx0=(std::max)(0,int((e[0]-.5*e[2])/q)),gx1=(std::min)(g.W-1,int((e[0]+.5*e[2])/q)),gy0=(std::max)(0,int((e[1]-.5*e[2])/q)),gy1=(std::min)(g.H-1,int((e[1]+.5*e[2])/q));
            float dmed=float(prof);
            if(!(prof>0)){std::vector<float> dz;for(int y=gy0;y<=gy1;++y)for(int x=gx0;x<=gx1;++x)dz.push_back(g.dq[size_t(y)*g.W+x]);
                if(dz.size()<4){mv_em(g,q,e[0],e[1],mx,my);return;}
                dmed=det::mediana(dz);}
            std::vector<float> vx,vy;
            for(int y=gy0;y<=gy1;++y)for(int x=gx0;x<=gx1;++x){const size_t i=size_t(y)*g.W+x;if(std::fabs(g.dq[i]-dmed)<=P.npc2_faixa*dmed){vx.push_back(g.mvq[2*i]);vy.push_back(g.mvq[2*i+1]);}}
            if(vx.size()>=3){mx=double(det::mediana(vx))*q;my=double(det::mediana(vy))*q;return;}
            if(!(prof>0)){mv_em(g,q,e[0],e[1],mx,my);return;}
            const int r2=(std::max)(1,int(2.0*e[2]/q)),cx=int(e[0]/q),cy=int(e[1]/q);
            for(int y=(std::max)(0,cy-r2);y<=(std::min)(g.H-1,cy+r2);y+=2)for(int x=(std::max)(0,cx-r2);x<=(std::min)(g.W-1,cx+r2);x+=2){
                const size_t i=size_t(y)*g.W+x;if(g.dq[i]<=dmed*(1.f+P.npc2_faixa)){vx.push_back(g.mvq[2*i]);vy.push_back(g.mvq[2*i+1]);}}
            if(vx.size()>=8){mx=double(det::mediana(vx))*q;my=double(det::mediana(vy))*q;}else{mx=0;my=0;}};

        auto prof_em=[&](double x,double y,double l)->double{const double r=.25*l,yc=y+.1*l;
            const int x0=(std::max)(0,int((x-r)/q)),x1=(std::min)(g.W-1,int((x+r)/q)),y0=(std::max)(0,int((yc-r)/q)),y1=(std::min)(g.H-1,int((yc+r)/q));
            std::vector<float> v;for(int yy=y0;yy<=y1;++yy)for(int xx=x0;xx<=x1;++xx)v.push_back(g.dq[size_t(yy)*g.W+xx]);
            return v.empty()?0.0:double(det::mediana(v));};
        auto na_frente=[&](const Npc2Slot& n,double pf){return n.prof>0&&pf>n.prof*(1.0+P.npc2_ocl_faixa);};

        const double pg_ela=(P.npc_atras&&tem_est)?prof_em(est[0],est[1],est[2]):0.0;
        const double pc_ela=(P.npc_corpo&&tem_est)?prof_em(est[0],est[1],est[2]):0.0;

        const double pv_ela=(P.npc_vida&&tem_est)?prof_em(est[0],est[1],est[2]):0.0;dbg_prof_ela=pv_ela;
        auto frente_pf=[&](double pf){return P.npc_vida&&pv_ela>0&&pf>0&&pf>=double(P.npc_atras_prof)*pv_ela;};
        auto corpo_dela=[&](double x,double y,double pf){return P.npc_corpo&&tem_est&&pc_ela>0&&pf>0&&y-est[1]>=double(P.npc_corpo_dy)*est[2]&&
            std::fabs(x-est[0])<=double(P.npc_corpo_dx)*est[2]&&pf*double(P.npc_corpo_prof)>=pc_ela;};
        auto atras_dela=[&](double l,double pf){return P.npc_atras&&tem_est&&pg_ela>0&&pf>0&&l<=double(P.npc_atras_tam)*est[2]&&pg_ela>=double(P.npc_atras_prof)*pf;};

        auto miolo_livre=[&](double x0,double y0,double l,double pf){if(!(pf>0))return false;const double r=.35*l,yc=y0+.1*l;int c=0,f=0;
            for(int y=(std::max)(0,int((yc-r)/q));y<=(std::min)(g.H-1,int((yc+r)/q));++y)for(int x=(std::max)(0,int((x0-r)/q));x<=(std::min)(g.W-1,int((x0+r)/q));++x){
                ++c;if(g.dq[size_t(y)*g.W+x]>pf*(1.0+P.npc2_ocl_faixa))++f;}
            return c>0&&f<=double(P.npc_atras_cob)*c;};
        auto outra_prof=[&](const Npc2Slot& n,double pf){return n.prof>0&&std::fabs(pf-n.prof)>P.npc2_ocl_faixa*n.prof;};

        const double fps=dt>0?double(g.k)/dt:60.0;
        auto vel=[&](const double* e){double mx,my;mv_em(g,q,e[0],e[1],mx,my);
            const int gx=int(e[0])/q,gy=int(e[1])/q,r1=(std::max)(1,int(e[2]/q)),r3=3*r1;
            const int ic=std::clamp(gy,0,g.H-1)*g.W+std::clamp(gx,0,g.W-1);const float pc=g.dq[size_t(ic)];
            std::vector<float> vx,vy;
            for(int y=(std::max)(0,gy-r3);y<=(std::min)(g.H-1,gy+r3);y+=2)for(int x=(std::max)(0,gx-r3);x<=(std::min)(g.W-1,gx+r3);x+=2){
                if(std::abs(x-gx)<=r1&&std::abs(y-gy)<=r1)continue;const size_t i=size_t(y)*g.W+x;if(!(g.dq[i]<.8f*pc))continue;
                vx.push_back(g.mvq[2*i]);vy.push_back(g.mvq[2*i+1]);}
            if(vx.size()<8)return 0.0;
            return std::hypot(mx-double(det::mediana(vx))*q,my-double(det::mediana(vy))*q)*fps/(std::max)(e[2],1.0);};
        (void)corte;(void)cands;(void)ib;
        unsigned id_ant[npc_det_max]{};bool tinha[npc_det_max]{},coberto[npc_det_max]{};int motivo[npc_det_max]{};
        bool vida_sai[npc_det_max]{},era_pub[npc_det_max]{},oc_novo[npc_det_max]{},oc_volta[npc_det_max]{},camq[npc_det_max]{};double sai_est[npc_det_max][3]{};int sai_rec[npc_det_max][2]{},sai_lado[npc_det_max]{};
        auto solta=[&](Npc2Slot& n,int m,int k){era_pub[k]=era_pub[k]||n.vida_pub;n.tem=false;n.tem_rec=false;n.lado=0;n.yu_tem=false;n.rapido_t=0;n.ocl_t=0;n.prof=0;n.vida_pub=false;n.vida_oculto=false;n.vida_hits=0;n.cn.zera();n.cn_lado=0;n.esmaece=false;n.fora_t=n.prof_ruim_t=0;motivo[k]=m;};

        for(int k=0;k<npc_det_max;++k){id_ant[k]=n2s[k].id;tinha[k]=n2s[k].tem;if(k>=K&&n2s[k].tem)solta(n2s[k],6,k);}

        for(int k=0;k<K;++k){auto& n=n2s[k];
            n.est_ini[0]=n.est[0];n.est_ini[1]=n.est[1];n.est_ini[2]=n.est[2];
            n.conf_t+=dt;n.desde+=dt;

            auto anda=[&](double* e){double mx,my;mv_pessoa(e,n.prof,mx,my);
                if(std::hypot(mx,my)*g.k<=P.npc2_mv_max*(std::max)(e[2],1.0)){e[0]-=mx*g.k;e[1]-=my*g.k;}else ++n.mv_ruim;};
            if(n.yu_tem)anda(n.yu);
            if(n.tem)anda(n.est);
            if(P.npc_canon&&n.tem&&n.cn.tem){
                double e[3]{n.cn.cx,n.cn.cy,(std::max)(n.est[2],1.0)},mx,my;mv_pessoa(e,n.prof,mx,my);
                if(!(std::hypot(mx,my)*g.k<=P.npc2_mv_max*e[2])){mx=0;my=0;}
                n.cn.prever(mx*g.k,my*g.k,prof_em(n.est[0],n.est[1],n.est[2]));}

            if(n.tem&&n.prof>0){const double r=.35*n.est[2],yc=n.est[1]+.1*n.est[2];int c=0,f=0;
                for(int y=(std::max)(0,int((yc-r)/q));y<=(std::min)(g.H-1,int((yc+r)/q));++y)for(int x=(std::max)(0,int((n.est[0]-r)/q));x<=(std::min)(g.W-1,int((n.est[0]+r)/q));++x){
                    ++c;if(na_frente(n,g.dq[size_t(y)*g.W+x]))++f;}
                if(c>0&&f>=P.npc2_ocl_frac*c){n.ocl_t+=dt;++n.ocl_jobs;}else n.ocl_t=0;
                coberto[k]=n.ocl_t>0&&n.ocl_t<=P.npc2_ocl_max_s;}
            else n.ocl_t=0;
            if(coberto[k])n.conf_t=(std::max)(0.0,n.conf_t-dt);

            const double idade_r=(g.t>=0&&g.roi_t[k]>=0)?g.t-g.roi_t[k]:double(g.roi_idade[k])/60.0;
            if(n.tem&&(g.roi[k]||(P.npc_roi_conf>0&&g.n_roi[k]==0))&&g.roi_versao[k]!=n.roi_versao&&idade_r<=P.rosto_idade_max&&
               !(P.npc_roi_conf>0&&g.t>=0&&g.roi_t[k]>=0&&n.t_preso>=0&&g.roi_t[k]<n.t_preso)){
                n.roi_versao=g.roi_versao[k];const double esc=(std::max)(double(g.esc),1e-3),idade=std::clamp(double(g.roi_idade[k]),0.0,30.0);
                int jr=-1;double dr=1e30,rx=0,ry=0,rl=0,rsx=0,rsy=0;float rn=0;
                for(int i=0;i<g.n_roi[k];++i){const auto& f=g.roi[k][i];if(!(f.nota>=P.npc2_nota_seg)||!(f.h>0.f))continue;
                    double fx=(f.x+.5*f.w)/esc,fy=(f.y+.5*f.h)/esc;const double fh=f.h/esc;
                    double l=double(P.rosto_cab)*fh,y=fy-double(P.rosto_dy)*fh;double e2[3]{fx,y,l};double mx,my;mv_pessoa(e2,0.0,mx,my);fx-=mx*idade;y-=my*idade;
                    if((sobre_ela(fx,y)&&!(P.npc_atras&&atras_dela(l,prof_em(fx,y,l)))&&!(n.vida_pub&&frente_pf(prof_em(fx,y,l))))||outra_prof(n,prof_em(fx,y,l))||(P.npc_corpo&&corpo_dela(fx,y,prof_em(fx,y,l))))continue;
                    const double r=l/(std::max)(n.est[2],1.0),d=std::hypot(fx-n.est[0],y-n.est[1])/(std::max)(n.est[2],1.0);

                    const bool firme=f.nota>=P.npc2_nota_salto;
                    if((d>P.npc2_salto_roi||coberto[k])&&!firme)continue;
                    if(P.npc_leve&2){bool dono=false;
                        for(int j=0;j<K&&!dono;++j){const auto& o=n2s[j];if(j==k||!o.tem)continue;
                            const double dj=std::hypot(fx-o.est[0],y-o.est[1])/(std::max)(o.est[2],1.0);dono=dj<P.npc2_dup&&dj<d;}
                        if(dono){++n.roi_dono;continue;}}
                    if(d<=1.0&&r>=.5&&r<=2.0&&d<dr){dr=d;jr=i;rx=fx;ry=y;rl=l;rn=f.nota;rsx=mx*idade;rsy=my*idade;}}
                if(jr>=0){n.conf_t=0;n.nota=rn;++n.roi_ok;n.visto=true;++n.vida_hits;
                    if((P.npc_leve&2)&&dr>double(P.npc_leve_salto)){const double rr=rl/(std::max)(n.est[2],1.0);
                        if(rr<.75||rr>1.33){n.salto_id=true;++n.saltos_id;}}
                    if(dr>P.npc2_salto_roi){n.est[0]=rx;n.est[1]=ry;n.est[2]=rl;}
                    else{const double a=P.npc2_alfa_roi;n.est[0]+=a*(rx-n.est[0]);n.est[1]+=a*(ry-n.est[1]);n.est[2]+=a*(rl-n.est[2]);}
                    n.yu[0]=n.est[0];n.yu[1]=n.est[1];n.yu[2]=n.est[2];n.yu_tem=true;
                    const double pf=prof_em(n.est[0],n.est[1],n.est[2]);if(pf>0)n.prof+=.3*(pf-n.prof);
                    if(P.npc_canon){if(dr>P.npc2_salto_roi||n.salto_id)n.cn.zera();canon_medir(n,g.roi[k][jr],esc,rsx,rsy,pf);}}
                else ++n.roi_falta;
            }
        }

        const double idade_s=(g.t>=0&&g.rostos_t>=0)?g.t-g.rostos_t:double(g.rostos_idade)/60.0;
        if(g.rostos&&g.rostos_versao!=n2_versao&&idade_s<=P.rosto_idade_max){
            n2_versao=g.rostos_versao;
            struct Cab {double x,y,l;float nota;double prof,vel;bool usado;bool peito=false;double vimg=0;int fi=-1;double sx=0,sy=0;};std::vector<Cab> cs;
            const double idade=std::clamp(double(g.rostos_idade),0.0,30.0),esc=(std::max)(double(g.esc),1e-3);
            for(int i=0;i<g.n_rostos;++i){const auto& f=g.rostos[i];if(!(f.nota>=P.npc2_nota_seg)||!(f.h>0.f))continue;
                double fx=(f.x+.5*f.w)/esc,fy=(f.y+.5*f.h)/esc;const double fh=f.h/esc;
                double mx,my;mv_em(g,q,fx,fy,mx,my);fx-=mx*idade;fy-=my*idade;
                const double l=double(P.rosto_cab)*fh,y=fy-double(P.rosto_dy)*fh;
                if((!sobre_ela(fx,y)||(P.npc_atras&&atras_dela(l,prof_em(fx,y,l))))&&!(P.npc_corpo&&corpo_dela(fx,y,prof_em(fx,y,l)))){const double e3[3]{fx,y,l};cs.push_back({fx,y,l,f.nota,prof_em(fx,y,l),vel(e3),false});cs.back().fi=i;cs.back().sx=mx*idade;cs.back().sy=my*idade;
                    if(P.npc_vel_img==1){double vx,vy;mv_em(g,q,fx,y,vx,vy);cs.back().vimg=std::hypot(vx,vy)*fps/(std::max)(l,1.0);}}}
            dbg_cs.clear();for(const auto& c:cs)dbg_cs.push_back(std::array<double,6>{{c.x,c.y,c.l,double(c.nota),c.vel,c.vimg}});
            if(P.npc_peito)for(auto& c:cs)for(const auto& u:cs){if(&u==&c||!(u.prof>0)||!(c.prof>0))continue;const double dy=c.y-u.y;
                if(dy>=.5*u.l&&dy<=1.8*u.l&&std::fabs(c.x-u.x)<=u.l&&std::fabs(c.prof-u.prof)<=.3*u.prof){c.peito=true;break;}}

            auto dentro=[&](const Npc2Slot& n,const Cab& c){return n.tem&&std::fabs(c.x-n.est[0])<=.5*n.est[2]&&std::fabs(c.y-n.est[1])<=.5*n.est[2];};
            for(int k=0;k<K;++k){auto& n=n2s[k];if(!n.tem)continue;int ip=-1;double dp=1e30;
                for(int i=0;i<int(cs.size());++i){const auto& c=cs[size_t(i)];if(c.usado)continue;const double r=c.l/(std::max)(n.est[2],1.0),d=std::hypot(c.x-n.est[0],c.y-n.est[1]);
                    if(dentro(n,c)&&!outra_prof(n,c.prof)&&r>=.5&&r<=2.0&&d<dp){dp=d;ip=i;}}
                if(ip>=0){auto& c=cs[size_t(ip)];c.usado=true;n.conf_t=0;n.nota=c.nota;n.visto=true;++n.vida_hits;n.yu[0]=c.x;n.yu[1]=c.y;n.yu[2]=c.l;n.yu_tem=true;
                    const double a=P.npc2_alfa;n.est[0]+=a*(c.x-n.est[0]);n.est[1]+=a*(c.y-n.est[1]);n.est[2]+=a*(c.l-n.est[2]);
                    if(P.npc_canon&&c.fi>=0)canon_medir(n,g.rostos[c.fi],esc,c.sx,c.sy,c.prof);}}

            auto peso=[&](const Cab& c){double p=c.l/(1.0+c.vel/(std::max)(double(P.npc2_vel_ref),1e-3));
                if(n2c_tem&&std::hypot(c.x-n2c_est[0],c.y-n2c_est[1])<=n2c_est[2])p*=1.5;return p;};
            auto de_alguem=[&](const Cab& c){for(int k=0;k<K;++k){const auto& n=n2s[k];
                if(P.npc_frente&&n.tem&&c.l>=1.5*n.est[2]&&c.prof>0&&na_frente(n,c.prof))continue;
                if(n.tem&&(dentro(n,c)||std::hypot(c.x-n.est[0],c.y-n.est[1])<=((P.npc_todos&8)?.5:.75)*(std::max)(n.est[2],c.l)))return true;}return false;};

            bool preso1=false;for(int k=0;k<K;++k)preso1=preso1||n2s[k].tem;
            auto elegivel=[&](const Cab& c){for(int i=0;i<adq_el_n;++i)if(std::hypot(c.x-adq_el[i][0],c.y-adq_el[i][1])<=.5*(std::max)(c.l,adq_el[i][2]))return true;return false;};
            int im=-1;
            for(int i=0;i<int(cs.size());++i){const auto& c=cs[size_t(i)];if(c.usado)continue;
                const bool segue=n2c_tem&&n2c_n>=1&&std::hypot(c.x-n2c_est[0],c.y-n2c_est[1])<=n2c_est[2];
                if((c.nota<P.npc2_nota_novo&&!segue)||c.l*esc4k<P.npc2_cab_min||(P.npc2_vel_novo>0.f&&!(P.npc_vel_img==2&&K>1)&&(P.npc_vel_img==1?(std::min)(c.vel,c.vimg):c.vel)>P.npc2_vel_novo&&!preso1))continue;
                if(de_alguem(c)||(onde_ela_esteve(c.x,c.y)&&!(P.npc_atras&&atras_dela(c.l,c.prof)))||c.peito)continue;
                if((P.npc_atras&2)&&(sobre_ela(c.x,c.y)||onde_ela_esteve(c.x,c.y))&&!miolo_livre(c.x,c.y,c.l,c.prof))continue;

                if(P.npc_roi_conf>0&&P.npc_roi_veto_s>0.f){bool v=false;for(int iv=0;iv<veto_roi_n&&!v;++iv){auto* e=veto_roi[iv];const double r=c.l/(std::max)(e[2],1.0);
                    const double dtp=std::clamp(n2_relogio-e[6],0.0,1.0),px=e[0]+e[4]*dtp,py=e[1]+e[5]*dtp;
                    v=n2_relogio-e[3]<=double(P.npc_roi_veto_s)&&r>=.67&&r<=1.5&&std::hypot(c.x-px,c.y-py)<=P.npc_roi_veto_raio*(std::max)(c.l,e[2]);
                    if(v&&P.npc_roi_veto_segue){if(dtp>=.02){const double a=e[4]==0&&e[5]==0?1.0:.5;e[4]+=a*((c.x-e[0])/dtp-e[4]);e[5]+=a*((c.y-e[1])/dtp-e[5]);const double vm=std::hypot(e[4],e[5]),vl=4.0*c.l;if(vm>vl){e[4]*=vl/vm;e[5]*=vl/vm;}}
                        e[0]=c.x;e[1]=c.y;e[2]=c.l;e[3]=n2_relogio;e[6]=n2_relogio;}}
                    if(v){++roi_veto_n;continue;}}

                if((P.adquire&2)&&!tem_est&&ref_idade>=0&&elegivel(c))continue;
                if((P.npc_todos&2)&&c.nota>=P.todos_nota&&dir_n<8){double* d=dir_c[dir_n++];d[0]=c.x;d[1]=c.y;d[2]=c.l;d[3]=c.nota;d[4]=c.prof;d[5]=c.vel;}
                if(im<0||peso(c)>peso(cs[size_t(im)]))im=i;}
            if(im>=0){const auto& c=cs[size_t(im)];const bool mesmo=n2c_tem&&std::hypot(c.x-n2c_est[0],c.y-n2c_est[1])<=n2c_est[2];
                n2c_est[0]=c.x;n2c_est[1]=c.y;n2c_est[2]=c.l;n2c_nota=c.nota;n2c_prof=c.prof;n2c_vel=c.vel;n2c_n=mesmo?n2c_n+1:1;n2c_tem=true;n2c_visto_t=g.t;
                n2c_tl=g.rostos_t;if(n2c_n==1)n2c_t0=g.rostos_t;}
            else if(!(n2c_tem&&g.t>=0&&g.t-n2c_visto_t<=P.npc2_cand_falta_s)){n2c_tem=false;n2c_n=0;}
        }else if(n2c_tem){double mx,my;mv_em(g,q,n2c_est[0],n2c_est[1],mx,my);n2c_est[0]-=mx*g.k;n2c_est[1]-=my*g.k;}
        const bool lim_vel=P.npc2_vel_max>0.f;
        if(n2c_tem&&dt>0&&lim_vel)n2c_vel=n2c_n<=1?vel(n2c_est):n2c_vel+P.npc2_alfa*(vel(n2c_est)-n2c_vel);
        if(!n2c_tem)n2c_vel=0;

        auto segura_de=[&](const Npc2Slot& n){if(P.npc2_segura_peq>0.f&&n.est[2]*esc4k<P.npc2_peq_cab&&n.prof>0){const double pf=prof_em(n.est[0],n.est[1],n.est[2]);
            if(pf>0&&!outra_prof(n,pf))return (std::max)(double(P.npc2_segura_s),double(P.npc2_segura_peq));}return double(P.npc2_segura_s);};
        for(int k=0;k<K;++k){auto& n=n2s[k];if(!n.tem)continue;
            if(dt>0&&lim_vel)n.vel+=P.npc2_alfa*(vel(n.est)-n.vel);
            n.rapido_t=(lim_vel&&n.vel>P.npc2_vel_max)?n.rapido_t+dt:0.0;
            const bool fora_tela=n.est[0]<0||n.est[1]<0||n.est[0]>=g.w||n.est[1]>=g.h;
            int m=0;

            auto atras_preso=[&](const Npc2Slot& p){if(!(p.prof>0)||!atras_dela(p.est[2],p.prof))return false;const double pf=prof_em(p.est[0],p.est[1],p.est[2]);
                if(!(pf>0&&!outra_prof(p,pf)&&pg_ela>=double(P.npc_atras_prof)*pf))return false;
                const double r=.35*p.est[2],yc=p.est[1]+.1*p.est[2];int c=0,f=0;
                for(int y=(std::max)(0,int((yc-r)/q));y<=(std::min)(g.H-1,int((yc+r)/q));++y)for(int x=(std::max)(0,int((p.est[0]-r)/q));x<=(std::min)(g.W-1,int((p.est[0]+r)/q));++x){
                    ++c;if(na_frente(p,g.dq[size_t(y)*g.W+x]))++f;}
                return c>0&&f<=double(P.npc_atras_cob)*c;};

            auto camada_dele=[&](Npc2Slot& p)->bool{if(!(p.prof>0)||!atras_dela(p.est[2],p.prof))return false;

                const double r=.45*p.est[2],yc=p.est[1]+.1*p.est[2];std::vector<std::array<int,2>> fr,me,fo_;int c=0;
                for(int y=(std::max)(0,int((yc-r)/q));y<=(std::min)(g.H-1,int((yc+r)/q));++y)for(int x=(std::max)(0,int((p.est[0]-r)/q));x<=(std::min)(g.W-1,int((p.est[0]+r)/q));++x){
                    ++c;const double v=g.dq[size_t(y)*g.W+x];if(na_frente(p,v)){fr.push_back({x,y});fo_.push_back({x,y});}
                    else if(std::fabs(v-p.prof)<=P.npc2_ocl_faixa*p.prof)me.push_back({x,y});else fo_.push_back({x,y});}
                if(c<=0||fr.empty()||double(me.size())<double(P.npc_camada_vis)*c)return false;
                auto d2=[](const std::array<int,2>& a,const std::array<int,2>& b){return double((a[0]-b[0])*(a[0]-b[0])+(a[1]-b[1])*(a[1]-b[1]));};
                double melhor=-1;int bx=0,by=0;
                for(const auto& m:me){double de=1e30,dfo=1e30;for(const auto& f:fr)de=(std::min)(de,d2(m,f));
                    if(std::sqrt(de)*q<double(P.npc_camada_margem)*p.est[2])continue;
                    for(const auto& f:fo_)dfo=(std::min)(dfo,d2(m,f));if(dfo>melhor){melhor=dfo;bx=m[0];by=m[1];}}
                if(melhor<2.0)return false;
                p.cam[0]=(bx+.5)*q;p.cam[1]=(by+.5)*q;return true;};
            n.cam_tem=false;
            bool frente=false;
            if(P.npc_vida&&n.vida_pub&&n.prof>0&&sobre_ela(n.est[0],n.est[1])){const double pf=prof_em(n.est[0],n.est[1],n.est[2]);frente=!outra_prof(n,pf)&&frente_pf(pf);if(frente)++n.vida_frente;}

            const bool sep=P.npc_vida>=2&&n.vida_pub&&n.prof>0&&pv_ela>0&&(pv_ela>=double(P.npc_atras_prof)*n.prof||n.prof>=double(P.npc_atras_prof)*pv_ela);
            int vis2=-2;
            if(sep&&sobre_ela(n.est[0],n.est[1])){const double r=.45*n.est[2],yc=n.est[1]+.1*n.est[2];std::vector<std::array<int,2>> fr,me,fo_;int c=0;
                for(int y=(std::max)(0,int((yc-r)/q));y<=(std::min)(g.H-1,int((yc+r)/q));++y)for(int x=(std::max)(0,int((n.est[0]-r)/q));x<=(std::min)(g.W-1,int((n.est[0]+r)/q));++x){
                    ++c;const double v=g.dq[size_t(y)*g.W+x];if(na_frente(n,v)){fr.push_back({x,y});fo_.push_back({x,y});}
                    else if(std::fabs(v-n.prof)<=P.npc2_ocl_faixa*n.prof)me.push_back({x,y});else fo_.push_back({x,y});}
                vis2=-1;
                if(c>0&&double(me.size())>=double(P.npc_camada_vis)*c){
                    if(fr.empty()){const double pc=g.dq[size_t(std::clamp(int(n.est[1]/q),0,g.H-1))*g.W+size_t(std::clamp(int(n.est[0]/q),0,g.W-1))];
                        if(std::fabs(pc-n.prof)<=P.npc2_ocl_faixa*n.prof)vis2=0;}
                    if(vis2<0){auto d2=[](const std::array<int,2>& a,const std::array<int,2>& b){return double((a[0]-b[0])*(a[0]-b[0])+(a[1]-b[1])*(a[1]-b[1]));};
                        double melhor=-1;int bx=0,by=0;
                        for(const auto& mm:me){double de=1e30,dfo=1e30;for(const auto& f:fr)de=(std::min)(de,d2(mm,f));
                            if(!fr.empty()&&std::sqrt(de)*q<double(P.npc_camada_margem)*n.est[2])continue;
                            for(const auto& f:fo_)dfo=(std::min)(dfo,d2(mm,f));if(dfo>melhor){melhor=dfo;bx=mm[0];by=mm[1];}}
                        if(melhor>=2.0){n.cam[0]=(bx+.5)*q;n.cam[1]=(by+.5)*q;vis2=1;}}}}
            const bool na_zona=sobre_ela(n.est[0],n.est[1])&&!(P.npc_atras&&atras_preso(n))&&!frente&&!(vis2>=0);
            if(vis2==1){n.cam_tem=true;++n.cam_jobs;}
            const bool perto_zona=P.npc_vida>=2&&tem_est&&std::fabs(n.est[0]-est[0])<=.5*P.vida2_cam_zona*P.npc2_zona*est[2]&&std::fabs(n.est[1]-est[1])<=.5*P.vida2_cam_zona*P.npc2_zona*est[2];
            if(!n.cam_tem&&(na_zona||perto_zona)&&P.npc_camada&&P.npc_atras&&camada_dele(n)){n.cam_tem=true;++n.cam_jobs;}
            const bool corpo_n=P.npc_corpo&&corpo_dela(n.est[0],n.est[1],prof_em(n.est[0],n.est[1],n.est[2]));
            camq[k]=P.npc_vida>=2&&(perto_zona||sobre_ela(n.est[0],n.est[1])||n.vida_oculto);
            bool oculta=false;
            if(sep&&vis2==-1&&na_zona&&!n.cam_tem&&!corpo_n){oculta=true;if(!n.vida_oculto){n.vida_oculto=true;oc_novo[k]=true;++n.vida_ocultos;}}
            else if(n.vida_oculto&&!corpo_n&&(!na_zona||n.cam_tem)){n.vida_oculto=false;oc_volta[k]=true;}
            if(!oculta&&((na_zona&&!n.cam_tem)||corpo_n)){m=5;if((P.adquire&2)&&adq_job&&std::hypot(n.est[0]-est[0],n.est[1]-est[1])<=.5*est[2]){m=7;++promovido_n;}}else if(n.conf_t>=segura_de(n)||(!n.visto&&n.roi_falta>=n.roi_falta0+3))m=1;
            else if(P.npc_roi_conf>0&&n.roi_ok==n.roi_ok0&&n.roi_falta>=n.roi_falta0+unsigned(P.npc_roi_conf)){m=1;++roi_conf_soltou_n;
                if(P.npc_roi_veto_s>0.f){int iv=veto_roi_n<8?veto_roi_n++:0;if(veto_roi_n>=8){double vt=1e30;for(int i=0;i<8;++i)if(veto_roi[i][3]<vt){vt=veto_roi[i][3];iv=i;}}
                    veto_roi[iv][0]=n.est[0];veto_roi[iv][1]=n.est[1];veto_roi[iv][2]=n.est[2];veto_roi[iv][3]=n2_relogio;veto_roi[iv][4]=veto_roi[iv][5]=0;veto_roi[iv][6]=n2_relogio;}}else if(fora_tela)m=2;else if(n.rapido_t>=P.npc2_rapido_s)m=4;

            if((P.npc_todos&64)&&n.vida_pub){
                n.fora_t=fora_tela?n.fora_t+dt:0.0;
                {const double pf=prof_em(n.est[0],n.est[1],n.est[2]);n.prof_ruim_t=(fora_tela||!(pf>0)||outra_prof(n,pf))?n.prof_ruim_t+dt:0.0;}
                const bool perdida=n.fora_t>=double(P.todos_fora_s)||n.prof_ruim_t>=double(P.todos_prof_s)||n.conf_t>=double(P.todos_perde_s);
                if((m==1||m==2||m==4||(m==5&&!corpo_n))&&!perdida){m=0;if(!n.esmaece){n.esmaece=true;oc_novo[k]=true;++esmaeceu_n;}}
                else if(!m&&n.esmaece&&n.conf_t<1e-9&&!fora_tela&&!(na_zona&&!n.cam_tem)){n.esmaece=false;oc_volta[k]=true;}}
            if(m&&P.npc_vida&&n.vida_pub){int iv=0;if(vida_mem_n<8)iv=vida_mem_n++;else{double vt=1e30;for(int i=0;i<8;++i)if(vida_mem[i][3]<vt){vt=vida_mem[i][3];iv=i;}}
                vida_mem[iv][0]=n.est[0];vida_mem[iv][1]=n.est[1];vida_mem[iv][2]=n.est[2];vida_mem[iv][3]=vida_relogio;
                vida_mem[iv][4]=(m==5&&tem_est)?1.25*est[2]:0.0;}

            if(m&&P.npc_vida&&n.vida_pub&&(m==1||m==2||m==4)&&!sobre_ela(n.est[0],n.est[1])){vida_sai[k]=true;sai_est[k][0]=n.est[0];sai_est[k][1]=n.est[1];sai_est[k][2]=n.est[2];
                sai_rec[k][0]=n.rec[0];sai_rec[k][1]=n.rec[1];sai_lado[k]=n.tem_rec?n.lado:0;if(!sai_lado[k])vida_sai[k]=false;}
            if(m){solta(n,m,k);continue;}
            if(!coberto[k]&&n.conf_t>=P.npc2_troca_s&&n2c_tem&&n2c_nota>=P.npc2_nota_seg&&!na_frente(n,n2c_prof)){const double r=n2c_est[2]/(std::max)(n.est[2],1.0);
                if(r>=.7&&r<=1.4&&std::hypot(n2c_est[0]-n.est[0],n2c_est[1]-n.est[1])<=P.npc2_reancora*n.est[2]){
                    n.est[0]=n2c_est[0];n.est[1]=n2c_est[1];n.est[2]=n2c_est[2];n.nota=n2c_nota;n.conf_t=0;if(n2c_prof>0)n.prof=n2c_prof;
                    n.yu[0]=n2c_est[0];n.yu[1]=n2c_est[1];n.yu[2]=n2c_est[2];n.yu_tem=true;n2c_tem=false;n2c_n=0;++n.reancoras;++n.vida_hits;n.cn.zera();}}
        }

        bool algum=false;for(int k=0;k<K;++k)algum=algum||n2s[k].tem;
        const int kp=(P.npc_todos&16)?(std::min)(P.todos_perto,K):0;
        auto vaga_livre=[&](double l)->int{auto livre=[&](int k){return !n2s[k].tem&&!vida_sai[k];};
            if(kp<=0){for(int k=0;k<K;++k)if(livre(k))return k;return -1;}
            const bool grande=l*esc4k/.36>double((P.npc_todos&32)?450:P.todos_perto_l);
            for(int pass=0;pass<2;++pass){const bool g2=(pass==0)==grande;for(int k=g2?0:kp;k<(g2?kp:K);++k)if(livre(k))return k;}return -1;};

        const bool conf_tempo=!(P.npc2_conf_s>0.f)||n2c_tl-n2c_t0>=double(P.npc2_conf_s);
        if(n2c_tem&&((n2c_n>=P.npc2_conf_n&&conf_tempo)||(n2c_n>=1&&n2c_nota>=P.npc2_nota_rapida&&!algum))&&(!lim_vel||n2c_vel<=P.npc2_vel_max)){
            int kl=vaga_livre(n2c_est[2]);
            if(kl<0){double pior=-1;for(int k=0;k<K;++k){const auto& n=n2s[k];if(!coberto[k]&&n.conf_t>=P.npc2_troca_s&&n.conf_t>pior){pior=n.conf_t;kl=k;}}}
            if(kl<0)++sem_lugar_n;
            if(kl>=0){auto& n=n2s[kl];
                n.vel=n2c_vel;n.prof=n2c_prof;n.ocl_t=0;
                n.est[0]=n2c_est[0];n.est[1]=n2c_est[1];n.est[2]=n2c_est[2];n.nota=n2c_nota;n.tem=true;n.tem_rec=false;n.lado=0;n.id=++n2_prox_id;++n.presos;n.cn.zera();n.cn_lado=0;
                n.yu[0]=n2c_est[0];n.yu[1]=n2c_est[1];n.yu[2]=n2c_est[2];n.yu_tem=true;n.conf_t=0;n.desde=0;n.visto=false;n.roi_falta0=n.roi_falta;n.roi_ok0=n.roi_ok;n.t_preso=g.t;n2c_tem=false;n2c_n=0;n.vida_pub=false;n.vida_hits=0;n.vida_re=false;n.esmaece=false;n.fora_t=n.prof_ruim_t=0;
                if(P.npc_vida)for(int i=0;i<vida_mem_n;++i){const auto* e=vida_mem[i];const double r=n.est[2]/(std::max)(e[2],1.0);
                    if(r>=.67&&r<=1.5&&std::hypot(n.est[0]-e[0],n.est[1]-e[1])<=(std::max)(double(P.vida_re_dist)*(std::max)(e[2],n.est[2]),e[4])){n.vida_re=true;++vida_re_n;
                        for(int c=0;c<5;++c)vida_mem[i][c]=vida_mem[vida_mem_n-1][c];--vida_mem_n;break;}}}}

        if(P.npc_todos&2)for(int i=0;i<dir_n;++i){const double* c=dir_c[i];bool de=false;
            for(int k=0;k<K&&!de;++k){const auto& n=n2s[k];de=n.tem&&std::hypot(c[0]-n.est[0],c[1]-n.est[1])<=((P.npc_todos&8)?.5:.75)*(std::max)(n.est[2],c[2]);}
            if(de)continue;int kl=vaga_livre(c[2]);
            if(kl<0){++sem_lugar_n;break;}
            auto& n=n2s[kl];n.vel=c[5];n.prof=c[4];n.ocl_t=0;
            n.est[0]=c[0];n.est[1]=c[1];n.est[2]=c[2];n.nota=float(c[3]);n.tem=true;n.tem_rec=false;n.lado=0;n.id=++n2_prox_id;++n.presos;n.cn.zera();n.cn_lado=0;
            n.yu[0]=c[0];n.yu[1]=c[1];n.yu[2]=c[2];n.yu_tem=true;n.conf_t=0;n.desde=0;n.visto=false;n.roi_falta0=n.roi_falta;n.roi_ok0=n.roi_ok;n.t_preso=g.t;n.vida_pub=false;n.vida_hits=0;n.vida_re=false;n.esmaece=false;n.fora_t=n.prof_ruim_t=0;++diretos_n;
            if(n2c_tem&&std::hypot(c[0]-n2c_est[0],c[1]-n2c_est[1])<=n2c_est[2]){n2c_tem=false;n2c_n=0;}
            if(P.npc_vida)for(int j=0;j<vida_mem_n;++j){const auto* e=vida_mem[j];const double r=n.est[2]/(std::max)(e[2],1.0);
                if(r>=.67&&r<=1.5&&std::hypot(n.est[0]-e[0],n.est[1]-e[1])<=(std::max)(double(P.vida_re_dist)*(std::max)(e[2],n.est[2]),e[4])){n.vida_re=true;++vida_re_n;
                    for(int q2=0;q2<5;++q2)vida_mem[j][q2]=vida_mem[vida_mem_n-1][q2];--vida_mem_n;break;}}}
        dir_n=0;

        for(int a=0;a<K;++a)for(int b=a+1;b<K;++b){auto& na=n2s[a];auto& nb=n2s[b];if(!na.tem||!nb.tem)continue;

            const double lmin=(std::min)(na.est[2],nb.est[2]),lmax=(std::max)(na.est[2],nb.est[2]);
            if(lmax<=1.5*lmin&&std::hypot(na.est[0]-nb.est[0],na.est[1]-nb.est[1])<P.npc2_dup*lmin){

                if((P.npc_todos&32)&&na.vida_pub&&nb.vida_pub){
                    const double da=std::hypot(na.est[0]-na.est_ini[0],na.est[1]-na.est_ini[1])/(std::max)(na.est[2],1.0),db=std::hypot(nb.est[0]-nb.est_ini[0],nb.est[1]-nb.est_ini[1])/(std::max)(nb.est[2],1.0);
                    if((da>.4)!=(db>.4)){auto& j=da>.4?na:nb;j.est[0]=j.est_ini[0];j.est[1]=j.est_ini[1];j.est[2]=j.est_ini[2];j.cn.zera();++dup_desfeito_n;continue;}

                    if((P.npc_todos&64)&&na.esmaece!=nb.esmaece){const bool a_velho=na.desde>=nb.desde;auto& v=a_velho?na:nb;
                        if(v.esmaece){v.esmaece=false;oc_volta[a_velho?a:b]=true;v.conf_t=0;}solta(a_velho?nb:na,6,a_velho?b:a);continue;}}
                if(P.npc_vida&&na.vida_pub!=nb.vida_pub){if(na.vida_pub)solta(nb,6,b);else solta(na,6,a);}
                else if(na.desde>=nb.desde)solta(nb,6,b);else solta(na,6,a);}}

        auto dentro_logo=[&](const Npc2Slot& n){double mx,my;mv_em(g,q,n.est[0],n.est[1],mx,my);const double T=double(P.todos_fica)*fps,x=n.est[0]-mx*T,y=n.est[1]-my*T,mb=double(P.vida_borda)*n.est[2];
            return x>=mb&&y>=mb&&x<g.w-mb&&y<g.h-mb&&n.est[2]*esc4k<=double(P.todos_cab_max);};
        int mig_de[npc_det_max],mig_para[npc_det_max];for(int k=0;k<npc_det_max;++k){mig_de[k]=-1;mig_para[k]=-1;}
        if(kp>0)for(int k=kp;k<K;++k){auto& n=n2s[k];n.mig_n=(n.tem&&n.tem_rec&&n.lado>P.todos_mig_l)?n.mig_n+1:0;if(!n.tem||!n.vida_pub||n.vida_oculto||n.esmaece||!n.tem_rec||n.lado<=P.todos_mig_l)continue;
            if((P.npc_todos&32)&&n.mig_n<2)continue;
            int j=-1;for(int i=0;i<kp&&j<0;++i)if(!n2s[i].tem&&!vida_sai[i]&&!tinha[i])j=i;
            if(j<0)continue;
            vida_sai[k]=true;sai_est[k][0]=n.est[0];sai_est[k][1]=n.est[1];sai_est[k][2]=n.est[2];sai_rec[k][0]=n.rec[0];sai_rec[k][1]=n.rec[1];sai_lado[k]=n.lado;
            n2s[j]=n;if(!(P.npc_todos&32))n2s[j].id=++n2_prox_id;else{mig_de[j]=k;mig_para[k]=j;}n2s[j].mig_n=0;n2s[j].tem_rec=false;n2s[j].t_preso=g.t;n2s[j].saida=Resultado{};solta(n,6,k);n.mig_n=0;++migrou_n;}

        for(int k=0;k<npc_det_max;++k){auto& n=n2s[k];Resultado S{};bool mov=false;
            if(n.tem){++n.jobs;
                const double mb=double(P.vida_borda)*n.est[2];
                if(P.npc_vida&&!n.vida_pub&&n.vida_hits>=(n.vida_re?1:P.vida_conf_n)&&(!(P.npc_todos&32)||(n.conf_t<=double(P.todos_recente)&&dentro_logo(n)))&&(n.vida_re||n.desde>=double(P.vida_in_s)||(P.npc_vida>=2&&(n.desde>=double(P.vida2_in_s)||(P.npc_todos&4))&&n.roi_ok>=n.roi_ok0+((P.npc_todos&4)?1u:2u)&&n.nota>=P.vida2_nota&&
                   n.est[0]>=2*mb&&n.est[1]>=2*mb&&n.est[0]<g.w-2*mb&&n.est[1]<g.h-2*mb))&&(P.npc_roi_conf<=0||n.roi_ok>=n.roi_ok0+unsigned((std::max)(1,P.vida_roi_n)))&&
                   n.est[0]>=mb&&n.est[1]>=mb&&n.est[0]<g.w-mb&&n.est[1]<g.h-mb){n.vida_pub=true;S.vida_pub_novo=true;}
                S.vida_pend=P.npc_vida&&(!n.vida_pub||n.vida_oculto||n.esmaece);S.vida_re=n.vida_re;S.vida_oculto=n.vida_oculto||n.esmaece;S.vida_oculto_novo=oc_novo[k];S.vida_volta=oc_volta[k];S.vida_camq=camq[k]||n.vida_oculto;
                int alvo=int(std::ceil(1.5*n.est[2]*esc4k/128.0))*128;alvo=std::clamp(alvo,(std::min)(P.npc2_recorte_min,P.recorte_max),P.recorte_max);
                if(P.npc_fracao>0.f)n.lado=det::npc_lado_fracao(n.lado,n.est[2]*esc4k,P.npc_fracao,(std::min)(P.npc_frac_teto,P.recorte_max),P.recorte_max,P.npc_amplia);
                else if(P.npc_degraus){
                    const int rmin=(std::min)(P.npc2_recorte_min,P.recorte_max),st[3]{rmin,(std::max)(rmin,1024),(std::max)(rmin,1536)};
                    const double pede=1.5*n.est[2]*esc4k;auto degrau=[&](double v){for(int s:st)if(s>=v)return (std::min)(s,P.recorte_max);return (std::min)(st[2],P.recorte_max);};
                    if(!n.lado||pede>n.lado)n.lado=degrau(pede);else{const int d=degrau(1.15*pede);if(d<n.lado)n.lado=d;}}
                else if(!n.lado||alvo>n.lado||alvo<=n.lado-256)n.lado=alvo;
                if(P.npc2_recorte_forcado>0)n.lado=(std::min)(P.npc2_recorte_forcado,P.recorte_max);
                if(P.npc_canon){
                    if(n.cn.tem&&std::hypot(n.cn.cx-n.est[0],n.cn.cy-n.est[1])>n.est[2]){n.cn.zera();++n.cn_sem;}
                    const bool tr=n.cn.tem;const double cx=tr?n.cn.cx:n.est[0],cy=tr?n.cn.cy:n.est[1],s=tr?n.cn.s:n.est[2]/(std::max)(double(P.canon.razao),.1);
                    const int lmin=(std::max)(256,P.canon.rede/2),lmax=(std::min)(P.npc_frac_teto,P.recorte_max);
                    const int L=nrcanon::lado(s,esc4k,P.canon,lmin,lmax);
                    if(!n.cn_lado||std::abs(L-n.cn_lado)>=(std::max)(8,int(.015*n.cn_lado)))n.cn_lado=L;
                    n.lado=n.cn_lado;const double lr=n.lado/esc4k;
                    const double ccx=std::clamp(cx,.5*lr,(std::max)(.5*lr,g.w-.5*lr)),ccy=std::clamp(cy,.5*lr,(std::max)(.5*lr,g.h-.5*lr));
                    if(!n.tem_rec||std::hypot(ccx-n.cn_rec[0],ccy-n.cn_rec[1])*esc4k>=(std::max)(1.5,.008*n.lado)){n.cn_rec[0]=ccx;n.cn_rec[1]=ccy;mov=true;if(n.tem_rec)++n.movs;n.tem_rec=true;}
                    n.rec[0]=det::arred(n.cn_rec[0]);n.rec[1]=det::arred(n.cn_rec[1]);
                    S.rec_f_tem=true;S.rec_f[0]=float(n.cn_rec[0]);S.rec_f[1]=float(n.cn_rec[1]);}
                const double lr=n.lado/esc4k;
                if(!P.npc_canon){

                auto centro=[&](int* c){c[0]=det::arred(std::clamp(n.est[0],.5*lr,(std::max)(.5*lr,g.w-.5*lr)));c[1]=det::arred(std::clamp(n.est[1],.5*lr,(std::max)(.5*lr,g.h-.5*lr)));};
                if(!n.tem_rec){centro(n.rec);n.tem_rec=true;mov=true;}
                else{const double meio=.5*P.npc2_centro*lr;
                    if(std::fabs(n.est[0]-n.rec[0])+n.est[2]/2>meio||std::fabs(n.est[1]-n.rec[1])+n.est[2]/2>meio){int c[2];centro(c);
                        if(c[0]!=n.rec[0]||c[1]!=n.rec[1]){n.rec[0]=c[0];n.rec[1]=c[1];mov=true;++n.movs;}}}
                }
                S.on=true;S.tem_est=true;S.est[0]=float(n.est[0]);S.est[1]=float(n.est[1]);S.est[2]=float(n.est[2]);
                S.tem_recorte=true;S.recorte[0]=n.rec[0];S.recorte[1]=n.rec[1];S.recorte[2]=int(std::lround(n.lado*((std::max)(1.f,g.saida_h)/2160.0)));
                S.cab4k=float(n.est[2]*esc4k);S.rec_mov=mov;S.conf=n.nota;
                if(n.cam_tem){S.tem_camada=true;S.camada[0]=float(n.cam[0]);S.camada[1]=float(n.cam[1]);}
            }else if(vida_sai[k]){
                S.vida_sai=true;S.tem_est=true;S.est[0]=float(sai_est[k][0]);S.est[1]=float(sai_est[k][1]);S.est[2]=float(sai_est[k][2]);
                S.tem_recorte=true;S.recorte[0]=sai_rec[k][0];S.recorte[1]=sai_rec[k][1];S.recorte[2]=int(std::lround(sai_lado[k]*((std::max)(1.f,g.saida_h)/2160.0)));
                S.cab4k=float(sai_est[k][2]*esc4k);S.motivo_off=8;}
            else{S.motivo_off=8;}
            S.troca_alvo=((n.tem!=tinha[k])&&!vida_sai[k])||(n.tem&&n.id!=id_ant[k]);
            int m=motivo[k];
            if(n.solto_corte){n.solto_corte=false;S.troca_alvo=true;m=3;}
            if(n.salto_id){n.salto_id=false;if(n.tem)S.troca_alvo=true;}
            S.npc_cab4k=n2c_tem?float(n2c_est[2]*esc4k):0.f;S.rf_n=n2c_n;S.rf_motivo=m;S.vida_era_pub=era_pub[k];S.mig_de=mig_de[k];S.mig_para=mig_para[k];
            n.saida=S;}
    }

    void npc_passo(const Grades& g,const std::vector<det::Cand>& cands,const std::vector<double>& nb,int ib,bool corte,double esc4k,Resultado& R){
        const int W=g.W,H=g.H,q=P.q;
        const double ccx=.5*(W-1),ccy=.5*(H-1),rmin=.05*H;

        const float med=mediana_amostra(g.dq,size_t(W)*size_t(H));
        const float pj=ib>=0?float(cands[size_t(ib)].d):0.f,teto_f=pj>0.f?(std::min)(med,.5f*pj):med;
        int bx0=W,bx1=-1,by0=H;
        if(tem_est){const double l=est[2]/q;bx0=int(std::floor(est[0]/q-1.5*l));bx1=int(std::ceil(est[0]/q+1.5*l));by0=int(std::floor(est[1]/q-l));}
        double s=0;int n=0;
        for(int y=0;y<H;++y)for(int x=0;x<W;++x){
            if(x>=bx0&&x<=bx1&&y>=by0)continue;
            const size_t i=size_t(y)*W+x;if(!(g.dq[i]<=teto_f))continue;
            const double dx=x-ccx,dy=y-ccy,r=std::hypot(dx,dy);if(r<rmin)continue;
            const double v=-(double(g.mvq[2*i])*dx+double(g.mvq[2*i+1])*dy)/r;if(!std::isfinite(v))continue;
            s+=v;++n;}
        const double bruto=n>=64?s/n:0.0;

        const double dt=(g.t>=0&&npc_t_ant>=0)?std::clamp(g.t-npc_t_ant,0.0,.5):double(g.k)/60.0;npc_t_ant=g.t;
        npc_desde+=dt;
        if(corte){npc_ema=0;npc_ativo=false;npc_liga_t=npc_desl_t=0;}
        else if(n>=64)npc_ema+=double(P.npc_alfa)*(bruto-npc_ema);
        const double thr=P.npc_expande;
        if(!npc_ativo){npc_liga_t=npc_ema>thr?npc_liga_t+dt:0.0;
            if(npc_liga_t>=P.npc_liga_s&&npc_desde>=P.npc_fica_s){npc_ativo=true;npc_desl_t=0;}}
        else{npc_desl_t=npc_ema<.5*thr?npc_desl_t+dt:0.0;
            const bool ja=n>=64&&bruto<-.5*thr;
            if(ja||(npc_desl_t>=P.npc_desliga_s&&npc_desde>=P.npc_fica_s)){npc_ativo=false;npc_liga_t=0;}}
        R.npc_ativo=npc_ativo;R.npc_expande=float(npc_ema);R.npc_bruto=float(bruto);R.npc_celulas=n;
        bool mov=false;
        if(!(npc_ativo&&tem_est&&!corte)){npc_tem=false;npc_tem_rec=false;npc_perdido=0;npc_troca_c=0;}
        else{

            double px=0,py=0;
            if(npc_tem){const int gx=std::clamp(int(npc_est[0])/q,0,W-1),gy=std::clamp(int(npc_est[1])/q,0,H-1);double sx=0,sy=0;int m=0;
                for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){const int xx=gx+dx,yy=gy+dy;if(xx<0||yy<0||xx>=W||yy>=H)continue;const size_t i=size_t(yy)*W+xx;sx+=g.mvq[2*i];sy+=g.mvq[2*i+1];++m;}
                px=npc_est[0]-(m?sx/m*q*g.k:0.0);py=npc_est[1]-(m?sy/m*q*g.k:0.0);}
            int im=-1,ip=-1;double cm=0,dp=1e30;
            for(size_t i=0;i<cands.size();++i){if(int(i)==ib)continue;const auto& c=cands[i];
                const double lado=1.25*c.wh*q,cx=c.x*q,cy=c.ytop*q+.5*lado;
                if(std::hypot(cx-est[0],cy-est[1])<=est[2])continue;
                const double cab=lado*esc4k;
                if(cab<P.npc_min||cab>P.npc_recorte||nb[i]<P.npc_nota)continue;
                if(cab>cm){cm=cab;im=int(i);}
                if(npc_tem){const double ref=(std::max)(npc_est[2],1.0),d=std::hypot(cx-px,cy-py)/ref,t=lado/ref;
                    if(d<=1.0&&t<=P.npc_troca_fator&&t*P.npc_troca_fator>=1.0&&d<dp){dp=d;ip=int(i);}}
            }
            R.npc_cab4k=float(cm);
            auto adota=[&](int i,bool novo){const auto& c=cands[size_t(i)];const double lado=1.25*c.wh*q,cx=c.x*q,cy=c.ytop*q+.5*lado;
                if(novo){npc_est[0]=cx;npc_est[1]=cy;npc_est[2]=lado;npc_tem_rec=false;++npc_id;}
                else{const double a=P.alfa_pos,at=P.alfa_tam;npc_est[0]=px+a*(cx-px);npc_est[1]=py+a*(cy-py);npc_est[2]+=at*(lado-npc_est[2]);}
                npc_tem=true;npc_perdido=0;};
            if(ip>=0){
                const double cp=1.25*cands[size_t(ip)].wh*q*esc4k;
                npc_troca_c=(im>=0&&im!=ip&&cm>=P.npc_troca_fator*cp)?npc_troca_c+1:0;
                if(npc_troca_c>=P.npc_troca_n&&npc_desde>=P.npc_fica_s){adota(im,true);npc_troca_c=0;}else adota(ip,false);
            }else if(npc_tem&&++npc_perdido<=P.npc_segura){npc_est[0]=px;npc_est[1]=py;npc_troca_c=0;}
            else if(im>=0){adota(im,true);npc_troca_c=0;}
            else{npc_tem=false;npc_tem_rec=false;npc_troca_c=0;}
            if(npc_tem&&(npc_est[0]<0||npc_est[1]<0||npc_est[0]>=g.w||npc_est[1]>=g.h)){npc_tem=false;npc_tem_rec=false;}

            if(npc_tem){const double lr=P.npc_recorte/esc4k;
                if(!npc_tem_rec){npc_rec[0]=det::arred(npc_est[0]);npc_rec[1]=det::arred(npc_est[1]);npc_tem_rec=true;mov=true;}
                else{const double dz=P.zona_morta*lr;
                    const bool saiu=(std::fabs(npc_est[0]-npc_rec[0])+npc_est[2]/2>lr/2)||(std::fabs(npc_est[1]-npc_rec[1])+npc_est[2]/2>lr/2);
                    if(saiu||std::fabs(npc_est[0]-npc_rec[0])>dz||std::fabs(npc_est[1]-npc_rec[1])>dz){npc_rec[0]=det::arred(npc_est[0]);npc_rec[1]=det::arred(npc_est[1]);mov=true;}}}
        }
        const bool alvo=npc_tem;
        R.alvo_npc=alvo;R.troca_alvo=alvo!=npc_alvo_ant||(alvo&&npc_id!=npc_id_ant);npc_alvo_ant=alvo;npc_id_ant=npc_id;
        if(R.troca_alvo)npc_desde=0;
        if(alvo){R.est[0]=float(npc_est[0]);R.est[1]=float(npc_est[1]);R.est[2]=float(npc_est[2]);R.tem_est=true;
            R.recorte[0]=npc_rec[0];R.recorte[1]=npc_rec[1];R.recorte[2]=int(std::lround(P.npc_recorte*((std::max)(1.f,g.saida_h)/2160.0)));R.tem_recorte=true;
            R.on=true;R.corte=false;R.cab4k=float(npc_est[2]*esc4k);R.rec_mov=mov;R.motivo_off=0;}
    }
    void candidatos(const Grades& g,const std::vector<float>& lq,double kface,float dmin,std::vector<det::Cand>& cands) const {
        cands.clear();
        const int W=g.W,H=g.H,q=P.q;const float T=P.borda_ln;

        std::vector<unsigned char> e(size_t(W)*H,0);
        for(int y=0;y<H;++y)for(int x=0;x+1<W;++x){const float dx=lq[size_t(y)*W+x+1]-lq[size_t(y)*W+x];if(dx>T)e[size_t(y)*W+x]=1;if(-dx>T)e[size_t(y)*W+x+1]=1;}
        for(int y=0;y+1<H;++y)for(int x=0;x<W;++x){const float dy=lq[size_t(y+1)*W+x]-lq[size_t(y)*W+x];if(dy>T)e[size_t(y)*W+x]=1;if(-dy>T)e[size_t(y+1)*W+x]=1;}
        std::vector<unsigned char> ne(e.size());for(size_t i=0;i<e.size();++i)ne[i]=e[i]?0:1;
        std::vector<int> lab;std::vector<det::Comp> comps;det::rotular(ne.data(),W,H,false,lab,comps);
        std::vector<int> ordem(comps.size());for(size_t i=0;i<ordem.size();++i)ordem[i]=int(i)+1;
        std::stable_sort(ordem.begin(),ordem.end(),[&](int a,int b){return comps[size_t(a-1)].area>comps[size_t(b-1)].area;});
        std::vector<unsigned char> m,tampa;std::vector<int> labt;std::vector<det::Comp> compt;std::vector<float> larg,cent,vals;
        for(size_t oi=0;oi<ordem.size()&&oi<12;++oi){
            const int c=ordem[oi];const auto& cc=comps[size_t(c-1)];
            const int area=cc.area;if(area<P.area_min)break;
            const int x=cc.x0,y=cc.y0,w=cc.x1-cc.x0+1,h=cc.y1-cc.y0+1;
            m.assign(size_t(w)*h,0);vals.clear();
            for(int r=0;r<h;++r)for(int s=0;s<w;++s)if(lab[size_t(y+r)*W+x+s]==c){m[size_t(r)*w+s]=1;vals.push_back(lq[size_t(y+r)*W+x+s]);}
            const double dmed=std::exp(double(det::mediana(vals)));
            if(dmed<dmin)continue;
            if(w>.9*W&&h>.6*H)continue;
            const double wexp0=kface*dmed/q;
            tampa.assign(m.size(),0);
            for(int r=0;r<h;++r)for(int s=0;s<w;++s){const size_t i=size_t(r)*w+s;tampa[i]=m[i]&&(r==0||!m[i-size_t(w)]);}
            if(y==0)for(int s=0;s<w;++s)tampa[size_t(s)]=0;
            det::rotular(tampa.data(),w,h,true,labt,compt);
            for(size_t ti=0;ti<compt.size();++ti){
                const auto& t=compt[ti];const int tt=int(ti)+1;
                const int tw=t.x1-t.x0+1,ta=t.area;
                if(tw<2||ta<2)continue;
                if(tw>4*(std::max)(wexp0,4.0))continue;
                int ymin=1<<30;for(int r=t.y0;r<=t.y1;++r)for(int s=t.x0;s<=t.x1;++s)if(labt[size_t(r)*w+s]==tt){ymin=(std::min)(ymin,r);}
                double somx=0;int nx=0,acima_n=0,tot=0;
                for(int r=t.y0;r<=t.y1;++r)for(int s=t.x0;s<=t.x1;++s){
                    if(labt[size_t(r)*w+s]!=tt)continue;
                    if(r<=ymin+1){somx+=s;++nx;}
                    const int gy=r+y,gx=s+x;
                    const float a1=lq[size_t((std::max)(gy-1,0))*W+gx],a2=lq[size_t((std::max)(gy-2,0))*W+gx];
                    if((std::max)(a1,a2)>lq[size_t(gy)*W+gx]+T)++acima_n;++tot;
                }
                const int yy=ymin;const double xm=somx/(std::max)(nx,1);
                const double topo_oc=tot?double(acima_n)/tot:0.0;
                const int gx0=int((std::max)(0.0,x+xm-3)),gx1=int(x+xm+4);
                vals.clear();
                for(int r=y+yy;r<(std::min)(H,y+yy+7);++r)for(int s=gx0;s<(std::min)(W,gx1);++s)if(lab[size_t(r)*W+s]==c)vals.push_back(g.dq[size_t(r)*W+s]);
                if(vals.size()<3)continue;
                double dcab=det::mediana(vals);
                double wexp=kface*dcab/q;
                det::runs_cabeca(m.data(),w,h,xm,yy,int(4.5*wexp)+4,larg,cent);
                if(larg.size()<4)continue;
                const int a0=int(.15*wexp*1.3),a1=(std::max)(a0+2,int(.7*wexp*1.3));
                if(a0>=int(larg.size()))continue;
                std::vector<float> fa(larg.begin()+a0,larg.begin()+(std::min<size_t>)(size_t(a1),larg.size()));
                const double wh=det::mediana(fa);
                if(!std::isfinite(wh)||wh<2)continue;
                const int b0=int(1.4*wh),b1=int(3.2*wh)+1;
                const double ombro=b0<int(larg.size())?double(det::maxv(larg,size_t(b0),size_t(b1))):0.0;
                const double razao_omb=ombro/wh;
                double alt=double(larg.size())/wh;
                for(size_t i=0;i<larg.size();++i)if(larg[i]>1.6*wh){alt=double(i)/wh;break;}
                std::vector<float> fc(cent.begin()+(std::min<size_t>)(size_t(a0),cent.size()),cent.begin()+(std::min<size_t>)(size_t((std::max)(a1,a0+1)),cent.size()));
                const double xcab=det::mediana(fc)+x,ycab=yy+y;
                {const int ry0=int(ycab),ry1=int(ycab+wh)+1,rx0=int((std::max)(0.0,xcab-wh/2)),rx1=int(xcab+wh/2)+1;vals.clear();
                 for(int r=ry0;r<(std::min)(H,ry1);++r)for(int s=rx0;s<(std::min)(W,rx1);++s)if(lab[size_t(r)*W+s]==c)vals.push_back(g.dq[size_t(r)*W+s]);
                 if(vals.size()>=3)dcab=det::mediana(vals);}
                det::Cand k{};k.x=xcab;k.ytop=ycab;k.wh=wh;k.topo_oc=topo_oc;k.alt=alt;k.wexp=kface*dcab/q;k.d=dcab;k.omb=razao_omb;
                k.area=area;k.bx=x;k.by=y;k.bw=w;k.bh=h;k.fundo=y+h>=H-2;
                cands.push_back(k);
            }
        }
    }
    double pontua(const det::Cand& c,int W,int H,const Grades& g,double kface,bool plano=false,double* sem_rastreio=nullptr,double* fat=nullptr) const {
        (void)kface;
        const double sh=double(g.h)/720.0;
        const double f_omb=det::clip01(float((c.omb-1.15)/.6));
        const double rz=c.wh/(std::max)(c.wexp,1e-6);
        const double lr=std::log((std::max)(rz,1e-3))/.55;const double f_tam=std::exp(-lr*lr);
        const double f_corpo=(c.fundo?.6:0.0)+.4*det::clip01(float(c.area/(.04*W*H)));
        const double hx=c.x/W,hy=(c.ytop+.6*c.wh)/H;
        const double ax=(hx-P.pos_x)/P.sig_x,ay=(hy-P.pos_y)/P.sig_y;double f_pos=std::exp(-.5*(ax*ax+ay*ay));
        if(P.generico){
            if(P.usa_centro){const double bx=(hx-P.pos_centro)/P.sig_x;f_pos=(std::max)(f_pos,std::exp(-.5*(bx*bx+ay*ay)));}
            if(P.usa_direita){const double bx=(hx-P.pos_direita)/P.sig_x;f_pos=(std::max)(f_pos,std::exp(-.5*(bx*bx+ay*ay)));}
            if(plano)f_pos=1.0;
            if(trk_npc)f_pos=1.0;
        }
        const int yy=int((std::min<double>)(H-1,c.ytop+.5*c.wh)),xx=int((std::min<double>)(W-1,(std::max)(0.0,c.x)));
        const size_t ic=size_t(yy)*W+xx;
        const double mv_c=std::hypot(g.mvq[2*ic],g.mvq[2*ic+1]);
        const double f_mov=std::exp(-mv_c/(1.5+.5*mv_g));
        double base=.35*f_omb+.20*f_tam+.30*f_corpo+.15*f_mov;
        double f_seg=0;
        if(tem_prev){
            const double px=c.x+g.k*g.mvq[2*ic],py=c.ytop+.5*c.wh+g.k*g.mvq[2*ic+1];
            const double dist=std::hypot(px-prev_x,py-(prev_ytop+.5*prev_wh))/(std::max)(prev_wh,2.0);
            const double lw=std::log(c.wh/(std::max)(prev_wh,1.0))/.4;
            f_seg=std::exp(-(dist/.8)*(dist/.8))*std::exp(-lw*lw);
        }
        base*=(.35+.65*f_pos);
        double tot=tem_prev?.55*base+.45*f_seg:base;
        const double f_abs=det::clip01(float((c.wh*P.q/sh-28.)/32.));
        const double l1=(std::max)(0.0,std::log(rz/2.6))/.25,l2=(std::max)(0.0,std::log(.55/rz))/.25;
        const double f_coer=std::exp(-l1*l1-l2*l2);
        const double f_topo=det::clip01(float(1-(c.topo_oc-.2)/.4));
        const double f_alt=det::clip01(float((c.alt-.9)/.3));
        tot*=f_abs*f_coer*f_topo*f_alt;
        if(sem_rastreio)*sem_rastreio=base*f_abs*f_coer*f_topo*f_alt;
        if(fat){fat[0]=hx;fat[1]=hy;fat[2]=f_pos;fat[3]=f_omb;fat[4]=f_tam;fat[5]=f_corpo;fat[6]=f_alt;}
        return tot;
    }
    bool mede_fino(const Grades& g,const det::Cand& c,double& ox,double& oy,double& owh) const {
        const int w=g.w,h=g.h,q=P.q;const double wr=c.wh*q;
        const int x0=int((std::max)(0.0,c.x*q-1.6*wr)),x1=int((std::min<double>)(w,c.x*q+1.6*wr));
        const int y0=int((std::max)(0.0,c.ytop*q-.6*wr)),y1=int((std::min<double>)(h,c.ytop*q+1.6*wr));
        if(x1-x0<8||y1-y0<8)return false;
        const int rw=x1-x0,rh=y1-y0;const double lc=std::log(c.d);
        std::vector<unsigned char> m(size_t(rw)*rh),er(size_t(rw)*rh),mo(size_t(rw)*rh);
        for(int y=0;y<rh;++y)for(int x=0;x<rw;++x){const float v=g.d[size_t(y0+y)*w+x0+x];m[size_t(y)*rw+x]=std::fabs(std::log((std::max)(v,1e-6f))-lc)<.12?1:0;}

        for(int y=0;y<rh;++y)for(int x=0;x<rw;++x){unsigned char v=1;for(int dy=-1;dy<=1&&v;++dy)for(int dx=-1;dx<=1;++dx){const int yy=y+dy,xx=x+dx;if(yy<0||xx<0||yy>=rh||xx>=rw)continue;if(!m[size_t(yy)*rw+xx]){v=0;break;}}er[size_t(y)*rw+x]=v;}
        for(int y=0;y<rh;++y)for(int x=0;x<rw;++x){unsigned char v=0;for(int dy=-1;dy<=1&&!v;++dy)for(int dx=-1;dx<=1;++dx){const int yy=y+dy,xx=x+dx;if(yy<0||xx<0||yy>=rh||xx>=rw)continue;if(er[size_t(yy)*rw+xx]){v=1;break;}}mo[size_t(y)*rw+x]=v;}
        std::vector<int> lab;std::vector<det::Comp> comps;det::rotular(mo.data(),rw,rh,true,lab,comps);
        const int cx=int((std::min<double>)(rw-1,(std::max)(0.0,c.x*q-x0))),cy=int((std::min<double>)(rh-1,(std::max)(0.0,c.ytop*q-y0+.5*wr)));
        int l=lab[size_t(cy)*rw+cx];
        if(l==0){
            std::vector<int> cnt(comps.size()+1,0);bool tem=false;
            for(int y=(std::max)(0,cy-6);y<(std::min)(rh,cy+7);++y)for(int x=(std::max)(0,cx-6);x<(std::min)(rw,cx+7);++x){const int v=lab[size_t(y)*rw+x];if(v>0){++cnt[size_t(v)];tem=true;}}
            if(!tem)return false;
            l=int(std::max_element(cnt.begin(),cnt.end())-cnt.begin());
        }
        std::vector<unsigned char> mm(size_t(rw)*rh);for(size_t i=0;i<mm.size();++i)mm[i]=lab[i]==l?1:0;
        const int xc=det::arred(c.x*q-x0);const int r=(std::max)(3,int(.35*wr));
        std::vector<int> tops;
        for(int x=(std::max)(0,xc-r);x<(std::min)(rw,xc+r+1);++x){for(int y=0;y<rh;++y)if(mm[size_t(y)*rw+x]){tops.push_back(y);break;}}
        if(tops.empty())return false;
        std::sort(tops.begin(),tops.end());
        const double ytop=tops[(std::min)(tops.size()-1,(std::max<size_t>)(0,tops.size()/5))];
        std::vector<float> larg,cent;det::runs_cabeca(mm.data(),rw,rh,xc,int(ytop),int(1.3*wr)+2,larg,cent);
        const int a0=int(.2*wr),a1=(std::max)(int(.2*wr)+2,int(.8*wr));
        std::vector<float> seg,cs;
        for(int i=a0;i<(std::min<int>)(a1,int(larg.size()));++i){if(larg[size_t(i)]>0)seg.push_back(larg[size_t(i)]);cs.push_back(cent[size_t(i)]);}
        if(seg.empty())return false;
        owh=det::mediana(seg);ox=det::mediana(cs)+x0;oy=ytop+y0;
        return true;
    }
};
}
