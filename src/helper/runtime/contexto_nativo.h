#pragma once

#include "caminho_estreito.h"
#include "pasta_helper.h"
#include "sha256.h"
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>
namespace nrctx {

inline constexpr std::size_t pesos_bytes=147689451;
inline constexpr unsigned char pesos_small[32]={0x68,0xc7,0x82,0xc8,0xc0,0x21,0xd0,0xe0,0xa5,0xe0,0x4a,0xef,0xdf,0xa5,0xaa,0x53,0x3e,0x76,0x87,0x03,0x96,0x75,0xda,0xe9,0xbc,0x7d,0x1c,0xca,0x04,0x4d,0xc3,0x63};
inline constexpr unsigned char pesos_velha[32]={0x6b,0xf8,0xdc,0x93,0x1e,0xf3,0xcc,0xff,0xe1,0x8c,0x82,0xde,0x26,0xab,0x37,0x41,0x56,0xe7,0xf1,0x95,0x39,0xff,0xcf,0x8e,0xab,0xaa,0x25,0xdc,0xa5,0xcf,0x15,0xab};
enum class Pesos {conhecidos,so_velha};

inline constexpr std::uint64_t marca_legado=0x4E52313652535430ull;
#ifdef NRMP_CPU_TEST
inline bool teste_pesos_ok=true;
#endif

inline const char* conferir(const std::filesystem::path& arquivo,Pesos quais) noexcept {
#ifdef NRMP_CPU_TEST
    (void)arquivo;(void)quais;return teste_pesos_ok?nullptr:"pesos_teste_recusados";
#else
    static std::mutex* m=new std::mutex;static auto* admitidos=new std::vector<std::wstring>;static auto* recusados=new std::vector<std::wstring>;
    try{
        const std::wstring chave=(quais==Pesos::so_velha?L"V|":L"C|")+arquivo.wstring();
        {std::lock_guard<std::mutex> t(*m);for(const auto& a:*admitidos)if(a==chave)return nullptr;}
        unsigned char d[32]{};
        const char* motivo=nullptr;
        if(!nrsha::arquivo(arquivo.c_str(),pesos_bytes,d))motivo="pesos_recusados (ausente, tamanho diferente de 147689451 ou ilegivel)";
        else if(std::memcmp(d,pesos_velha,32)!=0&&(quais==Pesos::so_velha||std::memcmp(d,pesos_small,32)!=0))
            motivo=quais==Pesos::so_velha?"pesos_recusados (digest nao e o 6BF8DC93 da rede velha)":"pesos_recusados (digest nao e o 68C782C8 nem o 6BF8DC93)";
        std::lock_guard<std::mutex> t(*m);
        if(!motivo){admitidos->push_back(chave);return nullptr;}
        bool novo=true;for(const auto& r:*recusados)if(r==chave){novo=false;break;}
        if(novo){recusados->push_back(chave);
            char l[400];std::snprintf(l,sizeof l,"%s arquivo=%ls",motivo,arquivo.c_str());diagnostic_falha(l,0,0,0);}
        return motivo;
    }catch(...){return "pesos_recusados (excecao)";}
#endif
}

inline void carregar(Hip& hip,char* base,unsigned char* p,bool& construido,const std::filesystem::path& arquivo,Pesos quais,unsigned fase) {
    if(!construido){native<void*(*)(void*)>(base,0x1ce20)(p);construido=true;}
    if(const char* motivo=conferir(arquivo,quais))throw std::runtime_error(motivo);

    const std::string estreito=nrcaminho::estreito(arquivo);
    if(estreito.empty())throw std::runtime_error("caminho dos pesos nao representavel");
    const bool ok=native<bool(*)(void*,const std::string*)>(base,0x1e120)(p,&estreito);

    if(const int pendente=hip.error())throw HipFailure(pendente,pendente==2?"pesos_sem_vram":"pesos",fase,pendente!=2);
    hip.completed();
    if(!ok||!read<void*>(p,0x10)||!read<void*>(p,0x168)||!read<size_t>(p,0x160))throw std::runtime_error("pesos indisponiveis no contexto extra");
}

inline void sem_estado(unsigned char* p) noexcept {
    write<unsigned>(p,0xf0,0);write<unsigned>(p,0x120,0);write<unsigned>(p,0x38,0);write<void*>(p,0x118,nullptr);
}

inline std::filesystem::path pasta_pesos(const char* base) {
#ifdef NRMP_CPU_TEST
    (void)base;return nrpasta::proxy(nullptr);
#else
    const auto p=nrpasta::proxy(base);if(p.empty())throw std::runtime_error("proxy path unavailable");return p;
#endif
}
}
