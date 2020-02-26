/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef ZEPHYR_INCLUDE_MODEM_INFO_H_
#define ZEPHYR_INCLUDE_MODEM_INFO_H_

#ifdef CONFIG_CJSON_LIB
#include <cJSON.h>
#endif

/**
 * @file modem_info.h
 *
 * @brief API for retrieving modem information.
 * @defgroup modem_info API for retrieving modem information
 * @{
 */

/** Largest expected parameter response size. */
#define MODEM_INFO_MAX_RESPONSE_SIZE 100

/** Size of the JSON string. */
#define MODEM_INFO_JSON_STRING_SIZE 512

/** RSRP offset value. */
#define MODEM_INFO_RSRP_OFFSET_VAL 141

/** Maximum string size of the network mode string */
#define MODEM_INFO_NETWORK_MODE_MAX_SIZE 12

/**@brief RSRP event handler function protoype. */
typedef void (*rsrp_cb_t)(char rsrp_value);

/**@brief LTE link information data. */
enum modem_info {
	MODEM_INFO_RSRP,	/**< Signal strength. */
	MODEM_INFO_CUR_BAND,	/**< Current LTE band. */
	MODEM_INFO_SUP_BAND,	/**< Supported LTE bands. */
	MODEM_INFO_AREA_CODE,   /**< Tracking area code. */
	MODEM_INFO_UE_MODE,	/**< Current mode. */
	MODEM_INFO_OPERATOR,	/**< Current operator name. */
	MODEM_INFO_MCC,		/**< Mobile country code. */
	MODEM_INFO_MNC,		/**< Mobile network code. */
	MODEM_INFO_CELLID,	/**< Cell ID of the device. */
	MODEM_INFO_IP_ADDRESS,  /**< IP address of the device. */
	MODEM_INFO_UICC,	/**< UICC state. */
	MODEM_INFO_BATTERY,	/**< Battery voltage. */
	MODEM_INFO_TEMP,	/**< Temperature level. */
	MODEM_INFO_FW_VERSION,  /**< Modem firmware version. */
	MODEM_INFO_ICCID,	/**< SIM ICCID. */
	MODEM_INFO_LTE_MODE,	/**< LTE-M support mode. */
	MODEM_INFO_NBIOT_MODE,	/**< NB-IoT support mode. */
	MODEM_INFO_GPS_MODE,	/**< GPS support mode. */
	MODEM_INFO_IMSI,	/**< Mobile subscriber identity. */
	MODEM_INFO_IMEI,	/**< Modem serial number. */
	MODEM_INFO_DATE_TIME,	/**< Mobile network time and date */
	MODEM_INFO_COUNT,	/**< Number of legal elements in the enum. */
};

/**@brief LTE parameter data. **/
struct lte_param {
	u16_t value; /**< The retrieved value. */
	char value_string[MODEM_INFO_MAX_RESPONSE_SIZE]; /**< The retrieved value in string format. */
	char *data_name; /**< The name of the information type. */
	enum modem_info type; /**< The information type. */
};

/**@brief Network parameters. **/
struct network_param {
	struct lte_param current_band; /**< Current LTE band. */
	struct lte_param sup_band; /**< Supported LTE bands. */
	struct lte_param area_code; /**< Tracking area code. */
	struct lte_param current_operator; /**< Current operator. */
	struct lte_param mcc; /**< Mobile country code. */
	struct lte_param mnc; /**< Mobile network code. */
	struct lte_param cellid_hex; /**< Cell ID of the device (in HEX format). */
	struct lte_param ip_address; /**< IP address of the device. */
	struct lte_param ue_mode; /**< Current mode. */
	struct lte_param lte_mode; /**< LTE-M support mode. */
	struct lte_param nbiot_mode; /**< NB-IoT support mode. */
	struct lte_param gps_mode; /**< GPS support mode. */
	struct lte_param date_time; /**< Mobile network time and date */

	double cellid_dec; /**< Cell ID of the device (in decimal format). */
	char network_mode[MODEM_INFO_NETWORK_MODE_MAX_SIZE];
};

/**@brief SIM card parameters. */
struct sim_param {
	struct lte_param uicc; /**< UICC state. */
	struct lte_param iccid; /**< SIM ICCID. */
	struct lte_param imsi; /**< Mobile subscriber identity. */
};

/**@brief Device parameters. */
struct device_param {
	struct lte_param modem_fw; /**< Modem firmware version. */
	struct lte_param battery; /**< Battery voltage. */
	struct lte_param imei; /**< Modem serial number. */
	const char *board; /**< Board version. */
	const char *app_version; /**< Application version. */
	const char *app_name; /**< Application name. */
};

/**@brief Modem parameters. */
struct modem_param_info {
	struct network_param network; /**< Network parameters. */
	struct sim_param     sim; /**< SIM card parameters. */
	struct device_param  device;/**< Device parameters. */
};

/** @brief Initialize the modem information module.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int modem_info_init(void);


/** @brief Initialize the structure that stores modem information.
 *
 * @param modem_param Pointer to the modem parameter structure.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int modem_info_params_init(struct modem_param_info *modem_param);

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

/** @brief Request the name of a modem information data type.
 *
 * @param info The requested information type.
 * @param buf  The string where to store the name.
 *
 * @return Length of received data if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int modem_info_name_get(enum modem_info info, char *name);

/** @brief Request the data type of the current modem information
 *         type.
 *
 * @param info The requested information type.
 *
 * @return The data type of the requested modem information data.
 *         Otherwise, a (negative) error code is returned.
 */
enum at_param_type modem_info_type_get(enum modem_info info);

#ifdef CONFIG_CJSON_LIB
/** @brief Encode the modem parameters.
 *
 * The data is added to the string buffer with JSON formatting.
 *
 * @param modem_param Pointer to the modem parameter structure.
 * @param buf         The buffer where the string will be written.
 *
 * @return Length of the string buffer data if the operation was
 *         successful.
 *         Otherwise, a (negative) error code is returned.
 */
int modem_info_json_string_encode(struct modem_param_info *modem_param,
				  char *buf);


/** @brief Encode the modem parameters.
 *
 * The data is stored to a JSON object.
 *
 * @param modem_param Pointer to the modem parameter structure.
 * @param root_obj    The JSON object where to store the data.
 *
 * @return Number of JSON objects added to root_obj if the
 *         operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int modem_info_json_object_encode(struct modem_param_info *modem,
				  cJSON *root_obj);
#endif

/** @brief Obtain the modem parameters.
 *
 * The data is stored in the provided info structure.
 *
 * @param modem_param Pointer to the storage parameters.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int modem_info_params_get(struct modem_param_info *modem_param);

/** @} */

#endif /* ZEPHYR_INCLUDE_MODEM_INFO_H_ */
