/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Work based on EnOcean Proxy Server model, Copyright (c) 2020 Silvair sp. z o.o.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_silvair_enocean_srv Silvair EnOcean Server model
 * @{
 * @brief API for the Silvair EnOcean Server.
 */

#ifndef BT_MESH_SILVAIR_ENOCEAN_SRV_H__
#define BT_MESH_SILVAIR_ENOCEAN_SRV_H__

#include <bluetooth/mesh/models.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BT_MESH_COMPANY_ID_SILVAIR 0x0136
#define BT_MESH_MODEL_ID_SILVAIR_ENOCEAN_SRV 0x0001
#define BT_MESH_SILVAIR_ENOCEAN_PROXY_BUTTONS 2

/** @cond INTERNAL_HIDDEN */

extern const struct bt_mesh_model_op _bt_mesh_silvair_enocean_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_silvair_enocean_srv_cb;

#define BT_MESH_SILVAIR_ENOCEAN_PROXY_OP BT_MESH_MODEL_OP_3(0xF4,              \
	BT_MESH_COMPANY_ID_SILVAIR)

#define BT_MESH_SILVAIR_ENOCEAN_PROXY_MSG_MINLEN 1
#define BT_MESH_SILVAIR_ENOCEAN_PROXY_MSG_MAXLEN 23
/** @endcond */

/** Silvair Enocean Proxy Server state machine state */
enum bt_mesh_silvair_enocean_state {
	/** Idle state */
	BT_MESH_SILVAIR_ENOCEAN_STATE_IDLE,
	/** Waiting state */
	BT_MESH_SILVAIR_ENOCEAN_STATE_WAIT,
	/** Phase A state */
	BT_MESH_SILVAIR_ENOCEAN_STATE_PHASE_A,
	/** Phase B state */
	BT_MESH_SILVAIR_ENOCEAN_STATE_PHASE_B,
	/** Phase C state */
	BT_MESH_SILVAIR_ENOCEAN_STATE_PHASE_C,
	/** Phase D state */
	BT_MESH_SILVAIR_ENOCEAN_STATE_PHASE_D
};

/** Silvair Enocean Proxy Server instance. */
struct bt_mesh_silvair_enocean_srv {
	/** State for each button controlled by this instance */
	struct bt_mesh_silvair_enocean_button {
		/** Generic Level Client instance */
		struct bt_mesh_lvl_cli lvl;
		/** Generic OnOff Client instance */
		struct bt_mesh_onoff_cli onoff;
		/** Current state of the state machine */
		enum bt_mesh_silvair_enocean_state state;
		/** Target on/off in the state machine */
		bool target;
		/** Tick counter */
		int tick_count;
		/** Flag for moving to phase D */
		bool release_pending;
		/** Current delta */
		int32_t delta;
		/** Timer used for state machine timeouts */
		struct k_work_delayable timer;
	} buttons[BT_MESH_SILVAIR_ENOCEAN_PROXY_BUTTONS];

	/** Enocean device address */
	bt_addr_le_t addr;
	/** Publish parameters */
	struct bt_mesh_model_pub pub;
	/** Publication buffer */
	struct net_buf_simple pub_buf;
	/** Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_SILVAIR_ENOCEAN_PROXY_OP,
		BT_MESH_SILVAIR_ENOCEAN_PROXY_MSG_MAXLEN)];
	/** Access model pointer. */
	const struct bt_mesh_model *mod;
	/** Entry in global list of Enocean Proxy models. */
	sys_snode_t entry;
};

/** @def BT_MESH_MODEL_SILVAIR_ENOCEAN_SRV
 *
 * @brief Silvair Enocean Proxy Server model entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_silvair_enocean_srv instance.
 */
#define BT_MESH_MODEL_SILVAIR_ENOCEAN_SRV(_srv)                                \
	BT_MESH_MODEL_VND_CB(BT_MESH_COMPANY_ID_SILVAIR,                       \
			     BT_MESH_MODEL_ID_SILVAIR_ENOCEAN_SRV,             \
			     _bt_mesh_silvair_enocean_srv_op, &(_srv)->pub,    \
			     BT_MESH_MODEL_USER_DATA(                          \
				struct bt_mesh_silvair_enocean_srv, _srv),     \
			     &_bt_mesh_silvair_enocean_srv_cb)

/** @def BT_MESH_MODEL_SILVAIR_ENOCEAN_BUTTON
 *
 * @brief Model entries for a button for a Silvair Enocean Proxy Server model.
 *
 * One of these should be added for each rocker switch handled by the
 * Silvair Enocean Proxy Server instance, each on its own element. This macro
 * includes the Generic Level Client and Generic OnOff Client.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_lightness_srv instance.
 * @param[in] _idx Button index for the model entries. Must be in the range
 * 0 to @c BT_MESH_SILVAIR_ENOCEAN_PROXY_BUTTONS.
 */
#define BT_MESH_MODEL_SILVAIR_ENOCEAN_BUTTON(_srv, _idx)                       \
		BT_MESH_MODEL_LVL_CLI(&(_srv)->buttons[_idx].lvl),             \
		BT_MESH_MODEL_ONOFF_CLI(&(_srv)->buttons[_idx].onoff)

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_SILVAIR_ENOCEAN_SRV_H__ */

/** @} */
