/* Copyright (c) 2023, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file
 * @brief This file includes utility functions.
 *
 * @note In endianness conversions host order is assumed to be little endian.
 */

#ifndef UTILS_H_
#define UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <limits.h>

/**
 * @brief Swaps bytes on uint16_t.
 *
 * @param[in] val Value to be converted.
 *
 * @return uint16_t with swapped bytes.
 */
static inline uint16_t swap_uint16(uint16_t val)
{
    return (val << 8) | (val >> 8);
}

/**
 * @brief Swaps bytes on int16_t.
 *
 * @param[in] val Value to be converted.
 *
 * @return int16_t with swapped bytes.
 */
static inline int16_t swap_int16(int16_t val)
{
    return (val << 8) | ((val >> 8) & 0xff);
}

/**
 * @brief Converts uint16_t from host to network order.
 *
 * @param[in] val Value to be converted.
 *
 * @return uint16_t in network order (big endian).
 */
static inline uint16_t htons(uint16_t val)
{
    return swap_uint16(val);
}

/**
 * @brief Converts int16_t from host to network order.
 *
 * @param[in] val Value to be converted.
 *
 * @return int16_t in network order (big endian).
 */
static inline int16_t htons_signed(uint16_t val)
{
    return swap_int16(val);
}

/**
 * @brief Converts uint16_t from network to host order.
 *
 * @param[in] val Value to be converted.
 *
 * @return uint16_t in host order.
 */
static inline uint16_t ntohs(uint16_t val)
{
    return swap_uint16(val);
}

/**
 * @brief Converts int16_t from network to host order.
 *
 * @param[in] val Value to be converted.
 *
 * @return int16_t in host order.
 */
static inline int16_t ntohs_signed(uint16_t val)
{
    return swap_int16(val);
}

/**
 * @brief Divides with round up.
 *
 * @param[in] val     Dividend.
 * @param[in] divisor Divisor.
 *
 * @return Quotient round up.
 */
static inline uint32_t divide_round_up(uint32_t val, uint32_t divisor)
{
    return (val + (divisor - 1U)) / divisor;
}

/**
 * @brief Rounds up value to the multiple of multiple_base.
 *
 * @param[in] val           Value to be round up.
 * @param[in] multiple_base Multiple base.
 *
 * @return Round up value.
 */
static inline uint32_t round_up_to_multiple_of(uint32_t val, uint32_t multiple_base)
{
    return divide_round_up(val, multiple_base) * multiple_base;
}

#ifdef __cplusplus
}
#endif

#endif // UTILS_H_
