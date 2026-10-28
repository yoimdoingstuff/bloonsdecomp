#pragma once
#ifdef BLOONS_EMBEDDED
#ifdef BLOONS_ROCKBOX
// Rockbox's math.h contains constants, not these function declarations. Native
// plugin linkage supplies the matching toolchain's newlib libm archive.
extern "C" {
double sqrt(double);
double sin(double);
double cos(double);
double atan2(double,double);
double round(double);
double fabs(double);
}
#else
#include <math.h>
#endif
namespace bloons::math {
using ::sqrt;
using ::sin;
using ::cos;
using ::atan2;
using ::round;
inline double abs(double value) { return ::fabs(value); }
}
#else
#include <cmath>
namespace bloons { namespace math=std; }
#endif
