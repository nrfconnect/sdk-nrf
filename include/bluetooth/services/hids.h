/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#ifndef __HIDS_H
#define __HIDS_H

/**
 * @file
 * @defgroup nrf_bt_hids BLE GATT Human Interface Device Service API
 * @{
 * @brief API for the BLE GATT Human Interface Device (HID) Service.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth/gatt.h>

/** Length of the Boot Mouse Input Report. */
#define BOOT_MOUSE_INPUT_REP_CHAR_LEN	8
/** Length of the Boot Keyboard Input Report. */
#define BOOT_KB_INPUT_REP_CHAR_LEN	8
/** Length of the Boot Keyboard Output Report. */
#define BOOT_KB_OUTPUT_REP_CHAR_LEN	1

/** Length of encoded HID Information. */
#define HIDS_INFORMATION_CHAR_LEN	4

/**
 *  @brief Declare a HIDS instance.
 *
 *  @param _name Name of the HIDS instance.
 */
#define HIDS_DEF(_name)							       \
	static struct bt_gatt_attr					       \
		CONCAT(_name, _attr_tab)[CONFIG_NRF_BT_HIDS_ATTR_MAX] = { 0 }; \
	static struct hids _name =					       \
	{								       \
		.svc = { .attrs = CONCAT(_name, _attr_tab) },		       \
	}

/** HID Service information flags. */
enum hids_flags {
	/** Device is capable of sending a wake-signal to a host. */
	HIDS_REMOTE_WAKE          = BIT(0),
	/** Device advertises when bonded but not connected. */
	HIDS_NORMALLY_CONNECTABLE = BIT(1),
};

/** HID Service Protocol Mode events. */
enum hids_pm_evt {
	/** Boot mode entered. */
	HIDS_PM_EVT_BOOT_MODE_ENTERED,
	/** Report mode entered. */
	HIDS_PM_EVT_REPORT_MODE_ENTERED,
};

/** HID Service Control Point events. */
enum hids_cp_evt {
	/** Suspend command received. */
	HIDS_CP_EVT_HOST_SUSP,
	/** Exit suspend command received. */
	HIDS_CP_EVT_HOST_EXIT_SUSP,
};

/** HID notification events. */
enum hids_notif_evt {
	/** Notification enabled event. */
	HIDS_CCCD_EVT_NOTIF_ENABLED,
	/** Notification disabled event. */
	HIDS_CCCD_EVT_NOTIF_DISABLED,
};

/** @brief HID Service information.
 *
 * @param bcd_hid Version of the base USB HID specification.
 * @param b_country_code Country ID code.
 * @param flags Information flags (see @ref hids_flags).
 */
struct hids_info {
	u16_t bcd_hid;
	u8_t b_country_code;
	u8_t flags;
};

/** @brief Report data.
 *
 * @param data Pointer to the report data.
 * @param size Size of the report.
 */
struct hids_rep {
	u8_t *data;
	u8_t size;
};

/** @brief HID notification event handler.
 *
 *  @param evt Notification event.
 */
typedef void (*hids_notif_handler_t) (enum hids_notif_evt evt);

/** @brief HID Report event handler.
 *
 * @param rep Pointer to the report descriptor.
 */
typedef void (*hids_rep_handler_t) (struct hids_rep const *rep);

/** @brief Input Report.
 *
 * @param buff Report data.
 * @param ccc CCC descriptor.
 * @param id Report ID defined in the HIDS Report Map.
 * @param att_ind Index in the service attribute array.
 * @param handler Callback with the notification event.
 */
struct hids_inp_rep {
	struct hids_rep buff;
	struct bt_gatt_ccc_cfg ccc[1];
	u8_t id;
	u8_t att_ind;
	hids_notif_handler_t handler;
};

/** @brief Output or Feature Report.
 *
 * @param buff Report data.
 * @param id Report ID defined in the HIDS Report Map.
 * @param att_ind Index in the service attribute array.
 * @param handler Callback with updated report data.
 */
struct hids_outp_feat_rep {
	struct hids_rep buff;
	u8_t id;
	u8_t att_ind;
	hids_rep_handler_t handler;
};

/** @brief Boot Mouse Input Report.
 *
 * @param buff Report data.
 * @param ccc CCC descriptor.
 * @param att_ind Index in the service attribute array.
 * @param handler Callback with the notification event.
 */
struct hids_boot_mouse_inp_rep {
	u8_t buff[BOOT_MOUSE_INPUT_REP_CHAR_LEN];
	struct bt_gatt_ccc_cfg ccc[1];
	u8_t att_ind;
	hids_notif_handler_t handler;
};

/** @brief Boot Keyboard Input Report.
 *
 * @param buff Report data.
 * @param ccc CCC descriptor.
 * @param att_ind Index in the service attribute array.
 * @param handler Callback with the notification event.
 */
struct hids_boot_kb_inp_rep {
	u8_t buff[BOOT_KB_INPUT_REP_CHAR_LEN];
	struct bt_gatt_ccc_cfg ccc[1];
	u8_t att_ind;
	hids_notif_handler_t handler;
};

/** @brief Boot Keyboard Output Report.
 *
 * @param buff Report data.
 * @param att_ind Index in the service attribute array.
 * @param handler Callback with updated report data.
 */
struct hids_boot_kb_outp_rep {
	u8_t buff[BOOT_KB_OUTPUT_REP_CHAR_LEN];
	u8_t att_ind;
	hids_rep_handler_t handler;
};

/** @brief Collection of all input reports.
 *
 * @param reports Pointer to the report collection.
 * @param cnt Total number of reports.
 */
struct hids_inp_rep_group {
	struct hids_inp_rep reports[CONFIG_NRF_BT_HIDS_INPUT_REP_MAX];
	u8_t cnt;
};

/** @brief Collection of all output reports.
 *
 * @param reports Pointer to the report collection.
 * @param cnt Total number of reports.
 */
struct hids_outp_rep_group {
	struct hids_outp_feat_rep reports[CONFIG_NRF_BT_HIDS_OUTPUT_REP_MAX];
	u8_t cnt;
};

/** @brief Collection of all feature reports.
 *
 * @param reports Pointer to the report collection.
 * @param cnt Total number of reports.
 */
struct hids_feat_rep_group {
	struct hids_outp_feat_rep reports[CONFIG_NRF_BT_HIDS_FEATURE_REP_MAX];
	u8_t cnt;
};

/** @brief Report Map.
 *
 * @param data Pointer to the map.
 * @param size Size of the map.
 */
struct hids_rep_map {
	u8_t const *data;
	u8_t size;
};

/** @brief HID Protocol Mode event handler.
 *
 * @param evt Protocol Mode event (see @ref hids_pm_evt).
 */
typedef void (*hids_pm_evt_handler_t) (enum hids_pm_evt evt);

/** @brief Protocol Mode.
 *
 * @param value Current value of Protocol Mode.
 * @param evt_handler Callback with new Protocol Mode.
 */
struct protocol_mode {
	u8_t value;
	hids_pm_evt_handler_t evt_handler;
};

/** @brief HID Control Point event handler.
 *
 * @param evt Event indicating that the Control Point value has changed.
 * (see @ref hids_cp_evt).
 */
typedef void (*hids_cp_evt_handler_t) (enum hids_cp_evt evt);

/** @brief Control Point.
 *
 * @param value Current value of the Control Point.
 * @param evt_handler Callback with new Control Point state.
 */
struct control_point {
	u8_t value;
	hids_cp_evt_handler_t evt_handler;
};

/** @brief HID initialization.
 *
 * @param info HIDS Information.
 * @param inp_rep_group_init Input Report collection.
 * @param outp_rep_group_init Output Report collection.
 * @param feat_rep_group_init Feature Report collection.
 * @param rep_map Report Map.
 * @param pm_evt_handler Callback for Protocol Mode characteristic.
 * @param cp_evt_handler Callback for Control Point characteristic.
 * @param boot_mouse_notif_handler Callback for Boot Mouse Input Report.
 * @param boot_kb_notif_handler Callback for Boot Keyboard Input Report.
 * @param boot_kb_outp_rep_handler Callback for Boot Keyboard Output Report.
 * @param is_mouse Flag indicating that the device has mouse capabilities.
 * @param is_kb Flag indicating that the device has keyboard capabilities.
 */
struct hids_init {
	struct hids_info info;
	struct hids_inp_rep_group inp_rep_group_init;
	struct hids_outp_rep_group outp_rep_group_init;
	struct hids_feat_rep_group feat_rep_group_init;
	struct hids_rep_map rep_map;
	hids_pm_evt_handler_t pm_evt_handler;
	hids_cp_evt_handler_t cp_evt_handler;
	hids_notif_handler_t boot_mouse_notif_handler;
	hids_notif_handler_t boot_kb_notif_handler;
	hids_rep_handler_t boot_kb_outp_rep_handler;
	bool is_mouse;
	bool is_kb;
};

/** @brief HID Service structure.
 *
 * @param svc Descriptor of the service attribute array.
 * @param inp_rep_group Input Report collection.
 * @param outp_rep_group Output Report collection.
 * @param feat_rep_group Feature Report collection.
 * @param boot_mouse_inp_rep Boot Mouse Input Report.
 * @param boot_kb_inp_rep Boot Keyboard Input Report.
 * @param boot_kb_outp_rep Boot Keyboard Output Report.
 * @param rep_map Report Map.
 * @param pm Protocol Mode.
 * @param cp Control Point.
 * @param info Buffer with encoded HID Information.
 * @param is_mouse Flag indicating that the device has mouse capabilities.
 * @param is_kb Flag indicating that the device has keyboard capabilities.
 */
struct hids {
	struct bt_gatt_service svc;
	struct hids_inp_rep_group inp_rep_group;
	struct hids_outp_rep_group outp_rep_group;
	struct hids_feat_rep_group feat_rep_group;
	struct hids_boot_mouse_inp_rep boot_mouse_inp_rep;
	struct hids_boot_kb_inp_rep boot_kb_inp_rep;
	struct hids_boot_kb_outp_rep boot_kb_outp_rep;
	struct hids_rep_map rep_map;
	struct protocol_mode pm;
	struct control_point cp;
	u8_t info[HIDS_INFORMATION_CHAR_LEN];
	bool is_mouse;
	bool is_kb;
};


/** @brief Initialize the HIDS instance.
 *
 *  @param hids_obj HIDS instance.
 *  @param hids_init_obj HIDS initialization descriptor.
 *
 *  @retval 0 If the operation was successful. Otherwise, a (negative) error
 *  code is returned.
 */
int hids_init(struct hids *hids_obj,
	      struct hids_init const * const hids_init_obj);

/** @brief Uninitialize a HIDS instance.
 *
 *  @param hids_obj HIDS instance.
 *
 *  @retval 0 If the operation was successful. Otherwise, a (negative) error
 *  code is returned.
 */
int hids_uninit(struct hids *hids_obj);

/** @brief Send Input Report.
 *
 *  @param hids_obj HIDS instance.
 *  @param rep_index Index of report descriptor.
 *  @param rep Pointer to the report data.
 *  @param len Length of report data.
 *
 *  @retval 0 If the operation was successful. Otherwise, a (negative) error
 *  code is returned.
 */
int hids_inp_rep_send(struct hids *hids_obj, u8_t rep_index, u8_t const *rep,
		      u8_t len);

/** @brief Send Boot Mouse Input Report.
 *
 *  @param hids_obj HIDS instance.
 *  @param buttons Pointer to the state of the mouse buttons - if null,
 *                 buttons are set to the previously passed value.
 *  @param x_delta Horizontal movement.
 *  @param y_delta Vertical movement.
 *
 *  @retval 0 If the operation was successful. Otherwise, a (negative) error
 *  code is returned.
 */
int hids_boot_mouse_inp_rep_send(struct hids *hids_obj, const u8_t *buttons,
				 s8_t x_delta, s8_t y_delta);


/** @brief Send Boot Keyboard Input Report.
 *
 *  @param hids_obj HIDS instance.
 *  @param rep Pointer to the report data.
 *  @param len Length of report data.
 *
 *  @retval 0 If the operation was successful. Otherwise, a (negative) error
 *  code is returned.
 */
int hids_boot_kb_inp_rep_send(struct hids *hids_obj, u8_t const *rep,
			      u16_t len);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __HIDS_H */
