/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth/gatt.h>

/* @TODO Move it to the main Zephyr repository (include/bluetooth/uuid.h). */

/** @def BT_UUID_HIDS_PROTOCOL_MODE
 *  @brief HID Protocol Mode Characteristic
 */
#define BT_UUID_HIDS_PROTOCOL_MODE         BT_UUID_DECLARE_16(0x2a4e)
/** @def BT_UUID_BOOT_KEYBOARD_INPUT_REPORT
 *  @brief HID Boot Keyboard Input Report Characteristic
 */
#define BT_UUID_BOOT_KEYBOARD_INPUT_REPORT BT_UUID_DECLARE_16(0x2a22)
/** @def BT_UUID_BOOT_KEYBOARD_OUTPUT_REPORT
 *  @brief HID Boot Keyboard Output Report Characteristic
 */
#define BT_UUID_BOOT_KEYBOARD_OUTPUT_REP   BT_UUID_DECLARE_16(0x2a32)
/** @def BT_UUID_BOOT_MOUSE_INPUT_REPORT
 *  @brief HID Boot Mouse Input Report Characteristic
 */
#define BT_UUID_BOOT_MOUSE_INPUT_REPORT    BT_UUID_DECLARE_16(0x2a33)

#define BOOT_MOUSE_INPUT_REP_CHAR_LEN 8
#define HIDS_INFORMATION_CHAR_LEN     4


/** @def HIDS_DEF
 *  @brief HIDS instance declaration macro.
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

/* HID Information flags. */
enum {
	HIDS_REMOTE_WAKE          = BIT(0),
	HIDS_NORMALLY_CONNECTABLE = BIT(1),
};

/* HID Service event type. */
enum hids_evt_type {
	HIDS_EVT_HOST_SUSP,
	HIDS_EVT_HOST_EXIT_SUSP,
	HIDS_EVT_BOOT_MODE_ENTERED,
	HIDS_EVT_REPORT_MODE_ENTERED,
};

/* HID Service event descriptor. */
struct hids_evt {
	enum hids_evt_type type;
};

/* HID Information descriptor. */
struct hids_info {
	u16_t bcd_hid;
	u8_t b_country_code;
	u8_t flags;
};

/* Report data descriptor. */
struct hids_rep {
	u8_t *data;
	u8_t size;
};

/* Input Report descriptor. */
struct hids_inp_rep {
	struct hids_rep buff;
	struct bt_gatt_ccc_cfg ccc[1];
	u8_t id;
	u8_t att_ind;
};

/* Boot Mouse Input report descriptor. */
struct hids_boot_mouse_inp_rep {
	u8_t buff[BOOT_MOUSE_INPUT_REP_CHAR_LEN];
	struct bt_gatt_ccc_cfg ccc[1];
	u8_t att_ind;
};

/* Collection of all Input Reports. */
struct hids_inp_rep_group {
	struct hids_inp_rep reports[CONFIG_NRF_BT_HIDS_INPUT_REP_MAX];
	u8_t cnt;
};

/* Report Map descriptor. */
struct hids_rep_map {
	u8_t const *data;
	u8_t size;
};

/* HID Service event handler. */
typedef void (*hids_evt_handler_t) (struct hids_evt *evt);

/* HID initialization structure. */
struct hids_init {
	hids_evt_handler_t evt_handler;
	struct hids_info info;
	struct hids_inp_rep_group inp_rep_group_init;
	struct hids_rep_map rep_map;
	bool is_mouse;
};

/* HID Service structure. */
struct hids {
	hids_evt_handler_t evt_handler;
	struct bt_gatt_service svc;
	struct hids_inp_rep_group inp_rep_group;
	struct hids_boot_mouse_inp_rep boot_mouse_inp_rep;
	struct hids_rep_map rep_map;
	u8_t protocol_mode;
	u8_t ctrl_point;
	u8_t info[HIDS_INFORMATION_CHAR_LEN];
};


/** @brief Initializes HIDS instance.
 *
 *  @param hids_obj HIDS instance
 *  @param hids_init_obj HIDS initialization descriptor
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int hids_init(struct hids *hids_obj,
	      struct hids_init const * const hids_init_obj);


/** @brief Sends Input Report.
 *
 *  @param hids_obj HIDS instance
 *  @param rep_index Index of report descriptor
 *  @param p_rep Pointer to report data
 *  @param len Length of report data
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int hids_inp_rep_send(struct hids *hids_obj, u8_t rep_index, u8_t *rep,
		      u8_t len);

/** @brief Sends Boot Mouse Input Report.
 *
 *  @param hids_obj HIDS instance
 *  @param buttons State of mouse buttons
 *  @param x_delta Horizontal movement
 *  @param y_delta Vertical movement
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int hids_boot_mouse_inp_rep_send(struct hids *hids_obj, u8_t buttons,
				 s8_t x_delta, s8_t y_delta);

#ifdef __cplusplus
}
#endif
