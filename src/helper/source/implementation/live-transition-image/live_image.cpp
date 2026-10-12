#include "live_image.h"
#include "image_pins.h"
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <cstring>
#include <limits>
#include <vector>

namespace nrliveimage {
namespace {
bool reject(VerificationDiagnostic* d,VerificationStage stage,std::uint32_t rva=0,DWORD error=0,DWORD protection=0) noexcept {
    if(d)*d={stage,rva,error,protection};return false;
}
struct Hash {
    BCRYPT_ALG_HANDLE algorithm{};
    BCRYPT_HASH_HANDLE hash{};
    std::vector<std::uint8_t> object;
    ~Hash() { if(hash)BCryptDestroyHash(hash); if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0); }
};
struct File {
    HANDLE value=INVALID_HANDLE_VALUE;
    ~File() { if(value!=INVALID_HANDLE_VALUE)CloseHandle(value); }
};
bool readable_image_range(void* allocation,std::uintptr_t address,std::size_t size,bool executable,bool callback,DWORD& error,DWORD& actual_protection) noexcept {
    error=0;
    actual_protection=0;
    if(!size||address>std::numeric_limits<std::uintptr_t>::max()-size)return false;
    const auto end=address+size;
    while(address<end) {
        MEMORY_BASIC_INFORMATION info{};
        if(!VirtualQuery(reinterpret_cast<void*>(address),&info,sizeof(info))){error=GetLastError();return false;}
        actual_protection=info.Protect;
        if(info.AllocationBase!=allocation||
           info.State!=MEM_COMMIT||info.Type!=MEM_IMAGE||(info.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
        const auto protection=info.Protect&0xff;
        const bool rw=protection==PAGE_READWRITE||protection==PAGE_WRITECOPY;
        const bool rx=protection==PAGE_EXECUTE_READ||protection==PAGE_EXECUTE_WRITECOPY;
        if(callback?!rw:executable?!rx:!(rw||rx||protection==PAGE_READONLY))return false;
        const auto begin=reinterpret_cast<std::uintptr_t>(info.BaseAddress);
        if(begin>address||info.RegionSize>std::numeric_limits<std::uintptr_t>::max()-begin)return false;
        const auto next=begin+info.RegionSize;
        if(next<=address)return false;
        address=next<end?next:end;
    }
    return true;
}
bool copy_memory(std::uintptr_t source,void* destination,std::size_t size,DWORD& error) noexcept {
    SIZE_T copied=0;
    error=0;
    if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(source),destination,size,&copied)){error=GetLastError();return false;}
    if(copied!=size){error=ERROR_PARTIAL_COPY;return false;}return true;
}
}
const char* verification_stage_name(VerificationStage stage) noexcept {
    switch(stage){
    case VerificationStage::Verified:return "verified";
    case VerificationStage::InvalidInput:return "invalid_input";
    case VerificationStage::HeaderProtection:return "header_protection";
    case VerificationStage::ModuleIdentity:return "module_identity";
    case VerificationStage::ModulePath:return "module_path";
    case VerificationStage::DiskOpen:return "disk_open";
    case VerificationStage::DiskSize:return "disk_size";
    case VerificationStage::DiskRead:return "disk_read";
    case VerificationStage::DiskDigest:return "disk_digest";
    case VerificationStage::SpanProtection:return "span_protection";
    case VerificationStage::SpanRead:return "span_read";
    case VerificationStage::SpanMismatch:return "span_mismatch";
    case VerificationStage::CallbackProtection:return "callback_protection";
    case VerificationStage::CallbackRead:return "callback_read";
    case VerificationStage::CallbackPublished:return "callback_published";
    case VerificationStage::Exception:return "exception";
    }return "unknown";
}
bool inspect_mapped_snapshot(const void* data,std::size_t size) noexcept {
    return inspect_mapped_snapshot(data,size,nullptr);
}
bool inspect_mapped_snapshot(const void* data,std::size_t size,VerificationDiagnostic* diagnostic) noexcept {
    return inspect_mapped_snapshot(data,size,diagnostic,0);
}
bool inspect_mapped_snapshot(const void* data,std::size_t size,VerificationDiagnostic* diagnostic,std::uint64_t validated_load_base) noexcept {
    if(!data||size!=image_size)return reject(diagnostic,VerificationStage::InvalidInput);
    const auto* bytes=static_cast<const std::uint8_t*>(data);
    std::uint64_t mapped_base{};std::memcpy(&mapped_base,bytes+0xA8,sizeof(mapped_base));
    const bool bound_image_base=validated_load_base!=0&&mapped_base==validated_load_base;
    for(const auto& pin:pins::spans) {
        if(pin.rva>size||pin.size>size-pin.rva)return reject(diagnostic,VerificationStage::SpanMismatch,pin.rva);
        if(std::memcmp(bytes+pin.rva,pin.bytes,pin.size)!=0){
            for(std::size_t i=0;i<pin.size;++i){
                const auto rva=pin.rva+i;
                if(bound_image_base&&pin.rva==0&&rva>=0xA8&&rva<0xB0)continue;
                if(bytes[rva]==pin.bytes[i])continue;
                reject(diagnostic,VerificationStage::SpanMismatch,pin.rva);
                if(diagnostic){
                diagnostic->mismatch_rva=static_cast<std::uint32_t>(pin.rva+i);
                diagnostic->expected_byte=pin.bytes[i];diagnostic->observed_byte=bytes[pin.rva+i];
                diagnostic->window_size=static_cast<std::uint32_t>(pin.size-i<8?pin.size-i:8);
                std::memcpy(&diagnostic->expected_window,pin.bytes+i,diagnostic->window_size);
                std::memcpy(&diagnostic->observed_window,bytes+pin.rva+i,diagnostic->window_size);
                }
                return false;
            }
        }
    }
    std::uint64_t callback=0;
    std::memcpy(&callback,bytes+callback_rva,sizeof(callback));
    if(callback)return reject(diagnostic,VerificationStage::CallbackPublished,callback_rva);
    if(diagnostic)*diagnostic={VerificationStage::Verified,0,0};return true;
}
bool detail::sha256_matches(const void* data,std::size_t size,const std::uint8_t* expected) noexcept {
    if(!data||!expected||size>std::numeric_limits<ULONG>::max())return false;
    try {
        Hash state;
        if(BCryptOpenAlgorithmProvider(&state.algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
        ULONG length=0,received=0;
        if(BCryptGetProperty(state.algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&length),sizeof(length),&received,0)<0||
           received!=sizeof(length)||!length||length>65536)return false;
        state.object.resize(length);
        std::array<std::uint8_t,32> digest{};
        if(BCryptCreateHash(state.algorithm,&state.hash,state.object.data(),length,nullptr,0,0)<0||
           BCryptHashData(state.hash,const_cast<PUCHAR>(static_cast<const UCHAR*>(data)),static_cast<ULONG>(size),0)<0||
           BCryptFinishHash(state.hash,digest.data(),static_cast<ULONG>(digest.size()),0)<0)return false;
        return std::memcmp(digest.data(),expected,digest.size())==0;
    } catch(...) { return false; }
}
bool inspect_disk_snapshot(const void* data,std::size_t size) noexcept {

    return size==file_size&&detail::sha256_matches(data,size,pins::digest);
}
bool verify_live_image(void* base) noexcept {
    return verify_live_image(base,nullptr);
}
bool verify_live_image(void* base,VerificationDiagnostic* diagnostic) noexcept {
    if(diagnostic)*diagnostic={};
    try {
        const auto address=reinterpret_cast<std::uintptr_t>(base);
        if(!base||address>std::numeric_limits<std::uintptr_t>::max()-image_size)return reject(diagnostic,VerificationStage::InvalidInput);
        DWORD error=0,protection=0;
        if(!readable_image_range(base,address,0x400,false,false,error,protection))return reject(diagnostic,VerificationStage::HeaderProtection,0,error,protection);
        HMODULE actual=nullptr;
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              reinterpret_cast<LPCWSTR>(base),&actual))return reject(diagnostic,VerificationStage::ModuleIdentity,0,GetLastError());
        if(actual!=base)return reject(diagnostic,VerificationStage::ModuleIdentity);
        wchar_t path[32768]{};
        const auto path_size=GetModuleFileNameW(actual,path,32768);
        if(!path_size)return reject(diagnostic,VerificationStage::ModulePath,0,GetLastError());
        if(path_size>=32768)return reject(diagnostic,VerificationStage::ModulePath,0,ERROR_INSUFFICIENT_BUFFER);
        File file;
        file.value=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file.value==INVALID_HANDLE_VALUE)return reject(diagnostic,VerificationStage::DiskOpen,0,GetLastError());
        LARGE_INTEGER length{};
        if(!GetFileSizeEx(file.value,&length))return reject(diagnostic,VerificationStage::DiskSize,0,GetLastError());
        if(length.QuadPart!=file_size)return reject(diagnostic,VerificationStage::DiskSize);
        std::vector<std::uint8_t> disk(file_size);
        DWORD received=0;
        if(!ReadFile(file.value,disk.data(),static_cast<DWORD>(disk.size()),&received,nullptr))return reject(diagnostic,VerificationStage::DiskRead,0,GetLastError());
        if(received!=disk.size())return reject(diagnostic,VerificationStage::DiskRead,0,ERROR_HANDLE_EOF);
        if(!inspect_disk_snapshot(disk.data(),disk.size()))return reject(diagnostic,VerificationStage::DiskDigest);
        std::vector<std::uint8_t> snapshot(image_size);
        for(const auto& pin:pins::spans) {
            if(!readable_image_range(base,address+pin.rva,pin.size,pin.executable,false,error,protection))return reject(diagnostic,VerificationStage::SpanProtection,pin.rva,error,protection);
            if(!copy_memory(address+pin.rva,snapshot.data()+pin.rva,pin.size,error))return reject(diagnostic,VerificationStage::SpanRead,pin.rva,error);
        }
        if(!readable_image_range(base,address+callback_rva,8,false,true,error,protection))return reject(diagnostic,VerificationStage::CallbackProtection,callback_rva,error,protection);
        if(!copy_memory(address+callback_rva,snapshot.data()+callback_rva,8,error))return reject(diagnostic,VerificationStage::CallbackRead,callback_rva,error);
        return inspect_mapped_snapshot(snapshot.data(),snapshot.size(),diagnostic,address);
    } catch(...) { return reject(diagnostic,VerificationStage::Exception); }
}
}
