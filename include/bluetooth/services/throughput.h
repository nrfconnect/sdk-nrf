/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_THROUGHPUT_H_
#define BT_THROUGHPUT_H_

/**
 * @file
 * @defgroup bt_throughput Bluetooth LE GATT Throughput Service API
 * @{
 * @brief API for the Bluetooth LE GATT Throughput Service.
 */

#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/gatt_dm.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Throughput metrics. */
struct bt_throughput_metrics {

        /** Number of GATT writes received. */
	uint32_t write_count;

        /** Number of bytes received. */
	uint32_t write_len;

        /** Transfer speed in bits per second. */
	uint32_t write_rate;
};

/** @brief Throughput callback structure. */
struct bt_throughput_cb {
	/** @brief Data read callback.
	 *
	 * This function is called when data has been read from the
	 * Throughput Characteristic.
	 *
	 * @param[in] met Throughput metrics.
	 *
	 * @retval BT_GATT_ITER_CONTINUE To keep notifications enabled.
	 * @retval BT_GATT_ITER_STOP To disable notifications.
	 */
	uint8_t (*data_read)(const struct bt_throughput_metrics *met);

	/** @brief Data received callback.
	 *
	 * This function is called when data has been received by the
	 * Throughput Characteristic.
	 *
	 * @param[in] met Throughput metrics.
	 */
	void (*data_received)(const struct bt_throughput_metrics *met);

	/** @brief Data send callback.
	 *
	 * This function is called when data has been sent to the
	 * Throughput Characteristic.
	 *
	 * @param[in] met Throughput metrics.
	 */
	void (*data_send)(const struct bt_throughput_metrics *met);
};

/** @brief Throughput structure. */
struct bt_throughput {
	/** Throughput Characteristic handle. */
	uint16_t char_handle;

	/** GATT read parameters for the Throughput Characteristic. */
	struct bt_gatt_read_params read_params;

	/** Throughput callback structure. */
	struct bt_throughput_cb *cb;

	/** Connection object. */
	struct bt_conn *conn;
};

/** @brief Throughput Characteristic UUID. */
#define BT_UUID_THROUGHPUT_CHAR BT_UUID_DECLARE_16(0x1524)

#define BT_UUID_THROUGHPUT_VAL \
	BT_UUID_128_ENCODE(0x0483dadd, 0x6c9d, 0x6ca9, 0x5d41, 0x03ad4fff4abb)

/** @brief Throughput Service UUID. */
#define BT_UUID_THROUGHPUT                                                     \
	BT_UUID_DECLARE_128(BT_UUID_THROUGHPUT_VAL)

/** @brief Initialize the GATT Throughput Service.
 *
 *  @param[in] throughput Throughput Service instance.
 *  @param[in] cb Callbacks.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a negative error code is returned.
 */
int bt_throughput_init(struct bt_throughput *throughput,
		       const struct bt_throughput_cb *cb);

/** @brief Assign handles to the Throughput Service instance.
 *
 * This function should be called when a link with a peer has been established,
 * to associate the link to this instance of the module. This makes it
 * possible to handle several links and associate each link to a particular
 * instance of this module. The GATT attribute handles are provided by the
 * GATT Discovery Manager.
 *
 * @param[in] dm Discovery object.
 * @param[in,out] throughput Throughput Service instance.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 * @retval (-ENOTSUP) Special error code used when the UUID
 *         of the service does not match the expected UUID.
 */
int bt_throughput_handles_assign(struct bt_gatt_dm *dm,
				 struct bt_throughput *throughput);

/** @brief Read data from the server.
 *
 *  @note This procedure is asynchronous.
 *
 *  @param[in] throughput Throughput Service instance.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a negative error code is returned.
 */
int bt_throughput_read(struct bt_throughput *throughput);

/** @brief Write data to the server.
 *
 *  @param[in] throughput Throughput Service instance.
 *  @param[in] data Data.
 *  @param[in] len Data length.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a negative error code is returned.
 */
int bt_throughput_write(struct bt_throughput *throughput,
			const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_THROUGHPUT_H_ */
