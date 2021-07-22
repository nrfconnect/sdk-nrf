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

/* Purpose: parser interface intended for usage inside control module */

#ifndef PTT_PARSER_INTERNAL_H__
#define PTT_PARSER_INTERNAL_H__

#include "ptt_types.h"
#include "ptt_errors.h"

/** @brief Converts string to integer and checks upper and lower boundaries
 *
 *  Works with decimal values and hexadecimal values with '0x' or 0X' prefix.
 *  Will return error if converted value fails boundary check or can not be
 *  stored in long type.
 *
 *  @param[IN]  p_str       - pointer to NULL terminated string
 *  @param[OUT] p_out_value - pointer to store out value of procedure
 *  @param[IN]  base        - base to use in converting process. Use zero to let base be selected automatically
 *  @param[IN]  min         - lower boundary of expected integer
 *  @param[IN]  max         - higher boundary of expected integer
 *
 *  @return ptt_ret_t returns PTT_RET_SUCCESS, or error code
 */
ptt_ret_t ptt_parser_string_to_int(const char * p_str,
                                   int32_t    * p_out_value,
                                   uint8_t      base,
                                   int32_t      min,
                                   int32_t      max);

/** @brief Converts string to uint8_t, will be successful for all value that fit in one byte
 *
 *  This is a wrapper for @ref ptt_parser_string_to_int with min = INT8_MIN and max = UINT8_MAX.
 *  Used to get one byte from input regardless of it sign.
 *
 *  @param[IN]  p_str       - pointer to NULL terminated string
 *  @param[OUT] p_out_value - pointer to store out value of procedure
 *  @param[IN]  base        - base to use in converting process. Use zero to let base be selected automatically
 *
 *  @return ptt_ret_t returns PTT_RET_SUCCESS, or error code
 */
ptt_ret_t ptt_parser_string_to_byte(const char * p_str, uint8_t * p_out_value, uint8_t base);

/** @brief Converts string to int32_t
 *
 *  This is a wrapper for @ref ptt_parser_string_to_int with min = INT32_MIN and max = INT32_MAX.
 *
 *  @param[IN]  p_str       - pointer to NULL terminated string
 *  @param[OUT] p_out_value - pointer to store out value of procedure
 *  @param[IN]  base        - base to use in converting process. Use zero to let base be selected automatically
 *
 *  @return ptt_ret_t returns PTT_RET_SUCCESS, or error code
 */
ptt_ret_t ptt_parser_string_to_int32(const char * p_str, int32_t * p_out_value, uint8_t base);

/** @brief Converts string to int8_t
 *
 *  This is a wrapper for @ref ptt_parser_string_to_int with min = INT8_MIN and max = INT8_MAX.
 *
 *  @param[IN]  p_str       - pointer to NULL terminated string
 *  @param[OUT] p_out_value - pointer to store out value of procedure
 *  @param[IN]  base        - base to use in converting process. Use zero to let base be selected automatically
 *
 *  @return ptt_ret_t returns PTT_RET_SUCCESS, or error code
 */
ptt_ret_t ptt_parser_string_to_int8(const char * p_str, int8_t * p_out_value, uint8_t base);

/** @brief Converts string to uint8_t
 *
 *  This is a wrapper for @ref ptt_parser_string_to_int with min = 0 and max = UINT8_MAX.
 *
 *  @param[IN]  p_str       - pointer to NULL terminated string
 *  @param[OUT] p_out_value - pointer to store out value of procedure
 *  @param[IN]  base        - base to use in converting process. Use zero to let base be selected automatically
 *
 *  @return ptt_ret_t returns PTT_RET_SUCCESS, or error code
 */
ptt_ret_t ptt_parser_string_to_uint8(const char * p_str, uint8_t * p_out_value, uint8_t base);

/** @brief Converts string to int16_t
 *
 *  This is a wrapper for @ref ptt_parser_string_to_int with min = INT16_MIN and max = INT16_MAX.
 *
 *  @param[IN]  p_str       - pointer to NULL terminated string
 *  @param[OUT] p_out_value - pointer to store out value of procedure
 *  @param[IN]  base        - base to use in converting process. Use zero to let base be selected automatically
 *
 *  @return ptt_ret_t returns PTT_RET_SUCCESS, or error code
 */
ptt_ret_t ptt_parser_string_to_int16(const char * p_str, int16_t * p_out_value, uint8_t base);

/** @brief Converts string to uint16_t
 *
 *  This is a wrapper for @ref ptt_parser_string_to_int with min = 0 and max = UINT16_MAX.
 *
 *  @param[IN]  p_str       - pointer to NULL terminated string
 *  @param[OUT] p_out_value - pointer to store out value of procedure
 *  @param[IN]  base        - base to use in converting process. Use zero to let base be selected automatically
 *
 *  @return ptt_ret_t returns PTT_RET_SUCCESS, or error code
 */
ptt_ret_t ptt_parser_string_to_uint16(const char * p_str, uint16_t * p_out_value, uint8_t base);

/** @brief Converts string of hexadecimal values to array of uint8_t
 *
 *  Detects and cut out prefix '0x' or '0X'. Returns error if string can not be
 *  stored fully in provided array, has invalid symbols or is empty.
 *
 *  @param[IN]  p_hex_str     - pointer to NULL terminated string of hexadecimal values
 *  @param[OUT] p_out_value   - pointer to allocated array long enough to store max_len values
 *  @param[IN]  max_len       - maximum available space in array p_out_value
 *  @param[OUT] p_written_len - pointer to store length of converted array, can be NULL, contains incorrect value on error
 *
 *  @return ptt_ret_t returns PTT_RET_SUCCESS, or error code
 */
ptt_ret_t ptt_parser_hex_string_to_uint8_array(const char * p_hex_str,
                                               uint8_t    * p_out_value,
                                               uint8_t      max_len,
                                               uint8_t    * p_written_len);

#endif /* PTT_PARSER_INTERNAL_H__ */