//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Statistics over a single set of data (integer or floating point).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsIntegerUtils.h"
#include <cmath>

namespace ts {
    //!
    //! Statistics over a single set of data (integer or floating point).
    //! @ingroup libtscore cpp
    //! @tparam NUMBER Integer or floating point data type.
    //! A specialization is available for std::chrono::duration types.
    //! @tparam DEFAULT_FLOAT Default floating point type for finer results.
    //! Ignored if @a NUMBER is a floating point type.
    //!
    template <typename NUMBER, typename DEFAULT_FLOAT = double, class = void>
    class SingleDataStatistics
    {
    public:
        //!
        //! The signed type for @a NUMBER.
        //!
        using SIGNED = typename ts::make_signed<NUMBER>::type;

        //!
        //! The floating-point type for @a NUMBER finer results.
        //!
        using FLOAT = typename std::conditional<std::is_floating_point<NUMBER>::value, NUMBER, DEFAULT_FLOAT>::type;

        //!
        //! Reset the statistics collection.
        //!
        void reset();

        //!
        //! Accumulate one more data sample.
        //! @param [in] value Data sample.
        //!
        void feed(NUMBER value);

        //!
        //! Get the number of accumulated samples.
        //! @return The number of accumulated samples.
        //!
        size_t count() const { return _count; }

        //!
        //! Get the minimum value of all accumulated samples.
        //! @return The minimum value.
        //!
        NUMBER minimum() const { return _min; }

        //!
        //! Get the maximum value of all accumulated samples.
        //! @return The maximum value.
        //!
        NUMBER maximum() const { return _max; }

        //!
        //! Get the mean value of all accumulated samples.
        //! @return The mean value. When @a NUMBER is an integer type, the returned
        //! value is from the FLOAT floating-point type.
        //!
        FLOAT mean() const;

        //!
        //! Get the mean value of all accumulated samples, rounded to the nearest integer.
        //! @return The mean value, rounded to the nearest integer.
        //!
        NUMBER meanRound() const { return NUMBER(std::round(mean())); }

        //!
        //! Get the variance of all accumulated samples.
        //! @return The variance. When @a NUMBER is an integer type, the returned
        //! value is from the FLOAT floating-point type.
        //!
        FLOAT variance() const;

        //!
        //! Get the standard deviation of all accumulated samples.
        //! @return The standard deviation. When @a NUMBER is an integer type, the returned
        //! value is from the FLOAT floating-point type.
        //!
        FLOAT standardDeviation() const { return std::sqrt(variance()); }

        //!
        //! Get the mean value of all accumulated samples as a string.
        //! @param [in] width Minimum width of the string, left-padded with spaces.
        //! @param [in] precision Number of decimal digits.
        //! @return The mean value as a string.
        //!
        UString meanString(size_t width = 0, size_t precision = 2) const;

        //!
        //! Get the variance of all accumulated samples as a string.
        //! @param [in] width Minimum width of the string, left-padded with spaces.
        //! @param [in] precision Number of decimal digits.
        //! @return The variance as a string.
        //!
        UString varianceString(size_t width = 0, size_t precision = 2) const;

        //!
        //! Get the standard deviation of all accumulated samples as a string.
        //! @param [in] width Minimum width of the string, left-padded with spaces.
        //! @param [in] precision Number of decimal digits.
        //! @return The standard deviation as a string.
        //!
        UString standardDeviationString(size_t width = 0, size_t precision = 2) const;

    private:
        //
        //  References:
        //  [1] https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
        //
        size_t _count = 0;    // Total number of samples.
        NUMBER _min = 0;      // Minimum value.
        NUMBER _max = 0;      // Maximum value.
        SIGNED _var_k = 0;    // Variance: K constant (see [1])
        SIGNED _var_ex = 0;   // Variance: Ex accumulation (see [1])
        SIGNED _var_ex2 = 0;  // Variance: Ex2 accumulation (see [1])
    };

    // Specialization for std::chrono::duration types.
    //! @cond nodoxygen
    template <class DURATION>
    class SingleDataStatistics<DURATION, double, typename std::enable_if<std::is_arithmetic<typename DURATION::rep>::value>::type> :
        public SingleDataStatistics<typename DURATION::rep, double>
    {
    private:
        using Rep = typename DURATION::rep;
        using SuperClass = SingleDataStatistics<typename DURATION::rep, double>;
    public:
        template <class Rep2, class Period2>
        void feed(const cn::duration<Rep2,Period2>& value) { SuperClass::feed(cn::duration_cast<DURATION>(value).count()); }
        DURATION minimum() const { return DURATION(SuperClass::minimum()); }
        DURATION maximum() const { return DURATION(SuperClass::maximum()); }
        DURATION meanRound() const { return DURATION(SuperClass::meanRound()); }
    };
    //! @endcond
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Reset the statistics collection.
template <typename NUMBER, typename DEFAULT_FLOAT, class N>
void ts::SingleDataStatistics<NUMBER, DEFAULT_FLOAT, N>::reset()
{
    _count = 0;
    _min = 0;
    _max = 0;
    _var_k = 0;
    _var_ex = 0;
    _var_ex2 = 0;
}


// Accumulate one more data sample.
template <typename NUMBER, typename DEFAULT_FLOAT, class N>
void ts::SingleDataStatistics<NUMBER, DEFAULT_FLOAT, N>::feed(NUMBER value)
{
    if (_count == 0) {
        _min = _max = value;
        _var_k = SIGNED(value);
        _var_ex = _var_ex2 = 0;
    }
    else {
        _min = std::min(value, _min);
        _max = std::max(value, _max);
    }
    const SIGNED diff = SIGNED(value) - _var_k;
    _var_ex += diff;
    _var_ex2 += diff * diff;
    _count++;
}


// Get the mean.
template <typename NUMBER, typename DEFAULT_FLOAT, class N>
typename ts::SingleDataStatistics<NUMBER, DEFAULT_FLOAT, N>::FLOAT ts::SingleDataStatistics<NUMBER, DEFAULT_FLOAT, N>::mean() const
{
    return _count == 0 ? 0.0 : FLOAT(_var_k) + FLOAT(_var_ex) / FLOAT(_count);
}

// Get the variance.
template <typename NUMBER, typename DEFAULT_FLOAT, class N>
typename ts::SingleDataStatistics<NUMBER, DEFAULT_FLOAT, N>::FLOAT ts::SingleDataStatistics<NUMBER, DEFAULT_FLOAT, N>::variance() const
{
    // See reference [1] in file header.
    return _count < 2 ? 0.0 : (FLOAT(_var_ex2) - FLOAT(_var_ex * _var_ex) / FLOAT(_count)) / FLOAT(_count - 1);
}

// Get the mean as strings.
template <typename NUMBER, typename DEFAULT_FLOAT, class N>
ts::UString ts::SingleDataStatistics<NUMBER, DEFAULT_FLOAT, N>::meanString(size_t width, size_t precision) const
{
    return UString::Format(u"%*.*f", width, precision, mean());
}

// Get the variance as string.
template <typename NUMBER, typename DEFAULT_FLOAT, class N>
ts::UString ts::SingleDataStatistics<NUMBER, DEFAULT_FLOAT, N>::varianceString(size_t width, size_t precision) const
{
    return UString::Format(u"%*.*f", width, precision, variance());
}

// Get the standard deviation as string.
template <typename NUMBER, typename DEFAULT_FLOAT, class N>
ts::UString ts::SingleDataStatistics<NUMBER, DEFAULT_FLOAT, N>::standardDeviationString(size_t width, size_t precision) const
{
    return UString::Format(u"%*.*f", width, precision, standardDeviation());
}
