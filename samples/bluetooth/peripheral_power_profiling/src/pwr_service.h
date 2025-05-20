/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PWR_SERVICE_H_
#define PWR_SERVICE_H_

#include <zephyr/bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief PWR Service status. */
enum pwr_service_status {
	/** PWR Service notification enabled. */
	PWR_SERVICE_STATUS_NOTIFY_ENABLED,

	/** PWR Service notification disabled. */
	PWR_SERVICE_STATUS_NOTIFY_DISABLED
};

/** @brief Notification sent callback.
 *
 * @param[in] conn Connection object.
 * @param[in] user_data User data.
 */
typedef void (*pwr_service_notification_sent_cb)(struct bt_conn *conn, void *user_data);

/** @brief Event callback. Notifies application about notification state.
 *
 * @param[in] status Notification status.
 * @param[in] user_data User data.
 */
typedef void (*pwr_service_notify_status_changed_cb)(enum pwr_service_status status,
						     void *user_data);

/** @brief PWR Service callbacks structure. */
struct pwr_service_cb {
	/** Notification sent callback. */
	pwr_service_notification_sent_cb sent_cb;

	/** Notification status callback. */
	pwr_service_notify_status_changed_cb notify_status_cb;
};

/** @brief Send PWR Service data. The service sends an array of zeros. Its length depends on the
 * Kconfig option CONFIG_BT_POWER_PROFILING_DATA_LENGTH.
 *
 * @param[in] conn Connection object. It can be NULL.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a negative error code is returned.
 */
int pwr_service_notify(struct bt_conn *conn);

/** @brief Register PWR service callbacks. This must be called before enabling the Bluetooth stack.
 *
 * @param[in] cb Callback structure.
 * @param[in] user_data User data to be passed to callback calls.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a negative error code is returned.
 */
int pwr_service_cb_register(const struct pwr_service_cb *cb, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* PWR_SERVICE_H_ */
