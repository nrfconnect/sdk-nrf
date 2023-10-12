/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <entropy_poll.h>

#include "psa/crypto.h"

psa_status_t zephyr_get_entropy(uint32_t flags, size_t *estimate_bits, uint8_t *output,
				size_t output_size)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(rng));
	uint16_t request_len = MIN(UINT16_MAX, output_size);
	int err;

	/* Ignore flags */
	(void)flags;

	if (output == NULL || estimate_bits == NULL || output_size == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (!device_is_ready(dev)) {
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	err = entropy_get_entropy(dev, output, request_len);
	if (err) {
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	*estimate_bits = PSA_BYTES_TO_BITS(request_len);

	return PSA_SUCCESS;
}
