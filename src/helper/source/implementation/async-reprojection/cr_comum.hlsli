#ifndef NR_CR_COMUM_HLSLI
#define NR_CR_COMUM_HLSLI
#define NR_CR0(f) ((f) * (-0.5 + (f) * (1.0 - 0.5 * (f))))
#define NR_CR1(f) (1.0 + (f) * (f) * (-2.5 + 1.5 * (f)))
#define NR_CR2(f) ((f) * (0.5 + (f) * (2.0 - 1.5 * (f))))
#define NR_CR3(f) ((f) * (f) * (-0.5 + 0.5 * (f)))
#endif
