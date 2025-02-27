//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Implementation of bitrates in bits/second.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

#if defined(DOXYGEN)
//!
//! If this symbol is defined, bitrate values are represented as unsigned 64-bit integers.
//!
//! @see ts::Integer
//! @see ts::BitRate
//!
#define TS_BITRATE_INTEGER 1

//!
//! If this symbol is defined, bitrate values are represented as fractions of 64-bit integers.
//!
//! Using fractions instead of fixed-point numbers gives a better precision but seriously impacts the
//! overall performance of computations involving bitrates. The experience also demonstractes that using
//! fractions for bitrates introduces intermediate overflows, making long computations on bitrates
//! unusable in practice. Using fractions for bitrates is therefore currenly discouraged, until a new
//! implementation of fractions absorbs intermediate overflows using approximated fraction reduction,
//! at the expense of a loss of precision.
//!
//! @see ts::Fraction
//! @see ts::BitRate
//!
#define TS_BITRATE_FRACTION 1

//!
//! If this symbol is defined, bitrate values are represented as double floating-point.
//!
//! Using floating-point instead of fixed-point numbers may give a better precision in some cases.
//!
//! This is currently the default.
//!
//! @see ts::Double
//! @see ts::BitRate
//!
#define TS_BITRATE_FLOAT 1

//!
//! If this symbol is defined, bitrate values are represented as fixed-point.
//! The number of decimal digits is set by the macro TS_BITRATE_DECIMALS.
//!
//! @see ts::FixedPoint
//! @see ts::BitRate
//! @see TS_BITRATE_DECIMALS
//!
#define TS_BITRATE_FIXED 1

#else

// Default representation for BitRate values.
#if !defined(TS_BITRATE_INTEGER) && !defined(TS_BITRATE_FRACTION) && !defined(TS_BITRATE_FLOAT) && !defined(TS_BITRATE_FIXED)
    #define TS_BITRATE_FLOAT 1
#endif

#endif

//!
//! Define the precision (number of decimal digits) of fixed-point bitrate values.
//! This is used when bitrates are represented as fixed-point numbers instead of fractions or floating-point.
//!
//! @see ts::FixedPoint
//! @see ts::BitRate
//! @see TS_BITRATE_FIXED
//!
#if !defined(TS_BITRATE_DECIMALS)
    #define TS_BITRATE_DECIMALS 1
#endif

//!
//! Define the displayed precision (number of decimal digits) of floating-point bitrate values.
//! This is used when bitrates are represented as floating-point numbers instead of fractions or fixed-point.
//!
//! @see ts::FloatingPoint
//! @see ts::BitRate
//! @see TS_BITRATE_FLOAT
//!
#if !defined(TS_BITRATE_DISPLAY_DECIMALS)
    #define TS_BITRATE_DISPLAY_DECIMALS 2
#endif

// Required header for implementation of BitRate.
#if defined(TS_BITRATE_INTEGER)
    #include "tsInteger.h"
#elif defined(TS_BITRATE_FRACTION)
    #include "tsFraction.h"
#elif defined(TS_BITRATE_FLOAT)
    #include "tsFloatingPoint.h"
#elif defined(TS_BITRATE_FIXED)
    #include "tsFixedPoint.h"
#endif

namespace ts {
    //!
    //! Bitrate in bits/second.
    //!
    //! To get more precision over long computations or exotic modulations,
    //! a bitrate is implemented either as a 64-bit unsigned integer, a fixed-point
    //! value with decimal digits, a fraction of integers or a floating point value.
    //! This is a compile-time decision which is based on the macros TS_BITRATE_INTEGER,
    //! TS_BITRATE_FRACTION, TS_BITRATE_FLOAT and TS_BITRATE_FIXED.
    //!
    //! When implemented as a fixed-point value, the number of decimal digits
    //! is customizable using the macro TS_BITRATE_DECIMALS.
    //!
    //! When using fixed-point values, bitrates are represented with one decimal
    //! digit only. Tests with 2 digits were not positive. Intermediate overflows
    //! in some computations were encountered in some plugins working on large
    //! window sizes. Automatically detecting the overflow and reducing the
    //! window size accordingly works fine but the efficiency of the plugins
    //! is not as good as previously. Using 1 decimal digit is currently the
    //! best balance.
    //!
    //! @see TS_BITRATE_INTEGER
    //! @see TS_BITRATE_DECIMALS
    //! @see TS_BITRATE_FRACTION
    //! @see TS_BITRATE_FLOAT
    //!
#if defined(DOXYGEN)
    using BitRate = user_defined;
#elif defined(TS_BITRATE_INTEGER)
    using BitRate = Integer<uint64_t>;
#elif defined(TS_BITRATE_FRACTION)
    using BitRate = Fraction<uint64_t>;
#elif defined(TS_BITRATE_FLOAT)
    TS_LLVM_NOWARNING(implicit-int-float-conversion)
    using BitRate = FloatingPoint<double,TS_BITRATE_DISPLAY_DECIMALS>;
#elif defined(TS_BITRATE_FIXED)
    using BitRate = FixedPoint<int64_t, TS_BITRATE_DECIMALS>;
#else
    #error "undefined implementation of BitRate"
#endif

    //!
    //! Get a string description of the bitrate format.
    //! @return A string description of the bitrate format.
    //!
    TSDUCKDLL UString GetBitRateDescription();
}

extern "C" {
    // Compute the name of a symbol which describe the bitrate implementation.
    //! @cond nodoxygen
    #define TS_SYM2(a,b) TS_SYM2a(a,b)
    #define TS_SYM2a(a,b) a##_##b
    #if defined(TS_BITRATE_FRACTION)
        #define TSDUCK_LIBRARY_BITRATE_SYMBOL TSDUCK_LIBRARY_BITRATE_FRACTION
    #elif defined(TS_BITRATE_INTEGER)
        #define TSDUCK_LIBRARY_BITRATE_SYMBOL TSDUCK_LIBRARY_BITRATE_INTEGER
    #elif defined(TS_BITRATE_FLOAT)
        #define TSDUCK_LIBRARY_BITRATE_SYMBOL TSDUCK_LIBRARY_BITRATE_FLOAT
    #elif defined(TS_BITRATE_FIXED)
        #define TSDUCK_LIBRARY_BITRATE_SYMBOL TS_SYM2(TSDUCK_LIBRARY_BITRATE_FIXED,TS_BITRATE_DECIMALS)
    #else
        #error "undefined implementation of BitRate"
    #endif
    //! @endcond

    //!
    //! Generate a dependency on the bitrate implementation.
    //! Enforcing this dependency prevents mixing binaries which
    //! were compiled using different implementations of BitRate.
    //!
    extern const int TSDUCKDLL TSDUCK_LIBRARY_BITRATE_SYMBOL;
}

// In all compilation units which include tsBitRate.h, generate a reference
// to the symbol which describes the BitRate implementation.
//! @cond nodoxygen
TS_STATIC_REFERENCE(bitrate, &TSDUCK_LIBRARY_BITRATE_SYMBOL);
//! @endcond
