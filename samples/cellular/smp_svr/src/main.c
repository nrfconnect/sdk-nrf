/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <pm_config.h>
#include <zephyr/kernel.h>
#include <zephyr/stats/stats.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/dfu/mcuboot.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_smp_svr, CONFIG_LOG_DEFAULT_LEVEL);

static void image_validation(void)
{
	int rc;
	char buf[255];
	struct mcuboot_img_header header;

	boot_read_bank_header(PM_MCUBOOT_PRIMARY_ID, &header, sizeof(header));
	snprintk(buf, sizeof(buf), "%d.%d.%d-%d", header.h.v1.sem_ver.major,
		 header.h.v1.sem_ver.minor, header.h.v1.sem_ver.revision,
		 header.h.v1.sem_ver.build_num);

	LOG_INF("Booting image: build time: " __DATE__ " " __TIME__);
	LOG_INF("Image Version %s", buf);
	rc = boot_is_img_confirmed();
	LOG_INF("Image is%s confirmed OK", rc ? "" : " not");
	if (!rc) {
		if (boot_write_img_confirmed()) {
			LOG_ERR("Failed to confirm image");
		} else {
			LOG_INF("Marked image as OK");
		}
	}
}

int main(void)
{
	image_validation();

	while (1) {
		k_sleep(K_MSEC(10000));
	}
	return 0;
}
