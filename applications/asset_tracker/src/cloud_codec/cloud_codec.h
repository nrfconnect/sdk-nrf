/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**@file
 *
 * @defgroup cloud_codec Cloud codec
 * @brief  Module that encodes and decodes data destined for a cloud solution.
 * @{
 */

#ifndef CLOUD_CODEC_H__
#define CLOUD_CODEC_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Cloud sensor types. */
enum cloud_sensor {
	/** The GPS sensor on the device. */
	CLOUD_SENSOR_GPS,
	/** The FLIP movement sensor on the device. */
	CLOUD_SENSOR_FLIP,
	/** The Button press sensor on the device. */
	CLOUD_SENSOR_BUTTON,
	/** The TEMP sensor on the device. */
	CLOUD_SENSOR_TEMP,
	/** The Humidity sensor on the device. */
	CLOUD_SENSOR_HUMID,
	/** The Air Pressure sensor on the device. */
	CLOUD_SENSOR_AIR_PRESS,
	/** The Air Quality sensor on the device. */
	CLOUD_SENSOR_AIR_QUAL,
	/** The RSPR data obtained from the modem. */
	CLOUD_LTE_LINK_RSRP,
	/** The descriptive DEVICE data indicating its status. */
	CLOUD_DEVICE_INFO,
};

struct cloud_data {
	char *buf;
	size_t len;
};

/**@brief Sensor data parameters. */
struct cloud_sensor_data {
	/** The sensor that is the source of the data. */
	enum cloud_sensor type;
	/** Sensor data to be transmitted. */
	struct cloud_data data;
	/** Unique tag to identify the sent data.
	 *  Useful for matching the acknowledgment.
	 */
	u32_t tag;
};

/**
 * @brief Encode data from sensor data structure to a cloud data structure
 *	  containing a JSON string.
 *
 * @param sensor Pointer to sensor data.
 * @param output Pointer to encoded data structure.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
int cloud_encode_sensor_data(const struct cloud_sensor_data *const sensor,
				 struct cloud_data *const output);

/**
 * @brief Encode data to be transmitted to the digital twin,
 *	  from sensor data structure to a cloud data structure
 *	  containing a JSON string.
 *
 * @param sensor Pointer to sensor data.
 * @param output Pointer to encoded data structure.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
int cloud_encode_digital_twin_data(const struct cloud_sensor_data *sensor,
				 struct cloud_data *output);

/**
 * @brief Releases memory used by cloud data structure.
 *
 * @param data Pointer to cloud data to be released.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
static inline void cloud_release_data(struct cloud_data *data)
{
	k_free(data->buf);
}
/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* CLOUD_CODEC_H__ */
