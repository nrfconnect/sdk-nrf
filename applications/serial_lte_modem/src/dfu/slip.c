/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdbool.h>
#include "slip.h"

void encode_slip(uint8_t *dest_data, uint32_t *dest_size,
		 const uint8_t *src_data, uint32_t src_size)
{
	uint32_t n, n_dest_size;

	n_dest_size = 0;
	for (n = 0; n < src_size; n++) {
		uint8_t n_src_byte = *(src_data + n);

		if (n_src_byte == SLIP_END) {
			*dest_data++ = SLIP_ESC;
			*dest_data++ = SLIP_ESC_END;
			n_dest_size += 2;
		} else if (n_src_byte == SLIP_ESC) {
			*dest_data++ = SLIP_ESC;
			*dest_data++ = SLIP_ESC_ESC;
			n_dest_size += 2;
		} else {
			*dest_data++ = n_src_byte;
			n_dest_size++;
		}
	}

	*dest_data = SLIP_END;
	n_dest_size++;

	*dest_size = n_dest_size;
}

int decode_slip(uint8_t *dest_data, uint32_t *dest_size,
		const uint8_t *src_data, uint32_t src_size)
{
	int err_code = 1;
	uint32_t n, n_dest_size = 0;
	bool is_escaped = false;

	for (n = 0; n < src_size; n++) {
		uint8_t n_src_byte = *(src_data + n);

		if (n_src_byte == SLIP_END) {
			if (!is_escaped) {
				err_code = 0;  /* Done. OK */
			}
			break;
		} else if (n_src_byte == SLIP_ESC) {
			if (is_escaped) {
				/* should not get SLIP_ESC twice */
				err_code = 1;
				break;
			}
			is_escaped = true;
		} else if (n_src_byte == SLIP_ESC_END) {
			if (is_escaped) {
				is_escaped = false;
				*dest_data++ = SLIP_END;
			} else {
				*dest_data++ = n_src_byte;
			}
			n_dest_size++;
		} else if (n_src_byte == SLIP_ESC_ESC) {
			if (is_escaped) {
				is_escaped = false;
				*dest_data++ = SLIP_ESC;
			} else {
				*dest_data++ = n_src_byte;
			}
			n_dest_size++;
		} else {
			*dest_data++ = n_src_byte;
			n_dest_size++;
		}
	}

	*dest_size = n_dest_size;

	return err_code;
}
