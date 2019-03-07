/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef ZEPHYR_INCLUDE_MODEM_INFO_H_
#define ZEPHYR_INCLUDE_MODEM_INFO_H_

/**
 * @file modem_info.h
 *
 * @brief API for retrieving modem information.
 * @defgroup modem_info API for retrieving modem information
 * @{
 */

/** Largest expected parameter response size. */
#define MODEM_INFO_MAX_RESPONSE_SIZE 32

/** Size of the JSON string. */
#define MODEM_INFO_JSON_STRING_SIZE 128

/** RSRP offset value. */
#define MODEM_INFO_RSRP_OFFSET_VAL 141

/**@brief RSRP event handler function protoype. */
typedef void (*rsrp_cb_t)(char rsrp_value);

/**@brief LTE link information data. */
enum modem_info {
	MODEM_INFO_RSRP,	/**< Signal strength. */
	MODEM_INFO_BAND,	/**< Current LTE band. */
	MODEM_INFO_MODE,	/**< Current mode. */
	MODEM_INFO_OPERATOR,	/**< Current operator name. */
	MODEM_INFO_CELLID,	/**< Cell ID of the device. */
	MODEM_INFO_IP_ADDRESS,  /**< IP address of the device. */
	MODEM_INFO_UICC,	/**< UICC state. */
	MODEM_INFO_BATTERY,	/**< Battery voltage. */
	MODEM_INFO_TEMP,	/**< Temperature level. */
	MODEM_INFO_FW_VERSION,  /**< Modem firmware version. */
	MODEM_INFO_COUNT,	/**< Number of legal elements in the enum. */
};

/** @brief Initialize the link information driver.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int modem_info_init(void);

/** @brief Initialize the subscription of RSRP values.
 *
 * @param cb Callback function.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int modem_info_rsrp_register(rsrp_cb_t cb);

/** @brief Request the current modem status of any predefined
 *         information value as a string.
 *
 * If the data parameter returned by the modem is originally a
 * short, it is still returned as a string.
 *
 * @param info The requested information type.
 * @param buf  The string where to store the information.
 *
 * @return Length of received data if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int modem_info_string_get(enum modem_info info, char *buf);

/** @brief Request the current modem status of any predefined
 *         information value as a short.
 *
 * If the data parameter returned by the modem is originally a
 * string, this function fails.
 *
 * @param info The requested information type.
 * @param buf  The short where to store the information.
 *
 * @return Length of received data if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int modem_info_short_get(enum modem_info info, u16_t *buf);

/** @brief Function for requesting the name of a modem information
 *         data type.
 *
 * @param info The requested information type.
 * @param buf  The string where to store the name.
 *
 * @return Length of received data if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int modem_info_name_get(enum modem_info info, char *name);

/** @brief Function for requesting the data type of the current
 *         modem information type.
 *
 * @param info The requested information type.
 *
 * @return The data type of the requested modem information data.
 */
enum at_param_type modem_info_type_get(enum modem_info info);

/** @brief Function for requesting the current device status.
 *
 * The data is added to the string buffer with JSON formatting.
 *
 * @param buf  The string where to store the data.
 *
 * @return Length of the string buffer data if the operation was
 *         successful.
 *         Otherwise, a (negative) error code is returned.
 */
int modem_info_json_string_get(char *buf);

/** @} */

#endif /* ZEPHYR_INCLUDE_MODEM_INFO_H_ */
