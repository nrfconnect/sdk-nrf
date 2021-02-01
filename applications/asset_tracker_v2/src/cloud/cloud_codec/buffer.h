/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file
 *@brief Buffer library header.
 */

#ifndef BUFFER_H__
#define BUFFER_H__

#include <zephyr.h>
#include <stdbool.h>

/** @file
 *
 * @defgroup Buffer buffer
 * @brief    Library that handles data buffers.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Buffer types. Used to signify what buffer, operations is to be
 *         carried out with. Data added and retrieved from buffers has
 *         corresponding data types listed below. The types can be found in
 *         cloud_codec.h
 */
enum buffer_type {
	/* struct cloud_data_ui */
	BUFFER_UI,
	/* struct cloud_data_modem_static */
	BUFFER_MODEM_STATIC,
	/* struct cloud_data_modem_dynamic */
	BUFFER_MODEM_DYNAMIC,
	/* struct cloud_data_accelerometer */
	BUFFER_ACCELEROMETER,
	/* struct cloud_data_gps */
	BUFFER_GPS,
	/* struct cloud_data_sensors */
	BUFFER_SENSOR,
	/* struct cloud_data_battery */
	BUFFER_BATTERY,

	/* Number of buffers. */
	BUFFER_COUNT,
};

/**
 * @brief Get the last data item written to a buffer.
 *
 * @param[in] type Type of buffer the data item will be retrieved from.
 * @param[out] data This pointer will point to the data item after the API call.
 *
 * @return true if the buffer contains a valid last data item.
 * @return false if the buffer does not contain a valid last data item.
 */
bool buffer_last_known_get(enum buffer_type type, void *data);

/**
 * @brief Check if the last data item of a buffer is valid.
 *
 * @param[in] type Type of buffer.
 *
 * @return true if the last item is valid.
 * @return false if the last item is invalid.
 */
bool buffer_last_known_valid(enum buffer_type type);

/**
 * @brief Add data to buffer.
 *
 * @param[in] type Type of buffer data is to be added to.
 * @param[in] data Pointer to data that is to be added to the buffer.
 * @param[in] len Size of the data to be added to the buffer.
 *
 * @return 0 If successful. Otherwise, a (negative) error code is returned.
 */
int buffer_add_data(enum buffer_type type, void *data, size_t len);

/**
 * @brief Get data from buffer.
 * @warning After this API has been called on a buffer,
 *          buffer_get_data_finished() must be called to free the data entry.
 *
 * @param[in] type Type of buffer data is to be retrieved from.
 * @param[out] data This pointer will point to the data item after the API call.
 * @param[in] len Size of the data that is to be retrived from the buffer.
 *
 * @return 0 If successful. Otherwise, a (negative) error code is returned.
 */
int buffer_get_data(enum buffer_type type, void *data, size_t len);

/**
 * @brief Finished getting data from the buffer. Item entry will be freed.
 * @warning This API must be paired with buffer_get_data().
 *
 * @param[in] type Type of buffer data is to be freed.
 * @param[in] len Size of the data that is to be freed.
 *
 * @return 0 If successful. Otherwise, a (negative) error code is returned.
 */
int buffer_get_data_finished(enum buffer_type type, size_t len);

/**
 * @brief Get the item size of a buffer.
 *
 * @param[in] type Type of buffer.
 *
 * @return Size of the buffer item associated with a buffer.
 */
uint32_t buffer_get_item_size(enum buffer_type type);

/**
 * @brief Check if a buffer is empty
 *
 * @param[in] type Type of buffer.
 *
 * @return true If the buffer is empty.
 * @return false If the buffer contains data.
 */
bool buffer_is_empty(enum buffer_type type);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */
#endif /* BUFFER_H__ */
