#pragma once
#include "../../../runtime/nr_publico.h"

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <cstdio>
#include <string>
#include <vector>
#include <atomic>
namespace nrtela {
using Microsoft::WRL::ComPtr;
struct Item{ComPtr<ID3D12Resource> buf;ComPtr<ID3D12CommandAllocator> al;ComPtr<ID3D12GraphicsCommandList> l;D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT h{};unsigned k{};UINT64 valor{};long long qpc{};};
struct Estado{bool lido=false,falhou=false;unsigned n=0,em=900;unsigned long long presents=0;unsigned feitos=0,serie=0;std::atomic<bool> disparo{false};ComPtr<ID3D12Fence> f;UINT64 valor=0;std::vector<Item> pend;};
inline Estado& st(){static Estado e;return e;}
inline void disparar() noexcept {st().disparo.store(true);}
inline void gravar(Estado& e,bool tudo){
 for(auto it=e.pend.begin();it!=e.pend.end();){
  if(!tudo&&e.f->GetCompletedValue()<it->valor){++it;continue;}
  if(tudo&&e.f->GetCompletedValue()<it->valor){HANDLE ev=CreateEventW(nullptr,FALSE,FALSE,nullptr);e.f->SetEventOnCompletion(it->valor,ev);WaitForSingleObject(ev,2000);CloseHandle(ev);}
  void* m=nullptr;D3D12_RANGE r{0,SIZE_T(it->fp.Footprint.RowPitch)*it->h};
  if(SUCCEEDED(it->buf->Map(0,&r,&m))&&m){
   char nome[64];std::snprintf(nome,sizeof nome,"nr_tela_%u.raw",it->k+100u*e.serie);
   FILE* a=nullptr;if(fopen_s(&a,nome,"wb")==0&&a){std::fwrite(m,1,r.End,a);std::fclose(a);}
   std::snprintf(nome,sizeof nome,"nr_tela_%u.json",it->k+100u*e.serie);
   a=nullptr;if(fopen_s(&a,nome,"w")==0&&a){std::fprintf(a,"{\"width\":%u,\"height\":%u,\"row_pitch\":%u,\"format\":%u,\"index\":%u,\"qpc\":%lld}\n",it->fp.Footprint.Width,it->h,it->fp.Footprint.RowPitch,unsigned(it->fp.Footprint.Format),it->k,it->qpc);std::fclose(a);}
   D3D12_RANGE w{0,0};it->buf->Unmap(0,&w);}
  it=e.pend.erase(it);
 }
}
inline void capturar(void* swapchain,void* fila,const std::wstring& ini) noexcept {
 (void)swapchain;(void)fila;(void)ini;
}
}
