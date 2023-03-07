/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log_ctrl.h>
#include <net/nrf_cloud.h>

#include "fota_support.h"

LOG_MODULE_REGISTER(fota_support, CONFIG_MQTT_MULTI_SERVICE_LOG_LEVEL);

/* Called from nRF Cloud event handler in connection.c */
void on_fota_downloaded(void)
{
	/* Reboot on successful FOTA download. (Thus, enabling the new firmware!) */
	LOG_WRN("Rebooting...");
	LOG_PANIC();
	sys_reboot(SYS_REBOOT_COLD);
}

struct dfu_target_fmfu_fdev * get_full_modem_fota_fdev(void)
{
	if (IS_ENABLED(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)) {
		static struct dfu_target_fmfu_fdev ext_flash_dev = {
			.size = 0,
			.offset = 0,
			/* CONFIG_DFU_TARGET_FULL_MODEM_USE_EXT_PARTITION is enabled,
			 * so no need to specify the flash device here
			 */
			.dev = NULL
		};

		return &ext_flash_dev;
	}

	return NULL;
}

/* You may notice there is no logic to actually receive/download/apply FOTA updates.
 * That is because these features are automatically implemented for MQTT by enabling
 * CONFIG_NRF_CLOUD_FOTA (which is implicitly enabled by CONFIG_NRF_CLOUD_MQTT).
 *
 * Note, also that connection.c is responsible for actually enabling FOTA in the nRF
 * Cloud portal, (by correctly updating the device shadow) as well as for notifying
 * fota_support.c of FOTA download completion.
 */
