#pragma once

#include "rosto_jogador_state.h"
#include "rosto_jogador_detecta.h"
#include "ini_jogo.h"
#include "rosto_rt_jogo.h"
#include <string>

namespace nrrj {
inline std::atomic<bool> det_thread{false};

inline std::atomic<float> passo2_folga{0.f};
struct IniGanchos {
    void* ctx{};
    void(*orcamento)(void* ctx,int modo,unsigned folga_mb,unsigned outros_mb){};
    void(*rede_sync)(void* ctx,bool thread_rede){};
};

inline void ler_delta_inteiro(const wchar_t* ini) noexcept {
    if(!ini)return;
    nrrj::delta_inteiro_lig.store(NRCFG_INT(L"DlssNrOnAmd",nrrj::delta_inteiro_chave,0,ini)==1);
    double v0{},v1{};const bool l0=nrinijogo::ler_wcstod_em(ini,L"NeuralRostoJogadorElipseR0",v0,32),l1=nrinijogo::ler_wcstod_em(ini,L"NeuralRostoJogadorElipseR1",v1,32);
    const auto r=nrrj::elipse_raios_de(l0,v0,l1,v1);nrrj::elipse_r0.store(r.r0);nrrj::elipse_r1.store(r.r1);
    if(nrrj::delta_inteiro()){char l[288]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_DELTA_INTEIRO ligado r0=%.2f r1=%.2f borda=%.0f px@4K (Delta inteiro na elipse; desligados no rosto: BandaSigma, BordaCamada, NeuralRostoLuz/Cor e *Estilo, SomaCena; guarda so na banda media/fina)",double(r.r0),double(r.r1),double(nrrj::delta_inteiro_borda_px4k));nrrj::log(l);}
}

inline void ler_acum_modo(const wchar_t* ini) noexcept {
    if(!ini)return;
    const bool di=nrrj::delta_inteiro();const nrac::Padroes p=nrac::padroes(di);
    auto flt=[&](const wchar_t* k,float padrao,float lo,float hi){return nrinijogo::ler_real_limitado_em(ini,k,padrao,lo,hi,32);};
    nrrj::baixa_lenta.store(flt(L"NeuralRostoJogadorBaixaLenta",p.lenta,0.f,1.f));
    nrrj::baixa_rapida.store(flt(L"NeuralRostoJogadorBaixaRapida",p.rapida,0.f,1.f));
    nrrj::baixa_mov_px.store(flt(L"NeuralRostoJogadorBaixaMovPx",p.mov_px,.25f,64.f));
    nrrj::acum_sigma.store(flt(L"NeuralRostoJogadorAcumSigma",p.sigma,1.f,32.f));
    if(di){char l[320]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_DELTA_INTEIRO_ACUM baixa_lenta=%.3f baixa_rapida=%.3f mov_px=%.2f sigma=%.1f%s baixa_forca=%.2f acumula=%d compor_rapido=%d (luz larga da rede pela acumulacao; BaixaForca escala essa luz)",
        double(nrrj::baixa_lenta.load()),double(nrrj::baixa_rapida.load()),double(nrrj::baixa_mov_px.load()),double(nrrj::acum_sigma.load()),
        nrrj::acum_sigma.load()>nrac::sigma_teto_inteiro?" (ACIMA de 16: vaza na faixa zerada da borda)":"",double(nrrj::baixa_forca.load()),nrrj::acumula.load()?1:0,nrrj::compor_rapido.load()?1:0);nrrj::log(l);}
}

inline void ler_ini_rosto(const std::wstring& ini,const IniGanchos& g){
        wchar_t t[16]{};NRCFG_STR(L"DlssNrOnAmd",L"NeuralRostoJogador",L"0",t,16,ini.c_str());
        if(!nrrj::menu_escreveu.load())nrrj::ligado.store(nrrj::valor_de(t));
        nrrj::teto.store(nrrj::teto_de(t));

        nrrj::cada.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorCada",1,ini.c_str())),1,4)));
        nrrj::fora.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorFora",0,ini.c_str())==1);

        nrrj::cada_quadros.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorCadaQuadros",-1,ini.c_str())),-1,64));
        nrrj::corte_modo.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorCorte",0,ini.c_str())),0,31));

        {{double v{};if(nrinijogo::ler_wcstod_em(ini.c_str(),L"NeuralRostoJogadorCorteOk",v,32)&&std::isfinite(v)&&v>=0.&&v<=1.)nrrj::corte_ok.store(float(v));}
         nrrj::sinal_n.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorSinalN",6,ini.c_str())),0,64));
         nrrj::rosto_calibra.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRostoCalibra",0,ini.c_str())==1?1:0);
         nrrj::rosto_manda.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRostoManda",0,ini.c_str())==1?1:0);
         auto fl=[&](const wchar_t* k,float padrao){double v{};return nrinijogo::ler_wcstod_em(ini.c_str(),k,v,32)&&std::isfinite(v)&&v>.02&&v<=1.?float(v):padrao;};
         nrrj::alfa_pos.store(fl(L"NeuralRostoJogadorAlfaPos",.6f));nrrj::alfa_tam.store(fl(L"NeuralRostoJogadorAlfaTam",.25f));
         nrrj::rosto_ancora.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRostoAncora",0,ini.c_str())),0,30));
         nrrj::ancora_nota.store(fl(L"NeuralRostoJogadorAncoraNota",.75f));
         nrrj::nota_grande.store(fl(L"NeuralRostoJogadorNotaGrande",0.f));
         nrrj::soma_cena.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorSomaCena",0,ini.c_str())==1);
         nrrj::npc_borda_camada.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcBordaCamada",0,ini.c_str())==1);
         nrrj::nota_grande_px.store(float((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNotaGrandePx",900,ini.c_str())),200,4000)));
         nrrj::nota_rastreio.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNotaGrandeRastreio",0,ini.c_str())==1);
         nrrj::lugar_rosto_s.store(nrinijogo::ler_real_limitado_em(ini.c_str(),L"NeuralRostoJogadorLugarRosto",0.f,0.f,10.f,32));
         nrrj::adq_elig_min.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorAdqElegivelMin",0,ini.c_str())==1);
         nrrj::faixa_varre.store(unsigned(std::clamp(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorFaixaVarre",0,ini.c_str())),0,7)));
         nrrj::salto_cadeia.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorSaltoCadeia",0,ini.c_str())==1);
         nrrj::teto_tela_lig.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorTetoTela",1,ini.c_str())!=0);
         {double v=0.;nrinijogo::ler_wcstod_em(ini.c_str(),L"NeuralRostoJogadorEscalaSalto",v,32);
          nrrj::escala_salto.store(std::isfinite(v)&&v>1.01&&v<4.?float(v):0.f);}
         nrrj::rede_fixa.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRedeFixa",0,ini.c_str())),0,4)));

         {double v=0.;nrinijogo::ler_wcstod_em(ini.c_str(),L"NeuralRostoJogadorNpcOrcamentoMs",v,32);nrrj::npc_orc_ms.store(std::isfinite(v)&&v>0.&&v<=200.?float(v):0.f);}
         nrrj::npc_orc_kmax.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcOrcamentoMax",2,ini.c_str())),1,4)));
         nrrj::npc_orc_grande.store(float((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcGrandePx",480,ini.c_str())),0,4000)));
         nrrj::blur_rapido.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorBlurRapido",0,ini.c_str())==1);
         if(nrrj::agenda_ligada()||nrrj::blur_rapido.load()){char lp[200]{};NRLOGF(lp,sizeof lp,"ROSTO_PERF_CONFIG npc_orcamento_ms=%.1f npc_max_jobs=%u npc_grande_px=%.0f blur_rapido=%d",
             double(nrrj::npc_orc_ms.load()),nrrj::npc_orc_kmax.load(),double(nrrj::npc_orc_grande.load()),nrrj::blur_rapido.load()?1:0);nrrj::log(lp);}}
        nrrj::recorte_fixo_ini.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRecorteFixo",0,ini.c_str())),0,2048)&~127);
        nrrj::recorte_forcado.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRecorteForcado",0,ini.c_str())),0,2048));
        nrrj::zoom.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorZoom",0,ini.c_str())),0,63)));
        det_thread.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorDetThread",0,ini.c_str())==1);

        nrrj::falha_max.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoFalhaSeguidas",4,ini.c_str())),1,32)));
        nrrj::falha_pausa.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoFalhaPausaJobs",16,ini.c_str())),1,600)));

        {auto flt=[&](const wchar_t* k,float padrao,float lo,float hi){return nrinijogo::ler_real_limitado_em(ini.c_str(),k,padrao,lo,hi,32);};
         nrrj::acumula.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorAcumula",0,ini.c_str())==1);
         nrrj::acum_baixa.store(flt(L"NeuralRostoJogadorAcumBaixa",.15f,.02f,1.f));nrrj::acum_alta.store(flt(L"NeuralRostoJogadorAcumAlta",.4f,.02f,1.f));
         nrrj::acum_gama.store(flt(L"NeuralRostoJogadorAcumGama",1.f,0.f,8.f));
         nrrj::acum_alta_idade.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorAcumAltaIdade",0,ini.c_str())==1);
         nrrj::acum_alta_mov.store(flt(L"NeuralRostoJogadorAcumAltaMov",0.f,0.f,1.f)>0.f);
         nrrj::acum_gama_parado.store(flt(L"NeuralRostoJogadorAcumGamaParado",0.f,0.f,64.f));
         nrrj::entrada_media.store(flt(L"NeuralRostoJogadorEntradaMedia",0.f,0.f,1.f));
         nrrj::entrada_media_limiar.store(flt(L"NeuralRostoJogadorEntradaMediaLimiar",.02f,.001f,.5f));
         if(nrrj::entrada_media.load()>0.f){char lm[160]{};NRLOGF(lm,sizeof lm,"ROSTO_JOGADOR_ENTRADA_MEDIA a=%.3f limiar=%.3f (media da entrada da rede com o mesmo recorte)",double(nrrj::entrada_media.load()),double(nrrj::entrada_media_limiar.load()));nrrj::log(lm);}
         {double v{};const bool ok=nrinijogo::ler_wcstod_em(ini.c_str(),nrrj::entrada_sigma_chave,v,32);nrrj::entrada_sigma.store(nrrj::entrada_sigma_de(ok,v));
          if(nrrj::entrada_sigma.load()>0.f){char ls[160]{};NRLOGF(ls,sizeof ls,"ROSTO_JOGADOR_ENTRADA_SIGMA sigma=%.2f px@4K (entrada da rede do rosto suavizada; Delta contra a original)",double(nrrj::entrada_sigma.load()));nrrj::log(ls);}}

         nrrj::pula_limiar.store(flt(L"NeuralRostoJogadorPula",0.f,-1.f,1.f));
         nrrj::pula_max.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorPulaMax",2,ini.c_str())),1,3)));
         nrrj::pula_congela.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorPulaCongela",0,ini.c_str())==1);
         nrrj::pula_congela_px.store(flt(L"NeuralRostoJogadorPulaCongelaPx",1.f,0.f,64.f));
         nrrj::pula_congela_vel.store(flt(L"NeuralRostoJogadorPulaCongelaVel",.3f,0.f,16.f));

         nrrj::pose_barata.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorPoseBarata",0,ini.c_str())==1);
         nrrj::pose_yaw.store(flt(L"NeuralRostoJogadorPoseYaw",50.f,20.f,85.f));
         nrrj::pose_cadencia.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorPoseCadencia",2,ini.c_str())),1,4)));
         nrrj::pose_seguidas.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorPoseSeguidas",3,ini.c_str())),1,30)));
         if(!nrrj::pose_barata.load())nrrj::pose_longe.store(false);
         if(nrrj::pose_barata.load()){char lp[160]{};NRLOGF(lp,sizeof lp,"ROSTO_POSE_BARATA ligado yaw=%.0f cadencia=%u seguidas=%u",double(nrrj::pose_yaw.load()),nrrj::pose_cadencia.load(),nrrj::pose_seguidas.load());nrrj::log(lp);}
         if(nrrj::pula_congela.load()){char lc[140]{};NRLOGF(lc,sizeof lc,"ROSTO_JOGADOR_PULA_CONGELA ligado pula=%.3f (recorte pulado = copia do ultimo Delta, sem limite de pulos)",double(nrrj::pula_limiar.load()));nrrj::log(lc);}
         nrrj::passos.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorPassos",1,ini.c_str())),1,2)));
         nrrj::passo2_forca.store(flt(L"NeuralRostoJogadorPasso2Forca",.5f,.05f,2.f));
         passo2_folga.store(flt(L"NeuralRostoJogadorPasso2Folga",0.f,0.f,4.f));
         nrrj::frescor.store(flt(L"NeuralRostoJogadorFrescor",0.f,0.f,1.f));
         nrrj::predicao_s.store(flt(L"NeuralRostoJogadorPredicao",0.f,0.f,30.f));
         nrrj::identidade_s.store(flt(L"NeuralRostoJogadorIdentidade",0.f,0.f,60.f));
         nrrj::externo.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorExterno",1,ini.c_str())),0,1));
         nrrj::externo_dy.store(flt(L"NeuralRostoJogadorExternoDy",-.1f,-1.f,1.f));nrrj::externo_esc.store(flt(L"NeuralRostoJogadorExternoEscala",1.05f,.3f,3.f));

         nrrj::area_segura.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorAreaSegura",0,ini.c_str())),0,60)));
         nrrj::area_externa.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorAreaExterna",0,ini.c_str())==1);
         if(nrrj::area_segura.load()||nrrj::area_externa.load()){char la[160]{};NRLOGF(la,sizeof la,"ROSTO_JOGADOR_AREA_FILTROS segura=%u quadros externa=%d (rdr2-cabeca)",nrrj::area_segura.load(),nrrj::area_externa.load()?1:0);nrrj::log(la);}
         nrrj::identidade_bits.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorIdentidadeBits",1,ini.c_str())),0,255)));

         nrrj::cabeca_prof.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorCabecaProf",0,ini.c_str())),0,3));
         nrrj::cabeca_prof_liga_n.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorCabecaProfLigaN",3,ini.c_str())),1,30));
         nrrj::cabeca_prof_nota.store(flt(L"NeuralRostoJogadorCabecaProfNota",.35f,.05f,1.f));nrrj::cabeca_prof_liga.store(flt(L"NeuralRostoJogadorCabecaProfLiga",.5f,.05f,1.f));
         if(nrrj::cabeca_prof.load()){char lc[200]{};NRLOGF(lc,sizeof lc,"ROSTO_JOGADOR_CABECA_PROF_CONFIG bits=%d liga_n=%d nota=%.2f liga=%.2f (cabeca pela profundidade sem escala)",
             nrrj::cabeca_prof.load(),nrrj::cabeca_prof_liga_n.load(),double(nrrj::cabeca_prof_nota.load()),double(nrrj::cabeca_prof_liga.load()));nrrj::log(lc);}
         nrrj::predicao_vetos.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorPredicaoVetos",127,ini.c_str())),0,127)));
         nrrj::log_jobs.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorLogJobs",0,ini.c_str())==1);
         nrrj::frescor_borrado.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorFrescorBorrado",1,ini.c_str())!=0);

         nrrj::passo2_cada.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorPasso2Cada",1,ini.c_str())),1,4)));
         nrrj::rede_adapt.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRedeAdaptativa",0,ini.c_str())==1);
         nrrj::rede_adapt_alvo.store(flt(L"NeuralRostoJogadorRedeAlvoPx",420.f,64.f,2048.f));
         nrrj::rede_adapt_segura.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRedeAdaptativaSegura",30,ini.c_str())),0,600)));
         nrrj::compor_elipse.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorComporElipse",0,ini.c_str())==1);
         nrrj::passo1_forca.store(flt(L"NeuralRostoJogadorPasso1Forca",1.f,.25f,4.f));

         nrrj::rosto_hz.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRostoHz",10,ini.c_str())),0,30)));
         nrrj::rosto_hz_busca.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRostoHzBusca",30,ini.c_str())),0,60)));
         nrrj::rosto_nota.store(flt(L"NeuralRostoJogadorRostoNota",.7f,.3f,.99f));

         nrrj::npc2.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpc2",0,ini.c_str())==1);
         nrrj::npc2_segura_s.store(flt(L"NeuralRostoJogadorNpc2Segura",1.f,.3f,30.f));
         nrrj::npc2_acumula.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpc2Acumula",1,ini.c_str())==1);
         nrrj::npc2_vel_max.store(flt(L"NeuralRostoJogadorNpc2VelMax",0.f,0.f,50.f));
         nrrj::npc2_frescor.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpc2Frescor",0,ini.c_str())==1);
         nrrj::npc2_teto.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpc2Teto",1024,ini.c_str())),256,int(nrrj::n_max)))&~127u);
         nrrj::npc_max.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcMax",nrrj::npc_slots,ini.c_str())),1,nrrj::npc_slots));
         nrrj::adquire.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorAdquire",0,ini.c_str())),0,63));
         nrrj::captura_job.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorCapturaJob",0,ini.c_str())==1);

         nrrj::area_unica.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorAreaUnica",0,ini.c_str())==1);
         if(nrrj::area_unica.load())nrrj::log("ROSTO_JOGADOR_AREA_UNICA captura, filtros e estabilizador com a elipse da composicao na cabeca do job");
         nrrj::caminha_dela.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorCaminhaDela",0,ini.c_str())),0,3));
         nrrj::reancora.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorReancora",0,ini.c_str())),0,1));
         {const int f=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorYunetFaixa",0,ini.c_str()));
          nrrj::yunet_faixa.store(f>=1024&&f<=int(nrrj::faixa_w_max)&&f%64==0?unsigned(f):0u);}
         {const int f=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorYunetInteira",0,ini.c_str()));
          nrrj::yunet_inteira.store(f>640&&f<=int(nrrj::inteira_w_max)&&f%64==0?unsigned(f):0u);}
         {const int r=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcRecorteMin",0,ini.c_str()));nrrj::npc_recorte_min2.store(r==384||r==512?r:0);}
         {const int r=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcRecorteForcado",0,ini.c_str()));nrrj::npc_recorte_forcado.store(r>=256&&r<=1024&&r%128==0?r:0);}
         {const float f=flt(L"NeuralRostoJogadorNpcFracao",0.f,0.f,.6f);nrrj::npc_fracao.store(f>=.25f?f:0.f);}
         nrrj::npc_amplia.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcAmplia",0,ini.c_str())==1?1:0);
         {const int v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcLeve",0,ini.c_str()));nrrj::npc_leve.store(v>=1&&v<=3?v:0);
          nrrj::npc_leve_fracao.store(flt(L"NeuralRostoJogadorNpcLeveFracao",.33f,.2f,.6f));nrrj::npc_leve_salto.store(flt(L"NeuralRostoJogadorNpcLeveSalto",.6f,.3f,2.f));
          {const int r=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcLeveRecorte",1024,ini.c_str()));nrrj::npc_leve_recorte.store(r>=384&&r<=1024&&r%128==0?unsigned(r):1024u);}
          {const int r=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcLeveJanela",1024,ini.c_str()));nrrj::npc_leve_jan.store(r>=768&&r<=1536&&r%128==0?unsigned(r):1024u);}
          {const int r=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcLeveRede",512,ini.c_str()));nrrj::npc_leve_rede.store(r==384||r==512||r==640?unsigned(r):512u);}
          if(v&1){nrrj::npc_fracao.store(nrrj::npc_leve_fracao.load());nrrj::npc_amplia.store(1);}}
         {const int v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcCanonico",0,ini.c_str()));nrrj::npc_canon.store(v==1||v==2?v:0);
          {const int pp=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcCanonPerto",2,ini.c_str()));nrrj::npc_canon_perto.store(std::clamp(pp,0,2));}
          {const int jq=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcCanonJanelaPeq",768,ini.c_str()));nrrj::npc_canon_jan_peq.store(jq>=512&&jq<=1024&&jq%128==0?unsigned(jq):768u);}
          nrrj::npc_canon_fracao.store(flt(L"NeuralRostoJogadorNpcCanonFracao",.36f,.2f,.6f));nrrj::npc_canon_razao.store(flt(L"NeuralRostoJogadorNpcCanonRazao",2.35f,1.f,5.f));
          nrrj::npc_canon_corte.store(flt(L"NeuralRostoJogadorNpcCanonCorte",.6f,.05f,10.f));nrrj::npc_canon_beta.store(flt(L"NeuralRostoJogadorNpcCanonBeta",4.f,0.f,100.f));
          {const int r=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcCanonRede",384,ini.c_str()));nrrj::npc_canon_rede.store(r==512?512u:384u);}}
         {const int v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcTodos",0,ini.c_str()));nrrj::npc_todos.store(v>0&&v<=127?v:0);
          nrrj::npc_todos_nota.store(flt(L"NeuralRostoJogadorNpcTodosNota",.7f,.5f,.95f));
          nrrj::npc_todos_grandes.store(std::clamp(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcTodosGrandes",2,ini.c_str())),1,7));
          if(v>0&&v<=127){char lt[160]{};NRLOGF(lt,sizeof lt,"ROSTO_NPC2_TODOS bits=%d nota=%.2f (npc-max)",v,double(nrrj::npc_todos_nota.load()));nrrj::log(lt);}}
         nrrj::npc_limiar.store(flt(L"NeuralRostoJogadorNpcLimiar",0.f,0.f,1.f));
         nrrj::npc_peito.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcPeito",0,ini.c_str())==1?1:0);

         nrrj::detector_threads.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorDetectorThreads",2,ini.c_str())),1,8));
         nrrj::scrfd_b_inteira.store(flt(L"NeuralRostoJogadorDetectorInteira",.6f,.35f,.9f));nrrj::scrfd_b_faixa.store(flt(L"NeuralRostoJogadorDetectorFaixa",.5f,.35f,.9f));
         {char ld[200]{};NRLOGF(ld,sizeof ld,"ROSTO_NPC2_DETECTOR scrfd threads=%d nota_0.7=%.2f/%.2f (inteira/faixa e ROI do NPC)",nrrj::detector_threads.load(),double(nrrj::scrfd_b_inteira.load()),double(nrrj::scrfd_b_faixa.load()));nrrj::log(ld);}
         if(nrrj::npc2.load()){char l2[260]{};NRLOGF(l2,sizeof l2,"ROSTO_NPC2_CONFIG ligado npcs=%d teto=%u acumula=%d vel_max=%.2f segura=%.1f faixa=%u recmin=%d peito=%d (pipeline do rosto para cada NPC)",nrrj::npc_max.load(),nrrj::npc2_teto.load(),
             nrrj::npc2_acumula.load()?1:0,double(nrrj::npc2_vel_max.load()),double(nrrj::npc2_segura_s.load()),nrrj::yunet_faixa.load(),nrrj::npc_recorte_min2.load(),nrrj::npc_peito.load());nrrj::log(l2);}
         nrrj::npc_frente.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcFrente",0,ini.c_str())==1?1:0);
         nrrj::npc_conf_s.store(flt(L"NeuralRostoJogadorNpcConfS",0.f,0.f,1.f));
         {const int v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcRoiConf",0,ini.c_str()));nrrj::npc_roi_conf.store(v>0&&v<=16?v:0);}
         {const int v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcVelImg",0,ini.c_str()));nrrj::npc_vel_img.store(v==1||v==2?v:0);}
         {const int v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcAtras",0,ini.c_str()));nrrj::npc_atras.store(v>=1&&v<=3?v:0);}
         nrrj::npc_camada.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcCamada",0,ini.c_str())==1?1:0);
         nrrj::npc_corpo.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcCorpo",0,ini.c_str())==1?1:0);
         {const int v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcVida",0,ini.c_str()));nrrj::npc_vida.store(v==1||v==2?v:0);}
         nrrj::npc_vida_in.store(flt(L"NeuralRostoJogadorNpcVidaEntrada",.35f,0.f,2.f));
         if(nrrj::npc2.load()&&nrrj::npc_vida.load()){char lv[160]{};NRLOGF(lv,sizeof lv,"ROSTO_NPC2_VIDA modo=%d entrada_s=%.2f (pendente ate confirmar; saida pela rampa; na frente dela fica%s)",nrrj::npc_vida.load(),double(nrrj::npc_vida_in.load()),nrrj::npc_vida.load()>=2?"; oculto atras/na frente dela com a camada no quadro atual":"");nrrj::log(lv);}
         {const int v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcCabMin",96,ini.c_str()));nrrj::npc_cab_min.store(v>=32&&v<=512?float(v):96.f);}

         {const int om=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorOrcamento",0,ini.c_str()));
          if(g.orcamento)g.orcamento(g.ctx,om==1||om==2?om:0,unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorOrcamentoFolgaMb",256,ini.c_str())),0,8192)),
              unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorOrcamentoOutrosMb",0,ini.c_str())),0,262144)));}

         nrrj::orc_completo.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorOrcamentoCompleto",0,ini.c_str())==1);

         nrrj::orc_grace_fixa.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorOrcamentoGraceFixa",0,ini.c_str())==1);
         nrrj::orc_npc_mb.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorOrcamentoNpcMb",0,ini.c_str())),0,4096));
         nrrj::det_conjuntos.store(det_thread.load()?2:1);
         if(nrrj::npc2.load()){char l3[240]{};NRLOGF(l3,sizeof l3,"ROSTO_NPC2_CONFIG2 rosto_hz=%u frente=%d conf_s=%.3f inteira=%u adquire=%d roi_conf=%d vel_img=%d atras=%d cab_min=%.0f corpo=%d camada=%d",nrrj::rosto_hz.load(),nrrj::npc_frente.load(),double(nrrj::npc_conf_s.load()),nrrj::yunet_inteira.load(),nrrj::adquire.load(),nrrj::npc_roi_conf.load(),nrrj::npc_vel_img.load(),nrrj::npc_atras.load(),double(nrrj::npc_cab_min.load()),nrrj::npc_corpo.load(),nrrj::npc_camada.load());nrrj::log(l3);}
         if(nrrj::caminha_dela.load()){char l5[96]{};NRLOGF(l5,sizeof l5,"ROSTO_JOGADOR_CAMINHA_DELA_CONFIG caminha_dela=%d",nrrj::caminha_dela.load());nrrj::log(l5);}
         if(nrrj::reancora.load()){char l4[96]{};NRLOGF(l4,sizeof l4,"ROSTO_JOGADOR_REANCORA_CONFIG reancora=%d",nrrj::reancora.load());nrrj::log(l4);}
         if(nrrj::npc2.load()&&(nrrj::npc_fracao.load()>0.f||nrrj::npc_recorte_forcado.load()||nrrj::npc_limiar.load()>0.f)){char l4[160]{};NRLOGF(l4,sizeof l4,"ROSTO_NPC2_FRACAO f0=%.2f faixa=%.2f-%.2f teto=%u amplia=%d limiar=%.3f forcado=%d (npc-luz)",double(nrrj::npc_fracao.load()),.8*nrrj::npc_fracao.load(),1.3*nrrj::npc_fracao.load(),nrrj::npc2_teto.load(),nrrj::npc_amplia.load(),double(nrrj::npc_limiar.load()),nrrj::npc_recorte_forcado.load());nrrj::log(l4);}
         if(nrrj::npc2.load()&&nrrj::npc_canon.load()){char l6[200]{};NRLOGF(l6,sizeof l6,"ROSTO_NPC2_CANON_CONFIG fracao=%.2f razao=%.2f rede=%u corte=%.2f beta=%.2f (npc-canon)",double(nrrj::npc_canon_fracao.load()),double(nrrj::npc_canon_razao.load()),nrrj::npc_canon_rede.load(),double(nrrj::npc_canon_corte.load()),double(nrrj::npc_canon_beta.load()));nrrj::log(l6);}
         if(nrrj::npc2.load()&&nrrj::npc_leve.load()){char l5[200]{};NRLOGF(l5,sizeof l5,"ROSTO_NPC2_LEVE bits=%d fracao=%.2f rede=%u recorte=%u janela=%u salto=%.2f (npc-leve)",nrrj::npc_leve.load(),double(nrrj::npc_leve_fracao.load()),nrrj::npc_leve_rede.load(),nrrj::npc_leve_recorte.load(),nrrj::npc_leve_jan.load(),double(nrrj::npc_leve_salto.load()));nrrj::log(l5);}
         nrrj::segura.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorSegura",0,ini.c_str())),0,64)));
         nrrj::segura_pixel.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorSeguraPixel",0,ini.c_str())==1);
         nrrj::compor_rapido.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorComporRapido",0,ini.c_str())==1);

         nrrj::baixa_forca.store(flt(L"NeuralRostoJogadorBaixaForca",1.f,0.f,2.f));
         nrrj::banda_sigma.store(flt(L"NeuralRostoJogadorBandaSigma",16.f,4.f,2048.f));
         ler_delta_inteiro(ini.c_str());
         nrrj::corte_ok_ini.store(flt(L"NeuralRostoJogadorCorteOk",.07f,0.f,1.f));
         nrrj::desliga_jobs.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorDesligaJobs",12,ini.c_str())),1,6000));
         nrrj::npc_todos_rede.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcTodosRede",768,ini.c_str())),384,1536))&~127u);
         nrrj::npc_todos_jan.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcTodosJanela",768,ini.c_str())),384,2304))&~127u);
         nrrj::npc_todos_cab_max.store(flt(L"NeuralRostoJogadorNpcTodosCabMax",700.f,100.f,4000.f));
         nrrj::busca_sempre.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorBuscaSempre",0,ini.c_str())==1);
         nrrj::ceu_fora.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorCeuFora",0,ini.c_str())==1);

         nrrj::ruido_rt.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoRuidoRT",0,ini.c_str())),0,3)));
         nrrj::ruido_t0.store(flt(L"NeuralRostoRuidoRTLimiar0",.03f,0.f,4.f));
         nrrj::ruido_t1.store((std::max)(flt(L"NeuralRostoRuidoRTLimiar1",.06f,0.f,4.f),nrrj::ruido_t0.load()+1e-4f));
         nrrj::ruido_banda.store(flt(L"NeuralRostoRuidoRTBanda",.75f,0.f,1.f));
         nrrj::ruido_fino.store(flt(L"NeuralRostoRuidoRTFino",2.5f,.25f,8.f));
         nrrj::ruido_rosto0.store(flt(L"NeuralRostoRuidoRTRosto0",.25f,0.f,1.f));
         nrrj::ruido_rosto1.store((std::max)(flt(L"NeuralRostoRuidoRTRosto1",.5f,0.f,1.f),nrrj::ruido_rosto0.load()+1e-3f));
         nrrj::ruido_peso_sigma.store(flt(L"NeuralRostoRuidoRTPesoSigma",4.f,1.f,32.f));
         nrrj::ruido_escala.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoRuidoRTEscala",2,ini.c_str())),1,4)));
         nrrj::ruido_mistura.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoRuidoRTMistura",0,ini.c_str())==1);
         nrrj::ruido_alfa.store(flt(L"NeuralRostoRuidoRTAlfa",.15f,.01f,1.f));
         nrrj::ruido_alfa_var.store(flt(L"NeuralRostoRuidoRTAlfaVar",.10f,.01f,1.f));
         nrrj::ruido_gama.store(flt(L"NeuralRostoRuidoRTGama",2.5f,.5f,16.f));
         nrrj::ruido_pixel.store(flt(L"NeuralRostoRuidoRTPixel",.5f,.05f,8.f));
         nrrj::ruido_passo.store(flt(L"NeuralRostoRuidoRTPasso",2.f,1.f,8.f));
         nrrj::ruido_margem.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoRuidoRTMargem",16,ini.c_str())),4,256)));
         if(nrrj::ruido_rt.load()){char lr[320]{};NRLOGF(lr,sizeof lr,"ROSTO_RUIDO_CONFIG bits=%u banda=%.2f fino=%.2f rosto=%.2f..%.2f peso_sigma=%.1f limiar=%.3f..%.3f escala=%u mistura=%d alfa=%.2f alfa_var=%.2f gama=%.2f pixel=%.2f passo=%.0f margem=%u (rosto-rt)",
             nrrj::ruido_rt.load(),double(nrrj::ruido_banda.load()),double(nrrj::ruido_fino.load()),double(nrrj::ruido_rosto0.load()),double(nrrj::ruido_rosto1.load()),double(nrrj::ruido_peso_sigma.load()),
             double(nrrj::ruido_t0.load()),double(nrrj::ruido_t1.load()),nrrj::ruido_escala.load(),nrrj::ruido_mistura.load()?1:0,double(nrrj::ruido_alfa.load()),double(nrrj::ruido_alfa_var.load()),double(nrrj::ruido_gama.load()),double(nrrj::ruido_pixel.load()),double(nrrj::ruido_passo.load()),nrrj::ruido_margem.load());nrrj::log(lr);}

         nrrj::ruido_limpa.store(flt(L"NeuralRostoRuidoRTLimpa",0.f,0.f,1.f));
         nrrj::ruido_limpa_borda.store(flt(L"NeuralRostoRuidoRTLimpaBorda",.03f,.005f,.5f));
         nrrj::ruido_limpa_fina.store(flt(L"NeuralRostoRuidoRTLimpaFina",2.f,.5f,8.f));
         nrrj::ruido_limpa_grossa.store((std::max)(flt(L"NeuralRostoRuidoRTLimpaGrossa",16.f,2.f,64.f),nrrj::ruido_limpa_fina.load()+.5f));
         nrrj::ruido_limpa_contraste.store(flt(L"NeuralRostoRuidoRTLimpaContraste",.3f,0.f,4.f));
         nrrj::ruido_limpa_razao.store(flt(L"NeuralRostoRuidoRTLimpaRazao",1.4f,1.f,16.f));
         nrrj::ruido_fonte.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoRuidoRTFonte",0,ini.c_str())),0,2)));

         nrrj::ruido_peso.store(nrrj::ruido_rt.load()!=0u&&(nrrj::ruido_banda.load()>0.f||(nrrj::ruido_limpa.load()>0.f&&(nrrj::ruido_fonte.load()==0u||(nrrj::ruido_fonte.load()==2u&&nrrtjogo::estado()<0)))));
         if(nrrj::ruido_rt.load()&&nrrj::ruido_limpa.load()>0.f){char ll[240]{};NRLOGF(ll,sizeof ll,"ROSTO_RUIDO_LIMPA_CONFIG forca=%.2f borda=%.3f contraste=%.2f razao=%.2f banda=%.1f..%.1f px fonte=%u (0 detector, 1 sempre, 2 config.ini do jogo) (rosto-rt2)",
             double(nrrj::ruido_limpa.load()),double(nrrj::ruido_limpa_borda.load()),double(nrrj::ruido_limpa_contraste.load()),double(nrrj::ruido_limpa_razao.load()),double(nrrj::ruido_limpa_fina.load()),double(nrrj::ruido_limpa_grossa.load()),nrrj::ruido_fonte.load());nrrj::log(ll);}
         nrrj::npc_zona.store(flt(L"NeuralRostoJogadorNpcZona",1.15f,.3f,3.f));
         nrrj::diag_hist.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorDiagHist",0,ini.c_str())==1);
         nrrj::liga_forte.store(flt(L"NeuralRostoJogadorLigaForte",.75f,0.f,1.f));
         nrrj::adq_nota1.store(flt(L"NeuralRostoJogadorAdqNota1",.60f,0.f,1.f));
         nrrj::camada.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoCamada",0,ini.c_str())==1);
         nrrj::camada_segura.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoCamadaSegura",90,ini.c_str())),0,1200));
         nrrj::camada_meia.store(flt(L"NeuralRostoCamadaMeiaVida",20.f,1.f,600.f));
         nrrj::camada_margem.store(flt(L"NeuralRostoCamadaMargem",48.f,0.f,1024.f));
         nrrj::camada_regiao_limitada.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoCamadaRegiaoLimitada",0,ini.c_str())==1);
         nrrj::camada_forca.store(flt(L"NeuralRostoCamadaForca",1.f,0.f,2.f));nrrj::camada_normaliza.store(flt(L"NeuralRostoCamadaNormaliza",0.f,0.f,1.f)>.5f);nrrj::camada_fresco.store(flt(L"NeuralRostoCamadaFresco",0.f,0.f,1.f)>.5f);
         nrrj::teste_falha_ini.store(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoTesteFalhaIni",0,ini.c_str())));
         nrrj::teste_falha_fim.store(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoTesteFalhaFim",0,ini.c_str())));
         if(nrrj::camada.load()){char lc[200]{};NRLOGF(lc,sizeof lc,"ROSTO_CAMADA_CONFIG segura=%d meia_vida=%.1f margem=%.1f forca=%.2f regiao_limitada=%d",nrrj::camada_segura.load(),double(nrrj::camada_meia.load()),double(nrrj::camada_margem.load()),double(nrrj::camada_forca.load()),nrrj::camada_regiao_limitada.load()?1:0);nrrj::log(lc);}
         ler_acum_modo(ini.c_str());
         nrrj::baixa_pos_mov.store(flt(L"NeuralRostoJogadorBaixaPosMov",0.f,0.f,.999f));
         nrrj::acum_limiar.store(flt(L"NeuralRostoJogadorAcumLimiar",0.f,0.f,1.f));
         nrrj::borda_largura.store(flt(L"NeuralRostoJogadorBordaLargura",0.f,0.f,6.f));
         nrrj::borda_suave.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorBordaSuave",0,ini.c_str())),0,3)));

         nrrj::borda_camada.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorBordaCamada",0,ini.c_str())),0,2)));
         nrrj::borda_camada_reg.store(flt(L"NeuralRostoJogadorBordaCamadaReg",.05f,.001f,1.f));
         nrrj::borda_camada_z0.store(flt(L"NeuralRostoJogadorBordaCamadaZ0",1.15f,1.01f,8.f));
         nrrj::borda_camada_z1.store((std::max)(nrrj::borda_camada_z0.load()+.01f,flt(L"NeuralRostoJogadorBordaCamadaZ1",1.45f,1.02f,16.f)));
         nrrj::borda_teto.store(flt(L"NeuralRostoJogadorBordaTeto",0.f,0.f,8.f));
         if(nrrj::borda_teto.load()>0.f){char l6[100]{};NRLOGF(l6,sizeof l6,"ROSTO_JOGADOR_R5 borda_teto=%.2f",double(nrrj::borda_teto.load()));nrrj::log(l6);}
         nrrj::borda_camada_perto.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorBordaCamadaPerto",0,ini.c_str())==1);
         if(nrrj::borda_camada.load()){char l5[160]{};NRLOGF(l5,sizeof l5,"ROSTO_JOGADOR_R5 borda_camada=%u reg=%.3f perto=%d z=%.2f..%.2f",nrrj::borda_camada.load(),double(nrrj::borda_camada_reg.load()),nrrj::borda_camada_perto.load()?1:0,double(nrrj::borda_camada_z0.load()),double(nrrj::borda_camada_z1.load()));nrrj::log(l5);}
         if(nrrj::baixa_forca.load()!=1.f||nrrj::baixa_lenta.load()>0.f||nrrj::acum_limiar.load()>0.f||nrrj::borda_suave.load()!=0){char l4[220]{};NRLOGF(l4,sizeof l4,"ROSTO_JOGADOR_R4 baixa_forca=%.3f baixa_lenta=%.3f baixa_rapida=%.3f mov_px=%.2f limiar=%.4f borda_suave=%u largura=%.2f",double(nrrj::baixa_forca.load()),double(nrrj::baixa_lenta.load()),double(nrrj::baixa_rapida.load()),double(nrrj::baixa_mov_px.load()),double(nrrj::acum_limiar.load()),nrrj::borda_suave.load(),double(nrrj::borda_largura.load()));nrrj::log(l4);}
         if(nrrj::passo2_cada.load()>1||nrrj::compor_elipse.load()||nrrj::compor_rapido.load()||nrrj::ablacao.load()||nrrj::passo1_forca.load()!=1.f||nrrj::segura.load()){char lr[220]{};NRLOGF(lr,sizeof lr,"ROSTO_JOGADOR_R3 passo2_cada=%u compor_elipse=%d compor_rapido=%d ablacao=%u forca1=%.2f segura=%u segura_pixel=%d",nrrj::passo2_cada.load(),nrrj::compor_elipse.load()?1:0,nrrj::compor_rapido.load()?1:0,nrrj::ablacao.load(),double(nrrj::passo1_forca.load()),nrrj::segura.load(),nrrj::segura_pixel.load()?1:0);nrrj::log(lr);}
         if(nrrj::rede_adapt.load()){char la[160]{};NRLOGF(la,sizeof la,"ROSTO_JOGADOR_REDE_ADAPT ligada alvo=%.0f segura=%u (degraus 512/768/1024/1536, recorte fixo)",double(nrrj::rede_adapt_alvo.load()),nrrj::rede_adapt_segura.load());nrrj::log(la);}
         if(nrrj::frescor.load()>0.f){char lf[120]{};NRLOGF(lf,sizeof lf,"ROSTO_JOGADOR_FRESCOR limiar=%.3f",double(nrrj::frescor.load()));nrrj::log(lf);}
         if(nrrj::pula_limiar.load()!=0.f||nrrj::passos.load()>1){char lp[200]{};NRLOGF(lp,sizeof lp,"ROSTO_JOGADOR_R2 pula=%.4f pula_max=%u passos=%u forca2=%.2f folga2=%.2f",double(nrrj::pula_limiar.load()),nrrj::pula_max.load(),nrrj::passos.load(),double(nrrj::passo2_forca.load()),double(passo2_folga.load()));nrrj::log(lp);}

         nrrj::grade_rede.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorGrade",0,ini.c_str())),0,256)));
         nrrj::recorte_fator.store(flt(L"NeuralRostoJogadorRecorteFator",0.f,0.f,1.5f));
         {const float ev=flt(L"NeuralRostoJogadorEspera",0.f,-2.f,12.f);nrrj::espera_us.store(ev<=-1.5f?nrrj::espera_present:ev<0.f?nrrj::espera_auto:unsigned(std::lround(ev*1000.f)));}
         nrrj::espera_job.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorEsperaJob",0,ini.c_str())==1);
         nrrj::thread_rede.store(nrrj::fora.load()&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorThreadRede",0,ini.c_str())==1);
         if(g.rede_sync)g.rede_sync(g.ctx,nrrj::thread_rede.load());
         if(nrrj::thread_rede.load())nrrj::log("ROSTO_JOGADOR_THREAD_REDE rede do recorte numa thread propria (depois do k_flag_set da cena)");

         {const int sj=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorSemJob",0,ini.c_str()));
          nrrj::sem_job_q.store(nrrj::thread_rede.load()?nrrj::sj_q_de(sj,nrrj::cada_quadros.load()):0u);
          if(sj){char ls[160]{};NRLOGF(ls,sizeof ls,"ROSTO_JOGADOR_SEMJOB_CONFIG pedido=%d q=%u thread_rede=%d",sj,nrrj::sj_q(),nrrj::thread_rede.load()?1:0);nrrj::log(ls);}}
         if(nrrj::espera_us.load()){char lc[120]{};NRLOGF(lc,sizeof lc,"ROSTO_JOGADOR_ESPERA %u us depois do publish so_job_com_recorte=%d",nrrj::espera_us.load(),nrrj::espera_job.load()?1:0);nrrj::log(lc);}
         if(nrrj::grade_rede.load()||nrrj::recorte_fator.load()>0.f){char lb[160]{};NRLOGF(lb,sizeof lb,"ROSTO_JOGADOR_RECORTE grade=%u fator=%.3f",nrrj::grade_rede.load(),double(nrrj::recorte_fator.load()));nrrj::log(lb);}
         if(nrrj::acumula.load()){char la[200]{};NRLOGF(la,sizeof la,"ROSTO_JOGADOR_ACUMULA baixa=%.3f alta=%.3f gama=%.2f sigma=%.1f gama_parado=%.2f",double(nrrj::acum_baixa.load()),double(nrrj::acum_alta.load()),double(nrrj::acum_gama.load()),double(nrrj::acum_sigma.load()),double(nrrj::acum_gama_parado.load()));nrrj::log(la);}}
        char linha[240]{};NRLOGF(linha,sizeof linha,"ROSTO_JOGADOR_CONFIG ligado=%d teto=%u zoom=%u cada=%u fora=%d det_thread=%d menu=%d adquire=%d",nrrj::ligado.load()?1:0,nrrj::teto.load(),nrrj::zoom.load(),nrrj::cada.load(),nrrj::fora.load()?1:0,det_thread.load()?1:0,nrrj::menu_escreveu.load()?1:0,nrrj::adquire.load());
        nrrj::log(linha);
        nrorc::estado().ini_ok.store(true,std::memory_order_release);
}

inline void params_rastreio(Parametros& P,int q,bool orc_aplicada,int k_perto,int cap_perto){
    P.q=q;
    P.corte_modo=nrrj::corte_modo.load(std::memory_order_relaxed);
    P.corte_ok=nrrj::corte_ok_ini.load(std::memory_order_relaxed);
    P.liga_forte=nrrj::liga_forte.load(std::memory_order_relaxed);
    P.adq_nota1=nrrj::adq_nota1.load(std::memory_order_relaxed);
    P.sinal_n=nrrj::sinal_n.load(std::memory_order_relaxed);
    P.rosto_calibra=nrrj::rosto_calibra.load(std::memory_order_relaxed);P.rosto_manda=nrrj::rosto_manda.load(std::memory_order_relaxed);
    P.alfa_pos=nrrj::alfa_pos.load(std::memory_order_relaxed);P.alfa_tam=nrrj::alfa_tam.load(std::memory_order_relaxed);
    P.rosto_ancora=nrrj::rosto_ancora.load(std::memory_order_relaxed);P.ancora_nota=nrrj::ancora_nota.load(std::memory_order_relaxed);
    P.nota_grande=nrrj::nota_grande.load(std::memory_order_relaxed);P.nota_grande_px=nrrj::nota_grande_px.load(std::memory_order_relaxed);
    P.nota_rastreio=nrrj::nota_rastreio.load(std::memory_order_relaxed);
    P.lugar_rosto_s=nrrj::lugar_rosto_s.load(std::memory_order_relaxed);P.adq_elig_min=nrrj::adq_elig_min.load(std::memory_order_relaxed);
    P.salto_cadeia=nrrj::salto_cadeia.load(std::memory_order_relaxed)?1:0;
    P.id_s=nrrj::identidade_s.load(std::memory_order_relaxed);
    P.id_bits=nrrj::identidade_bits.load(std::memory_order_relaxed);
    P.cabeca_prof=nrrj::cabeca_prof.load(std::memory_order_relaxed);P.cp_liga_n=nrrj::cabeca_prof_liga_n.load(std::memory_order_relaxed);
    P.cp_nota=nrrj::cabeca_prof_nota.load(std::memory_order_relaxed);P.cp_liga=nrrj::cabeca_prof_liga.load(std::memory_order_relaxed);
    P.predicao_s=nrrj::predicao_s.load(std::memory_order_relaxed);P.pv_bits=nrrj::predicao_vetos.load(std::memory_order_relaxed);
    {const unsigned z=nrrj::zoom.load(std::memory_order_relaxed);
     if(z&1u){P.cab_max_4k=nrrj::zoom_cab_4k;P.recorte_max=int(nrrj::zoom_lado);}
     P.persistente=(z&4u)!=0;P.desliga_n_pers=nrrj::desliga_jobs.load(std::memory_order_relaxed);
      P.npc2_zona=nrrj::npc_zona.load(std::memory_order_relaxed);P.todos_cab_max=nrrj::npc_todos_cab_max.load(std::memory_order_relaxed);
     P.recorte_forcado=nrrj::recorte_forcado.load(std::memory_order_relaxed);
     if(z&16u){const int tr=int(nrrj::teto.load(std::memory_order_relaxed)),fx=nrrj::recorte_fixo_ini.load(std::memory_order_relaxed);
               (void)tr;P.recorte_fixo=(std::max)(256,fx>0?fx:1024);}
     P.cinema_livre=(z&2u)!=0;}
    P.rosto_nota=nrrj::rosto_nota.load(std::memory_order_relaxed);
    P.npc2=nrrj::npc2_ativo();P.npc2_segura_s=nrrj::npc2_segura_s.load(std::memory_order_relaxed);P.npc2_vel_max=nrrj::npc2_vel_max.load(std::memory_order_relaxed);
    P.npc_max=nrrj::npc_n()>0?nrrj::npc_n():1;
    P.adquire=nrrj::adquire.load(std::memory_order_relaxed);P.reancora=nrrj::reancora.load(std::memory_order_relaxed);P.caminha_dela=nrrj::caminha_dela.load(std::memory_order_relaxed);
    if(const int r=nrrj::npc_recorte_min2.load(std::memory_order_relaxed))P.npc2_recorte_min=r;
    P.npc2_recorte_forcado=nrrj::npc_recorte_forcado.load(std::memory_order_relaxed);
    P.npc_fracao=nrrj::npc_fracao.load(std::memory_order_relaxed);P.npc_frac_teto=int((nrrj::npc_leve.load(std::memory_order_relaxed)&1)?nrrj::npc_leve_recorte.load(std::memory_order_relaxed):nrrj::npc2_teto.load(std::memory_order_relaxed));
    P.npc_canon=nrrj::npc_canon.load(std::memory_order_relaxed);P.canon.fracao=nrrj::npc_canon_fracao.load(std::memory_order_relaxed);P.canon.razao=nrrj::npc_canon_razao.load(std::memory_order_relaxed);
    P.canon.min_corte=nrrj::npc_canon_corte.load(std::memory_order_relaxed);P.canon.beta=nrrj::npc_canon_beta.load(std::memory_order_relaxed);P.canon.rede=int(nrrj::npc_canon_rede.load(std::memory_order_relaxed));
    P.npc_leve=nrrj::npc_leve.load(std::memory_order_relaxed);P.npc_leve_salto=nrrj::npc_leve_salto.load(std::memory_order_relaxed);
    P.npc_amplia=nrrj::npc_amplia.load(std::memory_order_relaxed)?int(nrrj::npc_rede_min):0;
    P.npc_peito=nrrj::npc_peito.load(std::memory_order_relaxed);
    P.npc_frente=nrrj::npc_frente.load(std::memory_order_relaxed);
    P.npc2_conf_s=nrrj::npc_conf_s.load(std::memory_order_relaxed);
    P.npc_roi_conf=nrrj::npc_roi_conf.load(std::memory_order_relaxed);P.npc_vel_img=nrrj::npc_vel_img.load(std::memory_order_relaxed);
    P.npc_atras=nrrj::npc_atras.load(std::memory_order_relaxed);
    P.npc2_cab_min=nrrj::npc_cab_min.load(std::memory_order_relaxed);
    {const int tb=nrrj::npc_todos.load(std::memory_order_relaxed);P.npc_todos=tb;P.todos_nota=nrrj::npc_todos_nota.load(std::memory_order_relaxed);
     P.todos_perto=((tb&16)&&orc_aplicada)?k_perto:0;
     P.todos_perto_teto=((tb&16)&&orc_aplicada&&k_perto>0)?int(cap_perto):0;if(tb&2)P.npc2_cab_min=(std::min)(P.npc2_cab_min,64.f);}
    P.npc_corpo=nrrj::npc_corpo.load(std::memory_order_relaxed);
    P.npc_camada=nrrj::npc_camada.load(std::memory_order_relaxed);
    P.npc_vida=nrrj::npc_vida.load(std::memory_order_relaxed);P.vida_in_s=nrrj::npc_vida_in.load(std::memory_order_relaxed);
    P.perto=(nrrj::zoom.load(std::memory_order_relaxed)&32u)!=0;
}
}
