/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUPL_UTILS_H_
#define SUPL_UTILS_H_

/**
 * @brief Convert hex string into hex
 *        Example: in = {'D', 'E', 'A', 'D'} -> out = {0xDE, 0xAD}
 *
 * @param[in]   in        Hex string buffer to convert
 * @param[in]   in_len    Input buffer length
 * @param[out]  out       Hex buffer for the output
 * @param[in]   out_len   Hex buffer length, shall be at least (in_len / 2)
 *
 * @return  hex output length on success
 *         -1 on error
 */
int hexstr2hex(const char *in,
	       size_t in_len,
	       unsigned char *out,
	       size_t out_len);

/**
 * @brief Get length of a string (or a line).
 *        String shall be '\0' terminated.
 *        Newline characters are not included in the count.
 *        Example: "+CEREG: 1" -> 9
 *                 "+CEREG: 1\\r\\n" -> 9
 *
 * @param[in]   buf       String buffer
 * @param[in]   buf_len   Input buffer length
 *
 * @return length of the string
 *         -1 if the string is not '\0' terminated
 */
int get_line_len(const char *buf, size_t buf_len);

#endif /* SUPL_UTILS_H_ */
