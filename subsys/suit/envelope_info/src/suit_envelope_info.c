/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "suit_envelope_info.h"

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h> /* TODO: delete when calculating size is added*/

#include <zephyr/logging/log.h>

#include <zcbor_decode.h>

#define EMPTY_STORAGE_VALUE 0xFFFFFFFF

LOG_MODULE_REGISTER(suit_envelope_info, CONFIG_SUIT_LOG_LEVEL);

static const uint8_t *envelope_address;
static size_t envelope_size;

static suit_plat_err_t update_candidate_address_and_size_validate(const uint8_t *addr, size_t size)
{
	if (addr == NULL || *((uint32_t *)addr) == EMPTY_STORAGE_VALUE) {
		LOG_DBG("Invalid update candidate address: %p", (void *)addr);
		return SUIT_PLAT_ERR_INVAL;
	}

	if (size == 0) {
		LOG_DBG("Invalid update candidate size: %d", size);
		return SUIT_PLAT_ERR_INVAL;
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_envelope_info_candidate_stored(const uint8_t *address, size_t max_size)
{
	zcbor_state_t states[3];

	if (address == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t), address, max_size, 1, NULL,
			0);

	/* Expect SUIT envelope tag (107) and skip until the end of the envelope */
	if (!zcbor_tag_expect(states, 107) || !zcbor_any_skip(states, NULL)) {
		LOG_DBG("Malformed envelope");
		return SUIT_PLAT_ERR_CBOR_DECODING;
	}

	envelope_size = MIN(max_size, (size_t)states[0].payload - (size_t)address);
	envelope_address = address;

	return SUIT_PLAT_SUCCESS;
}

void suit_envelope_info_reset(void)
{
	envelope_size = 0;
	envelope_address = NULL;
}

suit_plat_err_t suit_envelope_info_get(const uint8_t **address, size_t *size)
{
	suit_plat_err_t ret = SUIT_PLAT_ERR_INVAL;

	ret = update_candidate_address_and_size_validate(envelope_address, envelope_size);

	if (ret == SUIT_PLAT_SUCCESS) {
		*address = envelope_address;
		*size = envelope_size;
	}

	return ret;
}
