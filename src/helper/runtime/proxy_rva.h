#pragma once

#include <cstdint>
namespace nrproxy {

inline constexpr std::uintptr_t rva_menu_visivel=0x923B0;
inline constexpr std::uintptr_t rva_menu_aviso=0x92440;
inline constexpr std::uintptr_t rva_enabled=0x94A8C;

inline constexpr unsigned rva_dispositivo=0x93F38;
inline constexpr unsigned rva_fila=0x93F40;
inline constexpr unsigned rva_contexto=0x93F48;
inline constexpr unsigned rva_motor_pronto=0x94268;
inline constexpr unsigned rva_motor_erro=0x9426A;

inline constexpr unsigned rva_cache_cor=0x94298;
inline constexpr unsigned rva_cache_mv=0x94398;
inline constexpr unsigned rva_cache_prof=0x94498;
inline constexpr unsigned rva_cache_expo=0x94598;

inline constexpr unsigned rva_inline_pedido=0x94710;
inline constexpr unsigned rva_inline_ativo=0x94711;
inline constexpr unsigned rva_interop_pedido=0x948F4;
inline constexpr unsigned rva_interop_ativo=0x948F5;
inline constexpr unsigned rva_aquecido=0x948B0;

inline constexpr unsigned rva_cerca=0x94990;
inline constexpr unsigned rva_cerca_valor=0x949A0;
inline constexpr unsigned rva_worker_job=0x949BC;
inline constexpr unsigned rva_cerca_alvo=0x949C0;
inline constexpr unsigned rva_job_pronto=0x949C8;
inline constexpr unsigned rva_lista_pendente=0x949D0;
inline constexpr unsigned rva_job_pendente=0x949D8;
inline constexpr unsigned rva_job_aceito=0x949DC;
inline constexpr unsigned rva_residuo=0x94A08;
inline constexpr unsigned rva_job_compondo=0x94A60;
inline constexpr unsigned rva_lista_interna=0x94A70;
inline constexpr unsigned rva_reset_pendente=0x94B78;
inline constexpr unsigned rva_painel_ms=0x94B90;
inline constexpr unsigned rva_painel_job=0x94B98;
inline constexpr unsigned rva_hip_dispositivo=0x94BA0;

inline constexpr unsigned rva_flags_ffx=0x94A80;
inline constexpr unsigned rva_usar_fsr=0x94A8E;
inline constexpr unsigned rva_usar_prof=0x94A8F;
inline constexpr unsigned rva_tom_ini=0x94A90;
inline constexpr unsigned rva_tom_ef=0x94698;
inline constexpr unsigned rva_estrutura_local=0x94AA4;
inline constexpr unsigned rva_estrutura_pele=0x94AA8;
inline constexpr unsigned rva_escala=0x94AAC;
inline constexpr unsigned rva_mascara_auto=0x94AB0;
inline constexpr unsigned rva_caminho_ini=0x94AC0;
inline constexpr unsigned rva_prof_global=0x94BD0;
inline constexpr unsigned rva_expo_job=0x90CC0;
}
