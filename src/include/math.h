//
// Math library
//
#if _MSC_VER > 1000
#pragma once
#endif

#ifndef MATH_H
#define MATH_H

#define HUGE_VAL _infinity
#define INFINITY _infinity
#define NAN _nan

//
// Definitions of useful mathematical constants
//

#define M_E        2.71828182845904523536     // e
#define M_LOG2E    1.44269504088896340736     // log2(e)
#define M_LOG10E   0.434294481903251827651    // log10(e)
#define M_LN2      0.693147180559945309417    // ln(2)
#define M_LN10     2.30258509299404568402     // ln(10)
#define M_PI       3.14159265358979323846     // pi
#define M_PI_2     1.57079632679489661923     // pi/2
#define M_PI_4     0.785398163397448309616    // pi/4
#define M_1_PI     0.318309886183790671538    // 1/pi
#define M_2_PI     0.636619772367581343076    // 2/pi
#define M_2_SQRTPI 1.12837916709551257390     // 2/sqrt(pi)
#define M_SQRT2    1.41421356237309504880     // sqrt(2)
#define M_SQRT1_2  0.707106781186547524401    // 1/sqrt(2)

//
// Floating point classes for fpclassify()
//

#define FP_NAN       0
#define FP_INFINITE  1
#define FP_ZERO      2
#define FP_SUBNORMAL 3
#define FP_NORMAL    4

#ifdef  __cplusplus
extern "C" {
#endif

extern const double _infinity;
extern const double _nan;

double acos(double x);

double asin(double x);

double atan(double x);

double atan2(double x, double y);

double ceil(double x);

double cos(double x);

double cosh(double x);

double exp(double x);

double fabs(double x);

double floor(double x);

double fmod(double x, double y);

double frexp(double x, int *n);

double ldexp(double x, int n);

double log(double x);

double log10(double x);

double modf(double x, double *y);

double pow(double x, double y);

double sin(double x);

double sinh(double x);

double sqrt(double x);

double tan(double x);

double tanh(double x);

double atof(const char *str);

int isinf(double x);

int isnan(double x);

int isfinite(double x);

int fpclassify(double x);

int isless(double x, double y);

int isgreater(double x, double y);

double round(double x);

double hypot(double x, double y);

#ifdef  __cplusplus
}
#endif

#endif
