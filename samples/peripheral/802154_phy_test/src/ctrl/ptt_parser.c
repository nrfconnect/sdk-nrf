/* Copyright (c) 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Use in source and binary forms, redistribution in binary form only, with
 * or without modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 2. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 3. This software, with or without modification, must only be used with a Nordic
 *    Semiconductor ASA integrated circuit.
 *
 * 4. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* Purpose: converting string of ASCII codes to integers */

#include "ptt_parser_internal.h"

#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#define PARSER_DECIMAL_BASE     10
#define PARSER_HEXADECIMAL_BASE 16
#define PARSER_BASE_NOT_FOUND   0xffu

static ptt_ret_t ptt_parser_hex_string_next_byte(const char * p_hex_str, uint8_t * p_out_num);

ptt_ret_t ptt_parser_string_to_int(const char * p_str,
                                   int32_t    * p_out_value,
                                   uint8_t      base,
                                   int32_t      min,
                                   int32_t      max)
{
    if (NULL == p_str)
    {
        return PTT_RET_NULL_PTR;
    }

    if (NULL == p_out_value)
    {
        return PTT_RET_NULL_PTR;
    }

    ptt_ret_t ret = PTT_RET_SUCCESS;

    char * p_end;

    long out_num = strtol(p_str, &p_end, base);

    /* "In particular, if *nptr is not '\0' but **endptr is '\0' on return, the entire string
     * is valid." <- from man strtol */
    /* also lets discard LONG_MIN/LONG_MAX values which show strtol failed */
    if (('\0' != *p_end) ||
        (out_num < min) || (out_num > max) ||
        (out_num == LONG_MIN) || (out_num == LONG_MAX))
    {
        ret = PTT_RET_INVALID_VALUE;
    }
    else
    {
        *p_out_value = out_num;
    }

    return ret;
}

inline ptt_ret_t ptt_parser_string_to_int32(const char * p_str, int32_t * p_out_value, uint8_t base)
{
    return ptt_parser_string_to_int(p_str, p_out_value, base, INT32_MIN, INT32_MAX);
}

inline ptt_ret_t ptt_parser_string_to_byte(const char * p_str, uint8_t * p_out_value, uint8_t base)
{
    int32_t   out_value;
    ptt_ret_t ret = ptt_parser_string_to_int(p_str, &out_value, base, INT8_MIN, UINT8_MAX);

    if (PTT_RET_SUCCESS == ret)
    {
        *p_out_value = out_value;
    }

    return ret;
}

inline ptt_ret_t ptt_parser_string_to_int8(const char * p_str, int8_t * p_out_value, uint8_t base)
{
    int32_t   out_value;
    ptt_ret_t ret = ptt_parser_string_to_int(p_str, &out_value, base, INT8_MIN, INT8_MAX);

    if (PTT_RET_SUCCESS == ret)
    {
        *p_out_value = out_value;
    }

    return ret;
}

inline ptt_ret_t ptt_parser_string_to_uint8(const char * p_str, uint8_t * p_out_value, uint8_t base)
{
    int32_t   out_value;
    ptt_ret_t ret = ptt_parser_string_to_int(p_str, &out_value, base, 0, UINT8_MAX);

    if (PTT_RET_SUCCESS == ret)
    {
        *p_out_value = out_value;
    }

    return ret;
}

inline ptt_ret_t ptt_parser_string_to_int16(const char * p_str, int16_t * p_out_value, uint8_t base)
{
    int32_t   out_value;
    ptt_ret_t ret = ptt_parser_string_to_int(p_str, &out_value, base, INT16_MIN, INT16_MAX);

    if (PTT_RET_SUCCESS == ret)
    {
        *p_out_value = out_value;
    }

    return ret;
}

inline ptt_ret_t ptt_parser_string_to_uint16(const char * p_str,
                                             uint16_t   * p_out_value,
                                             uint8_t      base)
{
    int32_t   out_value;
    ptt_ret_t ret = ptt_parser_string_to_int(p_str, &out_value, base, 0, UINT16_MAX);

    if (PTT_RET_SUCCESS == ret)
    {
        *p_out_value = out_value;
    }

    return ret;
}

ptt_ret_t ptt_parser_hex_string_to_uint8_array(const char * p_hex_str,
                                               uint8_t    * p_out_value,
                                               uint8_t      max_len,
                                               uint8_t    * p_written_len)
{
    if (NULL == p_hex_str)
    {
        return PTT_RET_NULL_PTR;
    }

    if (NULL == p_out_value)
    {
        return PTT_RET_NULL_PTR;
    }

    if (0 == max_len)
    {
        return PTT_RET_NO_FREE_SLOT;
    }

    /* cut away prefix, if present */
    if ((p_hex_str[0] == '0') &&
        ((p_hex_str[1] == 'X') || (p_hex_str[1] == 'x')))
    {
        p_hex_str += 2;
    }

    /* one byte is represented by 2 ASCII symbols => we expect even number of elements */
    if ((strlen(p_hex_str) % 2) || (!strlen(p_hex_str)))
    {
        return PTT_RET_INVALID_VALUE;
    }

    ptt_ret_t ret      = PTT_RET_SUCCESS;
    uint8_t   byte_cnt = 0;

    while ((PTT_RET_SUCCESS == ret) && strlen(p_hex_str))
    {
        if (byte_cnt < max_len)
        {
            ret        = ptt_parser_hex_string_next_byte(p_hex_str, &(p_out_value[byte_cnt]));
            p_hex_str += 2;
            ++byte_cnt;
        }
        else
        {
            ret = PTT_RET_NO_FREE_SLOT;
        }
    }

    if (NULL != p_written_len)
    {
        *p_written_len = byte_cnt;
    }

    return ret;
}

static ptt_ret_t ptt_parser_hex_string_next_byte(const char * p_hex_str, uint8_t * p_out_num)
{
    ptt_ret_t ret           = PTT_RET_SUCCESS;
    char      p_hex_byte[3] = {0};

    p_hex_byte[0] = p_hex_str[0];
    p_hex_byte[1] = p_hex_str[1];
    p_hex_byte[2] = '\0';

    char * p_end;
    long   out_num = strtol(p_hex_byte, &p_end, PARSER_HEXADECIMAL_BASE);

    /* "In particular, if *nptr is not '\0' but **endptr is '\0' on return, the entire string
     * is valid." <- from man strtol */
    if ((*p_end != '\0') ||
        (out_num < INT8_MIN) ||
        (out_num > UINT8_MAX))
    {
        ret = PTT_RET_INVALID_VALUE;
    }
    else
    {
        *p_out_num = (uint8_t)out_num;
    }

    return ret;
}

