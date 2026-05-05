/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <net/nrf_cloud.h>

#include "fota_support.h"
#include "sample_reboot.h"

LOG_MODULE_REGISTER(fota_support, CONFIG_WIFI_NRF_CLOUD_LOG_LEVEL);

void on_fota_downloaded(void)
{
	sample_reboot_normal();
}

/* You may notice there is no logic to actually receive/download/apply FOTA updates.
 * That is because these features are automatically implemented for MQTT by enabling
 * CONFIG_NRF_CLOUD_FOTA (which is implicitly enabled by CONFIG_NRF_CLOUD_MQTT).
 *
 * Note, also that connection.c is responsible for actually enabling FOTA in the nRF
 * Cloud portal, (by correctly updating the device shadow) as well as for notifying
 * fota_support.c of FOTA download completion.
 */
