/******************************************************************************
Copyright 2015 Tatsuya Yatagawa (tatsy)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/

#ifndef RAINY_COMMON_H_
#define RAINY_COMMON_H_

#include <cmath>
#include <iostream>

// ----------------------------------------------------------------------------
// Parameter constants
// ----------------------------------------------------------------------------
static const double PI = 4.0 * atan(1.0);
static const double INFTY = 1.0e20;
static const double EPS = 1.0e-8;

// ----------------------------------------------------------------------------
// Parallel for
// ----------------------------------------------------------------------------
#ifdef _OPENMP
#include <omp.h>
#define ompfor __pragma(omp parallel for) for
#else  // _OPENMP
#define ompfor for
#endif  // _OPENMP

// ----------------------------------------------------------------------------
// Assertion with message
// ----------------------------------------------------------------------------
#ifndef NDEBUG
#define msg_assert(PREDICATE, MSG) \
do { \
    if (!(PREDICATE)) { \
        std::cerr << "Asssertion \"" << #PREDICATE << "\" failed in " << __FILE__ \
        << " line " << __LINE__ << " : " << MSG << std::endl; \
        std::abort(); \
    } \
} while (false)
#else  // NDEBUG
#define msg_assert(PREDICATE, MSG) do {} while (false)
#endif  // NDEBUG

#endif  // RAINY_COMMON_H_
