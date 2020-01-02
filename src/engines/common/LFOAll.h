/*
 * Copyright (c) 2019 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef LS_LFO_ALL_H
#define LS_LFO_ALL_H

// This header pulls all LFO header files available in the sampler's code base.
//
// Since we have numerous different LFO wave form types, and for each of them
// even several different implementations, this header serves for including all
// the individual LFO implementations' header files and additionally it
// automatically selects (by typedefs) for each wave form one specific
// implementation (either because the configure script's benchmarks picked the
// most fastest/efficient implementation for this machine, or because the it was
// explicitly human selected (i.e. by configure script argument).

#include "../../common/global_private.h"
#include "LFOBase.h"

// IDs of the two possible implementations
// we get the implementation to pick from config.h
// the implementation IDs should be the same like in benchmarks/triang.cpp !
#define TRIANG_INT_MATH_SOLUTION       2
#define TRIANG_DI_HARMONIC_SOLUTION    3
#define TRIANG_INT_ABS_MATH_SOLUTION   5

// include the appropriate (unsigned) triangle LFO implementation
#if CONFIG_UNSIGNED_TRIANG_ALGO == TRIANG_INT_MATH_SOLUTION
# include "LFOTriangleIntMath.h"
#elif CONFIG_UNSIGNED_TRIANG_ALGO == TRIANG_INT_ABS_MATH_SOLUTION
# include "LFOTriangleIntAbsMath.h"
#elif CONFIG_UNSIGNED_TRIANG_ALGO == TRIANG_DI_HARMONIC_SOLUTION
# include "LFOTriangleDiHarmonic.h"
#else
# error "Unknown or no (unsigned) triangle LFO implementation selected!"
#endif

// include the appropriate (signed) triangle LFO implementation
#if CONFIG_SIGNED_TRIANG_ALGO == TRIANG_INT_MATH_SOLUTION
# include "LFOTriangleIntMath.h"
#elif CONFIG_SIGNED_TRIANG_ALGO == TRIANG_INT_ABS_MATH_SOLUTION
# include "LFOTriangleIntAbsMath.h"
#elif CONFIG_SIGNED_TRIANG_ALGO == TRIANG_DI_HARMONIC_SOLUTION
# include "LFOTriangleDiHarmonic.h"
#else
# error "Unknown or no (signed) triangle LFO implementation selected!"
#endif

//TODO: benchmark in configure and pull the relevant implementation for these wave forms as well like we do for triangle
#include "LFOSquareIntMath.h"
#include "LFOSquarePulse.h"
#include "LFOSawIntMathNew.h"
#include "LFOSawIntMathOld.h"
#include "LFOSineNumericComplexNr.h"
#include "LFOSineBuiltinFn.h"
#include "LFOPulse.h"

namespace LinuxSampler {

    #if CONFIG_UNSIGNED_TRIANG_ALGO == TRIANG_INT_MATH_SOLUTION
    typedef LFOTriangleIntMath<LFO::range_unsigned> LFOTriangleUnsigned;
    #elif CONFIG_UNSIGNED_TRIANG_ALGO == TRIANG_INT_ABS_MATH_SOLUTION
    typedef LFOTriangleIntAbsMath<LFO::range_unsigned> LFOTriangleUnsigned;
    #elif CONFIG_UNSIGNED_TRIANG_ALGO == TRIANG_DI_HARMONIC_SOLUTION
    typedef LFOTriangleDiHarmonic<LFO::range_unsigned> LFOTriangleUnsigned;
    #endif

    #if CONFIG_SIGNED_TRIANG_ALGO == TRIANG_INT_MATH_SOLUTION
    typedef LFOTriangleIntMath<LFO::range_signed> LFOTriangleSigned;
    #elif CONFIG_SIGNED_TRIANG_ALGO == TRIANG_INT_ABS_MATH_SOLUTION
    typedef LFOTriangleIntAbsMath<LFO::range_signed> LFOTriangleSigned;
    #elif CONFIG_SIGNED_TRIANG_ALGO == TRIANG_DI_HARMONIC_SOLUTION
    typedef LFOTriangleDiHarmonic<LFO::range_signed> LFOTriangleSigned;
    #endif

    typedef LFOSquareIntMath<LFO::range_signed> LFOSquareSigned;
    typedef LFOSquareIntMath<LFO::range_unsigned> LFOSquareUnsigned;

    typedef LFOSawIntMathNew<LFO::range_signed> LFOSawSigned;
    typedef LFOSawIntMathNew<LFO::range_unsigned> LFOSawUnsigned;

    typedef LFOSineNumericComplexNr<LFO::range_signed> LFOSineSigned;
    typedef LFOSineNumericComplexNr<LFO::range_unsigned> LFOSineUnsigned;

} // namespace LinuxSampler

#endif // LS_LFO_ALL_H
