#pragma once

#include <mutex>
namespace nrrostosquadro {
struct Area {float x{},y{},rx{},ry{};};
struct Lista {Area a[4];unsigned n{};};
inline std::mutex& trava(){static std::mutex m;return m;}
inline Lista& ultima(){static Lista l;return l;}
inline void publicar(const Lista& l) noexcept {std::lock_guard<std::mutex> g(trava());ultima()=l;}
inline Lista ler() noexcept {std::lock_guard<std::mutex> g(trava());return ultima();}
}
