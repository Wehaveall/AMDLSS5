#pragma once

#include "nr_publico.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include "ini_jogo.h"
#include "estencil_externo.h"
#include "ao.h"
#include "il.h"
namespace nrfiltros {

enum Linha : unsigned { CasLiga, CasForca, LumaLiga, LumaForca, LumaLimite, RostoContorno, CeuNitidez, ClarLiga, ClarForca, ClarRaio, ClarMeios, ClarZona, ClarSemPele,
                        NivLiga, NivPreto, NivBranco, NivSaidaPreto, NivSaidaBranco, TomLiga, TomGama, TomExpo, TomSat, TomBleach, TomDefog,
                        TempLiga, TempK, TempMatiz, PeleLiga, PeleLuz, PeleSat, PeleMatiz, PeleK,
                        HslLiga, HslFaixa, HslMatiz, HslSat, HslLuz, HslSuave, HslPele,
                        AoLiga, AoQualidade, AoIntensidade, AoRaio, AoEspessura, AoAlcance, AoPele, AoClaro, AoResolucao, AoVer,
                        IlLiga, IlEstilo, IlIntensidade, IlSaturacao, IlBrilho, IlQualidade, IlAtualizacao, IlPele, IlVer, LinhaCount };

struct Def { const char* rotulo; const wchar_t* chave; const char* const* valores; const float* numeros; unsigned n; unsigned padrao; int pai; bool no_ini_indice; };
inline constexpr const char* t_liga[]{"Desligado","Ligado"};
inline constexpr float n_liga[]{0.f,1.f};
inline constexpr const char* t_cas[]{"0.10","0.20","0.30","0.40","0.50","0.60","0.70","0.80","0.90","1.00"};
inline constexpr float n_cas[]{.1f,.2f,.3f,.4f,.5f,.6f,.7f,.8f,.9f,1.f};
inline constexpr const char* t_luma_f[]{"0.20","0.40","0.65","0.80","1.00","1.50","2.00","3.00"};
inline constexpr float n_luma_f[]{.2f,.4f,.65f,.8f,1.f,1.5f,2.f,3.f};
inline constexpr const char* t_luma_l[]{"0.010","0.020","0.035","0.050","0.075","0.100"};
inline constexpr float n_luma_l[]{.01f,.02f,.035f,.05f,.075f,.1f};
inline constexpr const char* t_clar_f[]{"0.10","0.25","0.40","0.60","0.80","1.00","1.50","2.00"};
inline constexpr float n_clar_f[]{.1f,.25f,.4f,.6f,.8f,1.f,1.5f,2.f};
inline constexpr const char* t_raio[]{"Pequeno","Médio","Grande","Muito grande"};
inline constexpr float n_raio[]{1.5f,3.f,6.f,8.5f};

inline constexpr const char* t_zona[]{"Tudo","Sombras"};

inline constexpr const char* t_sem_rosto[]{"Desligado","Pele","Cabeça"};
inline constexpr float n_sem_rosto[]{0.f,1.f,2.f};
inline constexpr float n_zona[]{0.f,1.f};
inline constexpr const char* t_meios[]{"0.00","0.25","0.50","0.75","1.00"};
inline constexpr float n_meios[]{0.f,.25f,.5f,.75f,1.f};

inline constexpr const char* t_preto[]{"0","4","8","13","16","20","24","32"};
inline constexpr float n_preto[]{0.f,4.f,8.f,13.f,16.f,20.f,24.f,32.f};
inline constexpr const char* t_branco[]{"255","250","245","240","235","225","210","200"};
inline constexpr float n_branco[]{255.f,250.f,245.f,240.f,235.f,225.f,210.f,200.f};

inline constexpr const char* t_saida_preto[]{"0","4","8","12","16","24","32","48","64"};
inline constexpr float n_saida_preto[]{0.f,4.f,8.f,12.f,16.f,24.f,32.f,48.f,64.f};
inline constexpr const char* t_saida_branco[]{"255","250","245","240","230","220","210","192"};
inline constexpr float n_saida_branco[]{255.f,250.f,245.f,240.f,230.f,220.f,210.f,192.f};
inline constexpr const char* t_gama[]{"0.80","0.85","0.90","0.94","0.96","0.98","1.00","1.05","1.10","1.15","1.20"};
inline constexpr float n_gama[]{.8f,.85f,.9f,.94f,.96f,.98f,1.f,1.05f,1.1f,1.15f,1.2f};
inline constexpr const char* t_tom_expo[]{"-0.50","-0.30","-0.20","-0.10","-0.05","0.00","+0.05","+0.10","+0.20","+0.30","+0.50"};
inline constexpr float n_tom_expo[]{-.5f,-.3f,-.2f,-.1f,-.05f,0.f,.05f,.1f,.2f,.3f,.5f};
inline constexpr const char* t_sat[]{"-0.30","-0.20","-0.10","0.00","+0.10","+0.20","+0.30","+0.50"};
inline constexpr float n_sat[]{-.3f,-.2f,-.1f,0.f,.1f,.2f,.3f,.5f};
inline constexpr const char* t_bleach[]{"0.00","0.03","0.05","0.10","0.20","0.30","0.50"};
inline constexpr float n_bleach[]{0.f,.03f,.05f,.1f,.2f,.3f,.5f};
inline constexpr const char* t_defog[]{"0.00","0.04","0.08","0.12","0.16","0.24","0.32"};
inline constexpr float n_defog[]{0.f,.04f,.08f,.12f,.16f,.24f,.32f};

inline constexpr const char* t_hsl_faixa[]{"Todas","Vermelhos","Laranjas","Amarelos","Verdes","Cianos","Azuis","Roxos","Magentas"};
inline constexpr float n_hsl_faixa[]{0.f,1.f,2.f,3.f,4.f,5.f,6.f,7.f,8.f};
inline constexpr unsigned HslFaixas=9;
inline constexpr const wchar_t* hsl_chave_faixa[HslFaixas]{L"Todas",L"Vermelho",L"Laranja",L"Amarelo",L"Verde",L"Ciano",L"Azul",L"Roxo",L"Magenta"};
inline constexpr const wchar_t* hsl_chave_ajuste[3]{L"Matiz",L"Sat",L"Luz"};
inline constexpr const char* t_hsl_matiz[]{"-30","-28","-26","-24","-22","-20","-18","-16","-14","-12","-10","-8","-6","-4","-2","0","+2","+4","+6","+8","+10","+12","+14","+16","+18","+20","+22","+24","+26","+28","+30"};
inline constexpr float n_hsl_matiz[]{-30.f,-28.f,-26.f,-24.f,-22.f,-20.f,-18.f,-16.f,-14.f,-12.f,-10.f,-8.f,-6.f,-4.f,-2.f,0.f,2.f,4.f,6.f,8.f,10.f,12.f,14.f,16.f,18.f,20.f,22.f,24.f,26.f,28.f,30.f};
inline constexpr const char* t_hsl_cem[]{"-100","-95","-90","-85","-80","-75","-70","-65","-60","-55","-50","-45","-40","-35","-30","-25","-20","-15","-10","-5","0","+5","+10","+15","+20","+25","+30","+35","+40","+45","+50","+55","+60","+65","+70","+75","+80","+85","+90","+95","+100"};
inline constexpr float n_hsl_cem[]{-100.f,-95.f,-90.f,-85.f,-80.f,-75.f,-70.f,-65.f,-60.f,-55.f,-50.f,-45.f,-40.f,-35.f,-30.f,-25.f,-20.f,-15.f,-10.f,-5.f,0.f,5.f,10.f,15.f,20.f,25.f,30.f,35.f,40.f,45.f,50.f,55.f,60.f,65.f,70.f,75.f,80.f,85.f,90.f,95.f,100.f};

inline constexpr const char* t_hsl_suave[]{"Estreita","Média","Larga"};
inline constexpr float n_hsl_suave[]{.25f,.6f,1.f};

inline constexpr const char* t_kelvin[]{"4000","4100","4200","4300","4400","4500","4600","4700","4800","4900","5000","5100","5200","5300","5400","5500","5600","5700","5800","5900","6000","6100","6200","6300","6400","6500","6600","6700","6800","6900","7000","7100","7200","7300","7400","7500","7600","7700","7800","7900","8000","8100","8200","8300","8400","8500","8600","8700","8800","8900","9000"};
inline constexpr float n_kelvin[]{4000.f,4100.f,4200.f,4300.f,4400.f,4500.f,4600.f,4700.f,4800.f,4900.f,5000.f,5100.f,5200.f,5300.f,5400.f,5500.f,5600.f,5700.f,5800.f,5900.f,6000.f,6100.f,6200.f,6300.f,6400.f,6500.f,6600.f,6700.f,6800.f,6900.f,7000.f,7100.f,7200.f,7300.f,7400.f,7500.f,7600.f,7700.f,7800.f,7900.f,8000.f,8100.f,8200.f,8300.f,8400.f,8500.f,8600.f,8700.f,8800.f,8900.f,9000.f};
inline constexpr const char* t_temp_matiz[]{"-20","-19","-18","-17","-16","-15","-14","-13","-12","-11","-10","-9","-8","-7","-6","-5","-4","-3","-2","-1","0","+1","+2","+3","+4","+5","+6","+7","+8","+9","+10","+11","+12","+13","+14","+15","+16","+17","+18","+19","+20"};
inline constexpr float n_temp_matiz[]{-20.f,-19.f,-18.f,-17.f,-16.f,-15.f,-14.f,-13.f,-12.f,-11.f,-10.f,-9.f,-8.f,-7.f,-6.f,-5.f,-4.f,-3.f,-2.f,-1.f,0.f,1.f,2.f,3.f,4.f,5.f,6.f,7.f,8.f,9.f,10.f,11.f,12.f,13.f,14.f,15.f,16.f,17.f,18.f,19.f,20.f};

inline void kelvin_rgb(double t,double c[3]) noexcept {
    const double t2=t*t,t3=t2*t;
    const double x=t<=4000.?-0.2661239e9/t3-0.2343589e6/t2+0.8776956e3/t+0.179910:-3.0258469e9/t3+2.1070379e6/t2+0.2226347e3/t+0.240390;
    const double x2=x*x,x3=x2*x;
    const double y=t<=2222.?-1.1063814*x3-1.34811020*x2+2.18555832*x-0.20219683:t<=4000.?-0.9549476*x3-1.37418593*x2+2.09137015*x-0.16748867:
                                                                                     3.0817580*x3-5.87338670*x2+3.75112997*x-0.37001483;
    const double X=x/y,Z=(1.-x-y)/y;
    c[0]=3.2404542*X-1.5371385-0.4985314*Z;c[1]=-0.9692660*X+1.8760108+0.0415560*Z;c[2]=0.0556434*X-0.2040259+1.0572252*Z;}

inline void temperatura_ganhos(float kelvin,float matiz,float g[3]) noexcept {
    g[0]=g[1]=g[2]=1.f;if(!(kelvin>=1667.f&&kelvin<=25000.f)||!std::isfinite(matiz)||(kelvin==6500.f&&matiz==0.f))return;
    double a[3],b[3];kelvin_rgb(kelvin,a);kelvin_rgb(6500.,b);const double r[3]{a[0]/b[0],a[1]/b[1]*std::exp2(-double(matiz)/100.),a[2]/b[2]};
    const double l=0.2126*r[0]+0.7152*r[1]+0.0722*r[2];if(!(l>0.)||!(r[0]>0.&&r[1]>0.&&r[2]>0.))return;
    for(int i=0;i<3;++i)g[i]=float(r[i]/l);}

inline constexpr const char* t_pele_luz[]{"-0.50","-0.48","-0.46","-0.44","-0.42","-0.40","-0.38","-0.36","-0.34","-0.32","-0.30","-0.28","-0.26","-0.24","-0.22","-0.20","-0.18","-0.16","-0.14","-0.12","-0.10","-0.08","-0.06","-0.04","-0.02","0.00","+0.02","+0.04","+0.06","+0.08","+0.10","+0.12","+0.14","+0.16","+0.18","+0.20","+0.22","+0.24","+0.26","+0.28","+0.30","+0.32","+0.34","+0.36","+0.38","+0.40","+0.42","+0.44","+0.46","+0.48","+0.50"};
inline constexpr float n_pele_luz[]{-0.5f,-0.48f,-0.46f,-0.44f,-0.42f,-0.4f,-0.38f,-0.36f,-0.34f,-0.32f,-0.3f,-0.28f,-0.26f,-0.24f,-0.22f,-0.2f,-0.18f,-0.16f,-0.14f,-0.12f,-0.1f,-0.08f,-0.06f,-0.04f,-0.02f,0.f,0.02f,0.04f,0.06f,0.08f,0.1f,0.12f,0.14f,0.16f,0.18f,0.2f,0.22f,0.24f,0.26f,0.28f,0.3f,0.32f,0.34f,0.36f,0.38f,0.4f,0.42f,0.44f,0.46f,0.48f,0.5f};
inline constexpr const char* t_pele_sat[]{"-30","-29","-28","-27","-26","-25","-24","-23","-22","-21","-20","-19","-18","-17","-16","-15","-14","-13","-12","-11","-10","-9","-8","-7","-6","-5","-4","-3","-2","-1","0","+1","+2","+3","+4","+5","+6","+7","+8","+9","+10","+11","+12","+13","+14","+15","+16","+17","+18","+19","+20","+21","+22","+23","+24","+25","+26","+27","+28","+29","+30"};
inline constexpr float n_pele_sat[]{-30.f,-29.f,-28.f,-27.f,-26.f,-25.f,-24.f,-23.f,-22.f,-21.f,-20.f,-19.f,-18.f,-17.f,-16.f,-15.f,-14.f,-13.f,-12.f,-11.f,-10.f,-9.f,-8.f,-7.f,-6.f,-5.f,-4.f,-3.f,-2.f,-1.f,0.f,1.f,2.f,3.f,4.f,5.f,6.f,7.f,8.f,9.f,10.f,11.f,12.f,13.f,14.f,15.f,16.f,17.f,18.f,19.f,20.f,21.f,22.f,23.f,24.f,25.f,26.f,27.f,28.f,29.f,30.f};
inline constexpr const char* t_pele_matiz[]{"-10.0","-9.5","-9.0","-8.5","-8.0","-7.5","-7.0","-6.5","-6.0","-5.5","-5.0","-4.5","-4.0","-3.5","-3.0","-2.5","-2.0","-1.5","-1.0","-0.5","0.0","+0.5","+1.0","+1.5","+2.0","+2.5","+3.0","+3.5","+4.0","+4.5","+5.0","+5.5","+6.0","+6.5","+7.0","+7.5","+8.0","+8.5","+9.0","+9.5","+10.0"};
inline constexpr float n_pele_matiz[]{-10.f,-9.5f,-9.f,-8.5f,-8.f,-7.5f,-7.f,-6.5f,-6.f,-5.5f,-5.f,-4.5f,-4.f,-3.5f,-3.f,-2.5f,-2.f,-1.5f,-1.f,-0.5f,0.f,0.5f,1.f,1.5f,2.f,2.5f,3.f,3.5f,4.f,4.5f,5.f,5.5f,6.f,6.5f,7.f,7.5f,8.f,8.5f,9.f,9.5f,10.f};
inline constexpr const char* t_pele_k[]{"5000","5100","5200","5300","5400","5500","5600","5700","5800","5900","6000","6100","6200","6300","6400","6500","6600","6700","6800","6900","7000","7100","7200","7300","7400","7500","7600","7700","7800","7900","8000"};
inline constexpr float n_pele_k[]{5000.f,5100.f,5200.f,5300.f,5400.f,5500.f,5600.f,5700.f,5800.f,5900.f,6000.f,6100.f,6200.f,6300.f,6400.f,6500.f,6600.f,6700.f,6800.f,6900.f,7000.f,7100.f,7200.f,7300.f,7400.f,7500.f,7600.f,7700.f,7800.f,7900.f,8000.f};

inline constexpr const char* t_ao_qual[]{"Baixa","Média","Alta","Ultra"};
inline constexpr float n_ao_qual[]{0.f,1.f,2.f,3.f};
inline constexpr const char* t_ao_intens[]{"0.00","0.05","0.10","0.15","0.20","0.25","0.30","0.35","0.40","0.45","0.50","0.55","0.60","0.65","0.70","0.75","0.80","0.85","0.90","0.95","1.00","1.05","1.10","1.15","1.20","1.25","1.30","1.35","1.40","1.45","1.50","1.55","1.60","1.65","1.70","1.75","1.80","1.85","1.90","1.95","2.00","2.05","2.10","2.15","2.20","2.25","2.30","2.35","2.40","2.45","2.50","2.55","2.60","2.65","2.70","2.75","2.80","2.85","2.90","2.95","3.00"};
inline constexpr float n_ao_intens[]{0.f,0.05f,0.1f,0.15f,0.2f,0.25f,0.3f,0.35f,0.4f,0.45f,0.5f,0.55f,0.6f,0.65f,0.7f,0.75f,0.8f,0.85f,0.9f,0.95f,1.f,1.05f,1.1f,1.15f,1.2f,1.25f,1.3f,1.35f,1.4f,1.45f,1.5f,1.55f,1.6f,1.65f,1.7f,1.75f,1.8f,1.85f,1.9f,1.95f,2.f,2.05f,2.1f,2.15f,2.2f,2.25f,2.3f,2.35f,2.4f,2.45f,2.5f,2.55f,2.6f,2.65f,2.7f,2.75f,2.8f,2.85f,2.9f,2.95f,3.f};
inline constexpr const char* t_ao_raio[]{"0.05","0.10","0.15","0.20","0.25","0.30","0.35","0.40","0.45","0.50","0.55","0.60","0.65","0.70","0.75","0.80","0.85","0.90","0.95","1.00","1.05","1.10","1.15","1.20","1.25","1.30","1.35","1.40","1.45","1.50","1.55","1.60","1.65","1.70","1.75","1.80","1.85","1.90","1.95","2.00","2.05","2.10","2.15","2.20","2.25","2.30","2.35","2.40","2.45","2.50","2.55","2.60","2.65","2.70","2.75","2.80","2.85","2.90","2.95","3.00"};
inline constexpr float n_ao_raio[]{0.05f,0.1f,0.15f,0.2f,0.25f,0.3f,0.35f,0.4f,0.45f,0.5f,0.55f,0.6f,0.65f,0.7f,0.75f,0.8f,0.85f,0.9f,0.95f,1.f,1.05f,1.1f,1.15f,1.2f,1.25f,1.3f,1.35f,1.4f,1.45f,1.5f,1.55f,1.6f,1.65f,1.7f,1.75f,1.8f,1.85f,1.9f,1.95f,2.f,2.05f,2.1f,2.15f,2.2f,2.25f,2.3f,2.35f,2.4f,2.45f,2.5f,2.55f,2.6f,2.65f,2.7f,2.75f,2.8f,2.85f,2.9f,2.95f,3.f};
inline constexpr const char* t_ao_esp[]{"0.02","0.05","0.10","0.15","0.25","0.35","0.50","0.75","1.00","1.50","2.00"};
inline constexpr float n_ao_esp[]{.02f,.05f,.1f,.15f,.25f,.35f,.5f,.75f,1.f,1.5f,2.f};
inline constexpr const char* t_ao_alc[]{"5","10","20","40","80","150","300","500"};
inline constexpr float n_ao_alc[]{5.f,10.f,20.f,40.f,80.f,150.f,300.f,500.f};
inline constexpr const char* t_ao_cem[]{"0","10","20","30","40","50","60","70","80","90","100"};
inline constexpr float n_ao_cem[]{0.f,10.f,20.f,30.f,40.f,50.f,60.f,70.f,80.f,90.f,100.f};
inline constexpr const char* t_ao_res[]{"Automática","Inteira","Metade"};
inline constexpr float n_ao_res[]{0.f,1.f,2.f};
inline constexpr const char* t_ao_ver[]{"Desligado","Só AO","AO cru","Normais","Profundidade","Bordas","Quadros","Máscaras"};
inline constexpr float n_ao_ver[]{0.f,1.f,2.f,3.f,4.f,5.f,6.f,7.f};

inline constexpr const char* t_il_estilo[]{"Sutil","Padrão","Forte"};
inline constexpr float n_il_estilo[]{0.f,1.f,2.f};
inline constexpr const char* t_il_intens[]{"0.0","0.1","0.2","0.3","0.4","0.5","0.6","0.7","0.8","0.9","1.0","1.1","1.2","1.3","1.4","1.5","1.6","1.7","1.8","1.9","2.0",
 "2.1","2.2","2.3","2.4","2.5","2.6","2.7","2.8","2.9","3.0","3.1","3.2","3.3","3.4","3.5","3.6","3.7","3.8","3.9","4.0","4.1","4.2","4.3","4.4","4.5","4.6","4.7","4.8","4.9","5.0",
 "5.1","5.2","5.3","5.4","5.5","5.6","5.7","5.8","5.9","6.0","6.1","6.2","6.3","6.4","6.5","6.6","6.7","6.8","6.9","7.0","7.1","7.2","7.3","7.4","7.5","7.6","7.7","7.8","7.9","8.0",
 "8.1","8.2","8.3","8.4","8.5","8.6","8.7","8.8","8.9","9.0","9.1","9.2","9.3","9.4","9.5","9.6","9.7","9.8","9.9","10.0"};
inline constexpr float n_il_intens[]{0.f,.1f,.2f,.3f,.4f,.5f,.6f,.7f,.8f,.9f,1.f,1.1f,1.2f,1.3f,1.4f,1.5f,1.6f,1.7f,1.8f,1.9f,2.f,
 2.1f,2.2f,2.3f,2.4f,2.5f,2.6f,2.7f,2.8f,2.9f,3.f,3.1f,3.2f,3.3f,3.4f,3.5f,3.6f,3.7f,3.8f,3.9f,4.f,4.1f,4.2f,4.3f,4.4f,4.5f,4.6f,4.7f,4.8f,4.9f,5.f,
 5.1f,5.2f,5.3f,5.4f,5.5f,5.6f,5.7f,5.8f,5.9f,6.f,6.1f,6.2f,6.3f,6.4f,6.5f,6.6f,6.7f,6.8f,6.9f,7.f,7.1f,7.2f,7.3f,7.4f,7.5f,7.6f,7.7f,7.8f,7.9f,8.f,
 8.1f,8.2f,8.3f,8.4f,8.5f,8.6f,8.7f,8.8f,8.9f,9.f,9.1f,9.2f,9.3f,9.4f,9.5f,9.6f,9.7f,9.8f,9.9f,10.f};
inline constexpr const char* t_il_sat[]{"0.0","0.1","0.2","0.3","0.4","0.5","0.6","0.7","0.8","0.9","1.0","1.1","1.2","1.3","1.4","1.5","1.6","1.7","1.8","1.9","2.0"};
inline constexpr float n_il_sat[]{0.f,.1f,.2f,.3f,.4f,.5f,.6f,.7f,.8f,.9f,1.f,1.1f,1.2f,1.3f,1.4f,1.5f,1.6f,1.7f,1.8f,1.9f,2.f};
inline constexpr const char* t_il_atual[]{"Todo quadro","Rodízio 2","Rodízio 4"};
inline constexpr float n_il_atual[]{0.f,1.f,2.f};
inline constexpr const char* t_il_ver[]{"Desligado","Só IL","IL crua","Radiância","Albedo","Quadros","Céu","Normais emissor"};
inline constexpr float n_il_ver[]{0.f,1.f,2.f,3.f,4.f,5.f,6.f,7.f};
template<class T,unsigned N> constexpr unsigned cnt(T(&)[N]){return N;}
inline constexpr Def defs[LinhaCount]{
 {"Nitidez CAS",L"NeuralFiltroCas",t_liga,n_liga,cnt(t_liga),0,-1,true},
 {"- Força do CAS",L"NeuralFiltroCasForca",t_cas,n_cas,cnt(t_cas),4,int(CasLiga),false},
 {"Nitidez LumaSharpen",L"NeuralFiltroLuma",t_liga,n_liga,cnt(t_liga),0,-1,true},
 {"- Força do LumaSharpen",L"NeuralFiltroLumaForca",t_luma_f,n_luma_f,cnt(t_luma_f),2,int(LumaLiga),false},
 {"- Limite do LumaSharpen",L"NeuralFiltroLumaLimite",t_luma_l,n_luma_l,cnt(t_luma_l),2,int(LumaLiga),false},

 {"- Só contornos no rosto",L"NeuralFiltroRostoContorno",t_liga,n_liga,cnt(t_liga),0,int(LumaLiga),true},

 {"- Nitidez no céu",L"NeuralFiltroNitidezCeu",t_liga,n_liga,cnt(t_liga),0,int(LumaLiga),true},
 {"Contraste local (Clarity)",L"NeuralFiltroClarity",t_liga,n_liga,cnt(t_liga),0,-1,true},
 {"- Força do Clarity",L"NeuralFiltroClarityForca",t_clar_f,n_clar_f,cnt(t_clar_f),2,int(ClarLiga),false},
 {"- Raio do Clarity",L"NeuralFiltroClarityRaio",t_raio,n_raio,cnt(t_raio),1,int(ClarLiga),true},
 {"- Meios-tons do Clarity",L"NeuralFiltroClarityMeios",t_meios,n_meios,cnt(t_meios),2,int(ClarLiga),false},
 {"- Zona do Clarity",L"NeuralFiltroClarityZona",t_zona,n_zona,cnt(t_zona),0,int(ClarLiga),true},
 {"- Clarity fora do rosto",L"NeuralFiltroClaritySemPele",t_sem_rosto,n_sem_rosto,cnt(t_sem_rosto),2,int(ClarLiga),true},

 {"Níveis (preto e branco)",L"NeuralFiltroNiveis",t_liga,n_liga,cnt(t_liga),0,-1,true},
 {"- Preto dos Níveis",L"NeuralFiltroNiveisPreto",t_preto,n_preto,cnt(t_preto),0,int(NivLiga),false},
 {"- Branco dos Níveis",L"NeuralFiltroNiveisBranco",t_branco,n_branco,cnt(t_branco),0,int(NivLiga),false},
 {"- Preto de saída dos Níveis",L"NeuralFiltroNiveisSaidaPreto",t_saida_preto,n_saida_preto,cnt(t_saida_preto),0,int(NivLiga),false},
 {"- Branco de saída dos Níveis",L"NeuralFiltroNiveisSaidaBranco",t_saida_branco,n_saida_branco,cnt(t_saida_branco),0,int(NivLiga),false},
 {"Tom e cor (Tonemap)",L"NeuralFiltroTom",t_liga,n_liga,cnt(t_liga),0,-1,true},
 {"- Gama do Tom",L"NeuralFiltroTomGama",t_gama,n_gama,cnt(t_gama),6,int(TomLiga),false},
 {"- Exposição do Tom",L"NeuralFiltroTomExposicao",t_tom_expo,n_tom_expo,cnt(t_tom_expo),5,int(TomLiga),false},
 {"- Saturação do Tom",L"NeuralFiltroTomSaturacao",t_sat,n_sat,cnt(t_sat),3,int(TomLiga),false},
 {"- Bleach do Tom",L"NeuralFiltroTomBleach",t_bleach,n_bleach,cnt(t_bleach),0,int(TomLiga),false},
 {"- Defog do Tom",L"NeuralFiltroTomDefog",t_defog,n_defog,cnt(t_defog),0,int(TomLiga),false},
 {"Temperatura (K)",L"NeuralFiltroTemperatura",t_liga,n_liga,cnt(t_liga),0,-1,true},
 {"- Kelvin",L"NeuralFiltroTemperaturaK",t_kelvin,n_kelvin,cnt(t_kelvin),25,int(TempLiga),false},
 {"- Matiz (verde/magenta)",L"NeuralFiltroTemperaturaMatiz",t_temp_matiz,n_temp_matiz,cnt(t_temp_matiz),20,int(TempLiga),false},
 {"Pele do rosto",L"NeuralFiltroPele",t_liga,n_liga,cnt(t_liga),0,-1,true},
 {"- Luz da pele",L"NeuralFiltroPeleLuz",t_pele_luz,n_pele_luz,cnt(t_pele_luz),25,int(PeleLiga),false},
 {"- Saturação da pele",L"NeuralFiltroPeleSaturacao",t_pele_sat,n_pele_sat,cnt(t_pele_sat),30,int(PeleLiga),false},
 {"- Matiz da pele",L"NeuralFiltroPeleMatiz",t_pele_matiz,n_pele_matiz,cnt(t_pele_matiz),20,int(PeleLiga),false},
 {"- Temperatura da pele (K)",L"NeuralFiltroPeleK",t_pele_k,n_pele_k,cnt(t_pele_k),15,int(PeleLiga),false},
 {"Cor por faixa (HSL)",L"NeuralFiltroHsl",t_liga,n_liga,cnt(t_liga),0,-1,true},

 {"- Faixa do HSL",nullptr,t_hsl_faixa,n_hsl_faixa,cnt(t_hsl_faixa),0,int(HslLiga),true},
 {"- Matiz do HSL",nullptr,t_hsl_matiz,n_hsl_matiz,cnt(t_hsl_matiz),15,int(HslLiga),false},
 {"- Saturação do HSL",nullptr,t_hsl_cem,n_hsl_cem,cnt(t_hsl_cem),20,int(HslLiga),false},
 {"- Luminosidade do HSL",nullptr,t_hsl_cem,n_hsl_cem,cnt(t_hsl_cem),20,int(HslLiga),false},
 {"- Suavidade do HSL",L"NeuralFiltroHslSuavidade",t_hsl_suave,n_hsl_suave,cnt(t_hsl_suave),1,int(HslLiga),true},
 {"- Proteger pele no HSL",L"NeuralFiltroHslPele",t_liga,n_liga,cnt(t_liga),0,int(HslLiga),true},

 {"Oclusão ambiente (AO)",L"NeuralFiltroAo",t_liga,n_liga,cnt(t_liga),0,-1,true},
 {"- Qualidade da AO",L"NeuralFiltroAoQualidade",t_ao_qual,n_ao_qual,cnt(t_ao_qual),1,int(AoLiga),true},
 {"- Intensidade da AO",L"NeuralFiltroAoIntensidade",t_ao_intens,n_ao_intens,cnt(t_ao_intens),20,int(AoLiga),false},

 {"- Raio da AO",L"NeuralFiltroAoRaio",t_ao_raio,n_ao_raio,cnt(t_ao_raio),7,int(AoLiga),false},
 {"- Espessura da AO",L"NeuralFiltroAoEspessura",t_ao_esp,n_ao_esp,cnt(t_ao_esp),2,int(AoLiga),false},
 {"- Alcance da AO",L"NeuralFiltroAoDistFim",t_ao_alc,n_ao_alc,cnt(t_ao_alc),4,int(AoLiga),false},
 {"- Proteger pele na AO",L"NeuralFiltroAoPele",t_ao_cem,n_ao_cem,cnt(t_ao_cem),10,int(AoLiga),false},
 {"- Menos AO em áreas claras",L"NeuralFiltroAoClaro",t_ao_cem,n_ao_cem,cnt(t_ao_cem),5,int(AoLiga),false},
 {"- Resolução da AO",L"NeuralFiltroAoResolucao",t_ao_res,n_ao_res,cnt(t_ao_res),0,int(AoLiga),true},
 {"- Ver a AO (depuração)",L"NeuralFiltroAoVer",t_ao_ver,n_ao_ver,cnt(t_ao_ver),0,int(AoLiga),true},

 {"Iluminação indireta (IL)",L"NeuralFiltroIl",t_liga,n_liga,cnt(t_liga),0,-1,true},
 {"- Estilo da IL",L"NeuralFiltroIlEstilo",t_il_estilo,n_il_estilo,cnt(t_il_estilo),1,int(IlLiga),true},
 {"- Intensidade da IL",L"NeuralFiltroIlIntensidade",t_il_intens,n_il_intens,cnt(t_il_intens),10,int(IlLiga),false},
 {"- Saturação da IL",L"NeuralFiltroIlSaturacao",t_il_sat,n_il_sat,cnt(t_il_sat),10,int(IlLiga),false},
 {"- Brilho do rebatimento",L"NeuralFiltroIlBrilho",t_ao_cem,n_ao_cem,cnt(t_ao_cem),8,int(IlLiga),false},
 {"- Qualidade da IL",L"NeuralFiltroIlQualidade",t_ao_qual,n_ao_qual,cnt(t_ao_qual),1,int(IlLiga),true},
 {"- Atualização da IL",L"NeuralFiltroIlAtualizacao",t_il_atual,n_il_atual,cnt(t_il_atual),1,int(IlLiga),true},
 {"- Proteger pele na IL",L"NeuralFiltroIlPele",t_ao_cem,n_ao_cem,cnt(t_ao_cem),10,int(IlLiga),false},
 {"- Ver a IL (depuração)",L"NeuralFiltroIlVer",t_il_ver,n_il_ver,cnt(t_il_ver),0,int(IlLiga),true},
};
static_assert(cnt(t_hsl_faixa)==HslFaixas);
static_assert(cnt(t_ao_intens)==61&&cnt(n_ao_intens)==61&&cnt(t_ao_raio)==60&&cnt(n_ao_raio)==60&&cnt(t_ao_esp)==cnt(n_ao_esp)&&cnt(t_ao_alc)==cnt(n_ao_alc));
static_assert(cnt(t_il_estilo)==cnt(n_il_estilo)&&cnt(t_il_intens)==101&&cnt(n_il_intens)==101&&cnt(t_il_sat)==21&&cnt(n_il_sat)==21&&cnt(t_il_atual)==cnt(n_il_atual)&&cnt(t_il_ver)==cnt(n_il_ver));
static_assert(cnt(t_kelvin)==51&&cnt(n_kelvin)==51&&cnt(t_temp_matiz)==41&&cnt(n_temp_matiz)==41);
static_assert(cnt(t_pele_luz)==51&&cnt(n_pele_luz)==51&&cnt(t_pele_sat)==61&&cnt(n_pele_sat)==61&&cnt(t_pele_matiz)==41&&cnt(n_pele_matiz)==41&&cnt(t_pele_k)==31&&cnt(n_pele_k)==31);
inline const Def& def(Linha l) noexcept {return defs[l<LinhaCount?l:0];}
inline std::atomic<unsigned> indice[LinhaCount]{};

inline std::atomic<int> hsl[HslFaixas][3]{};
inline unsigned hsl_faixa() noexcept {return indice[HslFaixa].load(std::memory_order_relaxed)%HslFaixas;}
inline unsigned hsl_indice(unsigned f,unsigned k) noexcept {
    const auto& d=defs[HslMatiz+k%3u];const int i=int(d.padrao)+hsl[f%HslFaixas][k%3u].load(std::memory_order_relaxed);
    return i<0?0u:unsigned(i)>=d.n?d.n-1u:unsigned(i);}
inline float hsl_valor(unsigned f,unsigned k) noexcept {return defs[HslMatiz+k%3u].numeros[hsl_indice(f,k)];}
inline void hsl_espelhar() noexcept {const unsigned f=hsl_faixa();for(unsigned k=0;k<3;++k)indice[HslMatiz+k].store(hsl_indice(f,k),std::memory_order_relaxed);}
inline void hsl_chave(unsigned f,unsigned k,wchar_t* s,std::size_t n) noexcept {std::swprintf(s,n,L"NeuralFiltroHsl%ls%ls",hsl_chave_faixa[f%HslFaixas],hsl_chave_ajuste[k%3u]);}
inline std::atomic<int> aberto{-1};
inline std::atomic<float> expo{0.f};
inline std::atomic<bool> lido{false};

inline std::atomic<bool> local{true};

struct Cabeca {float cx,cy,r,ry;};
inline Cabeca cabecas[4]{};inline unsigned n_cabecas=0;
inline std::atomic<bool> fora_rosto{true};

inline std::atomic<float> contorno_min{.15f},contorno_max{.35f},contorno_forca{1.f};

inline std::atomic<unsigned> mascara{0};

inline std::atomic<bool> com_fg{false};
inline bool carregado() noexcept {return lido.load(std::memory_order_acquire);}
inline int linha_de(const char* rotulo) noexcept {
    if(!rotulo)return -1;for(unsigned l=0;l<LinhaCount;++l)if(std::strcmp(defs[l].rotulo,rotulo)==0)return int(l);return -1;}

inline unsigned do_ini(const Def& d,const wchar_t* ini) noexcept {
    if(!d.chave)return d.padrao;
    wchar_t t[64]{};NRCFG_STR(L"DlssNrOnAmd",d.chave,L"",t,64,ini);if(!t[0])return d.padrao;
    wchar_t* fim=nullptr;const double v=std::wcstod(t,&fim);if(fim==t||!std::isfinite(v))return d.padrao;
    if(d.no_ini_indice){const long i=std::lround(v);return i>=0&&unsigned(i)<d.n?unsigned(i):d.padrao;}
    unsigned melhor=0;double dist=1e30;for(unsigned i=0;i<d.n;++i){const double e=std::fabs(double(d.numeros[i])-v);if(e<dist){dist=e;melhor=i;}}
    return melhor;}
inline void recarregar(const wchar_t* ini) noexcept {
    for(unsigned l=0;l<LinhaCount;++l)indice[l].store(ini?do_ini(defs[l],ini):defs[l].padrao,std::memory_order_relaxed);
    local.store(!ini||NRCFG_INT(L"DlssNrOnAmd",L"NeuralFiltroNitidezLocal",1,ini)!=0,std::memory_order_relaxed);
    fora_rosto.store(!ini||NRCFG_INT(L"DlssNrOnAmd",L"NeuralFiltroNitidezForaRosto",1,ini)!=0,std::memory_order_relaxed);
    com_fg.store(ini&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralFiltrosComFg",0,ini)==1,std::memory_order_relaxed);
    {double mn=.15,mx=.35,f=1.;
     if(ini){nrinijogo::ler_double_em(ini,L"NeuralFiltroRostoContornoMin",mn,32);nrinijogo::ler_double_em(ini,L"NeuralFiltroRostoContornoMax",mx,32);
             nrinijogo::ler_double_em(ini,L"NeuralFiltroRostoContornoForca",f,32);}
     if(!(mn>=0.&&mn<mx&&mx<=1.)){mn=.15;mx=.35;}if(!(f>=0.&&f<=4.))f=1.;
     contorno_min.store(float(mn),std::memory_order_relaxed);contorno_max.store(float(mx),std::memory_order_relaxed);contorno_forca.store(float(f),std::memory_order_relaxed);}

    {double a=0.,b=0.;const bool ta=ini&&nrinijogo::ler_double_em(ini,L"NeuralMascaraPersonagem",a,32),tb=ini&&nrinijogo::ler_double_em(ini,L"NeuralFiltroMascara",b,32);
     mascara.store(nrestext::bits_mascara(ta,a,tb,b),std::memory_order_relaxed);}
    for(unsigned f=0;f<HslFaixas;++f)for(unsigned k=0;k<3;++k){
        Def d=defs[HslMatiz+k];wchar_t c[64]{};hsl_chave(f,k,c,64);d.chave=c;
        hsl[f][k].store(ini?int(do_ini(d,ini))-int(d.padrao):0,std::memory_order_relaxed);}
    indice[HslFaixa].store(0,std::memory_order_relaxed);hsl_espelhar();
    nrao::recarregar(ini);
    nril::recarregar(ini);
    aberto.store(-1);lido.store(true,std::memory_order_release);}

inline void carregar(const wchar_t* ini) noexcept {bool f=false;if(lido.load(std::memory_order_acquire)||!ini)return;static std::atomic<bool> uma{false};if(uma.compare_exchange_strong(f,true))recarregar(ini);}
inline unsigned proximo(Linha l,int delta) noexcept {const auto& d=def(l);const unsigned c=indice[l].load()%d.n;return (c+(delta<0?d.n-1:1))%d.n;}
inline const char* texto(Linha l) noexcept {const auto& d=def(l);return d.valores[indice[l].load()%d.n];}
inline void gravar(Linha l,const wchar_t* ini) noexcept {
    if(!ini)return;const auto& d=def(l);const unsigned i=indice[l].load()%d.n;wchar_t t[32]{};
    if(d.no_ini_indice)std::swprintf(t,32,L"%u",i);else{const char* s=d.valores[i];unsigned k=0;for(;s[k]&&k<31;++k)t[k]=wchar_t(s[k]);t[k]=0;}

    wchar_t c[64]{};if(l>=HslMatiz&&l<=HslLuz){hsl_chave(hsl_faixa(),unsigned(l-HslMatiz),c,64);NRCFG_ESCREVER(L"DlssNrOnAmd",c,t,ini);return;}
    if(d.chave)NRCFG_ESCREVER(L"DlssNrOnAmd",d.chave,t,ini);}

inline void agir(Linha l,int delta,const wchar_t* ini) noexcept {
    if(l>=LinhaCount||delta<-1||delta>1)return;const auto& d=def(l);
    if(d.pai<0&&delta==0){int a=int(l);if(!aberto.compare_exchange_strong(a,-1))aberto.store(int(l));return;}
    indice[l].store(proximo(l,delta));

    if(l>=HslMatiz&&l<=HslLuz)hsl[hsl_faixa()][l-HslMatiz].store(int(indice[l].load())-int(d.padrao));else if(l==HslFaixa)hsl_espelhar();
    gravar(l,ini);
    if(d.pai<0){if(indice[l].load()!=0)aberto.store(int(l));else{int a=int(l);aberto.compare_exchange_strong(a,-1);}}}

inline void restaurar(const wchar_t* ini) noexcept {
    for(unsigned l=0;l<LinhaCount;++l)indice[l].store(defs[l].padrao,std::memory_order_relaxed);
    for(unsigned f=0;f<HslFaixas;++f)for(unsigned k=0;k<3;++k)hsl[f][k].store(0,std::memory_order_relaxed);
    indice[HslFaixa].store(0,std::memory_order_relaxed);hsl_espelhar();aberto.store(-1);
    nrao::restaurar(ini);
    nril::restaurar(ini);
    if(!ini)return;
    for(unsigned l=0;l<LinhaCount;++l)if(defs[l].chave)gravar(Linha(l),ini);
    for(unsigned f=0;f<HslFaixas;++f)for(unsigned k=0;k<3;++k){
        const auto& d=defs[HslMatiz+k];wchar_t c[64]{},t[16]{};hsl_chave(f,k,c,64);
        const char* s=d.valores[d.padrao];unsigned i=0;for(;s[i]&&i<15;++i)t[i]=wchar_t(s[i]);t[i]=0;
        NRCFG_ESCREVER(L"DlssNrOnAmd",c,t,ini);}}

inline bool no_grupo(int l,int a) noexcept {return l==a||defs[l].pai==a||(l==int(RostoContorno)&&a==int(CasLiga))||
                                                    (l==int(CeuNitidez)&&(a==int(CasLiga)||a==int(ClarLiga)));}
inline bool visivel(const char* rotulo) noexcept {const int l=linha_de(rotulo);if(l<0||defs[l].pai<0)return true;const int a=aberto.load(std::memory_order_relaxed);return a>=0&&no_grupo(l,a);}

inline bool selecionado(const char* rotulo) noexcept {
    const int a=aberto.load();if(a<0)return false;const int l=linha_de(rotulo);
    if(l>=0&&no_grupo(l,a))return false;
    int e=a;return aberto.compare_exchange_strong(e,-1);}

struct Parametros { unsigned quais; bool local,fora_rosto,contorno; float contorno_min,contorno_max,contorno_forca; unsigned mascara; float cas,luma_forca,luma_limite,clar_forca,clar_sigma,clar_meios,clar_zona,niv_preto,niv_branco,niv_saida_preto,niv_saida_branco,tom_gama,tom_expo,tom_sat,tom_bleach,tom_defog;
                    float hsl[HslFaixas][3]; float hsl_suave; bool hsl_pele; float temp[3];
                    float pele_ev,pele_sat,pele_matiz,pele_g[3];
                    unsigned ao_qual,ao_res,ao_ver; float ao_intens,ao_raio,ao_esp,ao_alcance,ao_pele,ao_claro;
                    bool ceu;
                    unsigned il_qual,il_atual,il_ver; float il_intens,il_sat,il_brilho,il_pele;
                    unsigned il_estilo; };
inline float num(Linha l) noexcept {const auto& d=def(l);return d.numeros[indice[l].load(std::memory_order_relaxed)%d.n];}
inline Parametros parametros() noexcept {
    auto on=[](Linha l){return indice[l].load(std::memory_order_relaxed)!=0;};
    Parametros p{};p.quais=(on(CasLiga)?1u:0u)|(on(LumaLiga)?2u:0u)|(on(ClarLiga)?4u:0u)|(on(NivLiga)?8u:0u)|(on(TomLiga)?16u:0u);
    p.local=local.load(std::memory_order_relaxed);p.fora_rosto=fora_rosto.load(std::memory_order_relaxed);
    p.contorno=on(RostoContorno);p.contorno_min=contorno_min.load(std::memory_order_relaxed);p.contorno_max=contorno_max.load(std::memory_order_relaxed);
    p.contorno_forca=contorno_forca.load(std::memory_order_relaxed);p.mascara=mascara.load(std::memory_order_relaxed);
    p.cas=num(CasForca);p.luma_forca=num(LumaForca);p.luma_limite=num(LumaLimite);p.clar_forca=num(ClarForca);p.clar_sigma=num(ClarRaio);p.clar_meios=num(ClarMeios);p.clar_zona=num(ClarZona);
    p.niv_preto=num(NivPreto)/255.f;p.niv_branco=num(NivBranco)/255.f;p.niv_saida_preto=num(NivSaidaPreto)/255.f;p.niv_saida_branco=num(NivSaidaBranco)/255.f;
    p.tom_gama=num(TomGama);p.tom_expo=num(TomExpo);p.tom_sat=num(TomSat);p.tom_bleach=num(TomBleach);p.tom_defog=num(TomDefog);
    if(on(HslLiga)){bool algum=false;
        for(unsigned f=0;f<HslFaixas;++f)for(unsigned k=0;k<3;++k){p.hsl[f][k]=hsl_valor(f,k);algum|=p.hsl[f][k]!=0.f;}
        if(algum)p.quais|=32u;}
    p.hsl_suave=num(HslSuave);p.hsl_pele=on(HslPele);
    temperatura_ganhos(num(TempK),num(TempMatiz),p.temp);
    if(on(TempLiga)&&(p.temp[0]!=1.f||p.temp[1]!=1.f||p.temp[2]!=1.f))p.quais|=64u;
    p.pele_ev=num(PeleLuz);p.pele_sat=num(PeleSat);p.pele_matiz=num(PeleMatiz);temperatura_ganhos(num(PeleK),0.f,p.pele_g);
    {const float e=std::exp2(p.pele_ev);for(float& g:p.pele_g)g*=e;}
    if(on(PeleLiga)&&(p.pele_ev!=0.f||p.pele_sat!=0.f||p.pele_matiz!=0.f||num(PeleK)!=6500.f))p.quais|=128u;
    if(on(ClarLiga)&&on(ClarSemPele))p.quais|=num(ClarSemPele)>1.5f?8192u:512u;

    if(on(AoLiga))p.quais|=1024u;

    if(on(IlLiga))p.quais|=4096u;
    p.ceu=!on(CeuNitidez)&&(p.quais&7u)!=0u;
    p.ao_qual=unsigned(num(AoQualidade));p.ao_res=unsigned(num(AoResolucao));p.ao_ver=unsigned(num(AoVer));p.ao_intens=num(AoIntensidade);p.ao_raio=num(AoRaio);
    p.ao_esp=num(AoEspessura);p.ao_alcance=num(AoAlcance);p.ao_pele=num(AoPele)/100.f;p.ao_claro=num(AoClaro)/100.f;
    p.il_qual=unsigned(num(IlQualidade));p.il_atual=unsigned(num(IlAtualizacao));p.il_ver=unsigned(num(IlVer));p.il_intens=num(IlIntensidade);p.il_sat=num(IlSaturacao);
    p.il_brilho=num(IlBrilho)/100.f;p.il_pele=num(IlPele)/100.f;p.il_estilo=unsigned(num(IlEstilo));
    return p;}

inline bool nitidez_rostos(const Parametros& p) noexcept {return p.local&&(p.fora_rosto||p.contorno);}

inline bool areas_rosto(const Parametros& p) noexcept {return nitidez_rostos(p)||(p.quais&(128u|512u|8192u))!=0u||((p.quais&1024u)!=0u&&p.ao_pele>0.f)||((p.quais&4096u)!=0u&&p.il_pele>0.f);}
}
