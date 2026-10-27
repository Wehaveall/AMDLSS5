#pragma once
#include <string>

namespace f2f {

void log_iniciar(void* modulo);
void logf(const char* fmt, ...);
std::wstring pasta_da_dll();

struct Ini {
  std::string provedor;
  int recriar_ao_ganchar = 1;
  int log_despachos = 3;

  int geracao_quadros = 0;
  int fg_ordem_ms = 20;
};
const Ini& ini();

}
