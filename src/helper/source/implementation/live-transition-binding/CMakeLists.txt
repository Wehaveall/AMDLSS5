cmake_minimum_required(VERSION 3.20)
project(nr_live_binding LANGUAGES CXX)
if(NOT TARGET nr_compat)
  add_subdirectory(../compat-diag compat-diag)
endif()
set(NR_REPROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../async-reprojection")
if(NOT NR_REPROJECT_DXC)
  set(NR_REPROJECT_DXC "C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/dxc.exe")
endif()
set(NR_REPROJECT_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated")

file(GLOB NR_REPROJECT_HLSLI CONFIGURE_DEPENDS "${NR_REPROJECT_DIR}/*.hlsli")
file(MAKE_DIRECTORY "${NR_REPROJECT_GENERATED}")
foreach(entry Capture Warp Reduzir Compose ComposePost Estabilizar EstabilizarCR Fill FillRapido LuzMomentos LuzCoeficientes LuzTemporal LuzProx LuzHalo)
  string(TOLOWER "${entry}" lower)

  set(extra)
  set(extra_dep)
  if(entry STREQUAL "Compose")
    set(shader "${CMAKE_CURRENT_SOURCE_DIR}/../../../../nr-controls-complete/shader/compose.hlsl")
    set(function main)
    set(extra -D NR_GANHO_HISTORICO=1)
  elseif(entry MATCHES "^Luz")

    set(shader "${CMAKE_CURRENT_SOURCE_DIR}/../../../../nr-controls-complete/shader/compose.hlsl")
    set(function ${entry})
    set(extra -D NR_GANHO_HISTORICO=1 -D NR_LUZ=1)
  elseif(entry STREQUAL "ComposePost")

    set(shader "${NR_REPROJECT_DIR}/compose_post.hlsl")

    set(extra_dep "${CMAKE_CURRENT_SOURCE_DIR}/../../../../nr-controls-complete/shader/compose.hlsl")
    set(function main)
  elseif(entry STREQUAL "EstabilizarCR")

    set(shader "${NR_REPROJECT_DIR}/estabilizar.hlsl")
    set(function Estabilizar)
    set(extra -D NR_EST_CR=1)
  elseif(entry STREQUAL "Estabilizar")

    set(shader "${NR_REPROJECT_DIR}/estabilizar.hlsl")
    set(function Estabilizar)
  elseif(entry STREQUAL "Fill" OR entry STREQUAL "FillRapido")

    set(shader "${NR_REPROJECT_DIR}/fill.hlsl")
    set(function ${entry})
  else()
    set(shader "${NR_REPROJECT_DIR}/reproject.hlsl")
    set(extra_dep "${NR_REPROJECT_DIR}/tom_comum.hlsli")
    set(function ${entry})
  endif()
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${function} -T cs_6_0 -O3 -WX -Ges -D NR_GRADE_CG2R=1 ${extra}
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${shader}"
    DEPENDS "${shader}" ${extra_dep} ${NR_REPROJECT_HLSLI} VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()

foreach(entry ComposePostNpc NpcMede)
  string(TOLOWER "${entry}" lower)
  if(entry STREQUAL "ComposePostNpc")
    set(function main)
  else()
    set(function ${entry})
  endif()
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${function} -T cs_6_0 -O3 -WX -Ges -D NR_GRADE_CG2R=1 -D NR_POST_NPC=1
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${NR_REPROJECT_DIR}/compose_post.hlsl"
    DEPENDS "${NR_REPROJECT_DIR}/compose_post.hlsl" "${CMAKE_CURRENT_SOURCE_DIR}/../../../../nr-controls-complete/shader/compose.hlsl" ${NR_REPROJECT_HLSLI} VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()

foreach(par "MascaraCelulas:1" "MascaraDistancia:2" "MascaraPreenche:3" "MascaraAplica:4" "MascaraDistZ:6" "MascaraCurva:7" "MascaraCurvaFim:8" "MascaraLimita:9" "MascaraCurvaSoma:10")
  string(REPLACE ":" ";" partes "${par}")
  list(GET partes 0 entry)
  list(GET partes 1 passo)
  string(TOLOWER "${entry}" lower)
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${entry} -T cs_6_0 -O3 -WX -Ges -D NR_MASC_PASSO=${passo}
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${NR_REPROJECT_DIR}/mascara.hlsl"
    DEPENDS "${NR_REPROJECT_DIR}/mascara.hlsl" ${NR_REPROJECT_HLSLI} VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()
add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_composepostmascara.h"
  COMMAND "${NR_REPROJECT_DXC}" -E main -T cs_6_0 -O3 -WX -Ges -D NR_GRADE_CG2R=1 -D NR_POST_MASCARA=1
    -Vn "g_nr_reproject_composepostmascara" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_composepostmascara.h"
    -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_composepostmascara.cso" "${NR_REPROJECT_DIR}/compose_post.hlsl"
  DEPENDS "${NR_REPROJECT_DIR}/compose_post.hlsl" "${CMAKE_CURRENT_SOURCE_DIR}/../../../../nr-controls-complete/shader/compose.hlsl" ${NR_REPROJECT_HLSLI} VERBATIM)
list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_composepostmascara.h")

add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_bordaspost.h"
  COMMAND "${NR_REPROJECT_DXC}" -E BordasPost -T cs_6_0 -O3 -WX -Ges -D NR_GRADE_CG2R=1 -D NR_POST_MASCARA=1
    -Vn "g_nr_reproject_bordaspost" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_bordaspost.h"
    -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_bordaspost.cso" "${NR_REPROJECT_DIR}/compose_post.hlsl"
  DEPENDS "${NR_REPROJECT_DIR}/compose_post.hlsl" "${CMAKE_CURRENT_SOURCE_DIR}/../../../../nr-controls-complete/shader/compose.hlsl" ${NR_REPROJECT_HLSLI} VERBATIM)
list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_bordaspost.h")

foreach(entry RjRecorte RjCompor RjComporR RjRostoImg RjCamada)
  string(TOLOWER "${entry}" lower)
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${entry} -T cs_6_0 -O3 -WX -Ges
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${NR_REPROJECT_DIR}/rosto_jogador.hlsl"
    DEPENDS "${NR_REPROJECT_DIR}/rosto_jogador.hlsl" "${NR_REPROJECT_DIR}/tom_comum.hlsli" ${NR_REPROJECT_HLSLI} VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()

foreach(entry RjRuido RjRecorteRuido)
  string(TOLOWER "${entry}" lower)
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${entry} -T cs_6_0 -O3 -WX -Ges
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${NR_REPROJECT_DIR}/rosto_ruido.hlsl"
    DEPENDS "${NR_REPROJECT_DIR}/rosto_ruido.hlsl" "${NR_REPROJECT_DIR}/rosto_jogador.hlsl" "${NR_REPROJECT_DIR}/tom_comum.hlsli" ${NR_REPROJECT_HLSLI} VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()

foreach(entry RjCaminho)
  string(TOLOWER "${entry}" lower)
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${entry} -T cs_6_0 -O3 -WX -Ges
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${NR_REPROJECT_DIR}/rosto_caminho.hlsl"
    DEPENDS "${NR_REPROJECT_DIR}/rosto_caminho.hlsl" ${NR_REPROJECT_HLSLI} VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()

foreach(entry RjGrade)
  string(TOLOWER "${entry}" lower)
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${entry} -T cs_6_0 -O3 -WX -Ges
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${NR_REPROJECT_DIR}/rosto_grade.hlsl"
    DEPENDS "${NR_REPROJECT_DIR}/rosto_grade.hlsl" ${NR_REPROJECT_HLSLI} VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()

foreach(entry RjCabeca)
  string(TOLOWER "${entry}" lower)
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${entry} -T cs_6_0 -O3 -WX -Ges
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${NR_REPROJECT_DIR}/rosto_cabeca.hlsl"
    DEPENDS "${NR_REPROJECT_DIR}/rosto_cabeca.hlsl" ${NR_REPROJECT_HLSLI} VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()

foreach(entry Croma CromaLds)
  string(TOLOWER "${entry}" lower)
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${entry} -T cs_6_0 -O3 -WX -Ges
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${NR_REPROJECT_DIR}/croma.hlsl"
    DEPENDS "${NR_REPROJECT_DIR}/croma.hlsl" "${NR_REPROJECT_DIR}/tom_comum.hlsli" ${NR_REPROJECT_HLSLI} VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()

foreach(entry LuzGrossa)
  string(TOLOWER "${entry}" lower)
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${entry} -T cs_6_0 -O3 -WX -Ges
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${NR_REPROJECT_DIR}/luzgrossa.hlsl"
    DEPENDS "${NR_REPROJECT_DIR}/luzgrossa.hlsl" "${NR_REPROJECT_DIR}/tom_comum.hlsli" ${NR_REPROJECT_HLSLI} VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()

foreach(par "BaseTom:BaseTom:base_tom" "CromaBase:Croma:croma" "CromaLdsBase:CromaLds:croma" "LuzGrossaBase:LuzGrossa:luzgrossa")
  string(REPLACE ":" ";" partes "${par}")
  list(GET partes 0 entry)
  list(GET partes 1 function)
  list(GET partes 2 arquivo)
  string(TOLOWER "${entry}" lower)
  set(extra)
  if(NOT entry STREQUAL "BaseTom")
    set(extra -D NR_BASE_PRONTA=1)
  endif()
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${function} -T cs_6_0 -O3 -WX -Ges ${extra}
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${NR_REPROJECT_DIR}/${arquivo}.hlsl"
    DEPENDS "${NR_REPROJECT_DIR}/${arquivo}.hlsl" "${NR_REPROJECT_DIR}/tom_comum.hlsli" ${NR_REPROJECT_HLSLI} VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()

foreach(entry TLMedeRef)
  string(TOLOWER "${entry}" lower)
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${entry} -T cs_6_0 -O3 -WX -Ges
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${NR_REPROJECT_DIR}/travaluz.hlsl"
    DEPENDS "${NR_REPROJECT_DIR}/travaluz.hlsl" ${NR_REPROJECT_HLSLI} VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()

foreach(entry Suave)
  string(TOLOWER "${entry}" lower)
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${entry} -T cs_6_0 -O3 -WX -Ges
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${NR_REPROJECT_DIR}/suave.hlsl"
    DEPENDS "${NR_REPROJECT_DIR}/suave.hlsl" "${NR_REPROJECT_DIR}/tom_comum.hlsli" ${NR_REPROJECT_HLSLI} VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()
add_library(nr_reproject_backend STATIC "${NR_REPROJECT_DIR}/backend.cpp" ${NR_REPROJECT_HEADERS})

set_source_files_properties("${NR_REPROJECT_DIR}/backend.cpp" PROPERTIES OBJECT_DEPENDS "${NR_REPROJECT_DIR}/rosto_jogador_d3d.inl;${CMAKE_CURRENT_SOURCE_DIR}/../../../runtime/rosto_jogador_state.h;${NR_REPROJECT_DIR}/cena_npc.inl;${CMAKE_CURRENT_SOURCE_DIR}/../../../runtime/cena_npc.h")
target_include_directories(nr_reproject_backend PRIVATE "${NR_REPROJECT_GENERATED}")
target_compile_features(nr_reproject_backend PUBLIC cxx_std_17)
target_compile_definitions(nr_reproject_backend PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(nr_reproject_backend PRIVATE /W4 /WX /permissive- /EHsc)
target_link_libraries(nr_reproject_backend PUBLIC d3d12 dxgi nr_compat)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/backend-rapido")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/cadeia")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/troca-modo")

foreach(modo 0 1 2 3 4)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/final-reserva-${modo}")

endforeach()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/recusa")

get_filename_component(NR_SDK_BIN "${NR_REPROJECT_DXC}" DIRECTORY)
set(NR_FXC "${NR_SDK_BIN}/fxc.exe")
set(NR_FILTROS_CS_DIR "${CMAKE_CURRENT_BINARY_DIR}/filtros-cs")
set(NR_FILTROS_CS_DIR "${NR_FILTROS_CS_DIR}" PARENT_SCOPE)
set(NR_RUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../runtime")
if(EXISTS "${NR_FXC}")
  file(MAKE_DIRECTORY "${NR_FILTROS_CS_DIR}")
  set(NR_FILTROS_CS_HEADERS)
  foreach(grupo "filtros:${NR_RUNTIME_DIR}/filtros_gpu.h:main,reduz,borra_h,borra_v,main_m,reduz_m,borra_h_m,borra_v_m,ceu" "fg_nitidez:${CMAKE_CURRENT_SOURCE_DIR}/../live-transition-helper/fg_nitidez.h:main,energia" "ao:${NR_RUNTIME_DIR}/ao_gpu.h:ao_prof,ao_principal,ao_denoise,ao_temporal,ao_aplica" "il:${NR_RUNTIME_DIR}/il_gpu.h:il_rad,il_principal,il_temporal,il_atrous")
    string(REGEX MATCH "^([a-z_]+):(.*):([a-z_,]+)$" _ "${grupo}")
    set(nome "${CMAKE_MATCH_1}")
    set(fonte "${CMAKE_MATCH_2}")
    string(REPLACE "," ";" entradas "${CMAKE_MATCH_3}")
    add_custom_command(OUTPUT "${NR_FILTROS_CS_DIR}/nr_${nome}.hlsl"
      COMMAND "${CMAKE_COMMAND}" "-DENTRADA=${fonte}" "-DSAIDA=${NR_FILTROS_CS_DIR}/nr_${nome}.hlsl" -P "${NR_RUNTIME_DIR}/extrair_hlsl.cmake"
      DEPENDS "${fonte}" "${NR_RUNTIME_DIR}/extrair_hlsl.cmake" VERBATIM)
    foreach(entrada IN LISTS entradas)
      add_custom_command(OUTPUT "${NR_FILTROS_CS_DIR}/nr_${nome}_${entrada}.h"
        COMMAND "${NR_FXC}" /nologo /T cs_5_0 /E ${entrada} /O3 /Vn "g_nr_${nome}_${entrada}" /Fh "${NR_FILTROS_CS_DIR}/nr_${nome}_${entrada}.h" "${NR_FILTROS_CS_DIR}/nr_${nome}.hlsl"
        DEPENDS "${NR_FILTROS_CS_DIR}/nr_${nome}.hlsl" VERBATIM)
      list(APPEND NR_FILTROS_CS_HEADERS "${NR_FILTROS_CS_DIR}/nr_${nome}_${entrada}.h")
    endforeach()
  endforeach()
  add_custom_target(nr_filtros_cs DEPENDS ${NR_FILTROS_CS_HEADERS})
  set(NR_FILTROS_CS ON PARENT_SCOPE)
  set(NR_FILTROS_CS ON)
else()
  message(STATUS "fxc ausente (${NR_FXC}): filtros e nitidez do FG compilam o HLSL em tempo de execucao")
  set(NR_FILTROS_CS OFF PARENT_SCOPE)
  set(NR_FILTROS_CS OFF)
endif()
if(NOT TARGET nr_ffx_descriptor)
  add_library(nr_ffx_descriptor STATIC ../active-scheduling/ffx_descriptor.cpp)
  target_compile_features(nr_ffx_descriptor PUBLIC cxx_std_17)
  target_include_directories(nr_ffx_descriptor PUBLIC ../active-scheduling)
endif()
add_library(nr_live_binding live_binding.cpp windows_host.cpp native_ini_hook.cpp hip_memory_observer.cpp wait_trace.cpp job_timeline.cpp captura_carimbo.cpp
  ../active-scheduling/active_scheduling.cpp ../active-binding/memory_contract.cpp
  ../live-transition-controller/controller.cpp
  ../live-transition-fence-proof/fence_proof.cpp ../live-transition-fence-proof/d3d12_backend.cpp
  ../live-transition-thread-proof/thread_proof.cpp ../live-transition-hip-proof/hip_proof.cpp)
if(NOT TARGET nr_live_image)
  add_subdirectory(../live-transition-image live-image)
endif()
if(NOT TARGET nr_recording_observer)
  add_subdirectory(../live-recording-observer recording-observer)
endif()
target_link_libraries(nr_live_binding PUBLIC nr_live_image nr_recording_observer nr_ffx_descriptor nr_reproject_backend nr_compat OneCore)
if(NR_FILTROS_CS)
  add_dependencies(nr_live_binding nr_filtros_cs)
  target_include_directories(nr_live_binding PRIVATE "${NR_FILTROS_CS_DIR}")
  target_compile_definitions(nr_live_binding PRIVATE NR_FILTROS_CS=1)
endif()
target_compile_features(nr_live_binding PUBLIC cxx_std_17)
target_include_directories(nr_live_binding PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(nr_live_binding PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0A00)
target_compile_options(nr_live_binding PRIVATE /W4 /WX /permissive- /EHsc)
