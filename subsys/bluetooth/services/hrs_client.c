/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/services/hrs_client.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hrs_client, CONFIG_BT_HRS_CLIENT_LOG_LEVEL);

#define HRS_MEASUREMENT_NOTIFY_ENABLED BIT(0)
#define HRS_MEASUREMENT_READ_IN_PROGRES BIT(1)
#define HRS_SENSOR_LOCATION_READ_IN_PROGRES BIT(2)
#define HRS_CONTROL_POINT_WRITE_PENDING BIT(3)

#define HRS_MEASUREMENT_FLAGS_VALUE_FORMAT      BIT(0)
#define HRS_MEASUREMENT_FLAGS_CONTACT_DETECTED  BIT(1)
#define HRS_MEASUREMENT_FLAGS_CONTACT_SUPPORTED BIT(2)
#define HRS_MEASUREMENT_FLAGS_ENERGY_EXPENDED   BIT(3)
#define HRS_MEASUREMENT_FLAGS_RR_INTERVALS      BIT(4)

static inline bool data_length_check(uint16_t current, uint16_t min_expected)
{
	return (current >= min_expected);
}

static void hrs_reinit(struct bt_hrs_client *hrs_c)
{
	hrs_c->measurement_char.handle = 0;
	hrs_c->measurement_char.ccc_handle = 0;
	hrs_c->measurement_char.notify_cb = NULL;

	hrs_c->sensor_location_char.handle = 0;
	hrs_c->sensor_location_char.read_cb = NULL;

	hrs_c->cp_char.handle = 0;

	hrs_c->conn = NULL;
	hrs_c->state = ATOMIC_INIT(0);
}

static int hrs_measurement_data_parse(struct bt_hrs_client_measurement *meas,
				      const uint8_t *data, uint16_t length)
{
	uint8_t flags;

	if (!data_length_check(length, sizeof(uint8_t))) {
		return -EMSGSIZE;
	}

	flags = *data++;

	meas->flags.value_format = (flags & HRS_MEASUREMENT_FLAGS_VALUE_FORMAT) ? 1 : 0;
	meas->flags.sensor_contact_detected = (flags & HRS_MEASUREMENT_FLAGS_CONTACT_DETECTED) ?
						1 : 0;
	meas->flags.sensor_contact_supported = (flags & HRS_MEASUREMENT_FLAGS_CONTACT_SUPPORTED) ?
						1 : 0;
	meas->flags.energy_expended_present = (flags & HRS_MEASUREMENT_FLAGS_ENERGY_EXPENDED) ?
						1 : 0;
	meas->flags.rr_intervals_present = (flags & HRS_MEASUREMENT_FLAGS_RR_INTERVALS) ? 1 : 0;

	length--;

	if (meas->flags.value_format) {
		if (!data_length_check(length, sizeof(uint16_t))) {
			return -EMSGSIZE;
		}

		meas->hr_value = sys_get_le16(data);
		data += sizeof(uint16_t);
		length -= sizeof(uint16_t);
	} else {
		if (!data_length_check(length, sizeof(uint8_t))) {
			return -EMSGSIZE;
		}

		meas->hr_value = *data++;
		length -= sizeof(uint8_t);
	}

	if (meas->flags.energy_expended_present) {
		if (!data_length_check(length, sizeof(uint16_t))) {
			return -EMSGSIZE;
		}

		meas->energy_expended = sys_get_le16(data);
		data += sizeof(uint16_t);
		length -= sizeof(uint16_t);
	}

	if (meas->flags.rr_intervals_present) {
		meas->rr_intervals_count = length / sizeof(uint16_t);

		if (meas->rr_intervals_count > ARRAY_SIZE(meas->rr_intervals)) {
			return -ENOMEM;
		}

		for (size_t i = 0; i < meas->rr_intervals_count; i++) {
			meas->rr_intervals[i] = sys_get_le16(data);
			data += sizeof(uint16_t);
		}
	}

	return 0;
}

static uint8_t on_hrs_sensor_location_read(struct bt_conn *conn, uint8_t err,
					   struct bt_gatt_read_params *params,
					   const void *data, uint16_t length)
{
	int err_cb = 0;
	struct bt_hrs_client *hrs_c;
	bt_hrs_client_read_sensor_location_cb read_cb;
	uint8_t location = (uint8_t)BT_HRS_CLIENT_SENSOR_LOCATION_OTHER;

	hrs_c = CONTAINER_OF(params, struct bt_hrs_client, sensor_location_char.read_params);

	if (err) {
		LOG_ERR("Read value error: %d", err);
		err_cb = err;
	} else if (length != sizeof(uint8_t)) {
		err_cb = -EMSGSIZE;
	} else {
		location = *((uint8_t *)data);
	}

	read_cb = hrs_c->sensor_location_char.read_cb;
	atomic_clear_bit(&hrs_c->state, HRS_SENSOR_LOCATION_READ_IN_PROGRES);

	if (read_cb) {
		read_cb(hrs_c, (enum bt_hrs_client_sensor_location)location, err_cb);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t on_hrs_measurement_notify(struct bt_conn *conn,
					 struct bt_gatt_subscribe_params *params,
					 const void *data, uint16_t length)
{
	int err;
	struct bt_hrs_client *hrs_c;
	struct bt_hrs_client_measurement hr_measurement;

	hrs_c = CONTAINER_OF(params, struct bt_hrs_client, measurement_char.notify_params);

	if (!data) {
		atomic_clear_bit(&hrs_c->state, HRS_MEASUREMENT_NOTIFY_ENABLED);
		LOG_DBG("[UNSUBSCRIBE] from Heart Rate Measurement characterictic");

		return BT_GATT_ITER_STOP;
	}

	LOG_HEXDUMP_DBG(data, length, "[NOTIFICATION] Heart Rate Measurement: ");

	err = hrs_measurement_data_parse(&hr_measurement, data, length);

	if (hrs_c->measurement_char.notify_cb) {
		hrs_c->measurement_char.notify_cb(hrs_c, &hr_measurement, err);
	}

	return BT_GATT_ITER_CONTINUE;
}

static void on_cp_write(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_write_params *params)
{
	struct bt_hrs_client *hrs_c;
	bt_hrs_client_write_cb write_cb;

	hrs_c = CONTAINER_OF(params, struct bt_hrs_client, cp_char.write_params);

	write_cb = hrs_c->cp_char.write_cb;
	atomic_clear_bit(&hrs_c->state, HRS_CONTROL_POINT_WRITE_PENDING);

	if (write_cb) {
		write_cb(hrs_c, err);
	}
}

int bt_hrs_client_measurement_subscribe(struct bt_hrs_client *hrs_c,
					bt_hrs_client_notify_cb notify_cb)
{
	int err;
	struct bt_gatt_subscribe_params *params = &hrs_c->measurement_char.notify_params;

	if (!hrs_c || !notify_cb) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&hrs_c->state, HRS_MEASUREMENT_NOTIFY_ENABLED)) {
		LOG_ERR("Heart Rate Measurement characterisic notifications already enabled.");
		return -EALREADY;
	}

	hrs_c->measurement_char.notify_cb = notify_cb;

	params->ccc_handle = hrs_c->measurement_char.ccc_handle;
	params->value_handle = hrs_c->measurement_char.handle;
	params->value = BT_GATT_CCC_NOTIFY;
	params->notify = on_hrs_measurement_notify;

	atomic_set_bit(params->flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	err = bt_gatt_subscribe(hrs_c->conn, params);
	if (err) {
		atomic_clear_bit(&hrs_c->state, HRS_MEASUREMENT_NOTIFY_ENABLED);
		LOG_ERR("Subscribe to Heart Rate Measurement characteristic failed");
	} else {
		LOG_DBG("Subscribed to Heart Rate Measurement characteristic");
	}

	return err;
}

int bt_hrs_client_measurement_unsubscribe(struct bt_hrs_client *hrs_c)
{
	int err;

	if (!hrs_c) {
		return -EINVAL;
	}

	if (!atomic_test_bit(&hrs_c->state, HRS_MEASUREMENT_NOTIFY_ENABLED)) {
		return -EFAULT;
	}

	err = bt_gatt_unsubscribe(hrs_c->conn, &hrs_c->measurement_char.notify_params);
	if (err) {
		LOG_ERR("Unsubscribing from Heart Rate Measurement characteristic failed, err: %d",
			err);
	} else {
		atomic_clear_bit(&hrs_c->state, HRS_MEASUREMENT_NOTIFY_ENABLED);
		LOG_DBG("Unsubscribed from Heart Rate Measurement characteristic");
	}

	return err;
}

int bt_hrs_client_sensor_location_read(struct bt_hrs_client *hrs_c,
				      bt_hrs_client_read_sensor_location_cb read_cb)
{
	int err;
	struct bt_gatt_read_params *params;

	if (!hrs_c || !read_cb) {
		return -EINVAL;
	}

	if (!bt_hrs_client_has_sensor_location(hrs_c)) {
		return -ENOTSUP;
	}

	if (atomic_test_and_set_bit(&hrs_c->state, HRS_SENSOR_LOCATION_READ_IN_PROGRES)) {
		return -EBUSY;
	}

	hrs_c->sensor_location_char.read_cb = read_cb;
	params = &hrs_c->sensor_location_char.read_params;

	params->handle_count = 1;
	params->single.handle = hrs_c->sensor_location_char.handle;
	params->single.offset = 0;
	params->func = on_hrs_sensor_location_read;

	err = bt_gatt_read(hrs_c->conn, params);
	if (err) {
		LOG_ERR("Reading Heart Rate Measurement characteristic failed, err: %d", err);
		atomic_clear_bit(&hrs_c->state, HRS_SENSOR_LOCATION_READ_IN_PROGRES);
	}

	return err;
}

bool bt_hrs_client_has_sensor_location(struct bt_hrs_client *hrs_c)
{
	return (hrs_c->sensor_location_char.handle != 0);
}

int bt_hrs_client_control_point_write(struct bt_hrs_client *hrs_c,
				      enum bt_hrs_client_cp_value value,
				      bt_hrs_client_write_cb write_cb)
{
	int err;
	struct bt_gatt_write_params *params;

	if (!hrs_c) {
		return -EINVAL;
	}

	if (!bt_hrs_client_has_control_point(hrs_c)) {
		return -ENOTSUP;
	}

	if (atomic_test_and_set_bit(&hrs_c->state, HRS_CONTROL_POINT_WRITE_PENDING)) {
		return -EBUSY;
	}

	hrs_c->cp_char.write_cb = write_cb;

	params = &hrs_c->cp_char.write_params;

	params->data = (uint8_t *)&value;
	params->length = sizeof(uint8_t);
	params->offset = 0;
	params->handle = hrs_c->cp_char.handle;
	params->func = on_cp_write;

	err = bt_gatt_write(hrs_c->conn, params);
	if (err) {
		LOG_ERR("Writing Heart Rate Control Point characteristic failed, err %d", err);
		atomic_clear_bit(&hrs_c->state, HRS_CONTROL_POINT_WRITE_PENDING);
	}

	return err;
}

bool bt_hrs_client_has_control_point(struct bt_hrs_client *hrs_c)
{
	return (hrs_c->cp_char.handle != 0);
}

int bt_hrs_client_handles_assign(struct bt_gatt_dm *dm, struct bt_hrs_client *hrs_c)
{
	const struct bt_gatt_dm_attr *gatt_service_attr =
			bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
			bt_gatt_dm_attr_service_val(gatt_service_attr);
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	if (!dm || !hrs_c) {
		return -EINVAL;
	}

	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_HRS)) {
		return -ENOTSUP;
	}
	LOG_DBG("Getting handles from Heart Rate service.");

	hrs_reinit(hrs_c);

	/* Heart Rate Measurement characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_HRS_MEASUREMENT);
	if (!gatt_chrc) {
		LOG_ERR("No Heart Rate Measurement characteristic found.");
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
					    BT_UUID_HRS_MEASUREMENT);
	if (!gatt_desc) {
		LOG_ERR("No Heart Rate Measurement characteristic value found.");
		return -EINVAL;
	}
	hrs_c->measurement_char.handle = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		LOG_ERR("No Heart Rate Measurement CCC descriptor found.");
		return -EINVAL;
	}

	hrs_c->measurement_char.ccc_handle = gatt_desc->handle;

	LOG_DBG("Heart Rate Measurement characteristic found");

	/* Body Sensor Location characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_HRS_BODY_SENSOR);
	if (!gatt_chrc) {
		LOG_DBG("No Body Sensor Location characteristic found");
	} else {
		gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
						    BT_UUID_HRS_BODY_SENSOR);
		if (!gatt_desc) {
			LOG_ERR("No Body Sensor Location characteristic value found.");
			return -EINVAL;
		}

		hrs_c->sensor_location_char.handle = gatt_desc->handle;

		LOG_DBG("Body Sensor Location characteristic found");
	}

	/* Heart Rate Control Point characteristic. */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_HRS_CONTROL_POINT);
	if (!gatt_chrc) {
		LOG_DBG("No Heart Rate Control Point characteristic found");
	} else {
		gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
						    BT_UUID_HRS_CONTROL_POINT);
		if (!gatt_desc) {
			LOG_ERR("No Heart Rate Control Point characteristic value found.");
			return -EINVAL;
		}

		hrs_c->cp_char.handle = gatt_desc->handle;

		LOG_DBG("Heart Rate Control Point characteristic found");
	}

	/* Finally - save connection object */
	hrs_c->conn = bt_gatt_dm_conn_get(dm);

	return 0;
}

int bt_hrs_client_init(struct bt_hrs_client *hrs_c)
{
	if (!hrs_c) {
		return -EINVAL;
	}

	memset(hrs_c, 0, sizeof(*hrs_c));

	return 0;
}
