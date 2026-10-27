#pragma once

#include "nr_publico.h"
#include <windows.h>
#include <share.h>
#include <cstdio>
#include "ini_jogo.h"
#include "log_fila.h"
#include "pasta_helper.h"
namespace nrsaida {

inline bool encurtar(DWORD motivo,const void* reservado,int chave) noexcept {
    return motivo==DLL_PROCESS_DETACH&&reservado!=nullptr&&chave!=0;
}
inline int chave() noexcept {
    return int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralSaidaRapida",1,nrinijogo::caminho()));
}

inline DWORD codigo_atual() noexcept {
    DWORD c=0;if(!GetExitCodeProcess(GetCurrentProcess(),&c)||c==STILL_ACTIVE)c=0;return c;
}

inline void no_detach(DWORD motivo,void* reservado) noexcept {
    if(motivo!=DLL_PROCESS_DETACH||!reservado)return;
    const int k=chave();
    const DWORD codigo=codigo_atual();
    if(encurtar(motivo,reservado,k))TerminateProcess(GetCurrentProcess(),codigo);
}
}
