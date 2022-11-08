/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/**
 * @file wifi_provisioning.h
 *
 * @brief WiFi Provisioning Service
 */
#ifndef WIFI_PROVISIONING_H_
#define WIFI_PROVISIONING_H_

#include <zephyr/bluetooth/uuid.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <net/wifi_credentials.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def PROV_SVC_VER
 *
 * Firmware version.
 */
#define PROV_SVC_VER	0x01

/**
 * @brief Initialize the provisioning module.
 *
 * @return 0 if module initialized successfully, negative error code otherwise.
 */
int wifi_prov_init(void);


/**
 * @brief Fetch the first stored settings entry.
 *
 * @param creds pointer to credentials struct
 */
void wifi_credentials_get_first(struct wifi_credentials_personal *creds);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_PROVISIONING_H_ */
