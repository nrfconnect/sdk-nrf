/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_PPI_SEQ_I2C_SPI_H__
#define NRF_PPI_SEQ_I2C_SPI_H__

/**@file
 *
 * @defgroup ppi_seq_i2c_spi PPI Sequencer for I2C/SPI
 * @{
 * @ingroup ppi_seq
 *
 * @brief Zephyr device for triggering periodic I2C/SPI transfers.
 *
 */

#include <string.h>
#include <nrfx_spim.h>
#include <nrfx_twim.h>
#include <ppi_seq/ppi_seq.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Generic transfer descriptor. */
union ppi_seq_i2c_spi_xfer_desc {
	/** SPIM transfer descriptor. */
	nrfx_spim_xfer_desc_t spim;

	/** TWIM transfer descriptor. */
	nrfx_twim_xfer_desc_t twim;
};

/** @brief Data passed to the callback on the batch completion. */
struct ppi_seq_i2c_spi_batch {
	/** Transfer details. RX and TX (if TX postincrementation was used) buffers
	 * contain data for all transfers executed in the batch.
	 */
	union ppi_seq_i2c_spi_xfer_desc desc;

	/** Number of completed transfers. */
	uint8_t batch_cnt;
};

/* Forward declaration. */
struct ppi_seq_i2c_spi;

/** @brief Callback called on batch completion.
 *
 * @param dev  I2C/SPI PPI sequencer instance.
 * @param batch Structure with data for the completed batch.
 * @param last True to indicate that it is the last callback and sequencer will be stopped.
 * @param user_data User data.
 */
typedef void (*ppi_seq_i2c_spi_cb_t)(const struct device *dev, struct ppi_seq_i2c_spi_batch *batch,
				     bool last, void *user_data);

/** @brief I2C/SPI PPI Sequencer job description.
 *
 * Descriptor provides pair of RX and TX buffers that will be used alternately for a batch.
 * First pair of buffers is provided in the standard nrfx_spim transfer descriptor.
 * RX buffer must be allocated for batch_cnt transfers.
 * TX buffer must have filled data for batch_cnt transfers if postincrementation is used.
 */
struct ppi_seq_i2c_spi_job {
	/** Transfer descriptor. Specifies RX and TX length and provides first pair of buffers. */
	union ppi_seq_i2c_spi_xfer_desc desc;

	/** Second TX buffer. Can be NULL if TX postincrementation is not used. */
	uint8_t *tx_second_buf;

	/** Second RX buffer. */
	uint8_t *rx_second_buf;

	/** After each completion of that number of transfers there is an interrupt and
	 * a callback.
	 */
	uint8_t batch_cnt;

	/** Number of times that batch shall be repeated. UINT32_MAX will continue until stopped. */
	size_t repeat;

	/** True to use TX postincrementation. It is needed if different data need to be transferred
	 * in each transfer.
	 */
	bool tx_postinc;

	/** Callback. */
	ppi_seq_i2c_spi_cb_t cb;

	/** User data. */
	void *user_data;
};

/** @brief Start the I2C/SPI PPI sequencer.
 *
 * @param dev I2C/SPI PPI sequencer.
 * @param period Period (in microseconds).
 * @param job Job descriptor.
 *
 * @retval 0 Successful start.
 * @retval negative Initialization failed.
 */
int ppi_seq_i2c_spi_start(const struct device *dev, size_t period, struct ppi_seq_i2c_spi_job *job);

/** @brief Stop the I2C/SPI PPI sequencer.
 *
 * @param dev I2C/SPI PPI sequencer.
 * @param immediate If true sequencer is stopped immediately.
 *
 * @retval 0 Successful stop.
 * @retval negative Stopping failed.
 */
int ppi_seq_i2c_spi_stop(const struct device *dev, bool immediate);

/** @brief Perform a single synchronous transfer.
 *
 * Instance can be used to perform additional synchronous operations like sensor initialization.
 * Module does not perform any target specific operations like cache handling or specific buffer
 * placement. It must be handled by the user.
 *
 * @param dev I2C/SPI PPI sequencer.
 * @param desc I2C/SPI transfer descriptor.
 *
 * @retval 0 Successful transfer.
 * @retval negative Transfer failed.
 */
int ppi_seq_i2c_spi_xfer(const struct device *dev, union ppi_seq_i2c_spi_xfer_desc *desc);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* NRF_PPI_SEQ_SPIM_H__ */
