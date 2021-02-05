/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_MQTT_
#define SLM_AT_MQTT_

/**@file slm_at_mqtt.h
 *
 * @brief Vendor-specific AT command for MQTT service.
 * @{
 */

#include <zephyr/types.h>
#include "slm_at_host.h"

/**
 * @brief MQTT AT command parser.
 *
 * @param at_cmd  AT command string.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_mqtt_parse(const char *at_cmd);

/**
 * @brief List MQTT AT commands.
 *
 */
void slm_at_mqtt_clac(void);

/**
 * @brief Initialize MQTT AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_mqtt_init(void);

/**
 * @brief Uninitialize MQTT AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_mqtt_uninit(void);

/** @} */

#endif /* SLM_AT_GPS_ */
