#ifndef NR_PROF_COMUM_HLSLI
#define NR_PROF_COMUM_HLSLI

#define NR_TOL_PROF(absoluta, relativa, a, b) max((absoluta), (relativa) * max(abs(a), abs(b)))

#define NR_MESMA_SUPERFICIE(a, b, tol) (isfinite(a) && isfinite(b) && (tol) > 0.0 && abs((a) - (b)) <= (tol))
#endif
