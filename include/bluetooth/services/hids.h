/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth/gatt.h>

#define BOOT_MOUSE_INPUT_REP_CHAR_LEN	8
#define BOOT_KB_INPUT_REP_CHAR_LEN	8
#define BOOT_KB_OUTPUT_REP_CHAR_LEN	1
#define HIDS_INFORMATION_CHAR_LEN	4

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

/* HID Service Protocol Mode event. */
enum hids_pm_evt {
	HIDS_PM_EVT_BOOT_MODE_ENTERED,
	HIDS_PM_EVT_REPORT_MODE_ENTERED,
};

/* HID Service Control Point event. */
enum hids_cp_evt {
	HIDS_CP_EVT_HOST_SUSP,
	HIDS_CP_EVT_HOST_EXIT_SUSP,
};

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

/* HID Report event handler. */
typedef void (*hids_rep_handler_t) (struct hids_rep const *rep);

/* Input Report descriptor. */
struct hids_inp_rep {
	struct hids_rep buff;
	struct bt_gatt_ccc_cfg ccc[1];
	u8_t id;
	u8_t att_ind;
};

/* Output Report descriptor. */
struct hids_outp_rep {
	struct hids_rep buff;
	u8_t id;
	u8_t att_ind;
	hids_rep_handler_t handler;
};

/* Boot Mouse Input report descriptor. */
struct hids_boot_mouse_inp_rep {
	u8_t buff[BOOT_MOUSE_INPUT_REP_CHAR_LEN];
	struct bt_gatt_ccc_cfg ccc[1];
	u8_t att_ind;
};

/* Boot Keyboard Input report descriptor. */
struct hids_boot_kb_inp_rep {
	u8_t buff[BOOT_KB_INPUT_REP_CHAR_LEN];
	struct bt_gatt_ccc_cfg ccc[1];
	u8_t att_ind;
};

/* Boot Keyboard Output report descriptor. */
struct hids_boot_kb_outp_rep {
	u8_t buff[BOOT_KB_OUTPUT_REP_CHAR_LEN];
	u8_t att_ind;
	hids_rep_handler_t handler;
};

/* Collection of all Input Reports. */
struct hids_inp_rep_group {
	struct hids_inp_rep reports[CONFIG_NRF_BT_HIDS_INPUT_REP_MAX];
	u8_t cnt;
};

/* Collection of all Output Reports. */
struct hids_outp_rep_group {
	struct hids_outp_rep reports[CONFIG_NRF_BT_HIDS_OUTPUT_REP_MAX];
	u8_t cnt;
};

/* Report Map descriptor. */
struct hids_rep_map {
	u8_t const *data;
	u8_t size;
};

/* HID Protocol Mode event handler. */
typedef void (*hids_pm_evt_handler_t) (enum hids_pm_evt evt);

struct protocol_mode {
	u8_t value;
	hids_pm_evt_handler_t evt_handler;
};

/* HID Control Point event handler. */
typedef void (*hids_cp_evt_handler_t) (enum hids_cp_evt evt);

struct control_point {
	u8_t value;
	hids_cp_evt_handler_t evt_handler;
};

/* HID initialization structure. */
struct hids_init {
	struct hids_info info;
	struct hids_inp_rep_group inp_rep_group_init;
	struct hids_outp_rep_group outp_rep_group_init;
	struct hids_rep_map rep_map;
	hids_pm_evt_handler_t pm_evt_handler;
	hids_cp_evt_handler_t cp_evt_handler;
	hids_rep_handler_t boot_kb_outp_rep_handler;
	bool is_mouse;
	bool is_kb;
};

/* HID Service structure. */
struct hids {
	struct bt_gatt_service svc;
	struct hids_inp_rep_group inp_rep_group;
	struct hids_outp_rep_group outp_rep_group;
	struct hids_boot_mouse_inp_rep boot_mouse_inp_rep;
	struct hids_boot_kb_inp_rep boot_kb_inp_rep;
	struct hids_boot_kb_outp_rep boot_kb_outp_rep;
	struct hids_rep_map rep_map;
	struct protocol_mode pm;
	struct control_point cp;
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
 *  @param rep Pointer to report data
 *  @param len Length of report data
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int hids_inp_rep_send(struct hids *hids_obj, u8_t rep_index, u8_t const *rep,
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


/** @brief Sends Boot Keyboard Input Report.
 *
 *  @param hids_obj HIDS instance
 *  @param rep Pointer to report data
 *  @param len Length of report data
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int hids_boot_kb_inp_rep_send(struct hids *hids_obj, u8_t const *rep,
			      u16_t len);


#ifdef __cplusplus
}
#endif
