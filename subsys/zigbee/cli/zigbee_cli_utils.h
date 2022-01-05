/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZIGBEE_CLI_UTILS_H__
#define ZIGBEE_CLI_UTILS_H__

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <shell/shell.h>

#include <zboss_api.h>
#include <zigbee/zigbee_app_utils.h>

/**@brief Finish the command by dumping 'Done'.
 *
 * @param prepend_newline      Whether to prepend a newline.
 */
static inline void zb_cli_print_done(const struct shell *shell,
				     bool prepend_newline)
{
	shell_fprintf(shell, SHELL_NORMAL,
		      (prepend_newline ? "\nDone\n" : "Done\n"));
}

/**@brief Print error message to the console.
 *
 * @param message          Pointer to the message which should be printed
 *                         as an error.
 * @param prepend_newline  Whether to prepend a newline.
 */
static inline void zb_cli_print_error(const struct shell *shell,
				      const char *message, bool prepend_newline)
{
	if (message) {
		shell_error(shell, prepend_newline ?
			    "\nError: %s" : "Error: %s", message);
	} else {
		shell_error(shell, prepend_newline ?
			    "\nError: Unknown error occurred" :
			    "Error: Unknown error occurred");
	}
}

/**@brief Print a list of items.
 *
 * Individual items in the list are delimited by comma.
 *
 * @param text_buffer   A pointer to text buffer.
 * @param hdr           The list header string.
 * @param fmt           A printf like format of an individual list item.
 * @param type          Type of the list item.
 * @param ptr           A pointer to the first item in the list.
 * @param size          The list size (in items).
 */
#define PRINT_LIST(text_buffer, hdr, fmt, type, ptr, size)                   \
{                                                                            \
	sprintf((text_buffer + strlen(text_buffer)), hdr);                   \
	for (type * item = (ptr); item < (ptr) + size - 1; item++) {         \
		type local_item = UNALIGNED_GET(item);                       \
		sprintf((text_buffer +                                       \
			strlen(text_buffer)), fmt ",", local_item);          \
		}                                                            \
	if (size > 0) {                                                      \
		sprintf((text_buffer +                                       \
			strlen(text_buffer)), fmt " ", *((ptr) + size - 1)); \
	}                                                                    \
}

/**@brief Convert ZCL attribute value to string.
 *
 * @param[out] str_buf    Pointer to a string buffer which will be filled.
 * @param[in]  buf_len    String buffer length.
 * @param[in]  attr_type  ZCL attribute type value.
 * @param[in]  attr       Pointer to ZCL attribute value.
 *
 * @return Number of bytes written into string buffer or negative value
 *         on error.
 */
int zb_cli_zcl_attr_to_str(char *str_buf, uint16_t buf_len,
			   zb_uint16_t attr_type, zb_uint8_t *attr);

/**@brief Parse uint8_t from input string.
 *
 * The reason for this explicit function is because newlib-nano sscanf()
 * does not support 1-byte target.
 *
 * @param[in]  bp     Pointer to input string.
 * @param[out] value  Pointer to output value.
 *
 * @return 1 on success, 0 otherwise.
 */
int zb_cli_sscan_uint8(const char *bp, uint8_t *value);

/**@brief Parse unsigned integers from input string.
 *
 * The reason for this explicit function is because of lack
 * of sscanf() function. This function is to be used to parse number
 * up to (UINT32_MAX).
 *
 * @param[in]  bp     Pointer to input string.
 * @param[out] value  Pointer to variable to store reuslt of the function.
 * @param[in]  size   Size, in bytes, that determines expected maximum value
 *                    of converted number.
 * @param[in]  base   Numerical base (radix) that determines the valid
 *                    characters and their interpretation.
 *
 * @return 1 on success, 0 otherwise.
 */
int zb_cli_sscan_uint(const char *bp, uint8_t *value, uint8_t size,
		      uint8_t base);

/**@brief Print buffer as hex string.
 *
 * @param shell    Pointer to the shell instance.
 * @param data     Pointer to data to be printed out.
 * @param size     Data size in bytes
 * @param reverse  If True then data is printed out in reverse order.
 */
void zb_cli_print_hexdump(const struct shell *shell, const uint8_t *data,
			  uint8_t size, bool reverse);

/**@brief Print 64bit value (address, extpan) as hex string.
 *
 * The value is expected to be low endian.
 *
 * @param shell     Pointer to the shell instance.
 * @param addr      64 data to be printed out.
 */
static inline void zb_cli_print_eui64(const struct shell *shell,
				      const zb_64bit_addr_t addr)
{
	zb_cli_print_hexdump(shell, (const uint8_t *)addr,
			     sizeof(zb_64bit_addr_t), true);
}

#endif /* ZIGBEE_CLI_UTILS_H__ */
