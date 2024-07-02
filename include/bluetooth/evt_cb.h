
/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_EVT_CB_H__
#define BT_EVT_CB_H__

/**
 * @file
 * @defgroup bt_evt_cb Event callback
 * @{
 * @brief APIs to setup event callbacks
 *
 * The event callbacks are triggered relative to the
 * start of a Link Layer event.
 *
 * This information can be used to reduce the time from data sampling
 * until data is sent on air.
 */

#include <stdint.h>
#include <zephyr/bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Connection event callbacks
 *
 * The callbacks will be called from the system workqueue.
 */
struct bt_conn_evt_cb {
	/** Connection event prepare callback
	 *
	 * @param[in] conn      The connection context.
	 * @param[in] user_data User provided data.
	 */
	void (*prepare)(struct bt_conn *conn,
			void *user_data);
};

/** Register a connect event callback for a connection.
 *
 * @param[in] conn                The connection context.
 * @param[in] cb                  The callback to be used.
 * @param[in] user_data           User data which will be provided in the callback.
 * @param[in] prepare_distance_us The distance in time from the start of the
 *                                callback to the start of the connection event.
 *
 *  @retval 0          The device callbak has been successfully registered.
 *  @retval -EALREADY  A callback has already been registered for this connection.
 *  @retval -ENOMEM    There is not enough PPI channels available to use this feature.
 */
int bt_conn_evt_cb_register(struct bt_conn *conn,
			    struct bt_conn_evt_cb cb,
			    void *user_data,
			    uint32_t prepare_distance_us);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_EVT_CB_H__ */
