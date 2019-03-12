/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @defgroup bt_gatt_throughput BLE GATT Throughput Service API
 * @{
 * @brief API for the BLE GATT Throughput Service.
 */

#ifndef BT_GATT_THROUGHPUT_H_
#define BT_GATT_THROUGHPUT_H_

#include <bluetooth/uuid.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt_dm.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Throughput metrics. */
struct bt_gatt_throughput_metrics {

        /** Number of GATT writes received. */
	u32_t write_count;

        /** Number of bytes received. */
	u32_t write_len;

        /** Transfer speed in bits per second. */
	u32_t write_rate;
};

/** @brief Throughput callback structure. */
struct bt_gatt_throughput_cb {
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
	u8_t (*data_read)(const struct bt_gatt_throughput_metrics *met);

	/** @brief Data received callback.
	 *
	 * This function is called when data has been received by the
	 * Throughput Characteristic.
	 *
	 * @param[in] met Throughput metrics.
	 */
	void (*data_received)(const struct bt_gatt_throughput_metrics *met);

	/** @brief Data send callback.
	 *
	 * This function is called when data has been sent to the
	 * Throughput Characteristic.
	 *
	 * @param[in] met Throughput metrics.
	 */
	void (*data_send)(const struct bt_gatt_throughput_metrics *met);
};

/** @brief Throughput structure. */
struct bt_gatt_throughput {
	/** Throughput Characteristic handle. */
	u16_t char_handle;

	/** GATT read parameters for the Throughput Characteristic. */
	struct bt_gatt_read_params read_params;

	/** Throughput callback structure. */
	struct bt_gatt_throughput_cb *cb;

	/** Connection object. */
	struct bt_conn *conn;
};

/** @brief Throughput Characteristic UUID. */
#define BT_UUID_THROUGHPUT_CHAR BT_UUID_DECLARE_16(0x1524)

/** @brief Throughput Service UUID. */
#define BT_UUID_THROUGHPUT                                                     \
	BT_UUID_DECLARE_128(0xBB, 0x4A, 0xFF, 0x4F, 0xAD, 0x03, 0x41, 0x5D,    \
			    0xA9, 0x6C, 0x9D, 0x6C, 0xDD, 0xDA, 0x83, 0x04)

/** @brief Initialize the GATT Throughput Service.
 *
 *  @param[in] throughput Throughput Service instance.
 *  @param[in] cb Callbacks.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a negative error code is returned.
 */
int bt_gatt_throughput_init(struct bt_gatt_throughput *throughput,
			    const struct bt_gatt_throughput_cb *cb);

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
int bt_gatt_throughput_handles_assign(struct bt_gatt_dm *dm,
				      struct bt_gatt_throughput *throughput);

/** @brief Read data from the server.
 *
 *  @note This procedure is asynchronous.
 *
 *  @param[in] throughput Throughput Service instance.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a negative error code is returned.
 */
int bt_gatt_throughput_read(struct bt_gatt_throughput *throughput);

/** @brief Write data to the server.
 *
 *  @param[in] throughput Throughput Service instance.
 *  @param[in] data Data.
 *  @param[in] len Data length.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a negative error code is returned.
 */
int bt_gatt_throughput_write(struct bt_gatt_throughput *throughput,
			     const u8_t *data, u16_t len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_GATT_THROUGHPUT_H_ */
