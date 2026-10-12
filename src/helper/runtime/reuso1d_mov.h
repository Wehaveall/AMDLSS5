#pragma once

namespace nrr1dmov {
struct Config {float limiar_px{0.f},fracao{10.f},saida{0.5f};unsigned calmos{3u};};
struct Medida {bool ok{false};float media{0.f},frac{0.f};};
struct Estado {bool forcado{false};unsigned calmos{0u};};
inline Medida medida_de(const unsigned s[5]) noexcept {
 if(!s[3])return Medida{};
 return Medida{true,float(s[0])/16.f/float(s[3]),100.f*float(s[1])/float(s[3])};
}

inline bool cheio(Estado& e,const Medida& m,const Config& c) noexcept {
 if(!(c.limiar_px>0.f)){e=Estado{};return false;}
 if(!m.ok)return e.forcado;
 if(m.media>=c.limiar_px||m.frac>=c.fracao){e.forcado=true;e.calmos=0;return true;}
 if(!e.forcado)return false;
 if(m.media<c.limiar_px*c.saida&&m.frac<c.fracao*c.saida){if(++e.calmos>=c.calmos){e=Estado{};return false;}}
 else e.calmos=0;
 return true;
}
}
