#include "coordinator.h"
#include "../compat-diag/compat.h"
#include "../../../runtime/rosto_jogador_state.h"

#include <array>
#include <cstdio>
#include <cstring>

namespace nrscale {

void Coordinator::refuse_decode(const void* descriptor) noexcept{

  const char* id=nrffx::refusal(descriptor,nullptr,0,nullptr,0);
  if(!id||id==decode_reported_)return;
  decode_reported_=id;
  char detalhe[600],campo[48];
  (void)nrffx::refusal(descriptor,detalhe,sizeof detalhe,campo,sizeof campo);
  char texto[700];std::snprintf(texto,sizeof texto,"scale_descriptor_unsupported %s",detalhe);
  if(std::strcmp(id,"ffx_reset_flag")==0){nrcompat::report(id,texto);return;}
  nrcompat::block(nrcompat::Origem::Descritor,id,texto,campo);
}

void Coordinator::make_sticky(const char* diagnostic) noexcept{
  if(!sticky_.exchange(true,std::memory_order_acq_rel))nrcompat::block(nrcompat::Origem::Binding,diagnostic,"coordinator sticky (so FSR ate reabrir o jogo)");
}
namespace {
struct Relay {
  GpuBackend* gpu{};NrFfxDispatch original{};void* original_context{};void* original_descriptor{};
  void* command{};std::uint32_t stage{};GpuStatus status{GpuStatus::Ready};std::int32_t original_result{6};bool original_called{};
};
std::int32_t relay(void* opaque,void*){
  auto& state=*static_cast<Relay*>(opaque);
  if(state.stage==1){
    state.status=state.gpu->reconstruct(state.command,state.stage);
    if(state.status!=GpuStatus::Ready)return 6;
  }
  state.original_called=true;state.original_result=state.original(state.original_context,state.original_descriptor);
  if(state.stage==2&&state.original_result==0){
    state.status=state.gpu->downsample(state.command,state.stage);
    if(state.status!=GpuStatus::Ready)return 6;
  }
  return state.original_result;
}
bool before_record(GpuStatus status){return status==GpuStatus::Unsupported||status==GpuStatus::FailedBeforeRecord;}
void poison(NrActiveDispatchResultV1* result){if(result){result->poisoned=1;result->outcome=NR_ACTIVE_REJECTED_STICKY;}}
}
std::int32_t Coordinator::dispatch(NrFfxDispatch original,void* context,void* descriptor,std::int32_t nesting,
    const NrActiveSelectionV1& selection,std::uint32_t* handled,NrActiveDispatchResultV1* result){
  if(handled)*handled=1;if(!original)return 6;
  if(sticky_.load(std::memory_order_acquire)||nesting!=0)return original(context,descriptor);
  if(adapter_&&selection.scale_percent==100&&active_percent_==100){
    std::int32_t code=6;
    if(adapted_dispatch(original,context,descriptor,nesting,selection,handled,result,code)==1){
      if(decode_reported_){decode_reported_=nullptr;nrcompat::unblock(nrcompat::Origem::Descritor);}
      return code;
    }
  }
  nrffx::Descriptor decoded{};if(!nrffx::decode(descriptor,decoded)){diagnostic_.store("scale_descriptor_unsupported",std::memory_order_release);refuse_decode(descriptor);

    if(nrrj::ativo()&&nrffx::host_reset(descriptor))nrrj::cortar();
    return original(context,descriptor);}

  if(decode_reported_){decode_reported_=nullptr;nrcompat::unblock(nrcompat::Origem::Descritor);}
  const auto requested=target({decoded.render_w,decoded.render_h,decoded.output_w,decoded.output_h},selection.scale_percent);
  if(!requested){diagnostic_.store("scale_target_unsupported",std::memory_order_release);return original(context,descriptor);}
  {std::lock_guard<std::mutex> lock(snapshot_mutex_);snapshot_.requested_percent=selection.scale_percent;snapshot_.stage=selection.stage;snapshot_.original=requested->original;snapshot_.scaled=requested->scaled;snapshot_.context=context;}
  if(selection.scale_percent==100&&active_percent_==100){diagnostic_.store("scale_100_bypass",std::memory_order_release);return neural_.dispatch(original,context,descriptor,nesting,selection,handled,result);}
  if(selection.scale_percent==100){
    Resources originals{decoded.input,decoded.output,decoded.motion,decoded.depth,requested->scaled};
    if(neural_.prepare(*requested,originals,context,selection.stage,selection.generation)!=PrepareStatus::Ready){diagnostic_.store("scale_waiting_neural_retirement",std::memory_order_release);return original(context,descriptor);}
    const auto released=gpu_.deactivate();if(released!=GpuStatus::Ready){if(before_record(released)){diagnostic_.store("scale_deactivate_before_record",std::memory_order_release);return original(context,descriptor);}diagnostic_.store("scale_deactivate_after_record",std::memory_order_release);make_sticky(diagnostic_.load(std::memory_order_acquire));poison(result);return 6;}
    active_percent_=100;{std::lock_guard<std::mutex> lock(snapshot_mutex_);snapshot_.effective_percent=100;}diagnostic_.store("scale_100_active",std::memory_order_release);return neural_.dispatch(original,context,descriptor,nesting,selection,handled,result);
  }
  Resources resources{};const auto prepared=gpu_.prepare(decoded,*requested,resources);
  if(prepared!=GpuStatus::Ready){if(before_record(prepared)){diagnostic_.store("scale_prepare_before_record",std::memory_order_release);return original(context,descriptor);}diagnostic_.store("scale_prepare_after_record",std::memory_order_release);make_sticky(diagnostic_.load(std::memory_order_acquire));poison(result);return 6;}
  const auto neural_ready=neural_.prepare(*requested,resources,context,selection.stage,selection.generation);
  if(neural_ready!=PrepareStatus::Ready){diagnostic_.store(neural_ready==PrepareStatus::Pending?"scale_waiting_neural_resources":"scale_neural_prepare_failed",std::memory_order_release);return original(context,descriptor);}
  const auto activated=gpu_.activate();
  if(activated!=GpuStatus::Ready){if(before_record(activated)){diagnostic_.store("scale_activate_before_record",std::memory_order_release);return original(context,descriptor);}diagnostic_.store("scale_activate_after_record",std::memory_order_release);make_sticky(diagnostic_.load(std::memory_order_acquire));poison(result);return 6;}
  active_percent_=selection.scale_percent;
  nrffx::ScaledFields fields{};fields.input=resources.input;fields.output=resources.output;fields.motion=resources.motion;fields.depth=resources.depth;
  fields.render_w=resources.geometry.render_w;fields.render_h=resources.geometry.render_h;fields.output_w=resources.geometry.output_w;fields.output_h=resources.geometry.output_h;
  fields.motion_x=static_cast<float>(fields.render_w);fields.motion_y=static_cast<float>(fields.render_h);
  std::array<std::byte,nrffx::descriptor_size> scaled{};
  if(!nrffx::encode_scaled_copy(descriptor,nrffx::descriptor_size,fields,scaled.data(),scaled.size())){diagnostic_.store("scale_descriptor_encode_failed",std::memory_order_release);return original(context,descriptor);}
  if(selection.stage==1){
    const auto down=gpu_.downsample(decoded.command,selection.stage);
    if(down!=GpuStatus::Ready){if(before_record(down)){diagnostic_.store("scale_downsample_before_record",std::memory_order_release);return original(context,descriptor);}diagnostic_.store("scale_downsample_after_record",std::memory_order_release);make_sticky(diagnostic_.load(std::memory_order_acquire));poison(result);return 6;}
  }
  Relay state{&gpu_,original,context,descriptor,decoded.command,selection.stage};
  NrActiveDispatchResultV1 local{};auto* outcome=result?result:&local;
  const auto code=neural_.dispatch(relay,&state,scaled.data(),nesting,selection,handled,outcome);
  if(state.status!=GpuStatus::Ready){
    if(!before_record(state.status)){diagnostic_.store(selection.stage==1?"scale_reconstruct_after_record":"scale_downsample_after_record",std::memory_order_release);make_sticky(diagnostic_.load(std::memory_order_acquire));poison(outcome);}
    else diagnostic_.store(selection.stage==1?"scale_reconstruct_before_record":"scale_downsample_before_record",std::memory_order_release);
    if(selection.stage==2&&state.original_called)return state.original_result;
    return 6;
  }
  if(selection.stage==2&&outcome->capture_attempted){
    const auto up=gpu_.reconstruct(decoded.command,selection.stage);
    if(up!=GpuStatus::Ready){if(!before_record(up)){diagnostic_.store("scale_reconstruct_after_record",std::memory_order_release);make_sticky(diagnostic_.load(std::memory_order_acquire));poison(outcome);}else diagnostic_.store("scale_reconstruct_before_record",std::memory_order_release);return state.original_called?state.original_result:6;}
  }
  {std::lock_guard<std::mutex> lock(snapshot_mutex_);snapshot_.effective_percent=selection.scale_percent;}
  diagnostic_.store("scale_active_dispatch",std::memory_order_release);
  return selection.stage==2&&state.original_called?state.original_result:code;
}

void Coordinator::report_adapter(const AdapterPlan& plan,const nrffx::RawDescriptor& d,const NrResourceDesc& input,const NrResourceDesc& motion) noexcept{
  if(!plan.adapt&&!*plan.reason)return;
  std::uint32_t pre{},mx{},my{};std::memcpy(&pre,&d.pre_exposure,4);std::memcpy(&mx,&d.motion_x,4);std::memcpy(&my,&d.motion_y,4);
  const std::uint64_t key=(std::uint64_t(input.format)<<56)^(std::uint64_t(motion.format)<<48)^(std::uint64_t(pre)<<16)^(std::uint64_t(mx)*31u)^my^
    (plan.adapt?1u:0u)^(std::uint64_t(input.flags)<<40)^reinterpret_cast<std::uintptr_t>(plan.reason);
  if(key==adapter_reported_)return;
  adapter_reported_=key;char detalhe[320];
  std::snprintf(detalhe,sizeof detalhe,"cor fmt %u flags %u -> %s; movimento fmt %u escala %.6g,%.6g render %ux%u -> %s; profundidade -> %s; preExposure %.6g -> saida x%.6g",
    input.format,input.flags,plan.color?"RGBA16F propria":"do jogo",motion.format,double(d.motion_x),double(d.motion_y),d.render_w,d.render_h,
    plan.motion?"RG16F propria":"do jogo",plan.depth?"R32F propria":"do jogo",double(d.pre_exposure),double(plan.output_scale));
  nrcompat::report(plan.adapt?"input_adapter_active":plan.reason,detalhe);
}
int Coordinator::adapted_dispatch(NrFfxDispatch original,void* context,void* descriptor,std::int32_t nesting,
    const NrActiveSelectionV1& selection,std::uint32_t* handled,NrActiveDispatchResultV1* result,std::int32_t& code){
  nrffx::RawDescriptor raw{};if(!nrffx::decode_raw(descriptor,raw)||raw.reset)return 0;
  NrResourceDesc input{},output{},motion{},depth{};
  if(!adapter_->describe(raw.input,input)||!adapter_->describe(raw.output,output)||!adapter_->describe(raw.motion,motion)||!adapter_->describe(raw.depth,depth)){
    adapter_reason_.store("adapt_describe_failed",std::memory_order_release);return 0;}

  const std::uint64_t geometry=(std::uint64_t(raw.render_w)<<48)^(std::uint64_t(raw.render_h)<<32)^(std::uint64_t(raw.output_w)<<16)^raw.output_h;
  if(selection.generation!=adapter_generation_||geometry!=adapter_geometry_){adapter_generation_=selection.generation;adapter_geometry_=geometry;adapter_sticky_=0;}
  const auto plan=plan_adapter(raw,input,output,motion,depth,adapter_sticky_);
  adapter_reason_.store(plan.reason,std::memory_order_release);
  report_adapter(plan,raw,input,motion);
  if(!plan.adapt)return 0;
  adapter_sticky_|=plan.roles();
  AdaptedInputs replaced{};const auto converted=adapter_->convert(raw,plan,replaced);
  if(converted!=GpuStatus::Ready){
    if(before_record(converted)){adapter_reason_.store("adapt_convert_before_record",std::memory_order_release);return 2;}

    diagnostic_.store("scale_adapt_after_record",std::memory_order_release);make_sticky("scale_adapt_after_record");poison(result);
    code=original(context,descriptor);return 1;
  }
  nrffx::AdaptedFields fields{};
  if(plan.color){fields.input=replaced.color;fields.input_state=replaced.color_state;}
  if(plan.motion){fields.motion=replaced.motion;fields.motion_state=replaced.motion_state;}
  if(plan.depth){fields.depth=replaced.depth;fields.depth_state=replaced.depth_state;}
  std::array<std::byte,nrffx::descriptor_size> adapted{};
  if(!nrffx::encode_adapted_copy(descriptor,nrffx::descriptor_size,fields,adapted.data(),adapted.size())){

    adapter_reason_.store("adapt_descriptor_encode_failed",std::memory_order_release);code=original(context,descriptor);return 1;
  }
  diagnostic_.store("scale_100_adapted",std::memory_order_release);
  code=neural_.dispatch(original,context,adapted.data(),nesting,selection,handled,result);
  if(plan.output_scale!=1.f){
    const auto finished=adapter_->finish(raw,plan);
    if(finished!=GpuStatus::Ready&&!before_record(finished)){
      diagnostic_.store("scale_adapt_finish_after_record",std::memory_order_release);make_sticky("scale_adapt_finish_after_record");poison(result);
    }else if(finished!=GpuStatus::Ready)adapter_reason_.store("adapt_finish_before_record",std::memory_order_release);
  }
  return 1;
}
}
