/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_RADIO_NOTIFICATION_CB_H__
#define BT_RADIO_NOTIFICATION_CB_H__

/**
 * @file
 * @defgroup bt_radio_notification_cb Radio Notification callback
 * @{
 * @brief APIs to set up radio notification callbacks
 *
 * The radio notification callbacks are triggered relative to the
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

/** Recommended prepare distance for the connection event callback.
 *
 * The selected distance assumes a maximum processing delay and a maximum
 * of 2 seconds between packets from devices with worst case clock accuracy.
 *
 * When shorter connection intervals are used or when it known that the local
 * clock accuracy is better a shorter prepare distance can be used.
 *
 * See @ref bt_radio_notification_conn_cb_register for more details about
 * prepare distance selection and clock drift.
 */
#define BT_RADIO_NOTIFICATION_CONN_CB_PREPARE_DISTANCE_US_RECOMMENDED 3000

/** Radio Notification connection callbacks
 *
 * The callbacks will be called from the system workqueue.
 */
struct bt_radio_notification_conn_cb {
	/** Radio Notification prepare callback
	 *
	 * The prepare callback will be triggered a configurable amount
	 * of time before the connection event starts.
	 *
	 * @param[in] conn The connection context.
	 */
	void (*prepare)(struct bt_conn *conn);
};

/** Register a radio notification callback struct for connections.
 *
 * When the prepare callback is used to provide data to the Bluetooth stack,
 * the application needs to reserve enough time to allow the data to be
 * forwarded to the link layer.
 *
 * When used with a peripheral connection, the prepare distance also needs to take
 * clock drift into account to avoid that the callback triggers too late.
 * The clock drift is determined by the distance between packets and the clock
 * accuracy of both the central and the peripheral.
 * The Bluetooth specification allows a worst case clock accurary of 500 ppm.
 * That gives a worst case combined clock accurary of 1000 ppm.
 * This results in 1 ms drift per second.
 *
 * See Bluetooth Core Specification, Vol 6, Part B, Section 4.2.4
 * for more details on clock drift.
 *
 * @param[in] cb                  The callback structure to be used.
 *                                The memory pointed to needs to be kept alive by the user.
 * @param[in] prepare_distance_us The distance in time from the start of the
 *   prepare callback to the start of the connection event.
 *   See also @ref BT_RADIO_NOTIFICATION_CONN_CB_PREPARE_DISTANCE_US_RECOMMENDED.
 *
 *  @retval 0          The callback structure has been successfully registered.
 *  @retval -EALREADY  A callback has already been registered.
 *  @retval -ENOMEM    There are not enough PPI channels available to use this feature.
 */
int bt_radio_notification_conn_cb_register(const struct bt_radio_notification_conn_cb *cb,
					   uint32_t prepare_distance_us);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_RADIO_NOTIFICATION_CB_H__ */
