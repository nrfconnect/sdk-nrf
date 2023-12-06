/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, CONFIG_CHIP_APP_LOG_LEVEL);

int main()
{
	#ifdef CONFIG_BOARD_NRF52840DK_NRF52840
		volatile static uint8_t x[20000];
		for(size_t i = 0; i < sizeof(x); i++)
		{
			x[i] = 10;
		}
	#endif

	#ifdef CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP
		volatile static uint8_t y[100000];
		for(size_t i = 0; i < sizeof(y); i++)
		{
			y[i] = 10;
		}

	#endif


	CHIP_ERROR err = AppTask::Instance().StartApp();

	LOG_ERR("Exited with code %" CHIP_ERROR_FORMAT, err.Format());
	return err == CHIP_NO_ERROR ? EXIT_SUCCESS : EXIT_FAILURE;
}
