#pragma once
#include <cstddef>

namespace nr1d43contract {

inline constexpr size_t module_bytes = 2719832;
inline constexpr unsigned char sha256[32] = {0x29, 0x3d, 0xcb, 0xa5, 0x52, 0x10, 0x90, 0x9a, 0xa9, 0x73, 0x47, 0xd5, 0x59, 0x1a, 0xb9, 0x12, 0xc0, 0x23, 0x95, 0x34, 0x62, 0xf9, 0x29, 0x24, 0x0a, 0x81, 0x0e, 0x95, 0x21, 0xfc, 0xd4, 0xe7};
inline constexpr unsigned symbol_count = 14;
inline constexpr const char* symbols[symbol_count] = {
  "_Z8k_repack12RepackParams"  ,
  "_Z9k_expand2ILi1ELb1EEv12ExpandParams"  ,
  "_Z12k_reg1d_convILi1ELb1ELb1EEv12ConvParams1d"  ,
  "_Z12k_reg1d_convILi1ELb0ELb1EEv12ConvParams1d"  ,
  "_Z12k_reg1d_convILi2ELb0ELb1EEv12ConvParams1d"  ,
  "_Z11k_reg1d_qkvILb0ELb1EEv14Reg1dQkvParams"  ,
  "_Z12k_reg1d_attnILi1ELb1EEv15Reg1dAttnParams"  ,
  "_Z12k_reg1d_convILi1ELb1ELb0EEv12ConvParams1d"  ,
  "_Z12k_reg1d_convILi1ELb0ELb0EEv12ConvParams1d"  ,
  "_Z12k_reg1d_convILi2ELb0ELb0EEv12ConvParams1d"  ,
  "_Z11k_reg1d_qkvILb0ELb0EEv14Reg1dQkvParams"  ,
  "_Z12k_reg1d_attnILi1ELb0EEv15Reg1dAttnParams"  ,
  "_Z11k_reg1d_qkvILb1ELb1EEv14Reg1dQkvParams"  ,
  "_Z11k_reg1d_qkvILb1ELb0EEv14Reg1dQkvParams"
};
inline constexpr unsigned struct_bytes[symbol_count] = {40, 32, 80, 80, 80, 72, 48, 80, 80, 80, 72, 48, 72, 72};
}
