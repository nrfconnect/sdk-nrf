/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "string_conversion.h"

#define STR_MAX_CHARACTERS      160
#define STR_7BIT_ESCAPE_IND     0x80
#define STR_7BIT_CODE_MASK      0x7F
#define STR_7BIT_ESCAPE_CODE    0x1B

/**
 * @brief Conversion table from ASCII (with ISO-8859-15 extension) to GSM 7 bit
 * Default Alphabet character set (3GPP TS 23.038 chapter 6.2.1).
 *
 * @details Table index equals the ASCII character code and the value stored in
 * the index is the corresponding GSM 7 bit character code.
 *
 * Notes:
 * - Use of GSM extension table is marked by setting bit 8 to 1. In that case
 *   the lowest 7 bits indicate character code in the default extension table.
 *   In the resulting 7-bit string that is coded as 2 characters: the "escape
 *   to extension" code 0x1B followed by character code in the default extension
 *   table.
 * - A space character (0x20) is used in case there is no matching or similar
 *   character available in the GSM 7 bit char set.
 * - When possible, closest similar character is used, like plain letters
 *   instead of letters with different accents, if there is no equivalent
 *   character available.
 */
static const uint8_t ascii_to_7bit_table[256] = {

	/* Standard ASCII, character codes 0-127 */
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,  /* 0-7:   Control characters */
	0x20, 0x20, 0x0A, 0x20, 0x20, 0x0D, 0x20, 0x20,  /* 8-15:  ...LF,..CR...      */

	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,  /* 16-31: Control characters */
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,

	0x20, 0x21, 0x22, 0x23, 0x02, 0x25, 0x26, 0x27,  /* 32-39: SP ! " # $ % & ' */
	0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,  /* 40-47: ( ) * + , - . /  */

	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,  /* 48-55: 0 1 2 3 4 5 6 7  */
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,  /* 56-63: 8 9 : ; < = > ?  */

	0x00, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,  /* 64-71: @ A B C D E F G  */
	0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,  /* 72-79: H I J K L M N O  */

	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,  /* 80-87: P Q R S T U V W  */
	0x58, 0x59, 0x5A, 0xBC, 0xAF, 0xBE, 0x94, 0x11,  /* 88-95: X Y Z [ \ ] ^ _  */

	0x27, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,  /* 96-103: (` -> ') a b c d e f g */
	0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,  /* 104-111:h i j k l m n o  */

	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,  /* 112-119: p q r s t u v w  */
	0x78, 0x79, 0x7A, 0xA8, 0xC0, 0xA9, 0xBD, 0x20,  /* 120-127: x y z { | } ~ DEL */

	/* Character codes 128-255 (beyond standard ASCII) have different possible
	 * interpretations. This table has been done according to ISO-8859-15.
	 */
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,  /* 128-159: Undefined   */
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,

	0x20, 0x40, 0x63, 0x01, 0xE5, 0x03, 0x53, 0x5F,  /* 160-167: ..£, €... */
	0x73, 0x63, 0x20, 0x20, 0x20, 0x2D, 0x20, 0x20,  /* 168-175 */

	0x20, 0x20, 0x20, 0x20, 0x5A, 0x75, 0x0A, 0x20,  /* 176-183 */
	0x7A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x59, 0x60,  /* 184-191 */

	0x41, 0x41, 0x41, 0x41, 0x5B, 0x0E, 0x1C, 0x09,  /* 192-199: ..Ä, Å... */
	0x45, 0x1F, 0x45, 0x45, 0x49, 0x49, 0x49, 0x49,  /* 200-207 */

	0x44, 0x5D, 0x4F, 0x4F, 0x4F, 0x4F, 0x5C, 0x2A,  /* 208-215: ..Ö... */
	0x0B, 0x55, 0x55, 0x55, 0x5E, 0x59, 0x20, 0x1E,  /* 216-223 */

	0x7F, 0x61, 0x61, 0x61, 0x7B, 0x0F, 0x1D, 0x63,  /* 224-231: ..ä, å... */
	0x04, 0x05, 0x65, 0x65, 0x07, 0x69, 0x69, 0x69,  /* 232-239 */

	0x20, 0x7D, 0x08, 0x6F, 0x6F, 0x6F, 0x7C, 0x2F,  /* 240-247: ..ö... */
	0x0C, 0x06, 0x75, 0x75, 0x7E, 0x79, 0x20, 0x79   /* 248-255 */
};


/**
 * @brief Conversion table from GSM 7 bit Default Alphabet character set
 * (3GPP TS 23.038 chapter 6.2.1) to ASCII (with ISO-8859-15 extension).
 *
 * @details Table index equals the GSM 7 bit character code and the value stored
 * in the index is the corresponding ASCII character code.
 *
 * Notes:
 * - Table indexes 128-255 are used for conversion of characters in GSM default
 *   alphabet extension table, i.e. character codes following the "escape to
 *   extension" code 0x1B.
 * - A space character (0x20) is used in case there is no matching or similar
 *   character available in the ASCII char set, and for the undefined extension
 *   codes.
 */
static const uint8_t gsm7bit_to_ascii_table[256] = {

	/* GSM 7 bit Default Alphabet table */
	0x40, 0xA3, 0x24, 0xA5, 0xE8, 0xE9, 0xF9, 0xEC,  /*  0- 7: @£$...        */
	0xF2, 0xC7, 0x0A, 0xD8, 0xF8, 0x0D, 0xC5, 0xE5,  /*  8-15: ...LF..CR..Åå */

	0x20, 0x5F, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,  /* 16-23: ._...         */
	0x20, 0x20, 0x20, 0x20, 0xC6, 0xE6, 0xDF, 0xC9,  /* 24-31: ..Escape.ÆæßÉ */

	0x20, 0x21, 0x22, 0x23, 0x20, 0x25, 0x26, 0x27,  /* 32-39: Space !"#¤%&' */
	0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,  /* 40-47: ()*+,-./      */

	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,  /* 48-55: 01234567      */
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,  /* 56-63: 89:;<=>?      */

	0xA1, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,  /* 64-71: ¡ABCDEFG      */
	0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,  /* 72-79: HIJKLMNO      */

	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,  /* 80-87: PQRSTUVW      */
	0x58, 0x59, 0x5A, 0xC4, 0xD6, 0xD1, 0xDC, 0xA7,  /* 88-95: XYZÄÖÑÜ§      */

	0xBF, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,  /*  96-103: ¿abcdefg    */
	0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,  /* 104-111: hijklmno    */

	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,  /* 112-119: pqrstuvw    */
	0x78, 0x79, 0x7A, 0xE4, 0xF6, 0xF1, 0xFC, 0xE0,  /* 120-127: xyzäöñüà    */

	/* GSM 7 bit Default Alphabet extension table:
	 * These codes are used for interpreting extended character codes
	 * following the "escape" code 0x1B.
	 */
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,  /* 128-135/ext 0-7      */
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,  /* 136-143/ext 8-15     */

	0x20, 0x20, 0x20, 0x20, 0x5E, 0x20, 0x20, 0x20,  /* 144-151/ext 16-23  ^ */
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,  /* 152-159/ext 24-31    */

	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,  /* 160-167/ext 32-39    */
	0x7B, 0x7D, 0x20, 0x20, 0x20, 0x20, 0x20, 0x5C,  /* 168-175/ext 40-47 {}\*/

	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,  /* 176-183/ext 48-55    */
	0x20, 0x20, 0x20, 0x20, 0x5B, 0x7E, 0x5D, 0x20,  /* 184-191/ext 56-63 [~]*/

	0x7C, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,  /* 192-199/ext 64-71 |  */
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,  /* 200-207/ext 72-79    */

	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,  /* 208-215/ext 80-87    */
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,  /* 216-223/ext 88-95    */

	0x20, 0x20, 0x20, 0x20, 0x20, 0xA4, 0x20, 0x20,  /* 224-231/ext 96-103 € */
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,  /* 232-239/ext 104-111  */

	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,  /* 240-247/ext 112-119  */
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,  /* 248-255/ext 120-127  */
};

uint8_t string_conversion_ascii_to_gsm7bit(
	const uint8_t *data,
	uint8_t  data_len,
	uint8_t *out_data,
	uint8_t *out_bytes,
	uint8_t *out_chars,
	bool     packing)
{
	uint8_t index_ascii = 0;
	uint8_t index_7bit = 0;
	uint8_t out_bytes_tmp = 0;
	uint8_t char_7bit;
	uint8_t char_ascii;

	if ((data == NULL) || (out_data == NULL)) {
		return 0;
	}

	for (index_ascii = 0; index_ascii < data_len; index_ascii++) {
		char_ascii = data[index_ascii];
		char_7bit = ascii_to_7bit_table[char_ascii];

		if ((char_7bit & STR_7BIT_ESCAPE_IND) == 0) {
			/* Character is in default alphabet table */
			if (index_7bit < STR_MAX_CHARACTERS) {
				out_data[index_7bit++] = char_7bit;
			} else {
				break;
			}
		} else {
			/* Character is in default extension table */
			if (index_7bit < STR_MAX_CHARACTERS - 1) {
				out_data[index_7bit++] = STR_7BIT_ESCAPE_CODE;
				out_data[index_7bit++] = (char_7bit & STR_7BIT_CODE_MASK);
			} else {
				break;
			}
		}
	}

	if (packing == true) {
		out_bytes_tmp = string_conversion_7bit_sms_packing(out_data, index_7bit);
	} else {
		out_bytes_tmp = index_7bit;
	}

	if (out_bytes != NULL) {
		*out_bytes = out_bytes_tmp;
	}
	if (out_chars != NULL) {
		*out_chars = index_7bit;
	}
	return index_ascii;
}

uint8_t string_conversion_gsm7bit_to_ascii(
	const uint8_t *data,
	uint8_t *out_data,
	uint8_t  num_char,
	bool     packed)
{
	uint8_t *buf_7bit;
	uint8_t index_ascii = 0;
	uint8_t index_7bit = 0;
	uint8_t char_7bit;

	if (packed == true) {
		num_char = string_conversion_7bit_sms_unpacking(data, out_data, num_char);
		buf_7bit = out_data;
	} else {
		buf_7bit = (uint8_t *)data;
	}

	if ((data == NULL) || (out_data == NULL)) {
		return 0;
	}

	for (index_7bit = 0; index_7bit < num_char; index_7bit++) {
		char_7bit = buf_7bit[index_7bit];

		if (char_7bit == STR_7BIT_ESCAPE_CODE) {
			index_7bit++;
			if (index_7bit < num_char) {
				char_7bit = buf_7bit[index_7bit];
				out_data[index_ascii] =
					gsm7bit_to_ascii_table[128 + char_7bit];
			} else {
				break;
			}
		} else {
			out_data[index_ascii] = gsm7bit_to_ascii_table[char_7bit];
		}
		index_ascii++;
	}

	return index_ascii;
}

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
uint8_t string_conversion_7bit_sms_packing(uint8_t *data, uint8_t data_len)
{
	uint8_t src = 0;
	uint8_t dst = 0;
	uint8_t shift = 0;

	if (data == NULL) {
		return 0;
	}

	while (src < data_len) {
		data[dst] = data[src] >> shift;
		src++;

		if (src < data_len) {
			data[dst] |= (data[src] << (7 - shift));
			shift++;

			if (shift == 7) {
				shift = 0;
				src++;
			}
		}
		dst++;
	}

	return dst;
}

uint8_t string_conversion_7bit_sms_unpacking(
	const uint8_t *packed,
	uint8_t *unpacked,
	uint8_t num_char)
{
	uint8_t shift = 1;
	uint8_t index_pack = 1;
	uint8_t index_char = 0;

	if ((packed == NULL) || (unpacked == NULL) || (num_char == 0)) {
		return 0;
	}

	unpacked[0] = packed[0] & STR_7BIT_CODE_MASK;
	index_char++;

	while (index_char < num_char) {
		unpacked[index_char] =
			(packed[index_pack] << shift) | (packed[index_pack - 1] >> (8 - shift));
		unpacked[index_char] &= STR_7BIT_CODE_MASK;

		shift++;
		if (shift == 8) {
			shift = 0;
		} else {
			index_pack++;
		}
		index_char++;
	}

	return index_char;
}
