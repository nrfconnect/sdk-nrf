/**
 * @file modem_info.h
 *
 * @brief Public APIs for the LTE Link Control driver.
 */

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef ZEPHYR_INCLUDE_MODEM_INFO_H_
#define ZEPHYR_INCLUDE_MODEM_INFO_H_

/* The largest expected parameter response size */
#define MODEM_INFO_MAX_RESPONSE_SIZE 32
#define MODEM_INFO_JSON_STRING_SIZE 128
#define MODEM_INFO_RSRP_OFFSET_VAL 141

/**@brief RSRP event handler function protoype. */
typedef void (*rsrp_cb_t)(char rsrp_value);

/**@brief LTE link info data. */
enum modem_info {
	MODEM_INFO_RSRP,	/**< Signal strength */
	MODEM_INFO_BAND,	/**< Current band */
	MODEM_INFO_MODE,	/**< Mode */
	MODEM_INFO_OPERATOR,	/**< Operator name */
	MODEM_INFO_CELLID,	/**< Cell ID */
	MODEM_INFO_IP_ADDRESS,/**< IP address */
	MODEM_INFO_UICC,	/**< UICC state */
	MODEM_INFO_BATTERY,	/**< Battery voltage */
	MODEM_INFO_TEMP,	/**< Temperature */
	MODEM_INFO_FW_VERSION,/**< FW Version */
	MODEM_INFO_COUNT,	/**< Number of legal elements in enum */
};

/** @brief Function for initializing the link information driver
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int modem_info_init(void);

/** @brief Function for initializing the subscription of RSRP values
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int modem_info_rsrp_register(rsrp_cb_t cb);

/** @brief Function for requesting the current modem status of any predefined
 * info value. The value is obtained as a string.
 *
 * @return Length of received data or (negative) error code otherwise.
 */
int modem_info_string_get(enum modem_info info, char *buf);

/** @brief Function for requesting the current modem status of any predefined
 * info value. The value is obtained as a string.
 *
 * @return Length of received data or (negative) error code otherwise.
 */
int modem_info_short_get(enum modem_info info, u16_t *buf);

/** @brief Function for requesting the string name of a modem info data type
 *
 * @return Length of received data or (negative) error code otherwise.
 */
int modem_info_name_get(enum modem_info info, char *name);

/** @brief Function for requesting the data type of the current modem info
 * type.
 *
 * @return The data type of the requested modem info data.
 */
enum at_param_type modem_info_type_get(enum modem_info info);

/** @brief Function for requesting the current device status. The
 * data will be added to the string buffer with JSON formatting.
 *
 * @return Length of string buffer or (negative) error code otherwise.
 */
int modem_info_json_string_get(char *buf);

#endif /* ZEPHYR_INCLUDE_MODEM_INFO_H_ */
