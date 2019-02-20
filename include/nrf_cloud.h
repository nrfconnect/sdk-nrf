/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef NRF_CLOUD_H__
#define NRF_CLOUD_H__

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup nrf_cloud nRF Cloud
 * @{
 */


/** @brief Asynchronous nRF Cloud events notified by the module. */
enum nrf_cloud_evt_type {
	/** The transport to the nRF Cloud is established. */
	NRF_CLOUD_EVT_TRANSPORT_CONNECTED = 0x1,
	/** There was a request from nRF Cloud to associate the device
	 * with a user on the nRF Cloud. On receiving this
	 * event, the user must enter the user association sequence using
	 * the @ref nrf_cloud_user_associate API.
	 */
	NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST,
	/** The device is successfully associated with a user. */
	NRF_CLOUD_EVT_USER_ASSOCIATED,
	/** The device can now start sending sensor data to the cloud. */
	NRF_CLOUD_EVT_READY,
	/** A sensor was successfully attached to the cloud.
	 * Supported sensor types are defined in @ref nrf_cloud_sensor.
	 */
	NRF_CLOUD_EVT_SENSOR_ATTACHED,
	/** The device received data from the cloud. */
	NRF_CLOUD_EVT_RX_DATA,
	/** The data sent to the cloud was acknowledged. */
	NRF_CLOUD_EVT_SENSOR_DATA_ACK,
	/** The transport was disconnected. */
	NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED,
	/** There was an error communicating with the cloud. */
	NRF_CLOUD_EVT_ERROR = 0xFF
};


/** @brief User association types supported by the nRF Cloud. */
enum nrf_cloud_ua {
	/** Button input. */
	NRF_CLOUD_UA_BUTTON,
};

/** @brief Sensor types supported by the nRF Cloud. */
enum nrf_cloud_sensor {
	/** The GPS sensor on the device. */
	NRF_CLOUD_SENSOR_GPS,
	/** The FLIP movement sensor on the device. */
	NRF_CLOUD_SENSOR_FLIP,
	/** The Button press sensor on the device. */
	NRF_CLOUD_SENSOR_BUTTON,
	/** The TEMP sensor on the device. */
	NRF_CLOUD_SENSOR_TEMP,
	/** The Humidity sensor on the device. */
	NRF_CLOUD_SENSOR_HUMID,
	/** The Air Pressure sensor on the device. */
	NRF_CLOUD_SENSOR_AIR_PRESS,
	/** The Air Quality sensor on the device. */
	NRF_CLOUD_SENSOR_AIR_QUAL,
	/** The RSPR data obtained from the modem. */
	NRF_CLOUD_LTE_LINK_RSRP,
	/** The descriptive DEVICE data indicating its status. */
	NRF_CLOUD_DEVICE_INFO,
};

/** @brief User input sequence values for user association type
 * @ref NRF_CLOUD_UA_BUTTON.
 */
enum nrf_cloud_ua_button {
	NRF_CLOUD_UA_BUTTON_INPUT_1 = 0x1, /**< Button Input 1. */
	NRF_CLOUD_UA_BUTTON_INPUT_2,       /**< Button Input 2. */
	NRF_CLOUD_UA_BUTTON_INPUT_3,       /**< Button Input 3. */
	NRF_CLOUD_UA_BUTTON_INPUT_4,       /**< Button Input 4. */
};

/**@brief Generic encapsulation for any data that is sent to the cloud. */
struct nrf_cloud_data {
        /** Length of the data. */
	u32_t len;
        /** Pointer to the data. */
	const void *ptr;
};

/**@brief User association types that are supported by the device. */
struct nrf_cloud_ua_list {
	/** Size of the list. */
	u8_t size;
	/** Supported user association types. */
	const enum nrf_cloud_ua *ptr;
};

/**@brief Sensors that are supported by the device. */
struct nrf_cloud_sensor_list {
	/** Size of the list. */
	u8_t size;
	/** Supported sensor types. */
	const enum nrf_cloud_sensor *ptr;
};

/**@brief User association parameters. */
struct nrf_cloud_ua_param {
	/** The type of user association that is used. */
	enum nrf_cloud_ua type;
	/** The user association sequence that is used. */
	struct nrf_cloud_data sequence;
};

/**@brief Connection parameters. */
struct nrf_cloud_connect_param {
	/** Supported user association types, must not be NULL. */
	const struct nrf_cloud_ua_list *ua;
	/** Supported sensor types. May be NULL. */
	const struct nrf_cloud_sensor_list *sensor;
};

/**@brief Parameters of attached sensors. */
struct nrf_cloud_sa_param {
	/** The sensor that is being attached. */
	enum nrf_cloud_sensor sensor_type;
};

/**@brief Sensor data transmission parameters. */
struct nrf_cloud_sensor_data {
	/** The sensor that is the source of the data. */
	enum nrf_cloud_sensor type;
	/** Sensor data to be transmitted. */
	struct nrf_cloud_data data;
	/** Unique tag to identify the sent data.
	 *  Useful for matching the acknowledgment.
	 */
	u32_t tag;
};

/**@brief Asynchronous events received from the module. */
struct nrf_cloud_evt {
	/** The event that occurred. */
	enum nrf_cloud_evt_type type;
	/** Any status associated with the event. */
	u32_t status;
	union {
		/** Requested UA information. Accompanies
		 *  @ref NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST event.
		 */
		struct nrf_cloud_ua_param ua_req;
		struct nrf_cloud_data data;
	} param;
};

/**
 * @brief  Event handler registered with the module to handle asynchronous
 * events from the module.
 *
 * @param[in]  evt The event and any associated parameters.
 */
typedef void (*nrf_cloud_event_handler_t)(const struct nrf_cloud_evt *evt);

/**@brief Initialization parameters for the module. */
struct nrf_cloud_init_param {
	/** Event handler that is registered with the module. */
	nrf_cloud_event_handler_t event_handler;
};

/**
 * @brief Initialize the module.
 *
 * @warning This API must be called exactly once, and it must return
 * successfully.
 *
 * @param[in] param Initialization parameters.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_init(const struct nrf_cloud_init_param *param);

/**
 * @brief Connect to the cloud.
 *
 * In any stage of connecting to the cloud, an @ref NRF_CLOUD_EVT_ERROR
 * might be received.
 * If it is received before @ref NRF_CLOUD_EVT_TRANSPORT_CONNECTED,
 * the application may repeat the call to @ref nrf_cloud_connect to try again.
 *
 * @param[in] param Parameters to be used for the connection.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_connect(const struct nrf_cloud_connect_param *param);

/**
 * @brief Send the user association information.
 *
 * If this API succeeds, the user should expect the @ref
 * NRF_CLOUD_EVT_USER_ASSOCIATED event.
 * If @ref NRF_CLOUD_EVT_ERROR is received, retry using this API.
 *
 * @param[in] param	User association information.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_user_associate(const struct nrf_cloud_ua_param *param);

/**
 * @brief Attach a sensor to the cloud.
 *
 * This API should only be called after receiving an
 * @ref NRF_CLOUD_EVT_READY event.
 * If the API succeeds, wait for the @ref NRF_CLOUD_EVT_SENSOR_ATTACHED
 * event before sending the sensor data.
 *
 * @param[in] param	Sensor information.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_sensor_attach(const struct nrf_cloud_sa_param *param);

/**
 * @brief Send sensor data reliably.
 *
 * This API should only be called after receiving an
 * @ref NRF_CLOUD_EVT_SENSOR_ATTACHED event.
 * If the API succeeds, you can expect the
 * @ref NRF_CLOUD_EVT_SENSOR_DATA_ACK event.
 *
 * @param[in] param Sensor data.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_sensor_data_send(const struct nrf_cloud_sensor_data *param);

/**
 * @brief Stream sensor data.
 *
 * This API should only be called after receiving an
 * @ref NRF_CLOUD_EVT_SENSOR_ATTACHED event.
 *
 * @param[in] param Sensor data.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_sensor_data_stream(const struct nrf_cloud_sensor_data *param);

/**
 * @brief Disconnect from the cloud.
 *
 * This API may be called any time after receiving the
 * @ref NRF_CLOUD_EVT_TRANSPORT_CONNECTED event.
 * If the API succeeds, you can expect the
 * @ref NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED event.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_disconnect(void);

/**
 * @brief Function that must be called periodically to keep the module
 * functional.
 */
void nrf_cloud_process(void);


  /** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_H__ */
