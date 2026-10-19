#pragma once

inline constexpr bool NR_SEM_GRAVACAO_K=NR_SEM_GRAVACAO!=0;

#define NRD(s) s
#define NRCFG_INT GetPrivateProfileIntW
#define NRCFG_STR GetPrivateProfileStringW
#define NRCFG_ESCREVER WritePrivateProfileStringW

#define NR_LOG_ATIVO 0
#define NRDIAG(...) __VA_ARGS__
#define NRLOGF std::snprintf
