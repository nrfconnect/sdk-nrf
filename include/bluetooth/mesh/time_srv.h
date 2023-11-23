/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_time_srv Time Server model
 * @{
 * @brief API for the Time Server model.
 */

#ifndef BT_MESH_TIME_SRV_H__
#define BT_MESH_TIME_SRV_H__

#include <bluetooth/mesh/time.h>
#include <bluetooth/mesh/model_types.h>
#include <time.h>


#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_time_srv;
struct tm;

/** @def BT_MESH_TIME_SRV_INIT
 *
 * @brief Initialization parameters for a @ref bt_mesh_time_srv instance.
 *
 * @param[in] _time_update_cb Time update callback (optional).
 */
#define BT_MESH_TIME_SRV_INIT(_time_update_cb)                                 \
	{                                                                      \
		.time_update_cb = _time_update_cb,                             \
		.pub = { .update = _bt_mesh_time_srv_update_handler,           \
			 .msg = NET_BUF_SIMPLE(BT_MESH_MODEL_BUF_LEN(          \
				 BT_MESH_TIME_OP_TIME_SET,                     \
				 BT_MESH_TIME_MSG_LEN_TIME_SET)) },            \
	}

/** @def BT_MESH_MODEL_TIME_SRV
 *
 * @brief Time Server model composition data entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_time_srv instance.
 */
#define BT_MESH_MODEL_TIME_SRV(_srv)                                           \
	BT_MESH_MODEL_CB(                                                      \
		BT_MESH_MODEL_ID_TIME_SRV, _bt_mesh_time_srv_op, &(_srv)->pub, \
		BT_MESH_MODEL_USER_DATA(struct bt_mesh_time_srv, _srv),        \
		&_bt_mesh_time_srv_cb),                                        \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_TIME_SETUP_SRV,                      \
		_bt_mesh_time_setup_srv_op,                                    \
		&(_srv)->setup_pub,                                            \
		BT_MESH_MODEL_USER_DATA(                                       \
		struct bt_mesh_time_srv, _srv),                                \
		&_bt_mesh_time_setup_srv_cb)

/** Time srv update types */
enum bt_mesh_time_update_types {
	/** Status update */
	BT_MESH_TIME_SRV_STATUS_UPDATE,
	/** Set update */
	BT_MESH_TIME_SRV_SET_UPDATE,
	/** Zone update */
	BT_MESH_TIME_SRV_ZONE_UPDATE,
	/** UTC update */
	BT_MESH_TIME_SRV_UTC_UPDATE,
};

/** Internal data structure of the Time Server instance. */
struct bt_mesh_time_srv_data {
	/* State at last sync point */
	struct {
		/* Device uptime at last sync point */
		int64_t uptime;
		/* Time status at last sync point */
		struct bt_mesh_time_status status;
	} sync;
	/* Zone change update context */
	struct bt_mesh_time_zone_change time_zone_change;
	/* UTC-delta change update context */
	struct bt_mesh_time_tai_utc_change tai_utc_change;
	/* The Time Role of the Server instance */
	enum bt_mesh_time_role role;
	/* The timestamp of the last published Time Status. */
	int64_t timestamp;
};

/** Time Server instance.
 *
 * Should be initialized with @ref BT_MESH_TIME_SRV_INIT.
 */
struct bt_mesh_time_srv {
	/** Model entry. */
	const struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/** Setup model publish parameters */
	struct bt_mesh_model_pub setup_pub;
	/** Acknowledged message tracking. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
	/** Model state structure */
	struct bt_mesh_time_srv_data data;
	/** Delayable work to randomize status relaying. */
	struct k_work_delayable status_delay;

	/** @brief Update callback.
	 *
	 *  Called whenever the server receives a state update from either:
	 * - @c bt_mesh_time_cli_time_set
	 * - @c bt_mesh_time_cli_zone_set
	 * - @c bt_mesh_time_cli_tai_utc_delta_set
	 * - @c bt_mesh_time_srv_time_status_send
	 *
	 *  @note This handler is optional.
	 *
	 *  @param[in]  srv Server instance.
	 */
	void (*const time_update_cb)(struct bt_mesh_time_srv *srv,
				     struct bt_mesh_msg_ctx *ctx,
				     enum bt_mesh_time_update_types type);
};

/** @brief Publish an unsolicited Time Status message.
 *
 * @param[in] srv    Time Server instance.
 * @param[in] ctx    Message context, or NULL to publish with the configured
 *                   parameters.
 *
 * @return 0              Successfully published the current Time state.
 * @retval -EOPNOTSUPP    The server Time Role state does not support sending
 *                        of unsolicited Time Status messages.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                        not configured.
 * @retval -EAGAIN        The device has not been provisioned.
 */
int bt_mesh_time_srv_time_status_send(struct bt_mesh_time_srv *srv,
				      struct bt_mesh_msg_ctx *ctx);

/** @brief Get the time status at a given uptime
 *
 * Converts the device uptime (from @c k_uptime_get) to TAI time.
 *
 * @note This function does not support status lookups for uptimes
 * that occurred before the last received set- or status update. In
 * addition, lookups for uptimes that happened before @c k_uptime_get
 * must be used with caution, since there is no guarantee that the
 * output data is fully reliable.
 *
 * @param[in]  srv Time Server instance.
 * @param[in]  uptime Uptime at the desired point in time.
 * @param[out] status The device's Time Status at the given uptime.
 *
 * @retval 0          Successfully retrieved the Time status.
 * @retval -EAGAIN    Invalid uptime input. Asking for status that
 *                    is in the past.
 */
int bt_mesh_time_srv_status(struct bt_mesh_time_srv *srv, uint64_t uptime,
			    struct bt_mesh_time_status *status);

/** @brief Get the device uptime at a given local time
 *
 * @note This function does not support lookups for local time
 * that occurred before the TAI-time of the last received set- or status
 * update. In addition, lookups for local time that happened before
 * @c k_uptime_get must be used with caution, since there is no guarantee
 * that the output data is fully reliable.
 *
 * @note Whenever a zone or UTC change makes the local time jump forward
 * in time, i.e. for day light savings adjustments, we get a situation
 * where some points in time will occur twice on the local timeline.
 * If the user where to input a local time that is affected by these
 * changes the response uptime will reflect the input local time added
 * with the new adjustment.
 * Example: A time zone change is scheduled 12:00:00 a clock an arbitrary
 * day to set the local clock forward by one hour. If the user requests
 * the uptime for 12:00:05 at that same day, the returned uptime will be
 * the same as if the user inputted the local time 13:00:05 of that day.
 *
 * @note Whenever a zone or UTC change makes the local time jump
 * backwards in time, i.e. for day light savings adjustments, we get a
 * situation where some points in time never will on the local timeline.
 * If the user where to input a local time that is affected by these
 * changes the response uptime will reflect the local time instance that
 * happens last.
 * Example: A time zone change is scheduled 12:00:00 a clock an arbitrary
 * day to set the local clock back one hour. If the user requests the
 * uptime for 12:00:00 at that same day, the returned uptime will reflect
 * the local time that under normal circumstances would be 13:00:00 that
 * day.
 *
 * @param[in]  srv Time Server instance.
 * @param[in]  timeptr Local time at the desired point.
 *
 * @retval Positive uptime if successful, otherwise negative error value.
 */
int64_t bt_mesh_time_srv_mktime(struct bt_mesh_time_srv *srv, struct tm *timeptr);

/** @brief Get the device local time at a given uptime
 *
 * @note This function does not support local time lookups for uptimes
 * that occurred before the last received set- or status update. In
 * addition, lookups for uptimes that happened before @c k_uptime_get
 * must be used with caution, since there is no guarantee that the
 * output data is fully reliable.
 *
 * @param[in]  srv Time Server instance.
 * @param[in]  uptime Uptime at the desired point in time.
 * @param[out] timeptr The device's local time at the given uptime.
 *
 * @retval Pointer to time struct if successful, NULL otherwise.
 */
struct tm *bt_mesh_time_srv_localtime_r(struct bt_mesh_time_srv *srv,
					int64_t uptime, struct tm *timeptr);

/** @brief Get the device local time at a given uptime
 *
 * @note This function does not support local time lookups for uptimes
 * that occurred before the last received set- or status update. In
 * addition, lookups for uptimes that happened before @c k_uptime_get
 * must be used with caution, since there is no guarantee that the
 * output data is fully reliable.
 *
 * @param[in]  srv Time Server instance.
 * @param[in]  uptime Uptime at the desired point in time.
 *
 * @retval Pointer to time struct if successful, NULL otherwise.
 */
struct tm *bt_mesh_time_srv_localtime(struct bt_mesh_time_srv *srv,
				      int64_t uptime);

/** @brief Get the device time uncertainty at a given uptime
 *
 * @note This function does not support uncertainty lookups for uptimes
 * that occurred before the last received set- or status update.
 *
 * @param[in]  srv Time Server instance.
 * @param[in]  uptime Uptime at the desired point in time.
 *
 * @retval Positive uncertainty if successful, negative error code otherwise.
 */
uint64_t bt_mesh_time_srv_uncertainty_get(struct bt_mesh_time_srv *srv,
				       int64_t uptime);

/** @brief Set the TAI status parameters for the server.
 *
 * @note This is an alternative way to set parameters
 * for the Time Server instance. In most cases these parameters will be
 * configured remotely by a Time Client or a neigbouring Time Server.
 *
 * @param[in] srv    Time Server instance.
 * @param[in] uptime Desired uptime to set.
 * @param[in] status Desired status configuration.
 */
void bt_mesh_time_srv_time_set(struct bt_mesh_time_srv *srv, int64_t uptime,
			       const struct bt_mesh_time_status *status);

/** @brief Set the zone change parameters for the server.
 *
 * @note This is an alternative way to set parameters
 * for the Time Server instance. In most cases these parameters will be
 * configured remotely by a Time Client or a neigbouring Time Server.
 *
 * @param[in] srv  Time Server instance.
 * @param[in] data Desired zone change configuration.
 */
void bt_mesh_time_srv_time_zone_change_set(
	struct bt_mesh_time_srv *srv,
	const struct bt_mesh_time_zone_change *data);

/** @brief Set the TAI UTC change parameters for the server.
 *
 * @note This is an alternative way to set parameters
 * for the Time Server instance. In most cases these parameters will be
 * configured remotely by a Time Client or a neigbouring Time Server.
 *
 * @param[in] srv  Time Server instance.
 * @param[in] data Desired TAI UTC change configuration.
 */
void bt_mesh_time_srv_tai_utc_change_set(
	struct bt_mesh_time_srv *srv,
	const struct bt_mesh_time_tai_utc_change *data);

/** @brief Set the role parameter for the server.
 *
 * @note This is an alternative way to set parameters
 * for the Time Server instance. In most cases these parameters will be
 * configured remotely by a Time Client or a neigbouring Time Server.
 *
 * @param[in] srv  Time Server instance.
 * @param[in] role Desired role configuration.
 */
void bt_mesh_time_srv_role_set(struct bt_mesh_time_srv *srv,
			       enum bt_mesh_time_role role);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_time_srv_op[];
extern const struct bt_mesh_model_op _bt_mesh_time_setup_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_time_srv_cb;
extern const struct bt_mesh_model_cb _bt_mesh_time_setup_srv_cb;
int _bt_mesh_time_srv_update_handler(const struct bt_mesh_model *model);
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_TIME_SRV_H__ */

/** @} */
