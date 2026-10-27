#pragma once

#include <windows.h>
#include <bcrypt.h>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>
#pragma comment(lib,"bcrypt.lib")
namespace nrsha {

inline bool sha256(const void* data,std::size_t size,unsigned char out[32]) noexcept {
    if(!out)return false;
    std::memset(out,0,32);
    if((!data&&size)||size>(std::numeric_limits<ULONG>::max)())return false;
    struct Estado {
        std::vector<unsigned char> objeto;BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE hash{};
        ~Estado(){if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);}
    };
    try{
        Estado e;
        if(BCryptOpenAlgorithmProvider(&e.alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
        ULONG tamanho=0,recebido=0;
        if(BCryptGetProperty(e.alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&tamanho),sizeof(tamanho),&recebido,0)<0||
           recebido!=sizeof(tamanho)||!tamanho||tamanho>65536)return false;
        e.objeto.resize(tamanho);
        unsigned char vazio=0;
        if(BCryptCreateHash(e.alg,&e.hash,e.objeto.data(),tamanho,nullptr,0,0)<0||
           BCryptHashData(e.hash,const_cast<PUCHAR>(data?static_cast<const UCHAR*>(data):&vazio),static_cast<ULONG>(size),0)<0||
           BCryptFinishHash(e.hash,out,32,0)<0){std::memset(out,0,32);return false;}
        return true;
    }catch(...){std::memset(out,0,32);return false;}
}

inline bool igual(const void* data,std::size_t size,const unsigned char* esperado) noexcept {
    if(!data||!esperado)return false;
    unsigned char d[32];
    return sha256(data,size,d)&&std::memcmp(d,esperado,32)==0;
}

inline bool arquivo(const wchar_t* caminho,std::size_t bytes,unsigned char out[32]) noexcept {
    if(!out||!caminho)return false;
    std::memset(out,0,32);
    struct Estado {
        std::vector<unsigned char> objeto;BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE hash{};HANDLE f=INVALID_HANDLE_VALUE;
        ~Estado(){if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);if(f!=INVALID_HANDLE_VALUE)CloseHandle(f);}
    };
    try{
        Estado e;
        e.f=CreateFileW(caminho,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
        if(e.f==INVALID_HANDLE_VALUE)return false;
        LARGE_INTEGER t{};if(!GetFileSizeEx(e.f,&t)||t.QuadPart<0||(bytes&&std::size_t(t.QuadPart)!=bytes))return false;
        if(BCryptOpenAlgorithmProvider(&e.alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
        ULONG tamanho=0,recebido=0;
        if(BCryptGetProperty(e.alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&tamanho),sizeof(tamanho),&recebido,0)<0||
           recebido!=sizeof(tamanho)||!tamanho||tamanho>65536)return false;
        e.objeto.resize(tamanho);
        if(BCryptCreateHash(e.alg,&e.hash,e.objeto.data(),tamanho,nullptr,0,0)<0)return false;
        std::vector<unsigned char> bloco(std::size_t(4)<<20);
        unsigned long long lidos=0;
        for(;;){
            DWORD n=0;if(!ReadFile(e.f,bloco.data(),DWORD(bloco.size()),&n,nullptr))return false;
            if(!n)break;
            if(BCryptHashData(e.hash,bloco.data(),n,0)<0)return false;
            lidos+=n;
        }
        if(lidos!=static_cast<unsigned long long>(t.QuadPart))return false;
        if(BCryptFinishHash(e.hash,out,32,0)<0){std::memset(out,0,32);return false;}
        return true;
    }catch(...){std::memset(out,0,32);return false;}
}

inline void hex(const void* data,std::size_t size,char (&out)[65]) noexcept {
    out[0]='?';out[1]=0;
    unsigned char d[32];if(!sha256(data,size,d))return;
    for(int i=0;i<32;++i)std::snprintf(out+2*i,3,"%02X",d[i]);
}
}
