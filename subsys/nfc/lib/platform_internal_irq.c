/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <nfc_platform.h>
#include "platform_internal.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(nfc_platform, CONFIG_NFC_PLATFORM_LOG_LEVEL);

static nfc_lib_cb_resolve_t nfc_cb_resolve;

int nfc_platform_internal_init(nfc_lib_cb_resolve_t cb_rslv)
{
	if (!cb_rslv) {
		return -EINVAL;
	}

	nfc_cb_resolve = cb_rslv;
	return 0;
}

void nfc_platform_cb_request(const void *ctx,
			     size_t ctx_len,
			     const uint8_t *data,
			     size_t data_len,
			     bool copy_data)
{
	__ASSERT(nfc_cb_resolve != NULL, "nfc_cb_resolve is not set");
	nfc_cb_resolve(ctx, data);
}

void nfc_platform_event_handler(nrfx_nfct_evt_t const *event)
{
	int err;

	switch (event->evt_id) {
	case NRFX_NFCT_EVT_FIELD_DETECTED:
		LOG_DBG("Field detected");

		err = nfc_platform_internal_hfclk_start();
		__ASSERT_NO_MSG(err >= 0);

		break;

	case NRFX_NFCT_EVT_FIELD_LOST:
		LOG_DBG("Field lost");

		err = nfc_platform_internal_hfclk_stop();
		__ASSERT_NO_MSG(err >= 0);

		break;

	default:
		/* No implementation required */
		break;
	}
}
