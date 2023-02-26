/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_HIDS_H_
#define BT_HIDS_H_

/**
 * @file
 * @defgroup bt_hids Bluetooth LE GATT Human Interface Device Service API
 * @{
 * @brief API for the Bluetooth LE GATT Human Interface Device (HID) Service.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth/gatt_pool.h>
#include <zephyr/bluetooth/gatt.h>
#include <bluetooth/conn_ctx.h>

#ifndef CONFIG_BT_HIDS_INPUT_REP_MAX
#define CONFIG_BT_HIDS_INPUT_REP_MAX 0
#endif

#ifndef CONFIG_BT_HIDS_OUTPUT_REP_MAX
#define CONFIG_BT_HIDS_OUTPUT_REP_MAX 0
#endif

#ifndef CONFIG_BT_HIDS_FEATURE_REP_MAX
#define CONFIG_BT_HIDS_FEATURE_REP_MAX 0
#endif

/** Length of the Boot Mouse Input Report. */
#define BT_HIDS_BOOT_MOUSE_REP_LEN     8
/** Length of the Boot Keyboard Input Report. */
#define BT_HIDS_BOOT_KB_INPUT_REP_LEN  8
/** Length of the Boot Keyboard Output Report. */
#define BT_HIDS_BOOT_KB_OUTPUT_REP_LEN 1

/** Length of encoded HID Information. */
#define BT_HIDS_INFORMATION_LEN	4

/**
 * @brief Declare a HIDS instance.
 *
 * @param _name Name of the HIDS instance.
 * @param ...               Lengths of HIDS reports
 */
#define BT_HIDS_DEF(_name, ...)					       \
	BT_CONN_CTX_DEF(_name,						       \
			CONFIG_BT_HIDS_MAX_CLIENT_COUNT,		       \
			_BT_HIDS_CONN_CTX_SIZE_CALC(__VA_ARGS__));	       \
	static struct bt_hids _name =				       \
	{								       \
		.gp = BT_GATT_POOL_INIT(CONFIG_BT_HIDS_ATTR_MAX),	       \
		.conn_ctx = &CONCAT(_name, _ctx_lib),			       \
	}


/**@brief Helping macro for @ref BT_HIDS_DEF, that calculates
 *        the link context size for HIDS instance.
 */
#define _BT_HIDS_CONN_CTX_SIZE_CALC(...)		   \
	(FOR_EACH(_BT_HIDS_GET_ARG1, (+), __VA_ARGS__)	+ \
	sizeof(struct bt_hids_conn_data))
#define _BT_HIDS_GET_ARG1(...) GET_ARG_N(1, __VA_ARGS__)

/** @brief Possible values for the Protocol Mode Characteristic value.
 */
enum bt_hids_pm {
	/** Boot protocol. */
	BT_HIDS_PM_BOOT = 0x00,
	/** Report protocol. */
	BT_HIDS_PM_REPORT = 0x01
};

/** @brief Report types as defined in the Report Reference Characteristic
 *         descriptor.
 */
enum bt_hids_report_type {
	/** Reserved value. */
	BT_HIDS_REPORT_TYPE_RESERVED = 0x00,

	/** Input Report. */
	BT_HIDS_REPORT_TYPE_INPUT = 0x01,

	/** Output report. */
	BT_HIDS_REPORT_TYPE_OUTPUT = 0x02,

	/** Feature Report. */
	BT_HIDS_REPORT_TYPE_FEATURE = 0x03
};

/** @brief HID Service information.
 */
struct bt_hids_info {
	/** Version of the base USB HID specification. */
	uint16_t bcd_hid;

	/** Country ID code. HID device hardware localization.
	 * Most hardware is not localized (value 0x00).
	 */
	uint8_t b_country_code;

	/** Information flags (see @ref bt_hids_flags). */
	uint8_t flags;
};

/** @brief HID Service information flags. */
enum bt_hids_flags {
	/** Device is capable of sending a wake-signal to a host. */
	BT_HIDS_REMOTE_WAKE = BIT(0),
	/** Device advertises when bonded but not connected. */
	BT_HIDS_NORMALLY_CONNECTABLE = BIT(1),
};

/** @brief HID Control Point settings. */
enum bt_hids_control_point {
	/** Suspend value for Control Point.  */
	BT_HIDS_CONTROL_POINT_SUSPEND = 0x00,

	/** Exit suspend value for Control Point.*/
	BT_HIDS_CONTROL_POINT_EXIT_SUSPEND = 0x01
};

/** HID Service Protocol Mode events. */
enum bt_hids_pm_evt {
	/** Boot mode entered. */
	BT_HIDS_PM_EVT_BOOT_MODE_ENTERED,
	/** Report mode entered. */
	BT_HIDS_PM_EVT_REPORT_MODE_ENTERED,
};

/** HID Service Control Point events. */
enum bt_hids_cp_evt {
	/** Suspend command received. */
	BT_HIDS_CP_EVT_HOST_SUSP,
	/** Exit suspend command received. */
	BT_HIDS_CP_EVT_HOST_EXIT_SUSP,
};

/** HID notification events. */
enum bt_hids_notify_evt {
	/** Notification enabled event. */
	BT_HIDS_CCCD_EVT_NOTIFY_ENABLED,
	/** Notification disabled event. */
	BT_HIDS_CCCD_EVT_NOTIFY_DISABLED,
};

/** @brief Report data.
 */
struct bt_hids_rep {
	/** Pointer to the report data. */
	uint8_t *data;

	/** Size of the report. */
	uint8_t size;
};

/** @brief HID notification event handler.
 *
 *  @param evt Notification event.
 */
typedef void (*bt_hids_notify_handler_t) (enum bt_hids_notify_evt evt);

/** @brief HID Report event handler.
 *
 * @param rep	Pointer to the report descriptor.
 * @param conn	Pointer to Connection Object.
 * @param write	@c true if handler is called for report write.
 */
typedef void (*bt_hids_rep_handler_t) (struct bt_hids_rep *rep,
				       struct bt_conn *conn,
				       bool write);

/** @brief Input Report.
 */
struct bt_hids_inp_rep {
	/** CCC descriptor. */
	struct _bt_gatt_ccc ccc;

	/** Report ID defined in the HIDS Report Map. */
	uint8_t id;

	/** Index in Input Report Array. */
	uint8_t idx;

	/** Index in the service attribute array. */
	uint8_t att_ind;

	/** Size of report. */
	uint8_t size;

	/** Report data offset. */
	uint8_t offset;

	/** Report permissions
	 *
	 * Use GATT attribute permission bit field values here.
	 * As input report can only be read only 3 flags are used:
	 * - BT_GATT_PERM_READ
	 * - BT_GATT_PERM_READ_ENCRYPT
	 * - BT_GATT_PERM_READ_AUTHEN
	 *
	 * If no attribute is chosen, the configured default is used.
	 *
	 * The CCC register would have set the proper permissions for read and
	 * write, based on the read permissions for the whole report.
	 */
	uint8_t perm;

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
	const uint8_t *rep_mask;

	/** Callback with the notification event. */
	bt_hids_notify_handler_t handler;
};


/** @brief Output or Feature Report.
 */
struct bt_hids_outp_feat_rep {
	/** Report ID defined in the HIDS Report Map. */
	uint8_t id;

	/** Index in Output/Feature Report Array. */
	uint8_t idx;

	/** Size of report. */
	uint8_t size;

	/** Report data offset. */
	uint8_t offset;

	/** Index in the service attribute array. */
	uint8_t att_ind;

	/** Report permissions
	 *
	 * Use GATT attribute permission bit field values here.
	 * Different permissions may be used for write and read:
	 * - BT_GATT_PERM_READ
	 * - BT_GATT_PERM_READ_ENCRYPT
	 * - BT_GATT_PERM_READ_AUTHEN
	 * - BT_GATT_PERM_WRITE
	 * - BT_GATT_PERM_WRITE_ENCRYPT
	 * - BT_GATT_PERM_WRITE_AUTHEN
	 *
	 * If no attribute is chosen, the configured default is used.
	 */
	uint8_t perm;

	/** Callback with updated report data. */
	bt_hids_rep_handler_t handler;
};

/** @brief Boot Mouse Input Report.
 */
struct bt_hids_boot_mouse_inp_rep {
	/** CCC descriptor. */
	struct _bt_gatt_ccc ccc;

	/** Index in the service attribute array. */
	uint8_t att_ind;

	/** Callback with the notification event. */
	bt_hids_notify_handler_t handler;
};

/** @brief Boot Keyboard Input Report.
 */
struct bt_hids_boot_kb_inp_rep {
	/** CCC descriptor. */
	struct _bt_gatt_ccc ccc;

	/** Index in the service attribute array. */
	uint8_t att_ind;

	/** Callback with the notification event. */
	bt_hids_notify_handler_t handler;
};

/** @brief Boot Keyboard Output Report.
 */
struct bt_hids_boot_kb_outp_rep {
	/** Index in the service attribute array. */
	uint8_t att_ind;

	/** Callback with updated report data. */
	bt_hids_rep_handler_t handler;
};

/** @brief Collection of all input reports.
 */
struct bt_hids_inp_rep_group {
	/** Pointer to the report collection. */
	struct bt_hids_inp_rep reports[CONFIG_BT_HIDS_INPUT_REP_MAX];

	/** Total number of reports. */
	uint8_t cnt;
};

/** @brief Collection of all output reports.
 */
struct bt_hids_outp_rep_group {
	/** Pointer to the report collection. */
	struct bt_hids_outp_feat_rep reports[CONFIG_BT_HIDS_OUTPUT_REP_MAX];

	/** Total number of reports. */
	uint8_t cnt;
};

/** @brief Collection of all feature reports.
 */
struct bt_hids_feat_rep_group {
	/** Pointer to the report collection. */
	struct bt_hids_outp_feat_rep reports[CONFIG_BT_HIDS_FEATURE_REP_MAX];

	/** Total number of reports. */
	uint8_t cnt;
};

/** @brief Report Map.
 */
struct bt_hids_rep_map {
	/** Pointer to the map. */
	uint8_t const *data;

	/** Size of the map. */
	uint16_t size;
};

/** @brief HID Protocol Mode event handler.
 *
 * @param evt  Protocol Mode event (see @ref bt_hids_pm_evt).
 * @param conn Pointer to Connection Object.
 */
typedef void (*bt_hids_pm_evt_handler_t) (enum bt_hids_pm_evt evt,
					  struct bt_conn *conn);


/** @brief Protocol Mode.
 */
struct bt_hids_pm_data {
	/** Callback with new Protocol Mode. */
	bt_hids_pm_evt_handler_t evt_handler;
};

/** @brief HID Control Point event handler.
 *
 * @param evt Event indicating that the Control Point value has changed.
 * (see @ref bt_hids_cp_evt).
 */
typedef void (*bt_hids_cp_evt_handler_t) (enum bt_hids_cp_evt evt);

/** @brief Control Point.
 */
struct bt_hids_cp {
	/** Current value of the Control Point. */
	uint8_t value;

	/** Callback with new Control Point state.*/
	bt_hids_cp_evt_handler_t evt_handler;
};

/** @brief HID initialization.
 */
struct bt_hids_init_param {
	/** HIDS Information. */
	struct bt_hids_info info;

	/** Input Report collection. */
	struct bt_hids_inp_rep_group inp_rep_group_init;

	/** Output Report collection. */
	struct bt_hids_outp_rep_group outp_rep_group_init;

	/** Feature Report collection. */
	struct bt_hids_feat_rep_group feat_rep_group_init;

	/** Report Map. */
	struct bt_hids_rep_map rep_map;

	/** Callback for Protocol Mode characteristic. */
	bt_hids_pm_evt_handler_t pm_evt_handler;

	/** Callback for Control Point characteristic. */
	bt_hids_cp_evt_handler_t cp_evt_handler;

	/** Callback for Boot Mouse Input Report. */
	bt_hids_notify_handler_t boot_mouse_notif_handler;

	/** Callback for Boot Keyboard Input Report. */
	bt_hids_notify_handler_t boot_kb_notif_handler;

	/** Callback for Boot Keyboard Output Report. */
	bt_hids_rep_handler_t boot_kb_outp_rep_handler;

	/** Flag indicating that the device has mouse capabilities. */
	bool is_mouse;

	/** Flag indicating that the device has keyboard capabilities. */
	bool is_kb;
};

/** @brief HID Service structure.
 */
struct bt_hids {
	/** Descriptor of the service attribute array. */
	struct bt_gatt_pool gp;

	/** Input Report collection. */
	struct bt_hids_inp_rep_group inp_rep_group;

	/** Output Report collection. */
	struct bt_hids_outp_rep_group outp_rep_group;

	/** Feature Report collection. */
	struct bt_hids_feat_rep_group feat_rep_group;

	/** Boot Mouse Input Report. */
	struct bt_hids_boot_mouse_inp_rep boot_mouse_inp_rep;

	/** Boot Keyboard Input Report. */
	struct bt_hids_boot_kb_inp_rep boot_kb_inp_rep;

	/** Boot Keyboard Output Report. */
	struct bt_hids_boot_kb_outp_rep boot_kb_outp_rep;

	/** Report Map. */
	struct bt_hids_rep_map rep_map;

	/** Protocol Mode. */
	struct bt_hids_pm_data pm;

	/** Control Point. */
	struct bt_hids_cp cp;

	/** Buffer with encoded HID Information. */
	uint8_t info[BT_HIDS_INFORMATION_LEN];

	/** Flag indicating that the device has mouse capabilities. */
	bool is_mouse;

	/** Flag indicating that the device has keyboard capabilities. */
	bool is_kb;

	/** Bluetooth connection contexts. */
	struct bt_conn_ctx_lib *conn_ctx;
};

/** @brief HID Connection context data structure.
 */
struct bt_hids_conn_data {
	/** Protocol Mode context data. */
	uint8_t pm_ctx_value;

	/** HIDS Boot Mouse Input Report Context. */
	uint8_t hids_boot_mouse_inp_rep_ctx[BT_HIDS_BOOT_MOUSE_REP_LEN];

	/** HIDS Boot Keyboard Input Report Context. */
	uint8_t hids_boot_kb_inp_rep_ctx[BT_HIDS_BOOT_KB_INPUT_REP_LEN];

	/** HIDS Boot Keyboard Output Report Context. */
	uint8_t hids_boot_kb_outp_rep_ctx[BT_HIDS_BOOT_KB_OUTPUT_REP_LEN];

	/** Pointer to Input Reports Context data. */
	uint8_t *inp_rep_ctx;

	/** Pointer to Output Reports Context data. */
	uint8_t *outp_rep_ctx;

	/** Pointer to Feature Reports Context data. */
	uint8_t *feat_rep_ctx;
};


/** @brief Initialize the HIDS instance.
 *
 *  @param hids_obj Pointer to HIDS instance.
 *  @param init_param HIDS initialization descriptor.
 *
 *  @return 0 If the operation was successful. Otherwise, a (negative) error
 *	      code is returned.
 */
int bt_hids_init(struct bt_hids *hids_obj,
		 const struct bt_hids_init_param *init_param);

/** @brief Uninitialize a HIDS instance.
 *
 *  @param hids_obj Pointer to HIDS instance.
 *
 *  @return 0 If the operation was successful. Otherwise, a (negative) error
 *	      code is returned.
 */
int bt_hids_uninit(struct bt_hids *hids_obj);

/**
 * @brief Function for informing the HID service about connection to the device
 *	  This function should be used in main application.
 *
 * @param hids_obj Pointer to HIDS instance.
 * @param conn Pointer to Connection Object.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error
 *	     code is returned.
 */
int bt_hids_connected(struct bt_hids *hids_obj, struct bt_conn *conn);

/**
 * @brief Function for informing the HID service about disconnection
 *	  from the device. This function should be used in main application.
 *
 * @param hids_obj Pointer to HIDS instance.
 * @param conn Pointer to Connection Object.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error
 *	     code is returned.
 */
int bt_hids_disconnected(struct bt_hids *hids_obj, struct bt_conn *conn);

/** @brief Send Input Report.
 *
 *  @note The function is not thread safe.
 *	     It cannot be called from multiple threads at the same time.
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
int bt_hids_inp_rep_send(struct bt_hids *hids_obj, struct bt_conn *conn,
			 uint8_t rep_index, uint8_t const *rep, uint8_t len,
			 bt_gatt_complete_func_t cb);

/** @brief Send Boot Mouse Input Report.
 *
 *  @note The function is not thread safe.
 *	     It cannot be called from multiple threads at the same time.
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
int bt_hids_boot_mouse_inp_rep_send(struct bt_hids *hids_obj,
				    struct bt_conn *conn,
				    const uint8_t *buttons,
				    int8_t x_delta, int8_t y_delta,
				    bt_gatt_complete_func_t cb);

/** @brief Send Boot Keyboard Input Report.
 *
 *  @note The function is not thread safe.
 *	     It cannot be called from multiple threads at the same time.
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
int bt_hids_boot_kb_inp_rep_send(struct bt_hids *hids_obj, struct bt_conn *conn,
				 uint8_t const *rep, uint16_t len,
				 bt_gatt_complete_func_t cb);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_HIDS_H_ */
