/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_LLS_H_
#define BT_LLS_H_

/**@file
 * @defgroup bt_lls Link Loss Service API
 * @{
 * @brief API for the Link Loss Service (LLS).
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>


/** @brief Link Loss Service callback structure. */
struct bt_lls_cb {
	/**
	 * @brief Callback function to stop alert.
	 *
	 * This callback is called when peer commands to disable alert.
	 */
	void (*no_alert)(void);

	/**
	 * @brief Callback function for alert level value.
	 *
	 * This callback is called when peer commands to alert.
	 */
	void (*mild_alert)(void);

	/**
	 * @brief Callback function for alert level value.
	 *
	 * This callback is called when peer commands to alert in the strongest possible way.
	 */
	void (*high_alert)(void);
};

void bt_lls_init(struct bt_lls_cb *lls_cb);

extern struct bt_conn_cb conn_callbacks;

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_LLS_H_ */
