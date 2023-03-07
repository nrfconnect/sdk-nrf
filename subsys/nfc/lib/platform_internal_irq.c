/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <nfc_platform.h>
#include "platform_internal.h"

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
