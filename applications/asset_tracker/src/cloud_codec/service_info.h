/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SERVICE_INFO_H__
#define SERVICE_INFO_H__

#include <zephyr.h>
#include <cJSON.h>

/**
 * @file service_info.h
 *
 * @brief API for generating service info JSON for the device shadow
 * @defgroup service_info API to generate service info JSON for device shadow.
 * @{
 */
#define SERVICE_INFO_FOTA_VER_CURRENT 2
#define SERVICE_INFO_FOTA_STR_BOOTLOADER "BOOT"
#define SERVICE_INFO_FOTA_STR_MODEM "MODEM"
#define SERVICE_INFO_FOTA_STR_APP "APP"

/** @brief Encode the service info to JSON.
 *
 * Service info is written to the JSON object.
 *
 * @param ui Array of UI strings.
 * @param ui_count Number of ui strings in the array.
 * @param fota Array of FOTA strings.
 * @param fota_count Number of FOTA strings in the array.
 * @param fota_version FOTA version number.
 * @param obj_out The JSON object where the data is stored.
 *
 * @return 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int service_info_json_object_encode(const char *const ui[],
				    const uint32_t ui_count,
				    const char *const fota[],
					const uint32_t fota_count,
					const uint16_t fota_version,
					cJSON *obj_out);

/** @} */

#endif /* SERVICE_INFO_H__ */
