#include "../../../runtime/nr_publico.h"
#include "tela_captura.h"
#include "../../../layout/menu_compacto.h"
#include "../../../runtime/native_ini_path.h"
#include "menu_input.h"
#include "menu_i18n.h"
#include "../async-reprojection/reproject_controls.h"
#include "../present-runtime/print_state.h"
#include "../../../runtime/expo_trava.h"
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cstdint>
#include <filesystem>
namespace nrmenuinput {
namespace {
using detail::Block;
using detail::Device;
struct Entry {std::atomic<void**> table{};detail::StateCall state{};detail::DataCall data{};};
Entry entries[2];
std::atomic<std::uint8_t*> proxy{};
std::mutex setup_mutex;
std::atomic<unsigned long long> blocked_state{},blocked_data{},blocked_raw{},blocked_messages{},blocked_pad{};
std::atomic<detail::KeyCall> async_key{},sync_key{};
std::atomic<detail::KeyboardCall> all_keys{};
using RawDataCall=UINT (WINAPI*)(HRAWINPUT,UINT,LPVOID,PUINT,UINT);
using RawBufferCall=UINT (WINAPI*)(PRAWINPUT,PUINT,UINT);
using PeekCall=BOOL (WINAPI*)(LPMSG,HWND,UINT,UINT,UINT);
using GetCall=BOOL (WINAPI*)(LPMSG,HWND,UINT,UINT);
using PadCall=DWORD (WINAPI*)(DWORD,XINPUT_STATE*);
using StrokeCall=DWORD (WINAPI*)(DWORD,DWORD,PXINPUT_KEYSTROKE);
std::atomic<RawDataCall> raw_data{};
std::atomic<RawBufferCall> raw_buffer{};
std::atomic<PeekCall> peek_w{},peek_a{};
std::atomic<GetCall> get_w{},get_a{};
std::atomic<PadCall> pad_state{};
std::atomic<StrokeCall> pad_stroke{};
detail::PollLatch poll_latch;
detail::EventLatch raw_latch,message_latch,dinput_latch;
detail::PadLatch pad_latch;
bool visible() noexcept {auto* b=proxy.load(std::memory_order_acquire);return b&&*reinterpret_cast<volatile std::uint8_t*>(b+0x923B0)!=0&&!nrmenucompacto::desenhando.load(std::memory_order_relaxed);}
Block block() noexcept {return nrmenu::input_block()==nrmenu::InputBlock::Everything?Block::Everything:Block::MenuKeys;}
SHORT WINAPI async_hook(int k){return detail::async_key(async_key.load(),k,visible(),block(),poll_latch);}
SHORT WINAPI sync_hook(int k){return detail::sync_key(sync_key.load(),k,visible(),block(),poll_latch);}
BOOL WINAPI keys_hook(PBYTE keys){return detail::keyboard_state(all_keys.load(),keys,visible(),block(),poll_latch);}
UINT WINAPI raw_data_hook(HRAWINPUT input,UINT command,LPVOID out,PUINT size,UINT header){
    const UINT result=raw_data.load()(input,command,out,size,header);
    if(command==RID_INPUT&&out&&result!=UINT(-1)&&result>=sizeof(RAWINPUTHEADER)&&header==sizeof(RAWINPUTHEADER))
        if(detail::filter_raw(*static_cast<RAWINPUT*>(out),result,visible(),block(),raw_latch))++blocked_raw;
    return result;
}
UINT WINAPI raw_buffer_hook(PRAWINPUT out,PUINT size,UINT header){
    const UINT result=raw_buffer.load()(out,size,header);
    if(out&&result!=UINT(-1)&&result>0&&header==sizeof(RAWINPUTHEADER)){
        const bool open=visible();const Block b=block();
        PRAWINPUT record=out;
        for(UINT i=0;i<result;++i){
            if(detail::filter_raw(*record,record->header.dwSize,open,b,raw_latch))++blocked_raw;

            record=reinterpret_cast<PRAWINPUT>((reinterpret_cast<ULONG_PTR>(record)+record->header.dwSize+7)&~ULONG_PTR(7));
        }
    }
    return result;
}
BOOL WINAPI peek_w_hook(LPMSG m,HWND h,UINT a,UINT b,UINT remove){const BOOL r=peek_w.load()(m,h,a,b,remove);if(r&&m&&detail::filter_message(*m,visible(),block(),message_latch))++blocked_messages;return r;}
BOOL WINAPI peek_a_hook(LPMSG m,HWND h,UINT a,UINT b,UINT remove){const BOOL r=peek_a.load()(m,h,a,b,remove);if(r&&m&&detail::filter_message(*m,visible(),block(),message_latch))++blocked_messages;return r;}
BOOL WINAPI get_w_hook(LPMSG m,HWND h,UINT a,UINT b){const BOOL r=get_w.load()(m,h,a,b);if(r>0&&m&&detail::filter_message(*m,visible(),block(),message_latch))++blocked_messages;return r;}
BOOL WINAPI get_a_hook(LPMSG m,HWND h,UINT a,UINT b){const BOOL r=get_a.load()(m,h,a,b);if(r>0&&m&&detail::filter_message(*m,visible(),block(),message_latch))++blocked_messages;return r;}
DWORD WINAPI pad_state_hook(DWORD user,XINPUT_STATE* state){
    const DWORD r=pad_state.load()(user,state);
    if(r==ERROR_SUCCESS&&state&&pad_latch.filter(user,state->Gamepad,visible(),block()))++blocked_pad;
    return r;
}
DWORD WINAPI pad_stroke_hook(DWORD user,DWORD reserved,PXINPUT_KEYSTROKE stroke){
    const DWORD r=pad_stroke.load()(user,reserved,stroke);
    if(r==ERROR_SUCCESS&&visible()&&block()==Block::Everything){++blocked_pad;return ERROR_EMPTY;}
    return r;
}
Entry* entry(void* device) noexcept {
    auto** table=*static_cast<void***>(device);
    for(auto& e:entries)if(e.table.load(std::memory_order_acquire)==table)return &e;
    return nullptr;
}
Device kind(void* device) noexcept {

    DIDEVCAPS caps{};caps.dwSize=sizeof(caps);
    auto** table=*static_cast<void***>(device);
    using Caps=HRESULT(STDMETHODCALLTYPE*)(void*,DIDEVCAPS*);
    const auto hr=reinterpret_cast<Caps>(table[3])(device,&caps);
    if(FAILED(hr))return Device::Other;
    const auto type=GET_DIDEVICE_TYPE(caps.dwDevType);
    return type==DI8DEVTYPE_KEYBOARD?Device::Keyboard:type==DI8DEVTYPE_MOUSE?Device::Mouse:Device::Other;
}
HRESULT STDMETHODCALLTYPE state_hook(void* device,DWORD bytes,void* out){
    auto* e=entry(device);if(!e)return DIERR_GENERIC;
    const bool open=visible();const Device k=kind(device);
    const auto hr=detail::state(e->state,device,bytes,out,k,open,block(),poll_latch);
    if(open&&k!=Device::Other&&SUCCEEDED(hr))++blocked_state;
    return hr;
}
HRESULT STDMETHODCALLTYPE data_hook(void* device,DWORD bytes,DIDEVICEOBJECTDATA* out,DWORD* count,DWORD flags){
    auto* e=entry(device);if(!e)return DIERR_GENERIC;
    const bool open=visible();const Device k=kind(device);
    const auto hr=detail::data(e->data,device,bytes,out,count,flags,k,open,block(),dinput_latch);
    if(open&&k!=Device::Other&&SUCCEEDED(hr))++blocked_data;
    return hr;
}
void log(const char* status) noexcept {
(void)status;
}
bool patch(void** slot,void* before,void* after) noexcept {
    DWORD old{};if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&old))return false;
    const bool ok=InterlockedCompareExchangePointer(slot,after,before)==before;
    DWORD ignored{};const bool restored=VirtualProtect(slot,sizeof(void*),old,&ignored)!=0;
    if(!restored)log("vtable_protection_restore_failed");
    return ok;
}
bool install(void* device) noexcept {
    if(kind(device)!=Device::Keyboard)return false;
    auto** table=*static_cast<void***>(device);

    MEMORY_BASIC_INFORMATION mbi{};HMODULE table_owner{};
    if(!VirtualQuery(table,&mbi,sizeof(mbi))||mbi.Type!=MEM_IMAGE||
       !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(table),&table_owner)){
        log("nonshared_keyboard_vtable_rejected");return false;
    }
    for(auto& e:entries)if(e.table.load()==table)return true;
    Entry* available{};for(auto& e:entries)if(!e.table.load()){available=&e;break;}
    if(!available)return false;
    auto& e=*available;
    e.state=reinterpret_cast<detail::StateCall>(table[9]);
    e.data=reinterpret_cast<detail::DataCall>(table[10]);
    if(!e.state||!e.data)return false;
    e.table.store(table,std::memory_order_release);
    const bool a=patch(&table[9],reinterpret_cast<void*>(e.state),reinterpret_cast<void*>(&state_hook));
    const bool b=patch(&table[10],reinterpret_cast<void*>(e.data),reinterpret_cast<void*>(&data_hook));

    log(a&&b?"keyboard_vtable_installed":a||b?"keyboard_vtable_partial":"keyboard_vtable_not_installed");
    return a&&b;
}
template<class Call> void* take(std::atomic<Call>& store,void* before,void* hook) noexcept {
    auto value=reinterpret_cast<Call>(before);auto old=store.load();
    if(old&&old!=value)return nullptr;
    store.store(value);return hook;
}
bool xinput_module(const char* name) noexcept {return _strnicmp(name,"xinput",6)==0;}
void install_game_imports() noexcept {

    auto* image=reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    if(!image)return;
    const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<=0||dos->e_lfanew>0x100000)return;
    const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(image+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC)return;
    const auto size=nt->OptionalHeader.SizeOfImage;
    const auto& imports=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if(!imports.VirtualAddress||imports.VirtualAddress>=size||imports.Size>size-imports.VirtualAddress)return;
    const auto fits=[size](std::size_t at,std::size_t length){return at<size&&length<=size-at;};
    unsigned changed{},keyboard{},raw{},messages{},pads{};
    for(std::size_t off=imports.VirtualAddress;fits(off,sizeof(IMAGE_IMPORT_DESCRIPTOR))&&off<imports.VirtualAddress+imports.Size;off+=sizeof(IMAGE_IMPORT_DESCRIPTOR)){
        const auto* desc=reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(image+off);
        if(!desc->Name)break;
        if(!desc->OriginalFirstThunk||!desc->FirstThunk||!fits(desc->Name,8))continue;
        const char* module=reinterpret_cast<const char*>(image+desc->Name);
        if(!std::memchr(module,0,std::min<std::size_t>(64,size-desc->Name)))continue;
        const bool xinput=xinput_module(module);
        for(std::size_t n=0;;++n){
            const auto nameoff=std::size_t(desc->OriginalFirstThunk)+n*sizeof(IMAGE_THUNK_DATA64);
            const auto slotoff=std::size_t(desc->FirstThunk)+n*sizeof(IMAGE_THUNK_DATA64);
            if(!fits(nameoff,sizeof(IMAGE_THUNK_DATA64))||!fits(slotoff,sizeof(IMAGE_THUNK_DATA64)))break;
            const auto name=reinterpret_cast<const IMAGE_THUNK_DATA64*>(image+nameoff)->u1.AddressOfData;
            if(!name)break;
            auto** slot=reinterpret_cast<void**>(image+slotoff);void* before=*slot;void* hook{};unsigned* counter{};
            if(IMAGE_SNAP_BY_ORDINAL64(name)){

                if(!xinput)continue;
                const auto ordinal=IMAGE_ORDINAL64(name);
                if(ordinal==2){hook=take(pad_state,before,reinterpret_cast<void*>(&pad_state_hook));counter=&pads;}
                else if(ordinal==8){hook=take(pad_stroke,before,reinterpret_cast<void*>(&pad_stroke_hook));counter=&pads;}
            }else{
                if(!fits(static_cast<std::size_t>(name),sizeof(IMAGE_IMPORT_BY_NAME)))continue;
                const auto* fn=reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(image+name)->Name;
                const auto available=size-static_cast<std::size_t>(name)-offsetof(IMAGE_IMPORT_BY_NAME,Name);
                if(!std::memchr(fn,0,available))continue;
                if(std::strcmp(fn,"GetAsyncKeyState")==0){hook=take(async_key,before,reinterpret_cast<void*>(&async_hook));counter=&keyboard;}
                else if(std::strcmp(fn,"GetKeyState")==0){hook=take(sync_key,before,reinterpret_cast<void*>(&sync_hook));counter=&keyboard;}
                else if(std::strcmp(fn,"GetKeyboardState")==0){hook=take(all_keys,before,reinterpret_cast<void*>(&keys_hook));counter=&keyboard;}
                else if(std::strcmp(fn,"GetRawInputData")==0){hook=take(raw_data,before,reinterpret_cast<void*>(&raw_data_hook));counter=&raw;}
                else if(std::strcmp(fn,"GetRawInputBuffer")==0){hook=take(raw_buffer,before,reinterpret_cast<void*>(&raw_buffer_hook));counter=&raw;}
                else if(std::strcmp(fn,"PeekMessageW")==0){hook=take(peek_w,before,reinterpret_cast<void*>(&peek_w_hook));counter=&messages;}
                else if(std::strcmp(fn,"PeekMessageA")==0){hook=take(peek_a,before,reinterpret_cast<void*>(&peek_a_hook));counter=&messages;}
                else if(std::strcmp(fn,"GetMessageW")==0){hook=take(get_w,before,reinterpret_cast<void*>(&get_w_hook));counter=&messages;}
                else if(std::strcmp(fn,"GetMessageA")==0){hook=take(get_a,before,reinterpret_cast<void*>(&get_a_hook));counter=&messages;}
                else if(xinput&&std::strcmp(fn,"XInputGetState")==0){hook=take(pad_state,before,reinterpret_cast<void*>(&pad_state_hook));counter=&pads;}
                else if(xinput&&std::strcmp(fn,"XInputGetKeystroke")==0){hook=take(pad_stroke,before,reinterpret_cast<void*>(&pad_stroke_hook));counter=&pads;}
            }
            if(hook&&before&&patch(slot,before,hook)){++changed;++*counter;}
        }
    }
    char line[160]{};
    std::snprintf(line,sizeof line,"game_imports keyboard=%u raw_input=%u messages=%u xinput=%u",keyboard,raw,messages,pads);
    log(changed?line:"game_keyboard_imports_not_found");
}
void persist_enabled(bool next) noexcept {
    try {
        wchar_t executable[32768]{};
        const DWORD length=GetModuleFileNameW(nullptr,executable,32768);
        if(length&&length<32768){
            const auto ini=std::filesystem::path(executable).parent_path()/L"dlssnr_on_amd.ini";
            if(!NRCFG_ESCREVER(L"DlssNrOnAmd",L"Enabled",next?L"1":L"0",ini.c_str()))log("f10_persist_failed");
        }
    }catch(...){log("f10_persist_failed");}
}
}
void tick(void* base) noexcept {
    if(!base)return;
    proxy.store(static_cast<std::uint8_t*>(base),std::memory_order_release);
    std::unique_lock<std::mutex> guard(setup_mutex,std::try_to_lock);if(!guard)return;

    static detail::ToggleKey shortcut;
    DWORD foreground_pid{};
    GetWindowThreadProcessId(GetForegroundWindow(),&foreground_pid);
    auto& enabled=*reinterpret_cast<volatile std::uint8_t*>(static_cast<std::uint8_t*>(base)+0x94A8C);

    {static int auto_s=-1;
     if(auto_s<0){auto_s=0;try{const auto ini=nrinipath::resolve(base,reinterpret_cast<const void*>(&tick));if(!ini.empty())auto_s=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralMenuAutoFecha",5,ini.c_str()));}catch(...){}
      if(auto_s<0||auto_s>3600)auto_s=0;if(auto_s){char t[48];std::snprintf(t,sizeof t,"menu_auto_fecha_%ds",auto_s);log(t);}}
     if(nrmenucompacto::modo.load()<0){int m=0;try{const auto ini=nrinipath::resolve(base,reinterpret_cast<const void*>(&tick));if(!ini.empty())m=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralMenuCompacto",1,ini.c_str()));}catch(...){}
      if(m<0||m>2)m=0;nrmenucompacto::modo.store(m);if(m){char t[40];std::snprintf(t,sizeof t,"menu_compacto_modo_%d",m);log(t);}}

     if(*reinterpret_cast<volatile std::uint8_t*>(static_cast<std::uint8_t*>(base)+0x923B0)!=0&&!nrmenucompacto::desenhando.load())nrmenucompacto::recolhido.store(false);
     if(auto_s>0){
      static ULONGLONG ultimo=0;static bool era=false;
      auto* b=static_cast<std::uint8_t*>(base);auto& vis=*reinterpret_cast<volatile std::uint8_t*>(b+0x923B0);
      const ULONGLONG agora=GetTickCount64();bool tecla=false;
      for(int k:{VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT,VK_RETURN,VK_END,VK_PRIOR,VK_NEXT,VK_HOME,VK_BACK})if(GetAsyncKeyState(k)&0x8000)tecla=true;
      const bool aberto=vis!=0;
      if(aberto&&(!era||tecla))ultimo=agora;
      era=aberto;

      if(aberto&&agora-ultimo>ULONGLONG(auto_s)*1000ull){
       vis=0;
       if(void* obj=*reinterpret_cast<void* volatile*>(b+0x92440)){int novo=0;using Aviso=void(__fastcall*)(void*,int*);(*reinterpret_cast<Aviso**>(obj))[2](obj,&novo);}
       era=false;
       if(nrmenucompacto::modo.load()==1){nrmenucompacto::recolhido.store(true);log("menu_recolhido_compacto_fg");}else log("menu_auto_fechado_fg");
      }
     }}

    static int aq_fase=-1;static unsigned aq_n=0;static ULONGLONG aq_t0=0,aq_feito=0;
    {auto* b8=static_cast<std::uint8_t*>(base);volatile std::uint8_t& aq_flag=*reinterpret_cast<volatile std::uint8_t*>(b8+0x948B0);
     if(aq_fase<0){aq_fase=0;try{const auto ini=nrinipath::resolve(base,reinterpret_cast<const void*>(&tick));
       if(!ini.empty()&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralAquecerInicio",1,ini.c_str())==0)aq_fase=3;}catch(...){}}
     if(aq_fase==0&&++aq_n>=1500){
      if(enabled==0&&aq_flag==0){enabled=1;aq_fase=1;aq_t0=GetTickCount64();log("aquecer_liga");}else aq_fase=3;}
     else if(aq_fase==1){const ULONGLONG ag=GetTickCount64();
      if(aq_flag!=0&&!aq_feito)aq_feito=ag;
      if((aq_feito&&ag-aq_feito>2000)||ag-aq_t0>60000){enabled=0;aq_fase=2;log(aq_feito?"aquecer_desliga":"aquecer_desliga_sem_aquecer");}}}
    if(detail::f10_step(shortcut,(GetAsyncKeyState(VK_F10)&0x8000)!=0,foreground_pid==GetCurrentProcessId(),enabled)){
        if(aq_fase==1){enabled=1;aq_fase=2;log("aquecer_f10_usuario");}
        const bool next=enabled!=0;
        log(next?"f10_enabled":"f10_disabled");
        persist_enabled(next);
    }

    {static std::wstring ini_expo;static bool ini_expo_lido=false;
     if(!ini_expo_lido){ini_expo_lido=true;try{ini_expo=nrinipath::resolve(base,reinterpret_cast<const void*>(&tick));}catch(...){}}
     if(!ini_expo.empty())nrexpotrava::tick(base,ini_expo.c_str());}

    static bool imports_attempted{};
    if(!imports_attempted){imports_attempted=true;install_game_imports();}
    static bool attempted{};
    if(!attempted){
        HMODULE module=GetModuleHandleW(L"dinput8.dll");
        if(module){
            using Create=HRESULT(WINAPI*)(HINSTANCE,DWORD,REFIID,void**,IUnknown*);
            auto create=reinterpret_cast<Create>(GetProcAddress(module,"DirectInput8Create"));
            HMODULE pinned{},self{};
            if(create&&GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(create),&pinned)&&
               GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&tick),&self)){
                attempted=true;

                IDirectInput8W* wide{};
                if(SUCCEEDED(create(GetModuleHandleW(nullptr),DIRECTINPUT_VERSION,IID_IDirectInput8W,reinterpret_cast<void**>(&wide),nullptr))&&wide){
                    IDirectInputDevice8W* dev{};if(SUCCEEDED(wide->CreateDevice(GUID_SysKeyboard,&dev,nullptr))&&dev){install(dev);dev->Release();}wide->Release();
                }
                IDirectInput8A* ansi{};
                if(SUCCEEDED(create(GetModuleHandleW(nullptr),DIRECTINPUT_VERSION,IID_IDirectInput8A,reinterpret_cast<void**>(&ansi),nullptr))&&ansi){
                    IDirectInputDevice8A* dev{};if(SUCCEEDED(ansi->CreateDevice(GUID_SysKeyboard,&dev,nullptr))&&dev){install(dev);dev->Release();}ansi->Release();
                }
                log("probe_complete");
            }
        }
    }
    static bool last{};const bool now=visible();if(now!=last){last=now;log(now?"menu_open":"menu_closed");}
}
}
