#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace nrrj {

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
};

struct Grades {
    int W=0,H=0;
    const float* dq=nullptr;
    const float* mvq=nullptr;
    int w=0,h=0;
    const float* d=nullptr;
    float k=1.f;
    float esc=3.f;
    float saida_h=2160.f;
};

struct Resultado {
    bool on=false,corte=false,grande=false,cinema=false,tem_est=false,tem_recorte=false,rec_mov=false;
    float conf=0.f,ok=1.f;
    float est[3]{};
    int recorte[3]{};
    float cab4k=0.f,escala=0.f;int perdido=0,ncand=0;

    bool recontou=false;
    float tend=-1.f;
    bool conv_errada=false,busca=false,adotou=false,duvida=false;float rival=0.f,rival_xy[3]{},nb_melhor=0.f;bool direita=false;int votos=0;unsigned motivo_off=0;float kface=0.f,med_prox=0.f,ok_neg=0.f;int sinal=1,hip=0;
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

            if(P.generico&&mv_g>P.sinal_mov){
                if(okn>ok+P.sinal_dif)++sinal_votos;else if(ok>okn+P.sinal_dif)sinal_votos=0;
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
        if(corte){tem_prev=false;tem_est=false;seg_corte=(P.corte_modo&2)?0:P.corte_seg;tem_recorte=false;tem_escala=false;}
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
        bool tem_meas=false;double mx=0,mytop=0,mwh=0,md=0;
        const double conf=ib>=0?bs:0.0;

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
            const double mvx=n?sx/n*q*g.k:0,mvy=n?sy/n*q*g.k:0;
            pred[0]=est[0]-mvx;pred[1]=est[1]-mvy;pred[2]=est[2];tem_pred=true;
        }
        double cx=0,cy=0,lado=0,salto=0;bool tem_caixa=false;
        if(tem_meas){
            lado=1.25*mwh;cx=mx;cy=mytop+.5*lado;tem_caixa=true;
            if(tem_pred){
                salto=std::hypot(cx-pred[0],cy-pred[1])/(std::max)(pred[2],1.0);
                if(salto>=.6&&conf<LIGA&&perdido<P.perde_n)tem_meas=false;
            }
        }
        if(tem_meas){
            tem_prev=true;prev_x=mx/q;prev_ytop=mytop/q;prev_wh=mwh/q;perdido=0;
            if(tem_pred&&salto<.6){const double a=P.alfa_pos,at=P.alfa_tam;
                est[0]=pred[0]+a*(cx-pred[0]);est[1]=pred[1]+a*(cy-pred[1]);est[2]=pred[2]+at*(lado-pred[2]);}
            else{est[0]=cx;est[1]=cy;est[2]=lado;}
            tem_est=true;
        }else{
            ++perdido;
            if(tem_pred&&perdido<=P.perde_n){
                est[0]=pred[0];est[1]=pred[1];est[2]=pred[2];tem_est=true;
                if(tem_prev){prev_x=pred[0]/q;prev_ytop=(pred[1]-.5*pred[2])/q;}
            }else{tem_est=false;tem_prev=false;tem_escala=false;}
        }
        const double esc4k=g.esc*(2160.0/(std::max)(1.f,g.saida_h));
        const double cab4k=tem_est?est[2]*esc4k:0.0;
        const bool grande=!P.persistente&&cab4k>P.cab_max_4k;
        double esc_m=0;bool tem_esc_m=false;
        if(tem_meas){esc_m=mwh/(std::max)(md,1e-6)/sh;tem_esc_m=true;
            if(tem_escala)escala=.8*escala+.2*esc_m;else{escala=esc_m;tem_escala=true;}}

        const double fcin=P.generico?((P.corte_modo&8)?(std::max)(1.0,kf/P.k_face):kf/P.k_face):1.0;
        const bool cinema=tem_escala&&escala>double(P.escala_max)*fcin;
        if(seg_corte>0)--seg_corte;
        const bool fora_img=P.generico&&tem_est&&(est[0]<0||est[1]<0||est[0]>=g.w||est[1]>=g.h);

        const bool bloqueio=corte||grande||(cinema&&!P.cinema_livre)||seg_corte>0||!tem_est||(fora_img&&!(P.persistente&&on))||(P.generico&&sinal_votos>0);
        if(bloqueio){on=false;cont_on=0;}
        else if(on){cont_off=(!tem_meas||conf<P.desliga)?cont_off+1:0;if(cont_off>=(P.persistente?P.desliga_n_pers:P.desliga_n)){on=false;cont_on=0;}}
        else{cont_on=(tem_meas&&conf>=LIGA2&&!duvida)?cont_on+1:0;
            const bool volta=P.persistente&&tem_meas&&ib>=0&&ref_idade>=0&&ref_idade<=P.corte_ref_jobs&&no_ref(cands[size_t(ib)],g);
            if(cont_on>=(volta?1:P.liga_n)){on=true;cont_off=0;}}
        if(P.generico){

            if(on||(tem_meas&&conf>=LIGA))sem_n=0;else ++sem_n;
            if(sem_n>=P.busca_apos&&!corte&&P.busca_passos>0){

                const int nh=2*P.busca_passos+1;int idx=(hip_cursor%nh)-P.busca_passos;++hip_cursor;
                if(idx==kf_idx){idx=(hip_cursor%nh)-P.busca_passos;++hip_cursor;}
                {
                    const double ka=double(P.k_face)*std::pow(2.0,.5*idx);
                    std::vector<det::Cand> ca;candidatos(g,lq,ka*sh,float(double(P.d_min)*P.k_face/ka),ca);
                    int jb=-1;double js=-1;for(size_t i=0;i<ca.size();++i){const double sc=pontua(ca[i],W,H,g,ka*sh,true);if(sc>js){js=sc;jb=int(i);}}
                    bool acha=false;double hx=0,hy=0;
                    if(jb>=0&&js>=LIGA){double fx,fy,fw;const auto& cb=ca[size_t(jb)];const double cw=(std::max)(cb.wh*q,1.0);
                        acha=mede_fino(g,cb,fx,fy,fw)&&fw/cw>=.6&&fw/cw<=1.7;hx=cb.x/W;hy=(cb.ytop+.6*cb.wh)/H;
                        const double esc4=g.esc*(2160.0/(std::max)(1.f,g.saida_h));
                        if(acha&&(1.25*fw*esc4>P.cab_max_4k||fw/(std::max)(cb.d,1e-6)/sh>double(P.escala_max)*ka/P.k_face))acha=false;}
                    const size_t ih=size_t(idx+P.busca_passos)&31u;int& hh=hip_hits[ih];
                    if(acha&&hh>0&&std::hypot(hx-hip_x[ih],hy-hip_y[ih])>.15)hh=0;
                    hh=acha?hh+1:0;if(acha){hip_x[ih]=hx;hip_y[ih]=hy;}
                    R.busca=true;R.hip=idx;
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
        R.on=on;R.corte=corte;R.ok=float(ok);R.grande=grande;R.cinema=cinema;R.conf=float(conf);
        R.tem_est=tem_est;if(tem_est){R.est[0]=float(est[0]);R.est[1]=float(est[1]);R.est[2]=float(est[2]);}
        R.tem_recorte=tem_recorte;if(tem_recorte){R.recorte[0]=recorte[0];R.recorte[1]=recorte[1];R.recorte[2]=int(std::lround(rec_lado*((std::max)(1.f,g.saida_h)/2160.0)));}
        if(tem_est){ref_cab[0]=est[0];ref_cab[1]=est[1];ref_cab[2]=est[2];ref_idade=0;}else if(ref_idade>=0)++ref_idade;
        R.rec_mov=rec_mov;R.cab4k=float(cab4k);R.escala=tem_esc_m?float(esc_m):0.f;R.perdido=perdido;R.ncand=int(cands.size());
        if(!on){unsigned m=0;if(corte)m|=1u;if(grande)m|=2u;if(cinema)m|=4u;if(!tem_est)m|=8u;if(seg_corte>0&&!corte)m|=16u;if(P.generico&&sinal_votos>0)m|=128u;if(duvida)m|=256u;if(fora_img)m|=512u;if(!m)m=32u;R.motivo_off=m;}
        R.kface=float(kf);R.sinal=sinal;R.votos=sinal_votos;if(!R.busca)R.hip=kf_idx;R.direita=P.usa_direita;
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
    void reset_corte(){tem_prev=false;tem_est=false;seg_corte=P.corte_seg;tem_recorte=false;tem_escala=false;on=false;cont_on=0;lq_ant.clear();ref_idade=-1;}

private:
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
    double pontua(const det::Cand& c,int W,int H,const Grades& g,double kface,bool plano=false,double* sem_rastreio=nullptr) const {
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
