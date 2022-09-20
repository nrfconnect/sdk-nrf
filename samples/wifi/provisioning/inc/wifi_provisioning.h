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

#ifdef __cplusplus
extern "C" {
#endif


#define PROV_SVC_VER_MAJOR	0x1
#define PROV_SVC_VER_MINOR	0x1
#define PROV_SVC_VER \
	((uint8_t)((PROV_SVC_VER_MAJOR << 4) + (PROV_SVC_VER_MINOR)))

struct wifi_config_t {
	uint8_t ssid[32];
	uint16_t ssid_len;
	uint8_t bssid[6];
	uint8_t password[64];
	uint16_t password_len;
	enum wifi_security_type auth_type;
	uint8_t band;
	uint8_t channel;
};

/**
 * @brief Get provisioning state of the device.
 *
 * @retval True if provisioned, otherwise false.
 */
bool wifi_prov_is_provsioned(void);

/**Following functions are interface to play with stored configuration.
 * As this is global variable and would be shared among multiple threads,
 * we encapsulate the opeartions so that caller does not need to take
 * care of the race condition.
 */

/**
 * @brief test if there exists valid configuration (i.e., the device is provisioned).
 *
 * @retval 0 If no valid configuration.
 *         1 If there is valid configuration.
 *          Otherwise, a (negative) error code defined in errno.h is returned.
 */
int wifi_has_config(void);

int wifi_get_config(struct wifi_config_t *config);

int wifi_set_config(struct wifi_config_t *config, bool ram_only);

int wifi_commit_config(void);

int wifi_remove_config(void);

int wifi_config_init(void);

int wifi_prov_init(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_PROVISIONING_H_ */
