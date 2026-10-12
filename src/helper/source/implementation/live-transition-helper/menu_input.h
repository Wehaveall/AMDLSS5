#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
#include <xinput.h>
#include <atomic>
#include <cstdint>
#include <cstring>

namespace nrmenuinput {

void tick(void* pinned_proxy_base) noexcept;
namespace detail {
enum class Block : unsigned { MenuKeys=0, Everything=1 };
struct ToggleKey {
    bool armed{};
    bool update(bool down,bool foreground) noexcept {
        if(!foreground){armed=false;return false;}
        if(!down){armed=true;return false;}
        const bool pressed=armed;armed=false;return pressed;
    }
};

inline bool f10_step(ToggleKey& key,bool down,bool foreground,volatile std::uint8_t& enabled) noexcept {
    if(!key.update(down,foreground))return false;
    enabled=std::uint8_t(enabled==0?1:0);return true;
}
inline bool always_key(int vk) noexcept { return vk==VK_END||vk==VK_F10; }
inline bool menu_key(int vk) noexcept {
    return always_key(vk)||vk==VK_RETURN||(vk>=VK_LEFT&&vk<=VK_DOWN)||
           vk==VK_NUMPAD2||vk==VK_NUMPAD4||vk==VK_NUMPAD6||vk==VK_NUMPAD8;
}
inline bool navigation_key(int vk) noexcept { return menu_key(vk); }
inline bool blocked_key(int vk,bool open,Block block) noexcept {
    if(vk<=0||vk>255)return false;
    if(always_key(vk))return true;
    return open&&(block==Block::Everything||menu_key(vk));
}

struct PollLatch {
    std::atomic<bool> held[512]{};
    bool mask(int key,bool down,bool blocked) noexcept {
        if(key<0||key>=512)return false;
        if(blocked){if(down)held[key].store(true,std::memory_order_relaxed);return true;}
        if(held[key].load(std::memory_order_relaxed)){
            if(!down){held[key].store(false,std::memory_order_relaxed);return false;}
            return true;
        }
        return false;
    }
};

struct EventLatch {
    std::atomic<bool> held[512]{};
    bool filter(int key,bool down,bool blocked) noexcept {
        if(key<0||key>=512)return false;
        if(!down){held[key].store(false,std::memory_order_relaxed);return false;}
        if(blocked){held[key].store(true,std::memory_order_relaxed);return true;}
        return held[key].load(std::memory_order_relaxed);
    }
};

inline int dik_to_vk(unsigned dik) noexcept {
    switch(dik){
    case DIK_END:return VK_END;case DIK_F10:return VK_F10;case DIK_RETURN:case DIK_NUMPADENTER:return VK_RETURN;
    case DIK_UP:return VK_UP;case DIK_DOWN:return VK_DOWN;case DIK_LEFT:return VK_LEFT;case DIK_RIGHT:return VK_RIGHT;
    case DIK_NUMPAD8:return VK_NUMPAD8;case DIK_NUMPAD2:return VK_NUMPAD2;case DIK_NUMPAD4:return VK_NUMPAD4;case DIK_NUMPAD6:return VK_NUMPAD6;
    default:return 0;
    }
}
inline bool blocked_dik(unsigned dik,bool open,Block block) noexcept {
    if(dik>255)return false;
    if(open&&block==Block::Everything)return true;
    const int vk=dik_to_vk(dik);
    return vk&&blocked_key(vk,open,block);
}

inline bool filter_raw_keyboard(RAWKEYBOARD& k,bool open,Block block,EventLatch& latch) noexcept {
    if(k.VKey==0xFF)return false;
    const bool down=(k.Flags&RI_KEY_BREAK)==0;
    if(!latch.filter(k.VKey,down,blocked_key(k.VKey,open,block)))return false;
    k.VKey=0xFF;k.MakeCode=KEYBOARD_OVERRUN_MAKE_CODE;return true;
}

inline bool filter_raw_mouse(RAWMOUSE& m,bool open,Block block) noexcept {
    if(!open||block!=Block::Everything)return false;
    constexpr USHORT ups=RI_MOUSE_LEFT_BUTTON_UP|RI_MOUSE_RIGHT_BUTTON_UP|RI_MOUSE_MIDDLE_BUTTON_UP|RI_MOUSE_BUTTON_4_UP|RI_MOUSE_BUTTON_5_UP;
    if(!(m.usFlags&MOUSE_MOVE_ABSOLUTE)){m.lLastX=0;m.lLastY=0;}
    m.usButtonFlags&=ups;m.usButtonData=0;return true;
}
inline bool filter_raw(RAWINPUT& r,UINT bytes,bool open,Block block,EventLatch& latch) noexcept {
    if(bytes<sizeof(RAWINPUTHEADER))return false;
    if(r.header.dwType==RIM_TYPEKEYBOARD&&bytes>=sizeof(RAWINPUTHEADER)+sizeof(RAWKEYBOARD))return filter_raw_keyboard(r.data.keyboard,open,block,latch);
    if(r.header.dwType==RIM_TYPEMOUSE&&bytes>=sizeof(RAWINPUTHEADER)+sizeof(RAWMOUSE))return filter_raw_mouse(r.data.mouse,open,block);
    return false;
}

inline bool filter_message(MSG& m,bool open,Block block,EventLatch& latch) noexcept {
    switch(m.message){
    case WM_KEYDOWN:case WM_SYSKEYDOWN:{
        const int vk=int(m.wParam&0xff);
        if(latch.filter(vk,true,blocked_key(vk,open,block))){m.message=WM_NULL;return true;}
        return false;}
    case WM_KEYUP:case WM_SYSKEYUP:{
        const int vk=int(m.wParam&0xff);latch.filter(vk,false,false);
        if(always_key(vk)){m.message=WM_NULL;return true;}
        return false;}
    case WM_CHAR:case WM_SYSCHAR:case WM_DEADCHAR:case WM_SYSDEADCHAR:
        if(open&&block==Block::Everything){m.message=WM_NULL;return true;}
        return false;
    default:
        if(open&&block==Block::Everything&&m.message>=WM_MOUSEFIRST&&m.message<=WM_MOUSELAST&&
           m.message!=WM_LBUTTONUP&&m.message!=WM_RBUTTONUP&&m.message!=WM_MBUTTONUP&&m.message!=WM_XBUTTONUP){
            m.message=WM_NULL;return true;
        }
        return false;
    }
}

struct PadLatch {
    std::atomic<unsigned> held[4]{};
    bool filter(DWORD user,XINPUT_GAMEPAD& pad,bool open,Block block) noexcept {
        if(user>=4)return false;
        if(open&&block==Block::Everything){
            held[user].fetch_or(pad.wButtons,std::memory_order_relaxed);
            pad=XINPUT_GAMEPAD{};return true;
        }
        const unsigned still=held[user].load(std::memory_order_relaxed)&pad.wButtons;
        held[user].store(still,std::memory_order_relaxed);
        if(!still)return false;
        pad.wButtons=WORD(pad.wButtons&~still);return true;
    }
};
using KeyCall=SHORT (WINAPI*)(int);
using KeyboardCall=BOOL (WINAPI*)(PBYTE);

inline SHORT async_key(KeyCall original,int vk,bool open,Block block,PollLatch& latch) noexcept {
    const SHORT value=original(vk);
    return latch.mask(vk,(value&0x8000)!=0,blocked_key(vk,open,block))?SHORT{0}:value;
}

inline SHORT sync_key(KeyCall original,int vk,bool open,Block block,PollLatch& latch) noexcept {
    const SHORT value=original(vk);
    return latch.mask(vk,(value&0x8000)!=0,blocked_key(vk,open,block))?SHORT(value&1):value;
}
inline BOOL keyboard_state(KeyboardCall original,PBYTE keys,bool open,Block block,PollLatch& latch) noexcept {
    const BOOL result=original(keys);
    if(result&&keys)for(int k=1;k<256;++k)if(latch.mask(k,(keys[k]&0x80)!=0,blocked_key(k,open,block)))keys[k]&=0x01;
    return result;
}
using StateCall=HRESULT (STDMETHODCALLTYPE*)(void*,DWORD,void*);
using DataCall=HRESULT (STDMETHODCALLTYPE*)(void*,DWORD,DIDEVICEOBJECTDATA*,DWORD*,DWORD);
enum class Device { Other, Keyboard, Mouse };

inline HRESULT state(StateCall original,void* device,DWORD bytes,void* out,Device kind,bool open,Block block,PollLatch& latch) noexcept {
    const HRESULT hr=original(device,bytes,out);
    if(FAILED(hr)||!out)return hr;
    if(kind==Device::Keyboard&&bytes>=256){
        auto* keys=static_cast<unsigned char*>(out);
        for(unsigned k=0;k<256;++k)if(latch.mask(int(256+k),(keys[k]&0x80)!=0,blocked_dik(k,open,block)))keys[k]=0;
    }else if(kind==Device::Mouse&&open&&block==Block::Everything)std::memset(out,0,bytes);
    return hr;
}

inline HRESULT data(DataCall original,void* device,DWORD bytes,DIDEVICEOBJECTDATA* out,DWORD* count,DWORD flags,
                    Device kind,bool open,Block block,EventLatch& latch) noexcept {

    const DWORD forwarded=open&&kind!=Device::Other?(flags&~DIGDD_PEEK):flags;
    const HRESULT hr=original(device,bytes,out,count,forwarded);
    if(FAILED(hr)||!out||!count||kind==Device::Other||bytes<sizeof(DIDEVICEOBJECTDATA_DX3))return hr;
    auto* base=reinterpret_cast<unsigned char*>(out);DWORD kept=0;
    for(DWORD i=0;i<*count;++i){
        auto* record=reinterpret_cast<DIDEVICEOBJECTDATA*>(base+std::size_t(i)*bytes);
        bool drop=false;
        if(kind==Device::Keyboard){
            const unsigned dik=record->dwOfs&0xff;
            drop=latch.filter(int(256+dik),(record->dwData&0x80)!=0,blocked_dik(dik,open,block));
        }else if(open&&block==Block::Everything){
            const bool button=record->dwOfs>=DIMOFS_BUTTON0&&record->dwOfs<=DIMOFS_BUTTON7;
            drop=!(button&&(record->dwData&0x80)==0);
        }
        if(drop)continue;
        if(kept!=i)std::memmove(base+std::size_t(kept)*bytes,record,bytes);
        ++kept;
    }
    *count=kept;
    return hr;
}
}
}
