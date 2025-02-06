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

/*
 * This is a "PSA Crypto Driver" for entropy.
 *
 * It uses a "Zephyr entropy driver" and can therefore only be used in
 * Zephyr images.
 *
 * This is used for two uses cases, the first use case is when hardware
 * crypto/entropy is not yet supported. This enables running software crypto
 * with a non cryptographically secure random generator to unblock development
 * when the device tree node with the DT label 'prng' is enabled.
 *
 * The second use case is for instance nrf52820 which has an NRF_RNG
 * peripheral, but does not have a HW crypto trng like cryptocell.
 * In this use case the device driver with the DT label 'rng'
 * is supported and this rng label is only applied for the Zephyr
 * driver that uses the HW peripheral NRF_RNG (entropy_nrf5.c).
 *
 * Note that NRF_RNG produces TRNG, not CSPRNG.
 */
#ifdef CONFIG_FAKE_ENTROPY_NRF_PRNG
#define DTS_RNG_NODE_LABEL prng
#else
#define DTS_RNG_NODE_LABEL rng
#endif

psa_status_t nrf_rng_get_entropy(uint32_t flags, size_t *estimate_bits, uint8_t *output,
				 size_t output_size)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(DTS_RNG_NODE_LABEL));
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
