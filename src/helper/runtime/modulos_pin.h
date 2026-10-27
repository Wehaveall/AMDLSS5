#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>
#include "sha256.h"
namespace nrmodpin {
struct Pin {const wchar_t* arquivo;std::size_t bytes;unsigned char sha256[32];};
inline constexpr Pin multipass{L"nr_multipass.hsaco",30968,{0x0d,0xf2,0x11,0x27,0x04,0x04,0x83,0x6c,0x85,0xc9,0xd4,0xca,0x27,0xc0,0xc9,0x28,0x23,0x4f,0xc5,0xa5,0xda,0x94,0xca,0x18,0x64,0x48,0x97,0x3f,0x5b,0xac,0xfe,0xf4}};
inline constexpr Pin resize{L"nr_resize.hsaco",19192,{0x25,0xc3,0xf3,0x4d,0xd6,0x59,0x6d,0xe5,0x9c,0x64,0xd1,0x13,0x60,0xf2,0x5d,0xa0,0x21,0xca,0xf2,0xb4,0x82,0xca,0x0b,0x41,0x83,0x79,0x05,0x7e,0x77,0x4f,0xca,0xcd}};
inline constexpr Pin cena{L"nr_cena.hsaco",206160,{0xbc,0xb0,0x47,0xbe,0xbc,0x38,0x6f,0x60,0xe0,0xfb,0x84,0x23,0x9d,0x19,0xd7,0xc5,0x3a,0xe9,0x7f,0x8e,0x9c,0x27,0x5d,0xf7,0xe8,0x06,0x02,0x2e,0xf6,0xd9,0xe1,0x66}};
enum class Leitura {ok,ausente,tamanho,digest};

inline bool ler_bytes(const std::filesystem::path& arquivo,std::size_t bytes,std::vector<unsigned char>& b) noexcept {
    try{
        std::ifstream f(arquivo,std::ios::binary|std::ios::ate);
        if(!f||f.tellg()!=static_cast<std::streamoff>(bytes))return false;
        b.resize(bytes);f.seekg(0);return bool(f.read(reinterpret_cast<char*>(b.data()),static_cast<std::streamsize>(bytes)));
    }catch(...){return false;}
}

inline bool ler_conferido(const std::filesystem::path& arquivo,std::size_t bytes,const unsigned char* sha,std::vector<unsigned char>& b,const unsigned char* alt=nullptr) noexcept {
    if(!ler_bytes(arquivo,bytes,b))return false;
    return nrsha::igual(b.data(),b.size(),sha)||(alt&&nrsha::igual(b.data(),b.size(),alt));
}
inline const char* texto(Leitura l) noexcept {return l==Leitura::ok?"ok":l==Leitura::ausente?"ausente":l==Leitura::tamanho?"tamanho_diferente":"digest_diferente";}

using Confere=bool(*)(const void*,std::size_t,const std::uint8_t*) noexcept;
inline Leitura ler(const std::filesystem::path& arquivo,const Pin* pin,Confere confere,std::vector<unsigned char>& bytes,std::size_t teto=std::size_t(64)<<20) noexcept {
    bytes.clear();
    try{
        std::ifstream f(arquivo,std::ios::binary|std::ios::ate);
        if(!f)return Leitura::ausente;
        const auto fim=f.tellg();
        if(fim<=0||std::size_t(fim)>teto)return Leitura::tamanho;
        if(pin&&std::size_t(fim)!=pin->bytes)return Leitura::tamanho;
        bytes.resize(std::size_t(fim));f.seekg(0);
        if(!f.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()))){bytes.clear();return Leitura::ausente;}
    }catch(...){bytes.clear();return Leitura::ausente;}
    if(pin&&(!confere||!confere(bytes.data(),bytes.size(),pin->sha256))){bytes.clear();return Leitura::digest;}
    return Leitura::ok;
}
}
