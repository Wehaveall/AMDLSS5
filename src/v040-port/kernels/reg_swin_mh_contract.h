#pragma once
#include <cstddef>

namespace nrmhcontract {

inline constexpr const char* source = "v041";
inline constexpr size_t module_bytes = 1657088;
inline constexpr unsigned char sha256[32] = {0x0f, 0x3b, 0xc0, 0xb8, 0xf2, 0xed, 0xa8, 0xf7, 0x5b, 0xc7, 0x93, 0x27, 0xa1, 0x07, 0x79, 0x35, 0x96, 0x24, 0x05, 0xe9, 0xf2, 0xd6, 0xe5, 0xb5, 0x7b, 0x7c, 0x05, 0x38, 0xbd, 0xea, 0xc6, 0x9c};
inline constexpr unsigned modes[8] = {0, 1, 2, 3, 4, 5, 8, 10};
inline constexpr const char* symbols[3][8] = {
 {
  "_Z13k_reg_swin_mhILi64ELi0EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi2EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi3EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi4EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi5EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi8EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi10EEv9VarParams"
 },
 {
  "_Z13k_reg_swin_mhILi128ELi0EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi2EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi3EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi4EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi5EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi8EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi10EEv9VarParams"
 },
 {
  "_Z13k_reg_swin_mhILi256ELi0EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi2EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi3EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi4EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi5EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi8EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi10EEv9VarParams"
 }
};
inline constexpr size_t layout_bytes = 7480;
inline constexpr unsigned char layout_sha256[32] = {0xac, 0x55, 0xd2, 0x10, 0xc1, 0x6c, 0xf3, 0xec, 0xa8, 0x47, 0x8c, 0xa2, 0x1d, 0x0e, 0x48, 0x6f, 0x11, 0xd4, 0xbf, 0x46, 0x29, 0x51, 0xe3, 0xfb, 0xb0, 0x3b, 0x64, 0x48, 0x86, 0x61, 0x41, 0xd8};
inline constexpr const char* to_mh = "nr_mh_to_reg";
inline constexpr const char* to_legacy = "nr_mh_to_legacy";

inline constexpr size_t v040_module_bytes = 1766128;
inline constexpr unsigned char v040_sha256[32] = {0x23, 0xad, 0x5d, 0x78, 0x14, 0x4a, 0x09, 0x21, 0x2c, 0xa4, 0xee, 0x08, 0x91, 0x86, 0x0b, 0x63, 0x56, 0xfd, 0x36, 0x3b, 0xb7, 0x95, 0xf1, 0x17, 0x13, 0x62, 0xe5, 0xbe, 0xf1, 0xd8, 0x3d, 0xb4};
inline constexpr const char* conv1d = "_Z12k_reg1d_convILi1ELb1EEv12ConvParams1d";
inline constexpr const char* reduce1d = "_Z14k_reg1d_reduce12ConvParams1d";

inline constexpr const char* vit[7] = {
  "_Z14k_reg_vit_ffwdILi1ELb0EEv13VitFfwdParams",
  "_Z14k_reg_vit_ffwdILi1ELb1EEv13VitFfwdParams",
  "_Z14k_reg_vit_convILi1ELi0EEv13VitConvParams",
  "_Z14k_reg_vit_convILi1ELi1EEv13VitConvParams",
  "_Z14k_reg_vit_convILi1ELi2EEv13VitConvParams",
  "_Z14k_reg_vit_convILi1ELi4EEv13VitConvParams",
  "_Z14k_reg_vit_attn10AttnParams"
};
inline constexpr size_t vit_layout_bytes = 6712;
inline constexpr unsigned char vit_layout_sha256[32] = {0x61, 0xd7, 0xb2, 0xc8, 0xb8, 0x32, 0x5b, 0xe7, 0x35, 0xbb, 0x75, 0x52, 0x3f, 0xc2, 0x03, 0xc4, 0x40, 0x89, 0x99, 0x08, 0x69, 0xe2, 0xed, 0xc5, 0x1a, 0x7b, 0xa4, 0xd1, 0xf3, 0x56, 0x42, 0x9a};

inline constexpr const char* swin32[8] = {
  "_Z12k_reg_swin32ILi0EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi2EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi4EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi5EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi8EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi20EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi32EEv9VarParams"
};
inline constexpr const char* vit_to_frag = "nr_vit_to_frag";
inline constexpr const char* vit_to_tin = "nr_vit_to_tin";

inline constexpr bool fast_available = true;
inline constexpr const char* fast_source = "v042";
inline constexpr size_t fast_module_bytes = 2840352;
inline constexpr unsigned char fast_sha256[32] = {0x67, 0xae, 0x93, 0x69, 0x66, 0xd5, 0x17, 0x9f, 0x4d, 0xad, 0x38, 0x73, 0x1f, 0x54, 0x3d, 0x52, 0x54, 0x00, 0xe0, 0x41, 0xdc, 0xc9, 0xc8, 0x88, 0x39, 0xa4, 0xc8, 0xfa, 0x94, 0xc2, 0x73, 0xfe};
inline constexpr unsigned fast_varparams_bytes = 176;
inline constexpr const char* fast_symbols[3][8] = {
 {
  "_Z13k_reg_swin_mhILi64ELi0ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi1ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi2ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi3ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi4ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi5ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi8ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi10ELb1EEv9VarParams"
 },
 {
  "_Z13k_reg_swin_mhILi128ELi0ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi1ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi2ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi3ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi4ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi5ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi8ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi10ELb1EEv9VarParams"
 },
 {
  "_Z13k_reg_swin_mhILi256ELi0ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi1ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi2ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi3ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi4ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi5ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi8ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi10ELb1EEv9VarParams"
 }
};
inline constexpr const char* fast_swin32[8] = {
  "_Z12k_reg_swin32ILi0ELb1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi1ELb1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi2ELb1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi4ELb1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi5ELb1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi8ELb1EEv9VarParams"  ,
  nullptr  ,
  nullptr
};
inline constexpr const char* fast_vit[7] = {
  "_Z14k_reg_vit_ffwdILi1ELb0ELb1EEv13VitFfwdParams",
  "_Z14k_reg_vit_ffwdILi1ELb1ELb1EEv13VitFfwdParams",
  "_Z14k_reg_vit_convILi1ELi0ELb1EEv13VitConvParams",
  "_Z14k_reg_vit_convILi1ELi1ELb1EEv13VitConvParams",
  "_Z14k_reg_vit_convILi1ELi2ELb1EEv13VitConvParams",
  "_Z14k_reg_vit_convILi1ELi4ELb1EEv13VitConvParams",
  "_Z14k_reg_vit_attnILb1EEv10AttnParams"
};

inline constexpr bool fast43_available = true;
inline constexpr const char* fast43_source = "v043";
inline constexpr size_t fast43_module_bytes = 2719832;
inline constexpr unsigned char fast43_sha256[32] = {0x54, 0x3c, 0xfe, 0xf7, 0xf5, 0x0f, 0xc3, 0x5e, 0xb0, 0x28, 0x6f, 0x85, 0x43, 0x40, 0x69, 0x01, 0x19, 0x6f, 0x3b, 0x46, 0x73, 0x8c, 0x2c, 0x16, 0x54, 0xf4, 0x72, 0x23, 0xbb, 0x02, 0xb6, 0x3a};
inline constexpr unsigned fast43_varparams_bytes = 176;
inline constexpr const char* fast43_symbols[3][8] = {
 {
  "_Z13k_reg_swin_mhILi64ELi0ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi1ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi2ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi3ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi4ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi5ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi8ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi10ELb1EEv9VarParams"
 },
 {
  "_Z13k_reg_swin_mhILi128ELi0ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi1ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi2ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi3ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi4ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi5ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi8ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi10ELb1EEv9VarParams"
 },
 {
  "_Z13k_reg_swin_mhILi256ELi0ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi1ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi2ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi3ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi4ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi5ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi8ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi10ELb1EEv9VarParams"
 }
};
inline constexpr const char* fast43_swin32[8] = {
  "_Z12k_reg_swin32ILi0ELb1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi1ELb1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi2ELb1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi4ELb1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi5ELb1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi8ELb1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi20ELb1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi32ELb1EEv9VarParams"
};
inline constexpr const char* fast43_vit[7] = {
  "_Z14k_reg_vit_ffwdILi1ELb0ELb1EEv13VitFfwdParams",
  "_Z14k_reg_vit_ffwdILi1ELb1ELb1EEv13VitFfwdParams",
  "_Z14k_reg_vit_convILi1ELi0ELb1EEv13VitConvParams",
  "_Z14k_reg_vit_convILi1ELi1ELb1EEv13VitConvParams",
  "_Z14k_reg_vit_convILi1ELi2ELb1EEv13VitConvParams",
  "_Z14k_reg_vit_convILi1ELi4ELb1EEv13VitConvParams",
  "_Z15k_reg_vit_attn2ILb1EEv10AttnParams"
};

inline constexpr bool fast51_available = true;
inline constexpr size_t fast51_module_bytes = 3133600;
inline constexpr unsigned char fast51_sha256[32] = {0xfa, 0x17, 0xff, 0xe5, 0xd8, 0xc7, 0x4a, 0xc1, 0xf1, 0xb1, 0x76, 0x03, 0xfe, 0x43, 0xd5, 0x05, 0x6d, 0x36, 0x15, 0xb4, 0x97, 0x8b, 0x20, 0x5a, 0x5c, 0x2a, 0xa5, 0xbe, 0x59, 0xaf, 0x35, 0x20};
inline constexpr unsigned fast51_varparams_bytes = 192;
inline constexpr const char* fast51_symbols[3][8] = {
 {
  "_Z13k_reg_swin_mhILi64ELi0ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi1ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi2ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi3ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi4ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi5ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi8ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi64ELi10ELb1EEv9VarParams"
 },
 {
  "_Z13k_reg_swin_mhILi128ELi0ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi1ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi2ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi3ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi4ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi5ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi8ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi128ELi10ELb1EEv9VarParams"
 },
 {
  "_Z13k_reg_swin_mhILi256ELi0ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi1ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi2ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi3ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi4ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi5ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi8ELb1EEv9VarParams",
  "_Z13k_reg_swin_mhILi256ELi10ELb1EEv9VarParams"
 }
};
inline constexpr const char* fast51_swin32[8] = {
  "_Z12k_reg_swin32ILi0ELb1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi1ELb1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi2ELb1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi4ELb1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi5ELb1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi8ELb1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi20ELb1EEv9VarParams"  ,
  "_Z12k_reg_swin32ILi32ELb1EEv9VarParams"
};
inline constexpr const char* fast51_vit[7] = {
  "_Z14k_reg_vit_ffwdILi1ELb0ELb1EEv13VitFfwdParams",
  "_Z14k_reg_vit_ffwdILi1ELb1ELb1EEv13VitFfwdParams",
  "_Z14k_reg_vit_convILi1ELi0ELb1EEv13VitConvParams",
  "_Z14k_reg_vit_convILi1ELi1ELb1EEv13VitConvParams",
  "_Z14k_reg_vit_convILi1ELi2ELb1EEv13VitConvParams",
  "_Z14k_reg_vit_convILi1ELi4ELb1EEv13VitConvParams",
  "_Z15k_reg_vit_attn2ILb1EEv10AttnParams"
};
}
