#pragma once

#include <atomic>
namespace nrcusto {
enum Ponto : unsigned {
    DispInicio = 0,
    CapInicio,
    BeginFim,
    ComposeInicio,
    ReduzirFim,
    WarpFim,
    FillFim,
    EstabFim,
    CadeiaFim,
    ComposeFim,
    NativaFim,
    DispFim,
    Pontos
};
enum Cat : unsigned { Vetado = 0, Ocupada = 1, Job = 2, Cats = 3 };
using MarcaFn = void (*)(void* lista, unsigned ponto) noexcept;
using CatFn = void (*)(unsigned cat) noexcept;
inline std::atomic<MarcaFn>& marca_fn() noexcept { static std::atomic<MarcaFn> f{nullptr}; return f; }
inline std::atomic<CatFn>& cat_fn() noexcept { static std::atomic<CatFn> f{nullptr}; return f; }
inline std::atomic<CatFn>& tardia_fn() noexcept { static std::atomic<CatFn> f{nullptr}; return f; }
inline void marca(void* lista, unsigned ponto) noexcept { if (auto f = marca_fn().load(std::memory_order_relaxed)) f(lista, ponto); }
inline void categoria(unsigned cat) noexcept { if (auto f = cat_fn().load(std::memory_order_relaxed)) f(cat); }
inline void tardia() noexcept { if (auto f = tardia_fn().load(std::memory_order_relaxed)) f(1u); }
}
