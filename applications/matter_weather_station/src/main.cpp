/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include <zephyr/logging/log.h>

#ifdef CONFIG_USB_DEVICE_STACK
#include <zephyr/usb/usb_device.h>
#endif

LOG_MODULE_REGISTER(app);

int main()
{
	CHIP_ERROR err = CHIP_NO_ERROR;

#ifdef CONFIG_USB_DEVICE_STACK
	err = chip::System::MapErrorZephyr(usb_enable(NULL));
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Failed to initialize USB device");
	}
#endif

	if (err == CHIP_NO_ERROR) {
		err = AppTask::Instance().StartApp();
	}

	LOG_ERR("Exited with code %" CHIP_ERROR_FORMAT, err.Format());
	return err == CHIP_NO_ERROR ? EXIT_SUCCESS : EXIT_FAILURE;
}
