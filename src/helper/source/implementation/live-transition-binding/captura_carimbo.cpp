#include "captura_carimbo_win.h"
#include "job_timeline.h"
#include "../../../runtime/log_fila.h"
#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <mutex>
#include <thread>

using Microsoft::WRL::ComPtr;

namespace nrcapt {
namespace {
struct Pedido { int job{}; ComPtr<ID3D12CommandQueue> fila; ComPtr<ID3D12Fence> cerca; std::uint64_t alvo{}; };

class Gravador {
public:
    void marcar(void* command, int job, Ponto p) noexcept {
        if (!command || job <= 0 || p >= Pontos) return;
        try {
            ComPtr<ID3D12GraphicsCommandList> lista;
            if (FAILED(static_cast<IUnknown*>(command)->QueryInterface(IID_PPV_ARGS(&lista))) ||
                lista->GetType() != D3D12_COMMAND_LIST_TYPE_DIRECT) return;
            std::lock_guard<std::mutex> trava(m_);
            if (!criar(lista.Get())) return;
            const unsigned s = static_cast<unsigned>(job) & 15u;
            if (job_[s].load() != job) { mascara_[s].store(0); job_[s].store(job); }
            if (mascara_[s].load() & (1u << p)) return;
            const unsigned i = indice(job, p);
            lista->EndQuery(heap_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, i);
            lista->ResolveQueryData(heap_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, i, 1, leitura_.Get(), UINT64(i) * 8u);
            mascara_[s].fetch_or(1u << p);
        } catch (...) {}
    }
    void submetida(int job, void* queue, void* fence, std::uint64_t alvo) noexcept {
        if (job <= 0 || !queue || !fence || !alvo) return;
        try {
            Pedido q{};q.job = job;q.alvo = alvo;
            if (FAILED(static_cast<IUnknown*>(queue)->QueryInterface(IID_PPV_ARGS(&q.fila))) ||
                FAILED(static_cast<IUnknown*>(fence)->QueryInterface(IID_PPV_ARGS(&q.cerca)))) return;
            {
                std::lock_guard<std::mutex> trava(fm_);
                if (fila_.size() >= 8) { ++descartados_; return; }
                fila_.push_back(std::move(q));
                if (!thread_) { thread_ = true; std::thread([this] { laco(); }).detach(); }
            }
            cv_.notify_one();
        } catch (...) {}
    }
private:
    bool criar(ID3D12GraphicsCommandList* lista) {
        ComPtr<ID3D12Device> device;
        if (FAILED(lista->GetDevice(IID_PPV_ARGS(&device)))) return false;
        if (heap_) return device.Get() == device_.Get();
        if (falhou_) return false;
        D3D12_QUERY_HEAP_DESC hd{};hd.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;hd.Count = 16u * Pontos;
        D3D12_HEAP_PROPERTIES hp{};hp.Type = D3D12_HEAP_TYPE_READBACK;
        D3D12_RESOURCE_DESC rd{};rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width = 16u * Pontos * 8u;rd.Height = 1;rd.DepthOrArraySize = 1;
        rd.MipLevels = 1;rd.SampleDesc.Count = 1;rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        void* p = nullptr;
        if (FAILED(device->CreateQueryHeap(&hd, IID_PPV_ARGS(&heap_))) ||
            FAILED(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&leitura_))) ||
            FAILED(leitura_->Map(0, nullptr, &p)) || !p) {
            heap_.Reset();leitura_.Reset();falhou_ = true;log("CAPTURA_CARIMBO falhou ao criar o heap de consultas");return false;
        }
        mapa_ = static_cast<volatile std::uint64_t*>(p);device_ = device;
        log("CAPTURA_CARIMBO ligado (NeuralCapturaCarimbo=1): 4 carimbos de GPU por captura + espera da cerca 0x94990");
        return true;
    }
    static void log(const char* texto) noexcept {
        {nrlogfila::linha(L"nr_reproject.log", "pid=%lu %s\n", GetCurrentProcessId(), texto);}
    }
    void laco() noexcept {
        HANDLE evento = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        const std::int64_t qpc_freq = qpc_frequency_();
        for (;;) {
            Pedido q;
            {
                std::unique_lock<std::mutex> trava(fm_);
                cv_.wait(trava, [this] { return !fila_.empty(); });
                q = std::move(fila_.front());fila_.pop_front();
            }
            try {
                UINT64 gpu_freq = 0, g0 = 0, c0 = 0;
                const bool calibrado = SUCCEEDED(q.fila->GetTimestampFrequency(&gpu_freq)) && SUCCEEDED(q.fila->GetClockCalibration(&g0, &c0));
                if (q.cerca->GetCompletedValue() >= q.alvo) { ++ja_completas_; continue; }
                if (!evento || FAILED(q.cerca->SetEventOnCompletion(q.alvo, evento)) || WaitForSingleObject(evento, 2000) != WAIT_OBJECT_0) continue;
                nrjob::timeline().stamp(q.job, nrjob::CercaPronta, nrjob::qpc());
                if (!calibrado || !mapa_) continue;
                const unsigned s = static_cast<unsigned>(q.job) & 15u;
                if (job_[s].load() != q.job) continue;
                const unsigned mascara = mascara_[s].load();
                for (unsigned p = 0; p < Pontos; ++p) {
                    if (!(mascara & (1u << p))) continue;
                    const std::uint64_t g = mapa_[indice(q.job, Ponto(p))];
                    if (g) nrjob::timeline().stamp(q.job, nrjob::Stamp(nrjob::GpuAntesBegin + p), gpu_para_qpc(g, g0, static_cast<std::int64_t>(c0), gpu_freq, qpc_freq));
                }
            } catch (...) {}
        }
    }
    static std::int64_t qpc_frequency_() noexcept {LARGE_INTEGER v{};QueryPerformanceFrequency(&v);return v.QuadPart;}

    std::mutex m_;
    ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12QueryHeap> heap_;
    ComPtr<ID3D12Resource> leitura_;
    volatile std::uint64_t* mapa_{};
    bool falhou_{};
    std::atomic<int> job_[16]{};
    std::atomic<unsigned> mascara_[16]{};
    std::mutex fm_;
    std::condition_variable cv_;
    std::deque<Pedido> fila_;
    bool thread_{};
    unsigned descartados_{}, ja_completas_{};
};
Gravador& gravador() noexcept {static Gravador* g = new Gravador();return *g;}
}

void marcar(void* command, int job, Ponto p) noexcept {gravador().marcar(command, job, p);}
void submetida(int job, void* queue, void* fence, std::uint64_t alvo) noexcept {gravador().submetida(job, queue, fence, alvo);}
}
