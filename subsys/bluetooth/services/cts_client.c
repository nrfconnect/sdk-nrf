/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <bluetooth/services/cts_client.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cts_client, CONFIG_BT_CTS_CLIENT_LOG_LEVEL);

/* The lowest valid Current Time year is the year when the Western calendar was introduced. */
#define CTS_YEAR_MIN 1582
/* The highest possible Current Time year. */
#define CTS_YEAR_MAX 9999

/* |     Year        |Month   |Day     |Hours   |Minutes |Seconds |Weekday |Fraction|Reason  |
 * |     2 bytes     |1 byte  |1 byte  |1 byte  |1 byte  |1 byte  |1 byte  |1 byte  |1 byte  | = 10 bytes.
 */
#define CTS_C_CURRENT_TIME_EXPECTED_LENGTH 10

enum {
	CTS_ASYNC_READ_PENDING,
	CTS_NOTIF_ENABLED
};

static void cts_reinit(struct bt_cts_client *cts_c)
{
	cts_c->conn = NULL;
	cts_c->handle_ct = 0;
	cts_c->handle_ct_ccc = 0;
	cts_c->handle_lti = 0;
	cts_c->state = ATOMIC_INIT(0);
}

int bt_cts_client_init(struct bt_cts_client *cts_c)
{
	if (!cts_c) {
		return -EINVAL;
	}

	memset(cts_c, 0, sizeof(struct bt_cts_client));

	return 0;
}

int bt_cts_handles_assign(struct bt_gatt_dm *dm, struct bt_cts_client *cts_c)
{
	const struct bt_gatt_dm_attr *gatt_service_attr =
		bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
		bt_gatt_dm_attr_service_val(gatt_service_attr);
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_CTS)) {
		return -ENOTSUP;
	}

	LOG_DBG("CTS found");

	cts_reinit(cts_c);

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_CTS_CURRENT_TIME);
	if (!gatt_chrc) {
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
					    BT_UUID_CTS_CURRENT_TIME);
	if (!gatt_desc) {
		return -EINVAL;
	}
	cts_c->handle_ct = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		return -EINVAL;
	}
	cts_c->handle_ct_ccc = gatt_desc->handle;

	LOG_DBG("Current Time characteristic found.");

	/* Try to locate optional Local Time Information characteristic. */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_GATT_LTI);
	if (gatt_chrc) {
		gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_LTI);
		if (gatt_desc) {
			cts_c->handle_lti = gatt_desc->handle;
			LOG_DBG("Local Time Information characteristic found. handle: %u",
				cts_c->handle_lti);
		}
	}

	/* Finally - save connection object */
	cts_c->conn = bt_gatt_dm_conn_get(dm);

	return 0;
}

/**@brief Function for decoding a read from the Current Time characteristic.
 *
 * @param[in] time    Current Time structure.
 * @param[in] data    Pointer to the buffer containing the Current Time.
 * @param[in] length  Length of the buffer containing the Current Time.
 *
 * @retval 0        If the time struct is valid.
 * @retval -EINVAL 	If the length does not match the expected size of the data.
 */
static int current_time_decode(struct bt_cts_current_time *time,
			       uint8_t const *data, uint32_t const length)
{
	if (length != CTS_C_CURRENT_TIME_EXPECTED_LENGTH) {
		/* Return to prevent accessing the out-of-bounds data. */
		return -EINVAL;
	}

	/* Date. */
	time->exact_time_256.year = sys_get_le16(data);
	data += sizeof(uint16_t);
	time->exact_time_256.month = *data++;
	time->exact_time_256.day = *data++;
	time->exact_time_256.hours = *data++;
	time->exact_time_256.minutes = *data++;
	time->exact_time_256.seconds = *data++;

	/* Day of week. */
	time->exact_time_256.day_of_week = *data++;

	/* Fractions of a second. */
	time->exact_time_256.fractions256 = *data++;

	/* Reason for updating the time. */
	time->adjust_reason.manual_time_update = *data & BIT(0);
	time->adjust_reason.external_reference_time_update =
		(*data & BIT(1)) ? 1 : 0;
	time->adjust_reason.change_of_time_zone = (*data & BIT(2)) ? 1 : 0;
	time->adjust_reason.change_of_daylight_savings_time =
		(*data & BIT(3)) ? 1 : 0;

	return 0;
}

/**@brief Function for sanity check of a Current Time structure.
 *
 * @param[in] time  Current Time structure.
 *
 * @retval 0        If the time struct is valid.
 * @retval -EINVAL  If the time is out of bounds.
 */
static int current_time_validate(struct bt_cts_current_time *time)
{
	if ((time->exact_time_256.year > CTS_YEAR_MAX) ||
	    ((time->exact_time_256.year < CTS_YEAR_MIN) &&
	     (time->exact_time_256.year != 0)) ||
	    time->exact_time_256.month > 12 || time->exact_time_256.day > 31 ||
	    time->exact_time_256.hours > 23 ||
	    time->exact_time_256.minutes > 59 ||
	    time->exact_time_256.seconds > 59 ||
	    time->exact_time_256.day_of_week > 7) {
		return -EINVAL;
	}

	return 0;
}

/**@brief Function for handling of Current Time read response.
 *
 * @param[in] conn    Connection object.
 * @param[in] err     ATT error code.
 * @param[in] params  Read parameters used.
 * @param[in] data    Pointer to the data buffer.
 * @param[in] length  The size of the received data.
 *
 * @retval BT_GATT_ITER_STOP  Stop reading
 */
static uint8_t bt_cts_read_callback(struct bt_conn *conn, uint8_t err,
				    struct bt_gatt_read_params *params,
				    const void *data, uint16_t length)
{
	struct bt_cts_client *cts_c;
	struct bt_cts_current_time current_time;
	bt_cts_read_cb read_cb;
	int err_cb;

	/* Retrieve CTS client module context. */
	cts_c = CONTAINER_OF(params, struct bt_cts_client, read_params);

	if (!err) {
		err_cb = current_time_decode(&current_time, data, length);
		if (!err_cb) {
			/* Verify that the time is valid. */
			err_cb = current_time_validate(&current_time);
		}
	} else {
		err_cb = err;
	}

	read_cb = cts_c->read_cb;
	atomic_clear_bit(&cts_c->state, CTS_ASYNC_READ_PENDING);
	cts_c->read_cb(cts_c, &current_time, err_cb);

	return BT_GATT_ITER_STOP;
}

/**@brief Function for handling of Current Time notifications.
 *
 * @param[in] conn    Connection object.
 * @param[in] params  Notification parameters used.
 * @param[in] data    Pointer to the data buffer.
 * @param[in] length  The size of the received data.
 *
 * @retval BT_GATT_ITER_CONTINUE  Continue notification
 */
static uint8_t bt_cts_notify_callback(struct bt_conn *conn,
				      struct bt_gatt_subscribe_params *params,
				      const void *data, uint16_t length)
{
	struct bt_cts_client *cts_c;
	struct bt_cts_current_time current_time;
	int err;

	/* Retrieve CTS client module context. */
	cts_c = CONTAINER_OF(params, struct bt_cts_client, notify_params);

	err = current_time_decode(&current_time, data, length);
	if (!err) {
		err = current_time_validate(&current_time);
	}

	if (!err && cts_c->notify_cb) {
		cts_c->notify_cb(cts_c, &current_time);
	}

	return BT_GATT_ITER_CONTINUE;
}

/**@brief Function for decoding a read from the Local Time Information characteristic.
 *
 * @param[in] lti     Local Time Information structure.
 * @param[in] data    Pointer to the buffer containing the Local Time Information.
 * @param[in] length  Length of the buffer containing the Local Time Information.
 *
 * @retval 0        If the lti struct is valid.
 * @retval -EINVAL  If the length does not match the expected size of the data.
 */
static int local_time_info_decode(struct bt_cts_local_time_info *lti, uint8_t const *data,
				  uint32_t const length)
{
	if (length < 2) {
		return -EINVAL;
	}

	/* Time zone in 15 minute increments (signed 8-bit) or BT_CTS_TZ_UNKNOWN. */
	int8_t tz = (int8_t)data[0];
	/* DST offset (uint8_t as defined in header enum). */
	uint8_t dst_u = data[1];

	/* Validate timezone: allowed values are -128 (unknown) or -48..56 (inclusive). */
	if ((tz != BT_CTS_TZ_UNKNOWN) && (tz < -48 || tz > 56)) {
		return -EINVAL;
	}

	/* Validate DST offset according to spec enumerations:
	 * Allowed: 0 (Standard), 2 (Half hour), 4 (Daylight), 8 (Double daylight),
	 * 255 = DST is not known. Other values are reserved/invalid.
	 */
	if (!(dst_u == 0 || dst_u == 2 || dst_u == 4 || dst_u == 8 || dst_u == 255)) {
		return -EINVAL;
	}

	lti->timezone = tz;
	lti->dst = (bt_cts_dst_offset_t)dst_u;

	return 0;
}

/**@brief Function for handling of Local Time Information read response.
 *
 * @param[in] conn    Connection object.
 * @param[in] err     ATT error code.
 * @param[in] params  Read parameters used.
 * @param[in] data    Pointer to the data buffer.
 * @param[in] length  The size of the received data.
 *
 * @retval BT_GATT_ITER_STOP  Stop reading
 */
static uint8_t bt_cts_read_lti_callback(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_read_params *params, const void *data,
					uint16_t length)
{
	struct bt_cts_client *cts_c;
	struct bt_cts_local_time_info lti;
	bt_cts_lti_read_cb read_cb;
	int err_cb;

	/* Retrieve CTS client module context. */
	cts_c = CONTAINER_OF(params, struct bt_cts_client, read_params_lti);

	if (!err) {
		err_cb = local_time_info_decode(&lti, data, length);
	} else {
		err_cb = err;
	}

	read_cb = cts_c->read_lti_cb;
	atomic_clear_bit(&cts_c->state, CTS_ASYNC_READ_PENDING);
	if (read_cb) {
		cts_c->read_lti_cb(cts_c, &lti, err_cb);
	}

	return BT_GATT_ITER_STOP;
}

int bt_cts_read_current_time(struct bt_cts_client *cts_c, bt_cts_read_cb func)
{
	int err;

	if (!cts_c || !func) {
		return -EINVAL;
	}
	if (!cts_c->conn) {
		return -EINVAL;
	}
	if (atomic_test_and_set_bit(&cts_c->state, CTS_ASYNC_READ_PENDING)) {
		return -EBUSY;
	}

	cts_c->read_cb = func;

	cts_c->read_params.func = bt_cts_read_callback;
	cts_c->read_params.handle_count = 1;
	cts_c->read_params.single.handle = cts_c->handle_ct;
	cts_c->read_params.single.offset = 0;

	err = bt_gatt_read(cts_c->conn, &cts_c->read_params);
	if (err) {
		atomic_clear_bit(&cts_c->state, CTS_ASYNC_READ_PENDING);
	}

	return err;
}

int bt_cts_read_local_time_info(struct bt_cts_client *cts_c, bt_cts_lti_read_cb func)
{
	int err;

	if (!cts_c || !func) {
		return -EINVAL;
	}
	if (!cts_c->conn) {
		return -EINVAL;
	}
	if (!cts_c->handle_lti) {
		/* Characteristic not present on peer. */
		return -ENOTSUP;
	}
	if (atomic_test_and_set_bit(&cts_c->state, CTS_ASYNC_READ_PENDING)) {
		return -EBUSY;
	}

	cts_c->read_lti_cb = func;

	cts_c->read_params_lti.func = bt_cts_read_lti_callback;
	cts_c->read_params_lti.handle_count = 1;
	cts_c->read_params_lti.single.handle = cts_c->handle_lti;
	cts_c->read_params_lti.single.offset = 0;

	err = bt_gatt_read(cts_c->conn, &cts_c->read_params_lti);
	if (err) {
		atomic_clear_bit(&cts_c->state, CTS_ASYNC_READ_PENDING);
	}

	return err;
}

int bt_cts_subscribe_current_time(struct bt_cts_client *cts_c,
				  bt_cts_notify_cb func)
{
	int err;

	if (!cts_c || !func) {
		return -EINVAL;
	}
	if (!cts_c->conn) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&cts_c->state, CTS_NOTIF_ENABLED)) {
		return -EALREADY;
	}

	cts_c->notify_cb = func;

	cts_c->notify_params.notify = bt_cts_notify_callback;
	cts_c->notify_params.value = BT_GATT_CCC_NOTIFY;
	cts_c->notify_params.value_handle = cts_c->handle_ct;
	cts_c->notify_params.ccc_handle = cts_c->handle_ct_ccc;
	atomic_set_bit(cts_c->notify_params.flags,
		       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	err = bt_gatt_subscribe(cts_c->conn, &cts_c->notify_params);
	if (err) {
		atomic_clear_bit(&cts_c->state, CTS_NOTIF_ENABLED);
	}

	return err;
}

int bt_cts_unsubscribe_current_time(struct bt_cts_client *cts_c)
{
	int err;

	if (!cts_c) {
		return -EINVAL;
	}

	if (!atomic_test_bit(&cts_c->state, CTS_NOTIF_ENABLED)) {
		return -EFAULT;
	}

	err = bt_gatt_unsubscribe(cts_c->conn, &cts_c->notify_params);

	atomic_clear_bit(&cts_c->state, CTS_NOTIF_ENABLED);

	return err;
}
