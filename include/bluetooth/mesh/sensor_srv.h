/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @defgroup bt_mesh_sensor_srv Sensor Server model
 *  @{
 *  @brief API for the Sensor Server.
 */

#ifndef BT_MESH_SENSOR_SRV_H__
#define BT_MESH_SENSOR_SRV_H__

#include <bluetooth/mesh/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_sensor_srv;

/** @def BT_MESH_SENSOR_SRV_INIT
 *
 *  @brief Initialization parameters for @ref bt_mesh_sensor_srv.
 *
 *  @note A sensor server can only represent one sensor instance of each
 *        sensor type. Duplicate sensors will be ignored.
 *
 *  @param[in] _sensors Array of pointers to sensors owned by this server.
 *  @param[in] _count Number of sensors in the array. Can at most be
 *                    @kconfig{CONFIG_BT_MESH_SENSOR_SRV_SENSORS_MAX}.
 */
#define BT_MESH_SENSOR_SRV_INIT(_sensors, _count)                              \
	{                                                                      \
		.sensor_array = _sensors,                                      \
		.sensor_count =                                                \
			MIN(CONFIG_BT_MESH_SENSOR_SRV_SENSORS_MAX, _count),    \
	}

/** @def BT_MESH_MODEL_SENSOR_SRV
 *
 *  @brief Sensor Server model composition data entry.
 *
 *  @param[in] _srv Pointer to a @ref bt_mesh_sensor_srv instance.
 */
#define BT_MESH_MODEL_SENSOR_SRV(_srv)                                         \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_SENSOR_SRV, _bt_mesh_sensor_srv_op,  \
			 &(_srv)->pub,                                         \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_sensor_srv,    \
						 _srv),                        \
			 &_bt_mesh_sensor_srv_cb),                             \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_SENSOR_SETUP_SRV,                    \
			 _bt_mesh_sensor_setup_srv_op,                         \
			 &(_srv)->setup_pub,                                   \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_sensor_srv,    \
					      _srv),                           \
			 &_bt_mesh_sensor_setup_srv_cb)

/** Sensor server instance. */
struct bt_mesh_sensor_srv {
	/** Sensors owned by this server. */
	struct bt_mesh_sensor *const *sensor_array;
	/** Ordered linked list of sensors. */
	sys_slist_t sensors;
	/** Publish sequence counter */
	uint16_t seq;
	/** Number of sensors. */
	uint8_t sensor_count;

	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_SENSOR_OP_STATUS,
		(CONFIG_BT_MESH_SENSOR_SRV_SENSORS_MAX *
		 BT_MESH_SENSOR_STATUS_MAXLEN))];
	/** Publish parameters for the setup server. */
	struct bt_mesh_model_pub setup_pub;
	/* Publication buffer */
	struct net_buf_simple setup_pub_buf;
	/* Publication data */
	uint8_t setup_pub_data[MAX(
		BT_MESH_MODEL_BUF_LEN(
			BT_MESH_SENSOR_OP_SETTING_STATUS,
			BT_MESH_SENSOR_MSG_MAXLEN_SETTING_STATUS),
		BT_MESH_MODEL_BUF_LEN(
			BT_MESH_SENSOR_OP_CADENCE_STATUS,
			BT_MESH_SENSOR_MSG_MAXLEN_CADENCE_STATUS))];
	/** Composition data model pointer. */
	struct bt_mesh_model *model;
};

/** @brief Publish a sensor value.
 *
 *  Immediately publishes the given sensor value, without checking thresholds
 *  or intervals.
 *
 *  @see bt_mesh_sensor_srv_pub
 *
 *  @param[in] srv    Sensor server instance.
 *  @param[in] ctx    Message context to publish with, or NULL to publish on the
 *                    configured publish parameters.
 *  @param[in] sensor Sensor to publish with.
 *  @param[in] value  Sensor value to publish, interpreted as an array of sensor
 *                    channel values matching the sensor channels specified by
 *                    the sensor type. The length of the array must match the
 *                    sensor channel count.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_sensor_srv_pub(struct bt_mesh_sensor_srv *srv,
			   struct bt_mesh_msg_ctx *ctx,
			   struct bt_mesh_sensor *sensor,
			   const struct sensor_value *value);


/** @brief Make the server to take a sample of the sensor, and publish if the
 *         value changed sufficiently.
 *
 *  Works the same way as @ref bt_mesh_sensor_srv_pub(), except that the server
 *  makes a decision on whether to publish the value based on the delta from the
 *  previous publication and the sensor's threshold parameters. Only single
 *  channel sensor values will be considered.
 *
 *  @param[in] srv    Sensor server instance.
 *  @param[in] sensor Sensor instance to sample.
 *
 *  @retval 0              The sensor value was published.
 *  @retval -EBUSY         Failed sampling the sensor value.
 *  @retval -EALREADY      The sensor value has not changed sufficiently to
 *                         require a publication.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 */
int bt_mesh_sensor_srv_sample(struct bt_mesh_sensor_srv *srv,
			      struct bt_mesh_sensor *sensor);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_cb _bt_mesh_sensor_srv_cb;
extern const struct bt_mesh_model_op _bt_mesh_sensor_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_sensor_setup_srv_cb;
extern const struct bt_mesh_model_op _bt_mesh_sensor_setup_srv_op[];
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_SENSOR_SRV_H__ */

/** @} */
