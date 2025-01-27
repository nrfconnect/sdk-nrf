/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HRT_H__
#define _HRT_H__

#include <stdint.h>
#include <stdbool.h>
#include <drivers/mspi/nrfe_mspi.h>
#include <zephyr/drivers/mspi.h>

#define VPRCSR_NORDIC_OUT_HIGH 1
#define VPRCSR_NORDIC_OUT_LOW  0

#define VPRCSR_NORDIC_DIR_OUTPUT 1
#define VPRCSR_NORDIC_DIR_INPUT	 0

#define BITS_IN_WORD 32
#define BITS_IN_BYTE 8

/** @brief Low level transfer parameters. */
struct hrt_xfer {

	/** @brief When true clock signal makes 1 transition less.
	 *         It is required for spi modes 1 and 3 due to hardware issue.
	 */
	bool eliminate_last_pulse;

	/** @brief Tx mode mask for csr dir register  */
	uint16_t tx_direction_mask;

	/** @brief Rx mode mask for csr dir register  */
	uint16_t rx_direction_mask;

};

/** @brief Write.
 *
 *  Function to be used to write data on SPI.
 *
 *  @param[in] xfer_ll_params Low level transfer parameters and data.
 */
void hrt_write(struct hrt_xfer *xfer_ll_params);

#endif /* _HRT_H__ */
