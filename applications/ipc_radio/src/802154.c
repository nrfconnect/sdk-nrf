/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <zephyr/kernel.h>
#include <nrf_802154_serialization_error.h>

void nrf_802154_serialization_error(const nrf_802154_ser_err_data_t *err)
{
	ARG_UNUSED(err);
	__ASSERT(false, "802.15.4 serialization error");
	k_oops();
}

void nrf_802154_sl_fault_handler(uint32_t id, int32_t line, const char *err)
{
	__ASSERT(false, "module_id: %u, line: %d, %s", id, line, err);
	k_oops();
}
