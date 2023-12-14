/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef WIFI_PROVISIONING_H_
#define WIFI_PROVISIONING_H_

#include <zephyr/types.h>
#include <zephyr/bluetooth/uuid.h>

/**@file
 * @defgroup bt_wifi_prov Wi-Fi Provisioning Service API
 * @{
 * @brief API for the Wi-Fi Provisioning Service.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* WiFi Provisioning Service UUID */
#define BT_UUID_PROV_VAL \
	BT_UUID_128_ENCODE(0x14387800, 0x130c, 0x49e7, 0xb877, 0x2881c89cb258)
#define BT_UUID_PROV \
	BT_UUID_DECLARE_128(BT_UUID_PROV_VAL)

/* Information characteristic UUID */
#define BT_UUID_PROV_INFO_VAL \
	BT_UUID_128_ENCODE(0x14387801, 0x130c, 0x49e7, 0xb877, 0x2881c89cb258)
#define BT_UUID_PROV_INFO \
	BT_UUID_DECLARE_128(BT_UUID_PROV_INFO_VAL)

/* Control Point characteristic UUID */
#define BT_UUID_PROV_CONTROL_POINT_VAL \
	BT_UUID_128_ENCODE(0x14387802, 0x130c, 0x49e7, 0xb877, 0x2881c89cb258)
#define BT_UUID_PROV_CONTROL_POINT \
	BT_UUID_DECLARE_128(BT_UUID_PROV_CONTROL_POINT_VAL)

/* Data out characteristic UUID */
#define BT_UUID_PROV_DATA_OUT_VAL \
	BT_UUID_128_ENCODE(0x14387803, 0x130c, 0x49e7, 0xb877, 0x2881c89cb258)
#define BT_UUID_PROV_DATA_OUT \
	BT_UUID_DECLARE_128(BT_UUID_PROV_DATA_OUT_VAL)

/**
 * @def PROV_SVC_VER
 *
 * Firmware version.
 */
#define PROV_SVC_VER	0x01

/**
 * @brief Get provisioning state.
 *
 * @return true if device is provisioned, false otherwise.
 */
bool bt_wifi_prov_state_get(void);

/**
 * @brief Initialize the provisioning module.
 *
 * @return 0 if module initialized successfully, negative error code otherwise.
 */
int bt_wifi_prov_init(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* WIFI_PROVISIONING_H_ */
