/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fota_download_delta_modem, CONFIG_LOG_DEFAULT_LEVEL);

/* Initialized to value different than success (0) */
static int dfu_result = -1;

NRF_MODEM_LIB_ON_DFU_RES(fota_delta_modem_dfu_res_hook,
			 on_modem_dfu_res, NULL);

static void on_modem_dfu_res(int dfu_res, void *ctx)
{
	dfu_result = dfu_res;
}

int fota_download_apply_delta_modem_update(void)
{
	int ret;
	int result;

	lte_lc_deinit();
	nrf_modem_lib_shutdown();
	ret = nrf_modem_lib_init();

	switch (dfu_result) {
	case NRF_MODEM_DFU_RESULT_OK:
		LOG_INF("MODEM UPDATE OK. Will run new firmware");
		result = 0;
		break;
	default:
		LOG_INF("MODEM UPDATE fail %d", ret);
		result = -1;
		break;
	}

	return result;
}
