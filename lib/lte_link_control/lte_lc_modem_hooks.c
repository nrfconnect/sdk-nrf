/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_modem_at.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>

NRF_MODEM_LIB_ON_SHUTDOWN(lte_lc_shutdown_hook, on_shutdown, NULL);

static void on_shutdown(void *ctx)
{
	(void)lte_lc_deinit();
}
