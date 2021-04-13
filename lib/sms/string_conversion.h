/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _STRING_CONVERSION_H_
#define _STRING_CONVERSION_H_

#include <stdint.h>
#include <stdbool.h>


/**
 * @brief Convert ASCII characters into GSM 7 bit Default Alphabet character set.
 *
 * @details ascii_to_7bit_table conversion table is used. Optionally perform
 * also packing for the resulting 7 bit string. Note that the 7 bit string may
 * be longer than the original due to possible extension table usage. Each
 * extended character needs an escape code in addition to the character code in
 * extension table.
 *
 * References: 3GPP TS 23.038 chapter 6.2.1: GSM 7 bit Default Alphabet
 *
 * Input parameters:
 *
 * @param[in] data Pointer to array of characters to be converted. No null termination.
 * @param[in] data_len Number of characters to be converted, max 160.
 * @param[out] out_data Pointer to buffer for the converted string. Shall have allocation
 *                of 160 bytes, or in case of less than 80 input characters, at
 *                least 2*data_len to make sure that buffer overflow will not happen.
 * @param[out] out_bytes Pointer to a byte to return number of valid bytes in out_data.
 *                May be NULL if not needed.
 * @param[out] out_chars Pointer to a byte to return number of 7 bit characters, i.e. septets
 *                (including possible escape characters) in out_data. May be NULL if
 *                not needed. Same as out_bytes, when packing=false.
 * @param[in] packing True if the converted 7bit string has to be packed.
 *
 * @return Number of converted characters (same as data_len if all converted successfully).
 */
uint8_t string_conversion_ascii_to_gsm7bit(const uint8_t *data,
						 uint8_t  data_len,
						 uint8_t *out_data,
						 uint8_t *out_bytes,
						 uint8_t *out_chars,
						 bool     packing);

/**
 * @brief Convert GSM 7 bit Default Alphabet characters to ASCII characters.
 *
 * @details gsm7bit_to_ascii_table conversion table is used. Perform also unpacking of
 * the 7 bit string before conversion, if caller indicates that the string is packed.
 *
 * References: 3GPP TS 23.038 chapter 6.2.1: GSM 7 bit Default Alphabet
 *
 * @param[in] data Pointer to array of characters to be converted. No null termination.
 * @param[out] out_data Pointer to buffer for the converted string. Shall have allocation
 *             of at least "num_char" bytes. Note that this function does not add
 *             null termination at the end of the string. It should be done by caller,
 *             when needed. (In that case it could be useful to actually allocate
 *             num_char+1 bytes here.)
 * @param[in] num_char Number of 7-bit characters to be unpacked, including possible escape codes.
 *            Also indicates maximum allowed number of characters to be stored to output
 *            buffer by this function.
 * @param[in] packed True if the 7bit string is packed, i.e. has to be unpacked before conversion.
 *
 * @return Number of valid bytes/characters in "out_data". May be less than "num_char" in the
 *         case that the 7 bit string contains "escape/extended code" sequences, that are converted
 *         to single ASCII characters.
 *
 */
uint8_t string_conversion_gsm7bit_to_ascii(const uint8_t *data,
						 uint8_t *out_data,
						 uint8_t  num_char,
						 bool     packed);

/**
 * @brief Performs SMS packing for a string using GSM 7 bit character set. The result
 *        is stored in the same memory buffer that contains the input string to be
 *        packed.
 *
 *        Description of the packing functionality:
 *        Unpacked data bits:
 *        bit number:   7   6   5   4   3   2   1   0
 *        data byte 0:  0  1a  1b  1c  1d  1e  1f  1g      d1
 *        data byte 1:  0  2a  3b  2c  2d  2e  2f  2g      d2
 *        data byte 2:  0  3a  3b  3c  3d  3e  3f  3g      d3
 *        and so on...
 *
 *        Packed data bits:
 *        bit number:   7   6   5   4   3   2   1   0
 *        data byte 0: 2g  1a  1b  1c  1d  1e  1f  1g      d1>>0 | d2<<7
 *        data byte 1: 3f  3g  2a  2b  2c  2d  2e  2f      d2>>1 | d3<<6
 *        data byte 2: 4e  4f  4g  3a  3b  3c  3d  3e      d3>>2 | d4<<5
 *        data byte 3: 5d  5e  5f  5g  4a  4b  4c  4d      d4>>3 | d5<<4
 *        data byte 4: 6c  6d  6e  6f  6g  5a  5b  5c      d5>>4 | d6<<3
 *        data byte 5: 7b  7c  7d  7e  7f  7g  6a  6b      d6>>5 | d7<<2
 *        data byte 6: 8a  8b  8c  8d  8e  8f  8g  7a      d7>>6 | d8<<1
 *        data byte 7: Ag  9a  9b  9c  9d  9e  9f  9g      d9>>0 | dA<<7
 *        data byte 8: Bf  Bg  Aa  Ab  Ac  Ad  Ae  Af      dA>>1 | dB<<6
 *        and so on...
 *
 * References: 3GPP TS 23.038 chapter 6.1.2.1: SMS Packing
 *
 * @param[in,out] data Pointer to array of characters to be packed (no null termination needed).
 *                       Also the packed characters are stored into this buffer.
 * @param[in] data_len Number of characters to be packed
 *
 * @return Number of valid bytes in the packed character data.
 */
uint8_t string_conversion_7bit_sms_packing(uint8_t *data, uint8_t data_len);

/**
 * @brief Performs unpacking of a packed GSM 7 bit string as described below.
 *
 *        Packed data bits:
 *        bit number:   7   6   5   4   3   2   1   0
 *        data byte 0: 2g  1a  1b  1c  1d  1e  1f  1g      p0
 *        data byte 1: 3f  3g  2a  2b  2c  2d  2e  2f      p1
 *        data byte 2: 4e  4f  4g  3a  3b  3c  3d  3e      p2
 *        data byte 3: 5d  5e  5f  5g  4a  4b  4c  4d      p3
 *        data byte 4: 6c  6d  6e  6f  6g  5a  5b  5c      p4
 *        data byte 5: 7b  7c  7d  7e  7f  7g  6a  6b      p5
 *        data byte 6: 8a  8b  8c  8d  8e  8f  8g  7a      p6
 *        data byte 7: Ag  9a  9b  9c  9d  9e  9f  9g      p7
 *        data byte 8: Bf  Bg  Aa  Ab  Ac  Ad  Ae  Af      p8
 *        and so on...
 *
 *        Unpacked data bits:
 *        bit number:   7   6   5   4   3   2   1   0
 *        data byte 0:  0  1a  1b  1c  1d  1e  1f  1g       p0 & 7F
 *        data byte 1:  0  2a  3b  2c  2d  2e  2f  2g      (p1 << 1 | p0 >> 7) & 7F
 *        data byte 2:  0  3a  3b  3c  3d  3e  3f  3g      (p2 << 2 | p1 >> 6) & 7F
 *        data byte 3:  0  4a  4b  4c  4d  4e  4f  4g      (p3 << 3 | p2 >> 5) & 7F
 *        data byte 4:  0  5a  5b  5c  5d  5e  5f  5g      (p4 << 4 | p3 >> 4) & 7F
 *        data byte 5:  0  6a  6b  6c  6d  6e  6f  6g      (p5 << 5 | p4 >> 3) & 7F
 *        data byte 6:  0  7a  7b  7c  7d  7e  7f  7g      (p6 << 6 | p5 >> 2) & 7F
 *        data byte 7:  0  8a  8b  8c  8d  8e  8f  8g      (p7 << 7 | p6 >> 1) & 7F
 *        data byte 8:  0  9a  9b  9c  9d  9e  9f  9g      (p7 << 0 | p6 >> 8) & 7F
 *        data byte 9:  0  Aa  Ab  Ac  Ad  Ae  Af  Ag      (p8 << 1 | p7 >> 7) & 7F
 *        data byte A:  0  Ba  Bb  Bc  Bd  Be  Bf  Bg      (p9 << 2 | p8 >> 6) & 7F
 *        and so on...
 *
 * References: 3GPP TS 23.038 chapter 6.1.2.1: SMS Packing
 *
 * @param[in] packed Pointer to buffer containing the packed string.
 * @param[in] num_char Number of 7-bit characters (i.e. septets) to be unpacked, including
 *                     possible escape codes. Also indicates maximum allowed number of
 *                     characters to be stored to output buffer by this function.
 * @param[out] unpacked Pointer to buffer to store the unpacked string. Allocated size shall be
 *                      at least "num_char" bytes.
 *
 * @return Number of valid bytes/characters in the unpacked string "unpacked".
 *
 */
uint8_t string_conversion_7bit_sms_unpacking(const uint8_t *packed,
						   uint8_t *unpacked,
						   uint8_t  num_char);

#endif /* STRING_CONVERSION_H_ */
