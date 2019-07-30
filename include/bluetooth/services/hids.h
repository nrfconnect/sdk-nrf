/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef BT_GATT_HIDS_H_
#define BT_GATT_HIDS_H_

/**
 * @file
 * @defgroup bt_gatt_hids BLE GATT Human Interface Device Service API
 * @{
 * @brief API for the BLE GATT Human Interface Device (HID) Service.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth/gatt_pool.h>
#include <bluetooth/gatt.h>
#include <bluetooth/conn_ctx.h>

/** Length of the Boot Mouse Input Report. */
#define BT_GATT_HIDS_BOOT_MOUSE_REP_LEN		8
/** Length of the Boot Keyboard Input Report. */
#define BT_GATT_HIDS_BOOT_KB_INPUT_REP_LEN	8
/** Length of the Boot Keyboard Output Report. */
#define BT_GATT_HIDS_BOOT_KB_OUTPUT_REP_LEN	1

/** Length of encoded HID Information. */
#define BT_GATT_HIDS_INFORMATION_LEN	4

/**
 * @brief Declare a HIDS instance.
 *
 * @param _name Name of the HIDS instance.
 * @param ...               Lengths of HIDS reports
 */
#define BT_GATT_HIDS_DEF(_name, ...)					       \
	BT_CONN_CTX_DEF(_name,						       \
			CONFIG_BT_GATT_HIDS_MAX_CLIENT_COUNT,		       \
			_BT_GATT_HIDS_CONN_CTX_SIZE_CALC(__VA_ARGS__));	       \
	static struct bt_gatt_hids _name =				       \
	{								       \
		.gp = BT_GATT_POOL_INIT(CONFIG_BT_GATT_HIDS_ATTR_MAX),	       \
		.conn_ctx = &CONCAT(_name, _ctx_lib),			       \
	}


/**@brief Helping macro for @ref BT_GATT_HIDS_DEF, that calculates
 *        the link context size for BLE HIDS instance.
 */
#define _BT_GATT_HIDS_CONN_CTX_SIZE_CALC(...)		   \
	(MACRO_MAP(_BLE_GATT_HIDS_REPORT_ADD, __VA_ARGS__) \
	sizeof(struct bt_gatt_hids_conn_data))

/**@brief Helping macro for @ref _BT_GATT_HIDS_CONN_CTX_SIZE_CALC,
 *        that adds Input/Output/Feature report lengths.
 */
#define _BLE_GATT_HIDS_REPORT_ADD(_report_size) (_report_size) +

/** HID Service information flags. */
enum bt_gatt_hids_flags {
	/** Device is capable of sending a wake-signal to a host. */
	BT_GATT_HIDS_REMOTE_WAKE          = BIT(0),
	/** Device advertises when bonded but not connected. */
	BT_GATT_HIDS_NORMALLY_CONNECTABLE = BIT(1),
};

/** HID Service Protocol Mode events. */
enum bt_gatt_hids_pm_evt {
	/** Boot mode entered. */
	BT_GATT_HIDS_PM_EVT_BOOT_MODE_ENTERED,
	/** Report mode entered. */
	BT_GATT_HIDS_PM_EVT_REPORT_MODE_ENTERED,
};

/** HID Service Control Point events. */
enum bt_gatt_hids_cp_evt {
	/** Suspend command received. */
	BT_GATT_HIDS_CP_EVT_HOST_SUSP,
	/** Exit suspend command received. */
	BT_GATT_HIDS_CP_EVT_HOST_EXIT_SUSP,
};

/** HID notification events. */
enum bt_gatt_hids_notif_evt {
	/** Notification enabled event. */
	BT_GATT_HIDS_CCCD_EVT_NOTIF_ENABLED,
	/** Notification disabled event. */
	BT_GATT_HIDS_CCCD_EVT_NOTIF_DISABLED,
};

/** @brief HID Service information.
 */
struct bt_gatt_hids_info {
	/** Version of the base USB HID specification. */
	u16_t bcd_hid;

	/** Country ID code. */
	u8_t b_country_code;

	/** Information flags (see @ref bt_gatt_hids_flags). */
	u8_t flags;
};

/** @brief Report data.
 */
struct bt_gatt_hids_rep {
	/** Pointer to the report data. */
	u8_t *data;

	/** Size of the report. */
	u8_t size;
};

/** @brief HID notification event handler.
 *
 *  @param evt Notification event.
 */
typedef void (*bt_gatt_hids_notif_handler_t) (enum bt_gatt_hids_notif_evt evt);

/** @brief HID Report event handler.
 *
 * @param rep	Pointer to the report descriptor.
 * @param conn	Pointer to Connection Object.
 * @param write	@c true if handler is called for report write.
 */
typedef void (*bt_gatt_hids_rep_handler_t) (struct bt_gatt_hids_rep *rep,
					    struct bt_conn *conn,
					    bool write);

/** @brief Input Report.
 */
struct bt_gatt_hids_inp_rep {
	/** CCC descriptor. */
	struct bt_gatt_ccc_cfg ccc[BT_GATT_CCC_MAX];

	/** Report ID defined in the HIDS Report Map. */
	u8_t id;

	/** Index in Input Report Array. */
	u8_t idx;

	/** Index in the service attribute array. */
	u8_t att_ind;

	/** Size of report. */
	u8_t size;

	/** Report data offset. */
	u8_t offset;

	/** Pointer to report mask. The least significant bit
	 * corresponds to the least significant byte of the report value.
	 * If this bit is set to 1 then the first byte of report is stored.
	 * For example, report is 16 byte length to mask this the 2 byte is
	 * needed, so if first byte is 0b11111111 then the first 8 byte of
	 * report is stored, if second byte is 0b00001111 the only next 4
	 * bytes so first 12 byte of report is stored. The user must provide
	 * a mask of the appropriate length to mask all bytes of the report.
	 * If rep_mask is NULL then whole report is stored.
	 */
	const u8_t *rep_mask;

	/** Callback with the notification event. */
	bt_gatt_hids_notif_handler_t handler;
};


/** @brief Output or Feature Report.
 */
struct bt_gatt_hids_outp_feat_rep {
	/** Report ID defined in the HIDS Report Map. */
	u8_t id;

	/** Index in Output/Feature Report Array. */
	u8_t idx;

	/** Size of report. */
	u8_t size;

	/** Report data offset. */
	u8_t offset;

	/** Index in the service attribute array. */
	u8_t att_ind;

	/** Callback with updated report data. */
	bt_gatt_hids_rep_handler_t handler;
};

/** @brief Boot Mouse Input Report.
 */
struct bt_gatt_hids_boot_mouse_inp_rep {
	/** CCC descriptor. */
	struct bt_gatt_ccc_cfg ccc[BT_GATT_CCC_MAX];

	/** Index in the service attribute array. */
	u8_t att_ind;

	/** Callback with the notification event. */
	bt_gatt_hids_notif_handler_t handler;
};

/** @brief Boot Keyboard Input Report.
 */
struct bt_gatt_hids_boot_kb_inp_rep {
	/** CCC descriptor. */
	struct bt_gatt_ccc_cfg ccc[BT_GATT_CCC_MAX];

	/** Index in the service attribute array. */
	u8_t att_ind;

	/** Callback with the notification event. */
	bt_gatt_hids_notif_handler_t handler;
};

/** @brief Boot Keyboard Output Report.
 */
struct bt_gatt_hids_boot_kb_outp_rep {
	/** Index in the service attribute array. */
	u8_t att_ind;

	/** Callback with updated report data. */
	bt_gatt_hids_rep_handler_t handler;
};

/** @brief Collection of all input reports.
 */
struct bt_gatt_hids_inp_rep_group {
	/** Pointer to the report collection. */
	struct bt_gatt_hids_inp_rep reports[CONFIG_BT_GATT_HIDS_INPUT_REP_MAX];

	/** Total number of reports. */
	u8_t cnt;
};

/** @brief Collection of all output reports.
 */
struct bt_gatt_hids_outp_rep_group {
	/** Pointer to the report collection. */
	struct bt_gatt_hids_outp_feat_rep reports[CONFIG_BT_GATT_HIDS_OUTPUT_REP_MAX];

	/** Total number of reports. */
	u8_t cnt;
};

/** @brief Collection of all feature reports.
 */
struct bt_gatt_hids_feat_rep_group {
	/** Pointer to the report collection. */
	struct bt_gatt_hids_outp_feat_rep reports[CONFIG_BT_GATT_HIDS_FEATURE_REP_MAX];

	/** Total number of reports. */
	u8_t cnt;
};

/** @brief Report Map.
 */
struct bt_gatt_hids_rep_map {
	/** Pointer to the map. */
	u8_t const *data;

	/** Size of the map. */
	u8_t size;
};

/** @brief HID Protocol Mode event handler.
 *
 * @param evt  Protocol Mode event (see @ref bt_gatt_hids_pm_evt).
 * @param conn Pointer to Connection Object.
 */
typedef void (*bt_gatt_hids_pm_evt_handler_t) (enum bt_gatt_hids_pm_evt evt,
					       struct bt_conn *conn);


/** @brief Protocol Mode.
 */
struct bt_gatt_hids_pm {
	/** Callback with new Protocol Mode. */
	bt_gatt_hids_pm_evt_handler_t evt_handler;
};

/** @brief HID Control Point event handler.
 *
 * @param evt Event indicating that the Control Point value has changed.
 * (see @ref bt_gatt_hids_cp_evt).
 */
typedef void (*bt_gatt_hids_cp_evt_handler_t) (enum bt_gatt_hids_cp_evt evt);

/** @brief Control Point.
 */
struct bt_gatt_hids_cp {
	/** Current value of the Control Point. */
	u8_t value;

	/** Callback with new Control Point state.*/
	bt_gatt_hids_cp_evt_handler_t evt_handler;
};

/** @brief HID initialization.
 */
struct bt_gatt_hids_init_param {
	/** HIDS Information. */
	struct bt_gatt_hids_info info;

	/** Input Report collection. */
	struct bt_gatt_hids_inp_rep_group inp_rep_group_init;

	/** Output Report collection. */
	struct bt_gatt_hids_outp_rep_group outp_rep_group_init;

	/** Feature Report collection. */
	struct bt_gatt_hids_feat_rep_group feat_rep_group_init;

	/** Report Map. */
	struct bt_gatt_hids_rep_map rep_map;

	/** Callback for Protocol Mode characteristic. */
	bt_gatt_hids_pm_evt_handler_t pm_evt_handler;

	/** Callback for Control Point characteristic. */
	bt_gatt_hids_cp_evt_handler_t cp_evt_handler;

	/** Callback for Boot Mouse Input Report. */
	bt_gatt_hids_notif_handler_t boot_mouse_notif_handler;

	/** Callback for Boot Keyboard Input Report. */
	bt_gatt_hids_notif_handler_t boot_kb_notif_handler;

	/** Callback for Boot Keyboard Output Report. */
	bt_gatt_hids_rep_handler_t boot_kb_outp_rep_handler;

	/** Flag indicating that the device has mouse capabilities. */
	bool is_mouse;

	/** Flag indicating that the device has keyboard capabilities. */
	bool is_kb;
};

/** @brief HID Service structure.
 */
struct bt_gatt_hids {
	/** Descriptor of the service attribute array. */
	struct bt_gatt_pool gp;

	/** Input Report collection. */
	struct bt_gatt_hids_inp_rep_group inp_rep_group;

	/** Output Report collection. */
	struct bt_gatt_hids_outp_rep_group outp_rep_group;

	/** Feature Report collection. */
	struct bt_gatt_hids_feat_rep_group feat_rep_group;

	/** Boot Mouse Input Report. */
	struct bt_gatt_hids_boot_mouse_inp_rep boot_mouse_inp_rep;

	/** Boot Keyboard Input Report. */
	struct bt_gatt_hids_boot_kb_inp_rep boot_kb_inp_rep;

	/** Boot Keyboard Output Report. */
	struct bt_gatt_hids_boot_kb_outp_rep boot_kb_outp_rep;

	/** Report Map. */
	struct bt_gatt_hids_rep_map rep_map;

	/** Protocol Mode. */
	struct bt_gatt_hids_pm pm;

	/** Control Point. */
	struct bt_gatt_hids_cp cp;

	/** Buffer with encoded HID Information. */
	u8_t info[BT_GATT_HIDS_INFORMATION_LEN];

	/** Flag indicating that the device has mouse capabilities. */
	bool is_mouse;

	/** Flag indicating that the device has keyboard capabilities. */
	bool is_kb;

	/** Bluetooth connection contexts. */
	struct bt_conn_ctx_lib *conn_ctx;
};

/** @brief HID Connection context data structure.
 */
struct bt_gatt_hids_conn_data {
	/** Protocol Mode context data. */
	u8_t pm_ctx_value;

	/** HIDS Boot Mouse Input Report Context. */
	u8_t hids_boot_mouse_inp_rep_ctx[BT_GATT_HIDS_BOOT_MOUSE_REP_LEN];

	/** HIDS Boot Keyboard Input Report Context. */
	u8_t hids_boot_kb_inp_rep_ctx[BT_GATT_HIDS_BOOT_KB_INPUT_REP_LEN];

	/** HIDS Boot Keyboard Output Report Context. */
	u8_t hids_boot_kb_outp_rep_ctx[BT_GATT_HIDS_BOOT_KB_OUTPUT_REP_LEN];

	/** Pointer to Input Reports Context data. */
	u8_t *inp_rep_ctx;

	/** Pointer to Output Reports Context data. */
	u8_t *outp_rep_ctx;

	/** Pointer to Feature Reports Context data. */
	u8_t *feat_rep_ctx;
};


/** @brief Initialize the HIDS instance.
 *
 *  @param hids_obj Pointer to HIDS instance.
 *  @param init_param HIDS initialization descriptor.
 *
 *  @return 0 If the operation was successful. Otherwise, a (negative) error
 *	      code is returned.
 */
int bt_gatt_hids_init(struct bt_gatt_hids *hids_obj,
		      const struct bt_gatt_hids_init_param *init_param);

/** @brief Uninitialize a HIDS instance.
 *
 *  @param hids_obj Pointer to HIDS instance.
 *
 *  @return 0 If the operation was successful. Otherwise, a (negative) error
 *	      code is returned.
 */
int bt_gatt_hids_uninit(struct bt_gatt_hids *hids_obj);

/**
 * @brief Function for notification HID service about connection to the device
 *	  This function should be used in main application.
 *
 * @param hids_obj Pointer to HIDS instance.
 * @param conn Pointer to Connection Object.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error
 *	     code is returned.
 */
int bt_gatt_hids_notify_connected(struct bt_gatt_hids *hids_obj,
				  struct bt_conn *conn);

/**
 * @brief Function for notification HID service about disconnection
 *	  from the device. This function should be used in main application.
 *
 * @param hids_obj Pointer to HIDS instance.
 * @param conn Pointer to Connection Object.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error
 *	     code is returned.
 */
int bt_gatt_hids_notify_disconnected(struct bt_gatt_hids *hids_obj,
				     struct bt_conn *conn);

/** @brief Send Input Report.
 *
 *  @warning The function is not thread safe.
 *	     It can not be called from multiple threads at the same time.
 *
 *  @param hids_obj Pointer to HIDS instance.
 *  @param conn Pointer to Connection Object.
 *  @param rep_index Index of report descriptor.
 *  @param rep Pointer to the report data.
 *  @param len Length of report data.
 *  @param cb Notification complete callback (can be NULL).
 *
 *  @return 0 If the operation was successful. Otherwise, a (negative) error
 *	      code is returned.
 */
int bt_gatt_hids_inp_rep_send(struct bt_gatt_hids *hids_obj,
			      struct bt_conn *conn, u8_t rep_index,
			      u8_t const *rep, u8_t len,
			      bt_gatt_complete_func_t cb);

/** @brief Send Boot Mouse Input Report.
 *
 *  @warning The function is not thread safe.
 *	     It can not be called from multiple threads at the same time.
 *
 *  @param hids_obj Pointer to HIDS instance.
 *  @param conn Pointer to Connection Object.
 *  @param buttons Pointer to the state of the mouse buttons - if null,
 *                 buttons are set to the previously passed value.
 *  @param x_delta Horizontal movement.
 *  @param y_delta Vertical movement.
 *  @param cb Notification complete callback (can be NULL).
 *
 *  @return 0 If the operation was successful. Otherwise, a (negative) error
 *	      code is returned.
 */
int bt_gatt_hids_boot_mouse_inp_rep_send(struct bt_gatt_hids *hids_obj,
					 struct bt_conn *conn,
					 const u8_t *buttons,
					 s8_t x_delta, s8_t y_delta,
					 bt_gatt_complete_func_t cb);

/** @brief Send Boot Keyboard Input Report.
 *
 *  @warning The function is not thread safe.
 *	     It can not be called from multiple threads at the same time.
 *
 *  @param hids_obj Pointer to HIDS instance.
 *  @param conn Pointer to Connection Object.
 *  @param rep Pointer to the report data.
 *  @param len Length of report data.
 *  @param cb Notification complete callback (can be NULL).
 *
 *  @return 0 If the operation was successful. Otherwise, a (negative) error
 *	      code is returned.
 */
int bt_gatt_hids_boot_kb_inp_rep_send(struct bt_gatt_hids *hids_obj,
				      struct bt_conn *conn, u8_t const *rep,
				      u16_t len, bt_gatt_complete_func_t cb);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_GATT_HIDS_H_ */
