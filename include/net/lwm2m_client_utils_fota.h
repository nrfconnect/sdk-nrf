/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file lwm2m_client_utils_fota.h
 * @defgroup lwm2m_client_utils_fota LwM2M FOTA
 * @ingroup lwm2m_client_utils
 * @{
 * @brief API for the LwM2M based FOTA
 */

#ifndef LWM2M_CLIENT_UTILS_FOTA_H__
#define LWM2M_CLIENT_UTILS_FOTA_H__

/**
 * @brief Update counter
 */
struct update_counter {
	int current; /**< Active image's state */
	int update; /**< Update image's state */
};

/**
 * @brief Counter type
 */
enum counter_type {
	COUNTER_CURRENT = 0, /**< Active image's counter */
	COUNTER_UPDATE /**< Update image's counter */
};

/**
 * @brief Read the update counter
 *
 * @param[in] obj_inst_id Firmware Object instance ID
 * @param[out] update_counter Update counter values for active and update images
 */
int fota_update_counter_read(uint16_t obj_inst_id, struct update_counter *update_counter);

/**
 * @brief Update the update counter
 */
int fota_update_counter_update(uint16_t obj_inst_id, enum counter_type type, uint32_t new_value);

/**
 * @brief Initialize FOTA settings
 */
int fota_settings_init(void);

#endif /* LWM2M_CLIENT_UTILS_FOTA_H__ */

/**@} */
