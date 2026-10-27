#pragma once
#include <string>

namespace xf {

void log_iniciar(void* modulo);
void logf(const char* fmt, ...);
std::wstring pasta_da_dll();

struct Ini {
  float perto = 0.1f, longe = 100000.0f, fov_graus = 60.0f;
  float jitter_sinal_x = 1.0f, jitter_sinal_y = 1.0f;
  int auto_exposicao = -1;
  int profundidade_infinita = 0;
  int usar_textura_exposicao = 1;
  int usar_mascara_responsiva = 1;
  int recriar_ao_ganchar = 1;
  int descritor_versao = 0;
  int log_execucoes = 20;
  float nitidez = 0.0f;

  bool filtros_nitidez_helper = false;
  int nitidez_com_filtros = 0;
  int versao[3] = {1, 3, 1};
  std::wstring ffx_dll;

  int gravar_estencil = 0;
  int gravar_estencil_tecla = 0x78;
  int gravar_estencil_execucao = 0;
  int gravar_estencil_quadros = 4;
  int gravar_estencil_intervalo = 30;
  int gravar_estencil_cor = 1;
  int gravar_estencil_estado = 0x40;

  int publicar_estencil = 0;
  std::string provedor;

  int geracao_quadros = -1;
  std::string fg_provedor;

  int fg_pula_repetido = 0;
  int fg_assincrono = 0;
  int fg_debug = 0;
  int fg_log_presents = 16;
  int fg_prepare_antes = 1;
  int fg_direto = 1;

  int fg_ordem = 1;
  int fg_ordem_ms = 20;
  int fg_teste_local = 1;
};
const Ini& ini();

}
