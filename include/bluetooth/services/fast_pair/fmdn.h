/*
 * Copyright (c) 2024-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_SERVICES_FAST_PAIR_FMDN_H_
#define BT_SERVICES_FAST_PAIR_FMDN_H_

/**
 * @file
 * @brief Deprecated FMDN header. Use <bluetooth/fast_pair/fhn/fhn.h> instead.
 *
 * This header provides backward-compatible aliases from the old FMDN API names
 * to the new FHN (Find Hub Network) API names.
 */

#warning "This header is deprecated. Use <bluetooth/fast_pair/fhn/fhn.h> instead."

#include <bluetooth/fast_pair/fhn/fhn.h>

/* Deprecated type aliases.
 *
 * Use #define instead of typedef so that the old tag names (e.g.
 * "struct bt_fast_pair_fmdn_info_cb") resolve to the new FHN tags
 * via the preprocessor. A typedef would only create a new type-name,
 * not a struct/enum tag, causing "incomplete type" errors when old
 * code uses the tag form.
 */
#define bt_fast_pair_fmdn_ring_src           bt_fast_pair_fhn_ring_src
#define bt_fast_pair_fmdn_ring_comp          bt_fast_pair_fhn_ring_comp
#define bt_fast_pair_fmdn_ring_volume        bt_fast_pair_fhn_ring_volume
#define bt_fast_pair_fmdn_ring_req_param     bt_fast_pair_fhn_ring_req_param
#define bt_fast_pair_fmdn_ring_cb            bt_fast_pair_fhn_ring_cb
#define bt_fast_pair_fmdn_ring_trigger       bt_fast_pair_fhn_ring_trigger
#define bt_fast_pair_fmdn_ring_state_param   bt_fast_pair_fhn_ring_state_param
#define bt_fast_pair_fmdn_motion_detector_cb bt_fast_pair_fhn_motion_detector_cb
#define bt_fast_pair_fmdn_adv_param          bt_fast_pair_fhn_adv_param
#define bt_fast_pair_fmdn_info_cb            bt_fast_pair_fhn_info_cb
#define bt_fast_pair_fmdn_read_mode          bt_fast_pair_fhn_read_mode
#define bt_fast_pair_fmdn_read_mode_cb       bt_fast_pair_fhn_read_mode_cb

/* Deprecated enum value aliases. */
#define BT_FAST_PAIR_FMDN_RING_SRC_FMDN_BT_GATT    BT_FAST_PAIR_FHN_RING_SRC_FHN_BT_GATT
#define BT_FAST_PAIR_FMDN_RING_SRC_DULT_BT_GATT    BT_FAST_PAIR_FHN_RING_SRC_DULT_BT_GATT
#define BT_FAST_PAIR_FMDN_RING_SRC_DULT_MOTION_DETECTOR \
	BT_FAST_PAIR_FHN_RING_SRC_DULT_MOTION_DETECTOR

#define BT_FAST_PAIR_FMDN_RING_COMP_RIGHT  BT_FAST_PAIR_FHN_RING_COMP_RIGHT
#define BT_FAST_PAIR_FMDN_RING_COMP_LEFT   BT_FAST_PAIR_FHN_RING_COMP_LEFT
#define BT_FAST_PAIR_FMDN_RING_COMP_CASE   BT_FAST_PAIR_FHN_RING_COMP_CASE

#define BT_FAST_PAIR_FMDN_RING_COMP_BM_NONE BT_FAST_PAIR_FHN_RING_COMP_BM_NONE
#define BT_FAST_PAIR_FMDN_RING_COMP_BM_ALL  BT_FAST_PAIR_FHN_RING_COMP_BM_ALL

#define BT_FAST_PAIR_FMDN_RING_VOLUME_DEFAULT BT_FAST_PAIR_FHN_RING_VOLUME_DEFAULT
#define BT_FAST_PAIR_FMDN_RING_VOLUME_LOW     BT_FAST_PAIR_FHN_RING_VOLUME_LOW
#define BT_FAST_PAIR_FMDN_RING_VOLUME_MEDIUM  BT_FAST_PAIR_FHN_RING_VOLUME_MEDIUM
#define BT_FAST_PAIR_FMDN_RING_VOLUME_HIGH    BT_FAST_PAIR_FHN_RING_VOLUME_HIGH

#define BT_FAST_PAIR_FMDN_RING_MSEC_PER_DSEC        BT_FAST_PAIR_FHN_RING_MSEC_PER_DSEC
#define BT_FAST_PAIR_FMDN_RING_TIMEOUT_DS_TO_MS(_t) BT_FAST_PAIR_FHN_RING_TIMEOUT_DS_TO_MS(_t)
#define BT_FAST_PAIR_FMDN_RING_TIMEOUT_MS_TO_DS(_t) BT_FAST_PAIR_FHN_RING_TIMEOUT_MS_TO_DS(_t)

#define BT_FAST_PAIR_FMDN_RING_TRIGGER_STARTED         BT_FAST_PAIR_FHN_RING_TRIGGER_STARTED
#define BT_FAST_PAIR_FMDN_RING_TRIGGER_FAILED          BT_FAST_PAIR_FHN_RING_TRIGGER_FAILED
#define BT_FAST_PAIR_FMDN_RING_TRIGGER_TIMEOUT_STOPPED BT_FAST_PAIR_FHN_RING_TRIGGER_TIMEOUT_STOPPED
#define BT_FAST_PAIR_FMDN_RING_TRIGGER_UI_STOPPED      BT_FAST_PAIR_FHN_RING_TRIGGER_UI_STOPPED
#define BT_FAST_PAIR_FMDN_RING_TRIGGER_GATT_STOPPED    BT_FAST_PAIR_FHN_RING_TRIGGER_GATT_STOPPED

#define BT_FAST_PAIR_FMDN_BATTERY_LEVEL_NONE BT_FAST_PAIR_FHN_BATTERY_LEVEL_NONE

#define BT_FAST_PAIR_FMDN_ADV_PARAM_INIT    BT_FAST_PAIR_FHN_ADV_PARAM_INIT
#define BT_FAST_PAIR_FMDN_ADV_PARAM_DEFAULT BT_FAST_PAIR_FHN_ADV_PARAM_DEFAULT

#define BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY BT_FAST_PAIR_FHN_READ_MODE_FHN_RECOVERY
#define BT_FAST_PAIR_FMDN_READ_MODE_DULT_ID       BT_FAST_PAIR_FHN_READ_MODE_DULT_ID

/* Deprecated function aliases. */
__deprecated static inline int bt_fast_pair_fmdn_ring_cb_register(
	const struct bt_fast_pair_fhn_ring_cb *cb)
{
	return bt_fast_pair_fhn_ring_cb_register(cb);
}

__deprecated static inline int bt_fast_pair_fmdn_ring_state_update(
	enum bt_fast_pair_fhn_ring_src src,
	const struct bt_fast_pair_fhn_ring_state_param *param)
{
	return bt_fast_pair_fhn_ring_state_update(src, param);
}

__deprecated static inline int bt_fast_pair_fmdn_motion_detector_cb_register(
	const struct bt_fast_pair_fhn_motion_detector_cb *cb)
{
	return bt_fast_pair_fhn_motion_detector_cb_register(cb);
}

__deprecated static inline int bt_fast_pair_fmdn_battery_level_set(
	uint8_t percentage_level)
{
	return bt_fast_pair_fhn_battery_level_set(percentage_level);
}

__deprecated static inline int bt_fast_pair_fmdn_adv_param_set(
	const struct bt_fast_pair_fhn_adv_param *adv_param)
{
	return bt_fast_pair_fhn_adv_param_set(adv_param);
}

__deprecated static inline int bt_fast_pair_fmdn_id_set(uint8_t id)
{
	return bt_fast_pair_fhn_id_set(id);
}

__deprecated static inline bool bt_fast_pair_fmdn_is_provisioned(void)
{
	return bt_fast_pair_fhn_is_provisioned();
}

__deprecated static inline int bt_fast_pair_fmdn_info_cb_register(
	struct bt_fast_pair_fhn_info_cb *cb)
{
	return bt_fast_pair_fhn_info_cb_register(cb);
}

__deprecated static inline int bt_fast_pair_fmdn_read_mode_cb_register(
	const struct bt_fast_pair_fhn_read_mode_cb *cb)
{
	return bt_fast_pair_fhn_read_mode_cb_register(cb);
}

__deprecated static inline int bt_fast_pair_fmdn_read_mode_enter(
	enum bt_fast_pair_fhn_read_mode mode)
{
	return bt_fast_pair_fhn_read_mode_enter(mode);
}

#endif /* BT_SERVICES_FAST_PAIR_FMDN_H_ */
