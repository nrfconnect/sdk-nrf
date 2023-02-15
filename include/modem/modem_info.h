/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZEPHYR_INCLUDE_MODEM_INFO_H_
#define ZEPHYR_INCLUDE_MODEM_INFO_H_

#ifdef CONFIG_CJSON_LIB
#include <cJSON.h>
#endif

#include <modem/at_params.h>

#ifdef __cplusplus
extern "C" {
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
#define RSRP_OFFSET_VAL 140

/** RSRQ offset value. */
#define RSRQ_OFFSET_VAL 19.5

/** RSRQ scale value. */
#define RSRQ_SCALE_VAL 0.5

/** Modem firmware version string can be up to 40 characters long. */
#define MODEM_INFO_FWVER_SIZE 41

/** Modem returns RSRP and RSRQ as index values which require
 * a conversion to dBm and dB respectively. See modem AT
 * command reference guide for more information.
 */
#define RSRP_IDX_TO_DBM(rsrp) ((rsrp) - RSRP_OFFSET_VAL)

#define RSRQ_IDX_TO_DB(rsrq) ((((float)(rsrq)) * RSRQ_SCALE_VAL) - \
			      RSRQ_OFFSET_VAL)

/**@brief RSRP event handler function prototype. */
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
	MODEM_INFO_APN,		/**< Access point name. */
	MODEM_INFO_COUNT,	/**< Number of legal elements in the enum. */
};

/**@brief LTE parameter data. **/
struct lte_param {
	uint16_t value; /**< The retrieved value. */
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
	struct lte_param apn; /**< Access point name (string). */
	struct lte_param rsrp; /**< Received signal strength. */

	double cellid_dec; /**< Cell ID of the device (in decimal format). */
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
 * @param buf  The buffer to store the null-terminated string.
 * @param buf_size The size of the buffer.
 *
 * @return Length of received data if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int modem_info_string_get(enum modem_info info, char *buf,
			  const size_t buf_size);
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
int modem_info_short_get(enum modem_info info, uint16_t *buf);

/** @brief Request the name of a modem information data type.
 *
 * @param info The requested information type.
 * @param name The string where to store the name.
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

/** @brief Obtain the UUID of the modem firmware build.
 *
 * The UUID is represented as a string, for example:
 * 25c95751-efa4-40d4-8b4a-1dcaab81fac9
 *
 * See the documentation for the AT command %XMODEMUUID.
 *
 * @param buf Pointer to the target buffer.
 * @param buf_size Size of target buffer.
 *
 * @retval 0 if the operation was successful.
 * @retval -EINVAL if the provided buf_size was not enough.
 *         Otherwise, a (negative) error code is returned.
 */
int modem_info_get_fw_uuid(char *buf, size_t buf_size);

/** @brief Obtain the short software identification.
 *
 * The short software identification is represented as a string, for example:
 * nrf9160_1.1.2
 *
 * See the documentation for the AT command %SHORTSWVER.
 *
 * @param buf Pointer to the target buffer.
 * @param buf_size Size of target buffer.
 *
 * @retval 0 if the operation was successful.
 * @retval -EINVAL if the provided buf_size was not enough.
 *         Otherwise, a (negative) error code is returned.
 */
int modem_info_get_fw_version(char *buf, size_t buf_size);

/** @brief Obtain the hardware version string.
 *
 * The hardware version is represented as a string, for example:
 * nRF9160 SICA B0A
 *
 * See the documentation for the AT command %HWVERSION.
 *
 * @param buf Pointer to the target buffer.
 * @param buf_size Size of target buffer.
 *
 * @retval 0 if the operation was successful.
 * @retval -EINVAL if a parameter was invalid.
 *         Otherwise, a (negative) error code is returned.
 */
int modem_info_get_hw_version(char *buf, uint8_t buf_size);

/** @brief Obtain the modem Software Version Number (SVN).
 *
 * The SVN is represented as a string, for example:
 * 12
 *
 * See the documentation for the AT command +CGSN.
 *
 * @param buf Pointer to the target buffer.
 * @param buf_size Size of target buffer.
 *
 * @retval 0 if the operation was successful.
 * @retval -EINVAL if the provided buf_size was not enough.
 *         Otherwise, a (negative) error code is returned.
 */
int modem_info_get_svn(char *buf, size_t buf_size);

/** @brief Obtain the battery voltage.
 *
 * Get the battery voltage, in mV, with a 4 mV resolution.
 *
 * @param val Pointer to the target variable.
 *
 * @retval 0 if the operation was successful.
 * @retval -EINVAL if the provided buf_size was not enough.
 *         Otherwise, a (negative) error code is returned.
 */
int modem_info_get_batt_voltage(int *val);

/** @brief Obtain the internal temperature.
 *
 * Get the internal temperature, in degrees Celsius.
 *
 * @param val Pointer to the target variable.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int modem_info_get_temperature(int *val);

/** @brief Obtain the RSRP.
 *
 * Get the reference signal received strength (RSRP), in dBm.
 *
 * @param val Pointer to the target variable.
 *
 * @retval 0 if the operation was successful.
 * @retval -ENOENT if there is no valid RSRP.
 *         Otherwise, a (negative) error code is returned.
 */
int modem_info_get_rsrp(int *val);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MODEM_INFO_H_ */
