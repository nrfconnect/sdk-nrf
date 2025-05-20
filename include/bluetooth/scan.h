/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_SCAN_H_
#define BT_SCAN_H_

/**
 * @file
 * @defgroup nrf_bt_scan BT Scanning module
 * @{
 * @brief BT Scanning module
 *
 * @details The Scanning Module handles the Bluetooth LE scanning for
 *          your application. The module offers several criteria
 *          for filtering the devices available for connection,
 *          and it can also work in the simple mode without using the filtering.
 *          If an event handler is provided, your main application can react to
 *          a filter match. The module can also be configured to automatically
 *          connect after it matches a filter.
 *
 * @note The Scanning Module also supports applications with a
 *       multicentral link.
 */

#include <zephyr/types.h>
#include <zephyr/sys/slist.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif


/**@defgroup BT_SCAN_FILTER_MODE Filter modes
 * @{
 */

/**@brief Filters the device name. */
#define BT_SCAN_NAME_FILTER 0x01

/**@brief Filters the device address. */
#define BT_SCAN_ADDR_FILTER 0x02

/**@brief Filters the UUID. */
#define BT_SCAN_UUID_FILTER 0x04

/**@brief Filters the appearance. */
#define BT_SCAN_APPEARANCE_FILTER 0x08

/**@brief Filters the device short name. */
#define BT_SCAN_SHORT_NAME_FILTER 0x10

/**@brief Filters the manufacturer data. */
#define BT_SCAN_MANUFACTURER_DATA_FILTER 0x20

/**@brief Uses the combination of all filters. */
#define BT_SCAN_ALL_FILTER 0x3F
/** @} */


/**@brief Scan types.
 */
enum bt_scan_type {
	/** Passive scanning. */
	BT_SCAN_TYPE_SCAN_PASSIVE,

	/** Active scanning. */
	BT_SCAN_TYPE_SCAN_ACTIVE
};

/**@brief Types of filters.
 */
enum bt_scan_filter_type {
	/** Filter for names. */
	BT_SCAN_FILTER_TYPE_NAME,

	/** Filter for short names. */
	BT_SCAN_FILTER_TYPE_SHORT_NAME,

	/** Filter for addresses. */
	BT_SCAN_FILTER_TYPE_ADDR,

	/** Filter for UUIDs. */
	BT_SCAN_FILTER_TYPE_UUID,

	/** Filter for appearances. */
	BT_SCAN_FILTER_TYPE_APPEARANCE,

	/** Filter for manufacturer data. */
	BT_SCAN_FILTER_TYPE_MANUFACTURER_DATA,
};

/**@brief Filter information structure.
 *
 * @details It contains information about the number of filters of a given type
 *          and whether they are enabled
 */
struct bt_scan_filter_info {
	/** Filter enabled. */
	bool enabled;

	/** Filter count. */
	uint8_t cnt;
};

/**@brief Filter status structure.
 */
struct bt_filter_status {
	/** Filter mode. */
	bool all_mode;

	/** Name filters info. */
	struct bt_scan_filter_info name;

	/** Short name filters info. */
	struct bt_scan_filter_info short_name;

	/** Address filters info. */
	struct bt_scan_filter_info addr;

	/** UUID filters info. */
	struct bt_scan_filter_info uuid;

	/** Appearance filter info. */
	struct bt_scan_filter_info appearance;

	/** Manufacturer filter info. */
	struct bt_scan_filter_info manufacturer_data;
};

/**@brief Advertising info structure.
 */
struct bt_scan_adv_info {
	/** Bluetooth LE advertising type. According to
	 *  Bluetooth Specification 7.8.5
	 */
	uint8_t adv_type;

	/** Received Signal Strength Indication in dBm. */
	int8_t rssi;
};

/**@brief A helper structure to set filters for the name.
 */
struct bt_scan_short_name {
	/** Pointer to the short name. */
	const char *name;

	/** Minimum length of the short name. */
	uint8_t min_len;
};

/**@brief A helper structure to set filters for the manufacturer data.
 */
struct bt_scan_manufacturer_data {
	/** Pointer to the manufacturer data. */
	uint8_t *data;

	/** Manufacturer data length. */
	uint8_t data_len;
};

/**@brief Structure for Scanning Module initialization.
 */
struct bt_scan_init_param {
	/** Scan parameters required to initialize the module.
	 * Can be initialized as NULL. If NULL, the parameters required to
	 * initialize the module are loaded from the static configuration.
	 */
	const struct bt_le_scan_param *scan_param;

#if CONFIG_BT_CENTRAL
	/** If set to true, the module automatically
	 * connects after a filter match.
	 */
	bool connect_if_match;
#endif /* CONFIG_BT_CENTRAL */

	/** Connection parameters. Can be initialized as NULL.
	 * If NULL, the default static configuration is used.
	 */
	const struct bt_le_conn_param *conn_param;
};

/**@brief Name filter status structure, used to inform the application
 *        which name filter is matched.
 */
struct bt_scan_name_filter_status {
	/** Set to true if this type of filter is matched. */
	bool match;

	/** Pointer to the matched filter name. */
	const char *name;

	/** Length of the matched name. */
	uint8_t len;
};

/**@brief Address filter status structure, used to inform the application
 *        which address filter is matched.
 */
struct bt_scan_addr_filter_status {
	/** Set to true if this type of filter is matched. */
	bool match;

	/** Pointer to the matched filter address. */
	const bt_addr_le_t *addr;
};

/**@brief UUID filter status structure, used to inform the application
 *        which UUID filters are matched.
 */
struct bt_scan_uuid_filter_status {
	/** Set to true if this type of filter is matched. */
	bool match;

	/** Array of pointers to the matched UUID filters. */
	const struct bt_uuid *uuid[CONFIG_BT_SCAN_UUID_CNT];

	/** Matched UUID count. */
	uint8_t count;
};

/**@brief Appearance filter status structure, used to inform the application
 *        which appearance filter is matched.
 */
struct bt_scan_appearance_filter_status {
	/** Set to true if this type of filter is matched. */
	bool match;

	/** Pointer to the matched filter appearance. */
	const uint16_t *appearance;
};

/**@brief Manufacturer data filter status structure, used to inform the
 *        application which manufacturer data filter is matched.
 */
struct bt_scan_manufacturer_data_filter_status {
	/** Set to true if this type of filter is matched. */
	bool match;

	/** Pointer to the matched filter manufacturer data. */
	const uint8_t *data;

	/** Length of the matched manufacturer data. */
	uint8_t len;
};

/**@brief Structure for setting the filter status.
 *
 * @details This structure is used for sending
 *          filter status to the main application.
 *          Filter status contains information about which
 *          kind of filter is matched and also appropriate
 *          filter data.
 *
 */
struct bt_scan_filter_match {
	/** Name filter status data. */
	struct bt_scan_name_filter_status name;

	/** Short name filter status data. */
	struct bt_scan_name_filter_status short_name;

	/** Address filter status data. */
	struct bt_scan_addr_filter_status addr;

	/** UUIDs filter status data. */
	struct bt_scan_uuid_filter_status uuid;

	/** Appearance filter status data. */
	struct bt_scan_appearance_filter_status appearance;

	/** Manufacturer data filter status data. */
	struct bt_scan_manufacturer_data_filter_status manufacturer_data;
};

/**@brief Structure containing device data needed to establish
 *        connection and advertising information.
 */
struct bt_scan_device_info {
	/** Received advertising packet information */
	const struct bt_le_scan_recv_info *recv_info;

	/** Connection parameters for LE connection. */
	const struct bt_le_conn_param *conn_param;

	/** Received advertising data. If further
	 *  data processing is needed, you should
	 *  use @em bt_data_parse() to get specific
	 *  advertising data type.
	 */
	struct net_buf_simple *adv_data;
};

/** @brief Initializing macro for scanning module.
 *
 * This is macro initializing necessary structures for @ref bt_scan_cb type.
 *
 * @param[in] _name Unique name for @ref bt_scan_cb structure.
 * @param[in] match_fun Scan filter matched function pointer.
 * @param[in] no_match_fun Scan filter unmatched (the device was not found)
		 function pointer.
 * @param[in] error_fun Error when connecting function pointer.
 * @param[in] connecting_fun Connecting data function pointer.
 */

#if CONFIG_BT_CENTRAL
#define BT_SCAN_CB_INIT(_name,				\
			match_fun,			\
			no_match_fun,			\
			error_fun,			\
			connecting_fun)			\
	static const struct cb_data _name ## _data = {	\
		.filter_match = match_fun,		\
		.filter_no_match = no_match_fun,	\
		.connecting_error = error_fun,		\
		.connecting = connecting_fun,		\
	};						\
	static struct bt_scan_cb _name = {		\
		.cb_addr = &_name ## _data,		\
	}
#else
#define BT_SCAN_CB_INIT(_name,				\
			match_fun,			\
			no_match_fun,			\
			error_fun,			\
			connecting_fun)			\
	static const struct cb_data _name ## _data = {	\
		.filter_match = match_fun,		\
		.filter_no_match = no_match_fun,	\
	};						\
	static struct bt_scan_cb _name = {		\
		.cb_addr = &_name ## _data,		\
	}
#endif /* CONFIG_BT_CENTRAL */

/** @brief Data for scanning callback structure.
 *
 * This structure is used for storing callback functions pointers.
 * It is used by @ref bt_scan_cb structure.
 */

struct cb_data {
	/**@brief Scan filter matched.
	 *
	 * @param[in] device_info Data needed to establish
	 *                        connection and advertising information.
	 * @param[in] filter_match Filter match status.
	 * @param[in] connectable Inform that device is connectable.
	 */
	void (*filter_match)(struct bt_scan_device_info *device_info,
			     struct bt_scan_filter_match *filter_match,
			     bool connectable);

	/**@brief Scan filter unmatched. The device was not found.
	 *
	 * @param[in] device_info Data needed to establish
	 *                        connection and advertising information.
	 * @param[in] connectable Inform that device is connectable.
	 *
	 * @note Even if the filters are disable and not set, then all devices
	 *       will be reported by this callback.
	 *       It can be useful if the scan is used without filters.
	 */
	void (*filter_no_match)(struct bt_scan_device_info *device_info,
				bool connectable);

#if CONFIG_BT_CENTRAL
	/**@brief Error when connecting.
	 *
	 * @param[in] device_info Data needed to establish
	 *                        connection and advertising information.
	 */
	void (*connecting_error)(struct bt_scan_device_info *device_info);

	/**@brief Connecting data.
	 *
	 * @param[in] device_info Data needed to establish
	 *                        connection and advertising information.
	 * @param[in] conn Connection object.
	 */
	void (*connecting)(struct bt_scan_device_info *device_info,
			   struct bt_conn *conn);
#endif /* CONFIG_BT_CENTRAL */
};

/** @brief Scanning callback structure.
 *
 *  This structure is used for tracking the state of a scanning.
 *  It is registered with the help of the @ref bt_scan_cb_register() API.
 *  It is permissible to register multiple instances of this @ref bt_scan_cb
 *  type, in case different modules of an application are interested in
 *  tracking the scanning state. If a callback is not of interest for
 *  an instance, it may be set to NULL and will as a consequence not be
 *  used for that instance.
 */

struct bt_scan_cb {
	const struct cb_data *cb_addr;
	sys_snode_t node;
};

/**@brief Register scanning callbacks.
 *
 * Register callbacks to monitor the state of scanning.
 *
 * @param cb Callback struct.
 */
void bt_scan_cb_register(struct bt_scan_cb *cb);

/**@brief Function for initializing the Scanning Module.
 *
 * @param[in] init Pointer to scanning module initialization structure.
 *                 Can be initialized as NULL. If NULL, the parameters
 *                 required to initialize the module are loaded from
 *                 static configuration. If module is to establish the
 *                 connection automatically, this must be initialized
 *                 with the relevant data.
 */
void bt_scan_init(const struct bt_scan_init_param *init);

/**@brief Function for starting scanning.
 *
 * @details This function starts the scanning according to
 *          the configuration set during the initialization.
 *
 * @param[in] scan_type The choice between active and passive scanning.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error
 *	   code is returned.
 */
int bt_scan_start(enum bt_scan_type scan_type);

/**@brief Function for stopping scanning.
 */
int bt_scan_stop(void);

/**@brief Function to update initial connection parameters.
 *
 * @note The function should not be used when scanning is active.
 *
 * @param[in] new_conn_param New initial connection parameters.
 */
void bt_scan_update_init_conn_params(struct bt_le_conn_param *new_conn_param);

#if CONFIG_BT_SCAN_FILTER_ENABLE

/**@brief Function for enabling filtering.
 *
 * @details The filters can be combined with each other.
 *          For example, you can enable one filter or several filters.
 *          For example, (BT_SCAN_NAME_FILTER | BT_SCAN_UUID_FILTER)
 *          enables UUID and name filters.
 *
 * @param[in] mode Filter mode: @ref BT_SCAN_FILTER_MODE.
 * @param[in] match_all If this flag is set, all types of enabled filters
 *                      must be matched before generating
 *                      @em BT_SCAN_EVT_FILTER_MATCH to the main
 *                      application. Otherwise, it is enough to
 *                      match one filter to trigger the filter match event.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error
 *	     code is returned.
 *
 */
int bt_scan_filter_enable(uint8_t mode, bool match_all);

/**@brief Function for disabling filtering.
 *
 * @details This function disables all filters.
 *          Even if the automatic connection establishing is enabled,
 *          the connection will not be established with the first
 *          device found after this function is called.
 */
void bt_scan_filter_disable(void);

/**@brief Function for getting filter status.
 *
 * @details This function returns the filter setting and whether
 *          it is enabled or disabled.

 * @param[out] status Pointer to Filter Status structure.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error
 *	     code is returned.
 */
int bt_scan_filter_status_get(struct bt_filter_status *status);

/**@brief Function for adding any type of filter to the scanning.
 *
 * @details This function adds a new filter by type.
 *          The filter will be added if
 *          there is available space in this filter type array, and
 *          if the same filter has not already been set.
 *
 * @param[in] type Filter type.
 * @param[in] data Pointer to the filter data to add.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error
 *	     code is returned.
 */
int bt_scan_filter_add(enum bt_scan_filter_type type,
		       const void *data);

/**@brief Function for removing all set filters.
 *
 * @details The function removes all previously set filters.
 *
 * @note After using this function the filters are still enabled.
 */
void bt_scan_filter_remove_all(void);

#endif /* CONFIG_BT_SCAN_FILTER_ENABLE */

/**@brief Function for changing the scanning parameters.
 *
 * @details Use this function to change scanning parameters.
 *          During the parameter change the scan is stopped.
 *          To resume scanning, use @ref bt_scan_start.
 *          Scanning parameters can be set to NULL. If so,
 *          the default static configuration is used.
 *
 * @param[in] scan_param Pointer to Scanning parameters.
 *                       Can be initialized as NULL.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error
 *	     code is returned.
 */
int bt_scan_params_set(struct bt_le_scan_param *scan_param);

/**@brief Clear connection attempts filter.
 *
 * @details Use this function to remove all entries
 *          from the connection attempts filter.
 */
void bt_scan_conn_attempts_filter_clear(void);

/**@brief Add a new device to the blocklist.
 *
 * @details Use this function to add a device to the blocklist.
 *          Scanning module does not generate any event for the
 *          blocklist device or does not try to connect such
 *          devices.
 *
 * @param[in] addr Device address.
 *
 * @retval 0 If the operation was successful. Otherwise, a (negative) error
 *	     code is returned.
 */
int bt_scan_blocklist_device_add(const bt_addr_le_t *addr);

/**@brief Clear the blocklist of the scanning module device.
 *
 * @details Use this function to remove all entries from
 *          the blocklist.
 */
void bt_scan_blocklist_clear(void);

/**@brief Function to update the autoconnect flag after a filter match.
 *
 * @note The function should not be used when scanning is active.
 *
 * @param[in] connect_if_match If set to true, the module automatically
 *			       connects after a filter match.
 */
void bt_scan_update_connect_if_match(bool connect_if_match);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* BT_SCAN_H_ */
