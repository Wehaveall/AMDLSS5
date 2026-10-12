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
file(MAKE_DIRECTORY "${NR_REPROJECT_GENERATED}")
foreach(entry Capture Warp Reduzir Compose ComposePost ComposePostCR ComposePostG ComposePostCRG Estabilizar EstabilizarCR Fill BilateralLumaFonte BilateralLumaAtual BilateralPrep BilateralFit BilateralApply LuzMomentos LuzCoeficientes LuzTemporal LuzProx LuzHalo)
  string(TOLOWER "${entry}" lower)

  set(extra)
  set(extra_dep)
  if(entry MATCHES "^Bilateral")

    set(shader "${NR_REPROJECT_DIR}/bilateral.hlsl")
    set(function ${entry})
  elseif(entry STREQUAL "Compose")
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
  elseif(entry STREQUAL "ComposePostCR")

    set(shader "${NR_REPROJECT_DIR}/compose_post.hlsl")
    set(extra_dep "${CMAKE_CURRENT_SOURCE_DIR}/../../../../nr-controls-complete/shader/compose.hlsl")
    set(function main)
    set(extra -D NR_POST_CR=1)
  elseif(entry STREQUAL "ComposePostG" OR entry STREQUAL "ComposePostCRG")

    set(shader "${NR_REPROJECT_DIR}/compose_post.hlsl")
    set(extra_dep "${CMAKE_CURRENT_SOURCE_DIR}/../../../../nr-controls-complete/shader/compose.hlsl")
    set(function main)
    if(entry STREQUAL "ComposePostCRG")
      set(extra -D NR_POST_GANHO=1 -D NR_POST_CR=1)
    else()
      set(extra -D NR_POST_GANHO=1)
    endif()
  elseif(entry STREQUAL "EstabilizarCR")

    set(shader "${NR_REPROJECT_DIR}/estabilizar.hlsl")
    set(function Estabilizar)
    set(extra -D NR_EST_CR=1)
  elseif(entry STREQUAL "Estabilizar")

    set(shader "${NR_REPROJECT_DIR}/estabilizar.hlsl")
    set(function Estabilizar)
  elseif(entry STREQUAL "Fill")

    set(shader "${NR_REPROJECT_DIR}/fill.hlsl")
    set(function ${entry})
  else()
    set(shader "${NR_REPROJECT_DIR}/reproject.hlsl")
    set(function ${entry})
  endif()
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${function} -T cs_6_0 -O3 -WX -Ges -D NR_GRADE_CG2R=1 ${extra}
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${shader}"
    DEPENDS "${shader}" ${extra_dep} VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()

foreach(entry HaloPostMomentos HaloPostRef HaloPostAplicar)
  string(TOLOWER "${entry}" lower)
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${entry} -T cs_6_0 -O3 -WX -Ges
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${NR_REPROJECT_DIR}/halo_post.hlsl"
    DEPENDS "${NR_REPROJECT_DIR}/halo_post.hlsl" VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()

add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_qmapa4.h"
  COMMAND "${NR_REPROJECT_DXC}" -E QMapa -T cs_6_0 -O3 -WX -Ges -D NR_QMAPA_Q4=1
    -Vn "g_nr_reproject_qmapa4" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_qmapa4.h"
    -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_qmapa4.cso" "${NR_REPROJECT_DIR}/q_post.hlsl"
  DEPENDS "${NR_REPROJECT_DIR}/q_post.hlsl" VERBATIM)
list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_qmapa4.h")
foreach(entry QMapa)
  string(TOLOWER "${entry}" lower)
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${entry} -T cs_6_0 -O3 -WX -Ges
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${NR_REPROJECT_DIR}/q_post.hlsl"
    DEPENDS "${NR_REPROJECT_DIR}/q_post.hlsl" VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()

foreach(entry RjRecorte RjCompor RjComporR)
  string(TOLOWER "${entry}" lower)
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${entry} -T cs_6_0 -O3 -WX -Ges
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${NR_REPROJECT_DIR}/rosto_jogador.hlsl"
    DEPENDS "${NR_REPROJECT_DIR}/rosto_jogador.hlsl" VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()

foreach(entry RjGrade)
  string(TOLOWER "${entry}" lower)
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${entry} -T cs_6_0 -O3 -WX -Ges
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${NR_REPROJECT_DIR}/rosto_grade.hlsl"
    DEPENDS "${NR_REPROJECT_DIR}/rosto_grade.hlsl" VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()
foreach(entry Croma CromaLds)
  string(TOLOWER "${entry}" lower)
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${entry} -T cs_6_0 -O3 -WX -Ges
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${NR_REPROJECT_DIR}/croma.hlsl"
    DEPENDS "${NR_REPROJECT_DIR}/croma.hlsl" VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()

foreach(entry LuzGrossa)
  string(TOLOWER "${entry}" lower)
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${entry} -T cs_6_0 -O3 -WX -Ges
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${NR_REPROJECT_DIR}/luzgrossa.hlsl"
    DEPENDS "${NR_REPROJECT_DIR}/luzgrossa.hlsl" VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()

foreach(entry Suave)
  string(TOLOWER "${entry}" lower)
  add_custom_command(OUTPUT "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
    COMMAND "${NR_REPROJECT_DXC}" -E ${entry} -T cs_6_0 -O3 -WX -Ges
      -Vn "g_nr_reproject_${lower}" -Fh "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h"
      -Fo "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.cso" "${NR_REPROJECT_DIR}/suave.hlsl"
    DEPENDS "${NR_REPROJECT_DIR}/suave.hlsl" VERBATIM)
  list(APPEND NR_REPROJECT_HEADERS "${NR_REPROJECT_GENERATED}/nr_reproject_${lower}.h")
endforeach()
add_library(nr_reproject_backend STATIC "${NR_REPROJECT_DIR}/backend.cpp" ${NR_REPROJECT_HEADERS})

set_source_files_properties("${NR_REPROJECT_DIR}/backend.cpp" PROPERTIES OBJECT_DEPENDS "${NR_REPROJECT_DIR}/halo_post.inl;${NR_REPROJECT_DIR}/q_post.inl;${NR_REPROJECT_DIR}/rosto_jogador_d3d.inl;${CMAKE_CURRENT_SOURCE_DIR}/../../../runtime/rosto_jogador_state.h")

target_include_directories(nr_reproject_backend PRIVATE "${NR_REPROJECT_GENERATED}")
target_compile_features(nr_reproject_backend PUBLIC cxx_std_17)
target_compile_definitions(nr_reproject_backend PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(nr_reproject_backend PRIVATE /W4 /WX /permissive- /EHsc)
target_link_libraries(nr_reproject_backend PUBLIC d3d12 dxgi nr_compat)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/backend-rapido")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/cadeia")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/troca-modo")

foreach(modo 0 1 2 3)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/final-reserva-${modo}")

endforeach()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/recusa")

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
target_compile_features(nr_live_binding PUBLIC cxx_std_17)
target_include_directories(nr_live_binding PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(nr_live_binding PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0A00)
target_compile_options(nr_live_binding PRIVATE /W4 /WX /permissive- /EHsc)
