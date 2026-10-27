#pragma once

#include <cstring>
namespace nrreproj {
struct Mv {const void* mv;int pitch,fmt,mvw,mvh;float sx,sy;int neth,netw;const void* dep;int dpitch,dfmt,dw,dh,dinv;};
static_assert(sizeof(Mv)==72,"Mv 72 B (o layout que os kernels leem)");
inline void copiar(const void* params,Mv& m) noexcept {
    const auto* p=static_cast<const unsigned char*>(params);
    std::memcpy(&m.mv,p+0x10,8);std::memcpy(&m.pitch,p+0x18,4);std::memcpy(&m.fmt,p+0x1c,4);std::memcpy(&m.mvw,p+0x20,4);std::memcpy(&m.mvh,p+0x24,4);
    std::memcpy(&m.sx,p+0x58,4);std::memcpy(&m.sy,p+0x5c,4);std::memcpy(&m.neth,p+0x68,4);std::memcpy(&m.netw,p+0x6c,4);
    std::memcpy(&m.dep,p+0x30,8);std::memcpy(&m.dpitch,p+0x38,4);std::memcpy(&m.dfmt,p+0x3c,4);std::memcpy(&m.dw,p+0x40,4);std::memcpy(&m.dh,p+0x44,4);std::memcpy(&m.dinv,p+0x48,4);
}
}
