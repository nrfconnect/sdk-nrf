/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_dm_srv Distance Measurement Server model
 * @{
 * @brief API for the Distance Measurement Server model.
 */

#ifndef BT_MESH_DM_SRV_H__
#define BT_MESH_DM_SRV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth/mesh/models.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/sys/slist.h>
#include <zephyr/bluetooth/mesh/access.h>
#include "dm_common.h"

/** Distance Measurement Server instance. */
struct bt_mesh_dm_srv {
	/** Transaction ID tracker. */
	struct bt_mesh_tid_ctx prev_transaction;
	/** Access model pointer. */
	struct bt_mesh_model *model;
	/** Flag indicating measurement in progress */
	bool is_busy;
	/** Default configuration */
	struct bt_mesh_dm_cfg cfg;
	/** Measurement ready semaphore */
	struct k_sem dm_ready_sem;
	/** Current measurement target address */
	uint16_t target_addr;
	/** Current measurement respond ctx */
	struct bt_mesh_msg_ctx rsp_ctx;
	/** Current role in measurement*/
	enum dm_dev_role current_role;
	/** Timeout work context */
	struct k_work_delayable timeout;

	struct {
		/** Pointer to result memory object */
		struct bt_mesh_dm_res_entry *res;
		/** Total number of entry spaces in memory object */
		uint8_t entry_cnt;
		/** Last working entry index */
		uint8_t last_entry_idx;
		/** Available valid entries */
		uint8_t available_entries;
	} results;
};

/** @def BT_MESH_MODEL_DM_SRV_INIT
 *
 * @brief Initialization parameters for a @ref bt_mesh_dm_srv
 * instance.
 *
 * @param[in] _mem Pointer to a @ref bt_mesh_dm_res_entry memory object.
 * @param[in] _cnt Array size of @ref bt_mesh_dm_res_entry memory object.
 */
#define BT_MESH_MODEL_DM_SRV_INIT(_mem, _cnt)                                            \
	{                                                                                          \
		.results.res = _mem,                                                               \
		.results.entry_cnt = _cnt,                                                         \
		.cfg.ttl = CONFIG_BT_MESH_DEFAULT_TTL,                                             \
		.cfg.timeout = CONFIG_BT_MESH_DM_SRV_DEFAULT_TIMEOUT,                            \
		.cfg.delay = CONFIG_BT_MESH_DM_SRV_REFLECTOR_DELAY,                      \
		.results.last_entry_idx = 0,                                                       \
		.results.available_entries = 0,                                                    \
	}

/** @def BT_MESH_MODEL_DM_SRV
 *
 * @brief Distance Measurement Server model entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_dm_srv instance.
 */
#define BT_MESH_MODEL_DM_SRV(_srv)                                                       \
	BT_MESH_MODEL_VND_CB(BT_MESH_VENDOR_COMPANY_ID, BT_MESH_MODEL_ID_DM_SRV,         \
			     _bt_mesh_dm_srv_op, NULL,                                   \
			     BT_MESH_MODEL_USER_DATA(struct bt_mesh_dm_srv, _srv),       \
			     &_bt_mesh_dm_srv_cb)

/** @cond INTERNAL_HIDDEN */

extern const struct bt_mesh_model_op _bt_mesh_dm_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_dm_srv_cb;

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_DM_SRV_H__ */

/** @} */
