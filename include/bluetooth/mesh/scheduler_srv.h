/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_scheduler_srv Scheduler Server model
 * @{
 * @brief API for the Scheduler Server model.
 */

#ifndef BT_MESH_SCHEDULER_SRV_H__
#define BT_MESH_SCHEDULER_SRV_H__

#include <zephyr/kernel.h>
#include <bluetooth/mesh/time_srv.h>
#include <bluetooth/mesh/scene_srv.h>
#include <bluetooth/mesh/scheduler.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_scheduler_srv;

/** @def BT_MESH_SCHEDULER_SRV_INIT
 *
 * @brief Initialization parameters for @ref bt_mesh_scheduler_srv.
 *
 * @param[in] _action_set_cb Action Set callback.
 * @param[in] _time_srv      Timer server that is used for action scheduling.
 */
#define BT_MESH_SCHEDULER_SRV_INIT(_action_set_cb, _time_srv)        \
	{                                                                      \
		.action_set_cb = _action_set_cb, .time_srv = _time_srv,        \
	}

/** @def BT_MESH_MODEL_SCHEDULER_SRV
 *
 *  @brief Scheduler Server model composition data entry.
 *
 *  @note If the Scheduler model is registered on the element then
 *        the Scene model shouldn't. The Scheduler model includes already
 *        the Scene model.
 *
 *  @param[in] _srv Pointer to a @ref bt_mesh_scheduler_srv instance.
 */
#define BT_MESH_MODEL_SCHEDULER_SRV(_srv)                                      \
	BT_MESH_MODEL_SCENE_SRV(&(_srv)->scene_srv),                           \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_SCHEDULER_SRV,                       \
			_bt_mesh_scheduler_srv_op,                             \
			&(_srv)->pub,                                          \
			BT_MESH_MODEL_USER_DATA(struct bt_mesh_scheduler_srv,  \
						_srv),                         \
			&_bt_mesh_scheduler_srv_cb),                           \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_SCHEDULER_SETUP_SRV,                    \
			_bt_mesh_scheduler_setup_srv_op,                       \
			NULL,                                                  \
			BT_MESH_MODEL_USER_DATA(struct bt_mesh_scheduler_srv,  \
						_srv),						   \
			&_bt_mesh_scheduler_setup_srv_cb)

/** Scheduler Server model instance */
struct bt_mesh_scheduler_srv {
	/** Model state related structure of the Scheduler Server instance. */
	struct {
		/* Scheduled work. */
		struct k_work_delayable delayed_work;
		/* Calculated TAI-time for action items. */
		struct bt_mesh_time_tai
		sched_tai[BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT];
		/* Index of the ongoing action. */
		uint8_t idx;
		/* Index of the last executed action. */
		uint8_t last_idx;
		/* Bit field indicating active entries
		 * in the Schedule Register.
		 */
		uint16_t active_bitmap;
		/* The Schedule Register state is a 16-entry,
		 * zero-based, indexed array
		 */
		struct bt_mesh_schedule_entry
		sch_reg[BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT];
	};
	/** Composition data model pointer. */
	struct bt_mesh_model *model;
	/** Publication state. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[
		BT_MESH_MODEL_BUF_LEN(BT_MESH_SCHEDULER_OP_ACTION_STATUS,
			BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS)];
	/** Referenced time server to get time for scheduling. */
	struct bt_mesh_time_srv *time_srv;
	/** Scene server instance that is extended by the Scheduler model. */
	struct bt_mesh_scene_srv scene_srv;

	/** @brief Scheduler action set callback.
	 *
	 *  Called whenever the server receives an action set both ack or unack
	 *
	 *  @note This handler is optional.
	 *
	 *  @param[in]  srv   Scheduler server instance.
	 *  @param[in]  ctx   Context information about received message.
	 *  @param[in]  idx   Scheduler Register state index.
	 *  @param[in]  entry Scheduler Register state entry.
	 */
	void (*const action_set_cb)(struct bt_mesh_scheduler_srv *srv,
				    struct bt_mesh_msg_ctx *ctx,
				    uint8_t idx,
				    struct bt_mesh_schedule_entry *entry);
};

/** @brief Update time of the scheduled action.
 *
 *  @note This API is required to update time if referenced time server updated
 *  Time Zone @ref BT_MESH_TIME_SRV_ZONE_UPDATE or
 *  UTC @ref BT_MESH_TIME_SRV_UTC_UPDATE.
 *
 *  @param[in] srv Scheduler server model.
 *
 *  @retval 0 Successfully updated time of the ongoing action.
 *  @retval -EINVAL Invalid parameters.
 */
int bt_mesh_scheduler_srv_time_update(struct bt_mesh_scheduler_srv *srv);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_cb _bt_mesh_scheduler_srv_cb;
extern const struct bt_mesh_model_op _bt_mesh_scheduler_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_scheduler_setup_srv_cb;
extern const struct bt_mesh_model_op _bt_mesh_scheduler_setup_srv_op[];
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_SCHEDULER_SRV_H__ */

/** @} */
