#ifndef NR_FILTRO_COMUM_HLSLI
#define NR_FILTRO_COMUM_HLSLI
#define NR_BILINEAR_COM_ESTADO(h, uvp, tam, VALIDO) {\
            const float2 s = uvp * tam - 0.5; const int2 b0 = int2(floor(s)); const float2 f = s - float2(b0);\
            float4 acc = float4(0.0, 0.0, 0.0, 0.0); float wsum = 0.0;\
            [unroll] for (int j = 0; j < 4; ++j) {\
                const int2 off = int2(j & 1, j >> 1);\
                const int2 q = clamp(b0 + off, int2(0, 0), int2(int(largura) - 1, int(altura) - 1));\
                const float4 v = anterior.Load(int3(q, 0));\
                const float wv = (off.x ? f.x : 1.0 - f.x) * (off.y ? f.y : 1.0 - f.y);\
                if (all(isfinite(v)) && VALIDO(v) && wv > 0.0) { acc += wv * v; wsum += wv; }\
            }\
            if (wsum > 0.25) h = acc / wsum;\
        }
#endif
