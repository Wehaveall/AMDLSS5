#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace nrrj {
struct CabecaProfParams {
    float borda_ln=.08f;
    float area_min=.0015f;
    float cab_min_px=28.f;
    float cab_max=.45f;
    float pos_x=.50f,pos_y=.40f,sig_x=.25f,sig_y=.25f;
    float esc_tol=.40f;
    int esc_n=6;
    float seg_dist=.8f,seg_tam=.4f;
    float nota_min=.30f;
};

class CabecaProf {
public:
    struct Saida {
        bool tem=false;
        double x=0,y=0,lado=0;
        double d=0;
        double nota=0;
        double base=0;
        bool seguiu=false;
        int n_seg=0;
        int ncand=0;
        double cx_g=0,ytop_g=0,wh_g=0;
    };
    CabecaProfParams P;

    double esc=0;int esc_cont=0;
    unsigned long long jobs=0,com_cabeca=0;

    void reset(){tem_ant=false;n_seg=0;}

    void aceita(const Saida& s){
        if(!s.tem||!(s.d>0)||!(s.wh_g>0))return;const double v=s.wh_g/s.d;if(!std::isfinite(v))return;
        if(esc_cont==0)esc=v;else esc+=.15*(v-esc);++esc_cont;}

    Saida passo(const Grades& g,const std::vector<float>& lq,const double* dica){
        Saida S{};++jobs;
        const int W=g.W,H=g.H,q=(std::max)(1,g.w/(std::max)(1,W));
        if(W<8||H<8||!g.dq||!g.mvq||lq.size()<size_t(W)*H){tem_ant=false;n_seg=0;return S;}
        const float T=P.borda_ln;

        e.assign(size_t(W)*H,0);
        for(int y=0;y<H;++y)for(int x=0;x+1<W;++x){const float dx=lq[size_t(y)*W+x+1]-lq[size_t(y)*W+x];if(dx>T)e[size_t(y)*W+x]=1;if(-dx>T)e[size_t(y)*W+x+1]=1;}
        for(int y=0;y+1<H;++y)for(int x=0;x<W;++x){const float dy=lq[size_t(y+1)*W+x]-lq[size_t(y)*W+x];if(dy>T)e[size_t(y)*W+x]=1;if(-dy>T)e[size_t(y+1)*W+x]=1;}
        ne.resize(e.size());for(size_t i=0;i<e.size();++i)ne[i]=e[i]?0:1;
        det::rotular(ne.data(),W,H,false,lab,comps,pilha);
        ordem.resize(comps.size());for(size_t i=0;i<ordem.size();++i)ordem[i]=int(i)+1;
        std::stable_sort(ordem.begin(),ordem.end(),[&](int a,int b){return comps[size_t(a-1)].area>comps[size_t(b-1)].area;});
        const double sh=double(g.h)/720.0,wmin=(std::max)(3.0,P.cab_min_px*sh/q),wmax=P.cab_max*H;
        cs.clear();
        for(size_t oi=0;oi<ordem.size()&&oi<16;++oi){
            const int c=ordem[oi];const auto& cc=comps[size_t(c-1)];
            if(cc.area<P.area_min*W*H)break;
            const int bx=cc.x0,by=cc.y0,w=cc.x1-cc.x0+1,h=cc.y1-cc.y0+1;
            m.assign(size_t(w)*h,0);
            for(int r=0;r<h;++r)for(int s=0;s<w;++s)if(lab[size_t(by+r)*W+bx+s]==c)m[size_t(r)*w+s]=1;
            tampa.assign(m.size(),0);
            for(int r=0;r<h;++r)for(int s=0;s<w;++s){const size_t i=size_t(r)*w+s;tampa[i]=m[i]&&(r==0||!m[i-size_t(w)]);}
            if(by==0)for(int s=0;s<w;++s)tampa[size_t(s)]=0;
            det::rotular(tampa.data(),w,h,true,labt,compt,pilha);
            for(size_t ti=0;ti<compt.size();++ti){
                const auto& t=compt[ti];const int tt=int(ti)+1;
                if(t.x1-t.x0+1<2||t.area<2)continue;
                int ymin=1<<30;for(int r=t.y0;r<=t.y1;++r)for(int s=t.x0;s<=t.x1;++s)if(labt[size_t(r)*w+s]==tt)ymin=(std::min)(ymin,r);
                double somx=0;int nx=0,acima=0,tot=0;
                for(int r=t.y0;r<=t.y1;++r)for(int s=t.x0;s<=t.x1;++s){
                    if(labt[size_t(r)*w+s]!=tt)continue;
                    if(r<=ymin+1){somx+=s;++nx;}
                    const int gy=r+by,gx=s+bx;
                    const float a1=lq[size_t((std::max)(gy-1,0))*W+gx],a2=lq[size_t((std::max)(gy-2,0))*W+gx];
                    if((std::max)(a1,a2)>lq[size_t(gy)*W+gx]+T)++acima;++tot;}
                const double topo_oc=tot?double(acima)/tot:0.0;
                if(topo_oc>.6)continue;
                const double xm=somx/(std::max)(nx,1);
                det::runs_cabeca(m.data(),w,h,xm,ymin,(std::min)(h-ymin,H/2+8),larg,cent);
                if(larg.size()<6)continue;

                double wh=0;for(size_t i=1;i<larg.size();++i)if(larg[i]>0&&double(i)>=.5*larg[i]){wh=larg[i];break;}
                for(int it=0;it<6&&wh>=1;++it){
                    const int a0=int(.15*wh*1.3),a1=(std::max)(a0+2,int(.7*wh*1.3));if(a0>=int(larg.size()))break;
                    fa.assign(larg.begin()+a0,larg.begin()+(std::min)(size_t(a1),larg.size()));
                    const double nw=det::mediana_em(fa);if(!std::isfinite(nw)||nw<1)break;
                    const bool parou=std::fabs(nw-wh)<.5;wh=nw;if(parou)break;}
                if(!(wh>=wmin)||wh>wmax)continue;
                const int a0=int(.15*wh*1.3),a1=(std::max)(a0+2,int(.7*wh*1.3));
                if(a0>=int(larg.size()))continue;
                const int b0=int(1.4*wh),b1=int(3.2*wh)+1;
                const double ombro=b0<int(larg.size())?double(det::maxv(larg,size_t(b0),size_t(b1))):0.0;
                double alt=double(larg.size())/wh;for(size_t i=0;i<larg.size();++i)if(larg[i]>1.6*wh){alt=double(i)/wh;break;}
                int corpo=0;for(size_t i=0;i<larg.size();++i){if(larg[i]>0)corpo=int(i)+1;else break;}
                const bool fundo=by+h>=H-2;
                fc.assign(cent.begin()+(std::min<size_t>)(size_t(a0),cent.size()),cent.begin()+(std::min<size_t>)(size_t(a1),cent.size()));
                if(fc.empty())continue;
                const double xc=det::mediana_em(fc)+bx,yt=double(ymin+by);
                vals.clear();
                {const int ry0=int(yt),ry1=int(yt+wh)+1,rx0=int((std::max)(0.0,xc-wh/2)),rx1=int(xc+wh/2)+1;
                 for(int r=ry0;r<(std::min)(H,ry1);++r)for(int s=rx0;s<(std::min)(W,rx1);++s)if(lab[size_t(r)*W+s]==c)vals.push_back(g.dq[size_t(r)*W+s]);}
                if(vals.size()<3)continue;
                Cab k{};k.x=xc;k.ytop=yt;k.wh=wh;k.d=det::mediana_em(vals);k.omb=ombro/wh;k.alt=alt;k.topo_oc=topo_oc;
                k.corpo=fundo?(std::max)(double(corpo)/wh,3.0):double(corpo)/wh;
                if(!(k.d>0))continue;
                cs.push_back(k);
            }
        }

        {fica.assign(cs.size(),1);
         for(size_t i=0;i<cs.size();++i){const auto& a=cs[i];
            for(size_t j=0;j<cs.size()&&fica[i];++j){if(j==i)continue;const auto& b=cs[j];
                if(std::fabs(std::log(b.d/a.d))>.3)continue;
                if(b.ytop<a.ytop&&std::fabs(b.x-a.x)<.5*a.wh&&b.wh<.8*a.wh)fica[i]=0;
                else if(std::fabs(b.x-a.x)<.25*a.wh&&std::fabs(std::log(b.wh/a.wh))<.15&&std::fabs(b.ytop-a.ytop)<.3*a.wh&&(b.ytop<a.ytop||(b.ytop==a.ytop&&j<i)))fica[i]=0;}}
         size_t n=0;for(size_t i=0;i<cs.size();++i)if(fica[i])cs[n++]=cs[i];cs.resize(n);}
        S.ncand=int(cs.size());
        if(cs.empty()){tem_ant=false;n_seg=0;return S;}
        double dmax=0;for(const auto& k:cs)dmax=(std::max)(dmax,k.d);

        bool tem_ref=false;double rx=0,ry=0,rw=0;
        if(dica&&dica[2]>0){rw=dica[2]/1.25/q;rx=dica[0]/q;ry=dica[1]/q-.5*1.25*rw;tem_ref=true;}
        else if(tem_ant){rx=ant_x;ry=ant_ytop;rw=ant_wh;tem_ref=true;}
        int ib=-1,is=-1;double bs=-1,ss=-1;
        for(size_t i=0;i<cs.size();++i){auto& k=cs[i];
            const double f_omb=det::clip01(float((k.omb-1.15)/.6)),f_alt=det::clip01(float((k.alt-.9)/.3)),f_topo=det::clip01(float(1-(k.topo_oc-.2)/.4));
            const double f_corpo=det::clip01(float((k.corpo-1.5)/2.0));
            const double hx=k.x/W,hy=(k.ytop+.6*k.wh)/H,ax=(hx-P.pos_x)/P.sig_x,ay=(hy-P.pos_y)/P.sig_y,f_pos=std::exp(-.5*(ax*ax+ay*ay));
            const double f_perto=det::clip01(float((k.d/(std::max)(dmax,1e-9)-.4)/.5));
            double f_esc=1.0;if(esc_cont>=P.esc_n&&esc>0){const double l=std::log((k.wh/k.d)/esc)/P.esc_tol;f_esc=std::exp(-l*l);}
            k.base=(.35*f_omb+.25*f_corpo+.20*f_alt+.20*f_topo)*f_alt*(.4+.6*f_pos)*(.5+.5*f_perto)*(.4+.6*f_esc);
            k.seg=0;
            if(tem_ref){const int gx=std::clamp(int(k.x),0,W-1),gy=std::clamp(int(k.ytop+.5*k.wh),0,H-1);const size_t ic=size_t(gy)*W+gx;
                const double px=k.x+g.k*g.mvq[2*ic],py=k.ytop+.5*k.wh+g.k*g.mvq[2*ic+1];
                const double dist=std::hypot(px-rx,py-(ry+.5*rw))/(std::max)(rw,2.0),lw=std::log(k.wh/(std::max)(rw,1.0))/P.seg_tam;
                k.seg=std::exp(-(dist/P.seg_dist)*(dist/P.seg_dist))*std::exp(-lw*lw);}
            k.nota=tem_ref?.5*k.base+.5*k.seg:k.base;
            if(k.nota>bs){bs=k.nota;ib=int(i);}
            if(k.seg>=.5&&k.nota>ss){ss=k.nota;is=int(i);}}

        if(is>=0&&ss>=.8*bs)ib=is;
        const auto& k=cs[size_t(ib)];
        if(k.nota<P.nota_min){tem_ant=false;n_seg=0;return S;}
        S.seguiu=tem_ref&&k.seg>=.5;
        n_seg=S.seguiu?n_seg+1:1;
        S.tem=true;S.nota=k.nota;S.base=k.base;S.n_seg=n_seg;S.d=k.d;S.cx_g=k.x;S.ytop_g=k.ytop;S.wh_g=k.wh;
        S.lado=1.25*k.wh*q;S.x=(k.x+.5)*q;S.y=k.ytop*q+.5*S.lado;
        tem_ant=true;ant_x=k.x;ant_ytop=k.ytop;ant_wh=k.wh;++com_cabeca;
        return S;
    }
private:
    struct Cab {double x,ytop,wh,d,omb,alt,topo_oc,corpo,base,seg,nota;};
    bool tem_ant=false;double ant_x=0,ant_ytop=0,ant_wh=0;int n_seg=0;

    std::vector<unsigned char> e,ne,m,tampa;std::vector<int> lab,labt,ordem,pilha;std::vector<det::Comp> comps,compt;
    std::vector<float> larg,cent,fa,fc,vals;std::vector<Cab> cs;std::vector<unsigned char> fica;
};
}
