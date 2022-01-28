/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * The RST file for this library can be found in
 * doc/nrf/libraries/networking/lwm2m_client_utils.rst.
 * Rendered documentation is available at
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/networking/lwm2m_client_utils.html.
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
 */
int fota_update_counter_read(struct update_counter *update_counter);

/**
 * @brief Update the update counter
 */
int fota_update_counter_update(enum counter_type type, uint32_t new_value);

/**
 * @brief Initialize FOTA settings
 */
int fota_settings_init(void);

#endif /* LWM2M_CLIENT_UTILS_FOTA_H__ */

/**@} */
