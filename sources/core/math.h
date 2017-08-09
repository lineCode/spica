#ifndef _MSC_VER
#pragma once
#endif

#ifndef _SPICA_MATH_H_
#define _SPICA_MATH_H_

#include "common.h"

namespace spica {

namespace math {

inline double erfinv(double x) {
    double w, p;
    x = clamp(x, -0.99999, 0.99999);
    w = -std::log((1 - x) * (1 + x));
    if (w < 5) {
        w = w - 2.5f;
        p = 2.81022636e-08f;
        p = 3.43273939e-07f + p * w;
        p = -3.5233877e-06f + p * w;
        p = -4.39150654e-06f + p * w;
        p = 0.00021858087f + p * w;
        p = -0.00125372503f + p * w;
        p = -0.00417768164f + p * w;
        p = 0.246640727f + p * w;
        p = 1.50140941f + p * w;
    } else {
        w = std::sqrt(w) - 3;
        p = -0.000200214257f;
        p = 0.000100950558f + p * w;
        p = 0.00134934322f + p * w;
        p = -0.00367342844f + p * w;
        p = 0.00573950773f + p * w;
        p = -0.0076224613f + p * w;
        p = 0.00943887047f + p * w;
        p = 1.00167406f + p * w;
        p = 2.83297682f + p * w;
    }
    return p * x;
}

inline double erf(double x) {
    // constants
    double a1 = 0.254829592f;
    double a2 = -0.284496736f;
    double a3 = 1.421413741f;
    double a4 = -1.453152027f;
    double a5 = 1.061405429f;
    double p = 0.3275911f;

    // Save the sign of x
    int sign = 1;
    if (x < 0) sign = -1;
    x = std::abs(x);

    // A&S formula 7.1.26
    double t = 1 / (1 + p * x);
    double y =
        1 -
        (((((a5 * t + a4) * t) + a3) * t + a2) * t + a1) * t * std::exp(-x * x);

    return sign * y;
}

}  // namespace math

}  // namespace spica

#endif  // _SPICA_MATH_H_
