/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <bluetooth/services/nsms.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_nsms, CONFIG_BT_NSMS_LOG_LEVEL);

/* Time to wait for the status data access.
 * We should never wait more than that.
 * If we reach the timeout it seems to be an implementation issue.
 */
#define NSMS_STATUS_LOCK_WAIT_TIME K_MSEC(100)


ssize_t bt_nsms_status_read(struct bt_conn *conn,
			    struct bt_gatt_attr const *attr,
			    void *buf, uint16_t len, uint16_t offset)
{
	int ret;
	int status;
	const struct bt_nsms_status_str *status_str = attr->user_data;

	LOG_DBG("Reading from status characteristic");
	ret = k_mutex_lock(status_str->mtx, NSMS_STATUS_LOCK_WAIT_TIME);
	__ASSERT(ret >= 0, "Cannot lock data mutex: %d", ret);

	status = bt_gatt_attr_read(conn, attr, buf, len, offset,
				   status_str->buf,
				   strnlen(status_str->buf, status_str->size));

	ret = k_mutex_unlock(status_str->mtx);
	__ASSERT(ret >= 0, "Cannot unlock data mutex: %d", ret);

	return status;
}


int bt_nsms_set_status(const struct bt_nsms *nsms_obj, const char *status)
{
	int ret;
	const struct bt_nsms_status_str *status_str = nsms_obj->status_attr->user_data;

	ret = k_mutex_lock(status_str->mtx, NSMS_STATUS_LOCK_WAIT_TIME);
	__ASSERT(ret >= 0, "Cannot lock data mutex: %d", ret);

	(void)strncpy(status_str->buf, status, status_str->size);

	if (strlen(status) > status_str->size) {
		/* Note: we can fill the whole buffer and do not need NULL termination
		 * in such situation. So no warning is generated when message fits the buffer
		 * without the NULL terminating character.
		 */
		LOG_WRN("The status message too long and cropped");
	}

	ret = k_mutex_unlock(status_str->mtx);
	__ASSERT(ret >= 0, "Cannot unlock data mutex: %d", ret);

	LOG_DBG("Notifying about status change");
	return bt_gatt_notify(NULL,
			      nsms_obj->status_attr,
			      status,
			      strnlen(status, status_str->size));
}
