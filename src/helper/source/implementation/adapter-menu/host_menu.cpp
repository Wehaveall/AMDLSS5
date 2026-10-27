#include "host_menu.h"
#include "../../../runtime/proxy_rva.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cstring>
#include <limits>
#include <string>
namespace nradapter {
bool accessible(const void* address, std::size_t length, Access access) noexcept {
    auto current=reinterpret_cast<std::uintptr_t>(address);
    if (!current || !length || length>std::numeric_limits<std::uintptr_t>::max()-current) return false;
    const auto limit=current+length;
    while (current<limit) {
        MEMORY_BASIC_INFORMATION info{};
        if (!VirtualQuery(reinterpret_cast<void*>(current),&info,sizeof(info)) || info.State!=MEM_COMMIT ||
            (info.Protect & (PAGE_GUARD|PAGE_NOACCESS))) return false;
        const auto protection=info.Protect & 0xff;
        const bool read=protection==PAGE_READONLY || protection==PAGE_READWRITE || protection==PAGE_WRITECOPY ||
                        protection==PAGE_EXECUTE_READ || protection==PAGE_EXECUTE_READWRITE || protection==PAGE_EXECUTE_WRITECOPY;
        const bool write=protection==PAGE_READWRITE || protection==PAGE_WRITECOPY ||
                         protection==PAGE_EXECUTE_READWRITE || protection==PAGE_EXECUTE_WRITECOPY;
        const bool execute=protection==PAGE_EXECUTE || protection==PAGE_EXECUTE_READ ||
                           protection==PAGE_EXECUTE_READWRITE || protection==PAGE_EXECUTE_WRITECOPY;
        if ((access==Access::Read && !read) || (access==Access::Write && !write) || (access==Access::Execute && !execute)) return false;
        const auto start=reinterpret_cast<std::uintptr_t>(info.BaseAddress);
        if (info.RegionSize>std::numeric_limits<std::uintptr_t>::max()-start) return false;
        const auto next=start+info.RegionSize;
        if (next<=current) return false;
        current=std::min(next,limit);
    }
    return true;
}
bool valid_vector(const NativeVector* rows, std::size_t& count) noexcept {
    count=0;
    if (reinterpret_cast<std::uintptr_t>(rows)%alignof(NativeVector) || !accessible(rows,sizeof(*rows),Access::Read) ||
        !accessible(rows,sizeof(*rows),Access::Write)) return false;
    const auto begin=reinterpret_cast<std::uintptr_t>(rows->begin);
    const auto end=reinterpret_cast<std::uintptr_t>(rows->end);
    const auto capacity=reinterpret_cast<std::uintptr_t>(rows->capacity);
    if (!begin || begin%alignof(nrmenu::NativeRow) || end<begin || capacity<end ||
        (end-begin)%sizeof(nrmenu::NativeRow) || (capacity-begin)%sizeof(nrmenu::NativeRow) ||
        capacity-begin>sizeof(nrmenu::NativeRow)*1024 || capacity==begin) return false;
    if (!accessible(rows->begin,capacity-begin,Access::Read) || !accessible(rows->begin,capacity-begin,Access::Write)) return false;
    count=(end-begin)/sizeof(nrmenu::NativeRow); return true;
}
bool valid_path(const std::filesystem::path& path) noexcept {
    try {
        const auto& text=path.native();
        if (text.empty() || text.size()>32767 || !path.is_absolute() || !path.has_filename()) return false;
        if (text.compare(0,4,L"\\\\.\\")==0) return false;
        if (path.filename()==L"." || path.filename()==L"..") return false;
        const bool extended=text.compare(0,4,L"\\\\?\\")==0;
        for (std::size_t i=0;i<text.size();++i) {
            const wchar_t ch=text[i];
            if (ch<32 || ch==L'"' || ch==L'<' || ch==L'>' || ch==L'|' || ch==L'*' || (ch==L'?' && !(extended && i==2))) return false;
        }
        return true;
    } catch (...) { return false; }
}
bool resolve_host(void* base, NativeHost& host) noexcept {
    try {

        const auto address=reinterpret_cast<std::uintptr_t>(base);
        if (address%alignof(void*) || !accessible(base,sizeof(IMAGE_DOS_HEADER),Access::Read)) return false;
        IMAGE_DOS_HEADER dos{}; std::memcpy(&dos,base,sizeof(dos));
        if (dos.e_magic!=IMAGE_DOS_SIGNATURE || dos.e_lfanew<sizeof(IMAGE_DOS_HEADER) || dos.e_lfanew>0x100000) return false;
        if (address>std::numeric_limits<std::uintptr_t>::max()-0x4000000) return false;
        auto bytes=static_cast<std::byte*>(base);
        auto nt_address=bytes+dos.e_lfanew;
        if (!accessible(nt_address,sizeof(IMAGE_NT_HEADERS64),Access::Read)) return false;
        IMAGE_NT_HEADERS64 nt{}; std::memcpy(&nt,nt_address,sizeof(nt));
        if (nt.Signature!=IMAGE_NT_SIGNATURE || nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 ||
            nt.FileHeader.SizeOfOptionalHeader!=sizeof(IMAGE_OPTIONAL_HEADER64) ||
            nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC || nt.OptionalHeader.SizeOfImage<0x94C21 ||
            nt.OptionalHeader.SizeOfImage>0x4000000 || nt.FileHeader.NumberOfSections==0 || nt.FileHeader.NumberOfSections>96) return false;
        const auto table_offset=static_cast<std::size_t>(dos.e_lfanew)+sizeof(nt);
        const auto table_bytes=std::size_t(nt.FileHeader.NumberOfSections)*sizeof(IMAGE_SECTION_HEADER);
        if (nt.OptionalHeader.SizeOfHeaders>nt.OptionalHeader.SizeOfImage || table_offset+table_bytes>nt.OptionalHeader.SizeOfHeaders ||
            !accessible(bytes+table_offset,table_bytes,Access::Read)) return false;
        auto admitted=[&](std::size_t rva,std::size_t length,DWORD required,Access access) {
            if (rva>nt.OptionalHeader.SizeOfImage || length>nt.OptionalHeader.SizeOfImage-rva || !accessible(bytes+rva,length,access)) return false;
            MEMORY_BASIC_INFORMATION region{};
            if (!VirtualQuery(bytes+rva,&region,sizeof(region)) || region.AllocationBase!=base) return false;
            unsigned matches=0;
            for (unsigned i=0;i<nt.FileHeader.NumberOfSections;++i) {
                IMAGE_SECTION_HEADER section{};
                std::memcpy(&section,bytes+table_offset+i*sizeof(section),sizeof(section));
                const auto start=std::size_t(section.VirtualAddress);
                const auto size=std::size_t(section.Misc.VirtualSize);
                if (start>nt.OptionalHeader.SizeOfImage || size>nt.OptionalHeader.SizeOfImage-start) return false;
                if (rva>=start && rva-start<=size && length<=size-(rva-start)) {
                    if ((section.Characteristics & required)!=required) return false;
                    ++matches;
                }
            }
            return matches==1;
        };
        if (!admitted(0x214A0,16,IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_MEM_READ,Access::Execute) ||
            !accessible(bytes+0x214A0,16,Access::Read) ||
            !admitted(0x923D0,sizeof(NativeVector),IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE,Access::Write) ||
            !admitted(0x94C20,1,IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE,Access::Read) ||
            !admitted(nrproxy::rva_caminho_ini,32,IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE,Access::Read)) return false;
        const unsigned char prologue[]={0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x56,0x57,0x55,0x53,0x48,0x83,0xec,0x28};
        if (std::memcmp(bytes+0x214A0,prologue,sizeof(prologue))!=0) return false;
        auto rows=reinterpret_cast<NativeVector*>(bytes+0x923D0);
        std::size_t count=0; if (!valid_vector(rows,count)) return false;
        nrmenu::NativeString name{}; std::memcpy(&name,bytes+nrproxy::rva_caminho_ini,sizeof(name));
        if (name.size==0 || name.size>32767 || name.size>name.capacity || name.capacity>32767) return false;
        const char* text=name.text;
        if (name.capacity>=16) std::memcpy(&text,name.text,sizeof(text));
        else if (name.size>=16) return false;
        if (!accessible(text,static_cast<std::size_t>(name.size)+1,Access::Read) || text[name.size]!='\0' ||
            std::memchr(text,0,static_cast<std::size_t>(name.size))) return false;
        const int length=MultiByteToWideChar(CP_ACP,MB_ERR_INVALID_CHARS,text,static_cast<int>(name.size),nullptr,0);
        if (length<=0) return false;
        std::wstring wide(static_cast<std::size_t>(length),L'\0');
        if (MultiByteToWideChar(CP_ACP,MB_ERR_INVALID_CHARS,text,static_cast<int>(name.size),wide.data(),length)!=length) return false;
        std::filesystem::path selected(wide); if (!valid_path(selected)) return false;
        if (bytes[0x94C20]!=std::byte{0} && bytes[0x94C20]!=std::byte{1}) return false;
        NativeHost candidate{base,rows,bytes[0x94C20]!=std::byte{0},std::move(selected),reinterpret_cast<Grow>(bytes+0x214A0)};
        if (admitted(nrproxy::rva_mascara_auto,sizeof(unsigned),IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE,Access::Write) &&
            accessible(bytes+nrproxy::rva_mascara_auto,sizeof(unsigned),Access::Read))
            candidate.auto_mask=reinterpret_cast<unsigned*>(bytes+nrproxy::rva_mascara_auto);
        if(admitted(0x71D080,16,IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE,Access::Write) && accessible(bytes+0x71D080,16,Access::Read)) {
            unsigned magic=0;std::memcpy(&magic,bytes+0x71D08C,sizeof(magic));
            if(magic==0x53545931u) {
                candidate.style_requested=reinterpret_cast<unsigned*>(bytes+0x71D080);
                candidate.style_effective=reinterpret_cast<unsigned*>(bytes+0x71D084);
            }
        }
        if(admitted(0x71D090,12,IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE,Access::Write) && accessible(bytes+0x71D090,12,Access::Read)) {
            unsigned magic=0;std::memcpy(&magic,bytes+0x71D098,sizeof(magic));
            if(magic==0x3153504Du) {
                candidate.multipass_requested=reinterpret_cast<unsigned*>(bytes+0x71D090);
                candidate.multipass_effective=reinterpret_cast<unsigned*>(bytes+0x71D094);
#ifdef NR_TONE_RUNTIME
                candidate.tone_state=&nrtone::state;
                candidate.compose_controls=&nrcompose::state;

                candidate.reproject_controls=&nrreproject::controls;
#endif
            }
        }
        host=std::move(candidate); return true;
    } catch (...) { return false; }
}
}
