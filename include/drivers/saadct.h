/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SAADCT_H
#define SAADCT_H

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <nrfx_saadc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup saadct SAADC + TIMER (SAADCT)
 * @brief Timer-triggered SAADC sampling on Nordic nRF devices.
 *
 * SAADCT uses an external TIMER instance and GPPI to trigger the SAADC sample
 * task at a configured rate. Measurement results are delivered as series of
 * interleaved channel samples stored in memory slabs.
 *
 * @{
 */

/** Size of the FIFO reservation field in a measurement block. */
#define SAADCT_MEAS_BLOCK_HEADER_SIZE sizeof(void *)

/** Size of a memory slab block for one SAADCT measurement series. */
#define SAADCT_MEAS_BLOCK_SIZE(num_meas, num_channels)                                         \
	(SAADCT_MEAS_BLOCK_HEADER_SIZE + (num_meas) * (num_channels) * sizeof(nrf_saadc_value_t))

typedef enum {
	/** One series is captured and the measurement stops automatically. */
	saadct_mode_one_shot,
	/** Series are captured continuously until @ref saadct_stop is called. */
	saadct_mode_continuous,
} saadct_mode_t;

/** SAADCT configuration. */
struct saadct_config {
	/** Number of samples per channel in one series. */
	uint32_t num_of_meas;
	/** Number of SAADC channels. */
	uint8_t num_of_channels;
	/** SAADC resolution. */
	nrf_saadc_resolution_t resolution;
	/** Operation mode. See @ref saadct_mode_t. */
	saadct_mode_t mode;
	/** Sample rate in Hz (TIMER-triggered). */
	uint32_t sample_rate_hz;
	/**
	 * Optional callback invoked when a measurement series completes.
	 *
	 * Called from the SAADC interrupt context.
	 */
	void (*user_handler)(void *context);
	/** User context passed to @p user_handler. */
	void *user_context;
	/** SAADC channel configuration array of length @p num_of_channels. */
	nrfx_saadc_channel_t const *channels_config;
};

/**
 * @brief Configure timer-triggered SAADC sampling.
 *
 * Must be called before @ref saadct_start.
 *
 * @param dev Pointer to the SAADCT device.
 * @param cfg Pointer to the configuration structure.
 *
 * @retval 0 On success.
 * @retval -EINVAL Invalid argument.
 */
int saadct_configure(const struct device *dev, const struct saadct_config *cfg);

/**
 * @brief Start timer-triggered SAADC sampling.
 *
 * @p slab must point to a memory slab created with \c K_MEM_SLAB_DEFINE.
 * Each slab block must be at least @ref SAADCT_MEAS_BLOCK_SIZE bytes large for
 * the configured number of measurements and channels.
 *
 * @param dev Pointer to the SAADCT device.
 * @param slab Memory slab used for measurement buffers.
 *
 * @retval 0 On success.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOMEM Not enough memory in @p slab.
 */
int saadct_start(const struct device *dev, struct k_mem_slab *slab);

/**
 * @brief Stop timer-triggered SAADC sampling.
 *
 * The function is non-blocking. When @p immediate is true, sampling stops
 * immediately and any incomplete series is discarded.
 *
 * When @p immediate is false, the driver completes the current series before
 * stopping hardware. While that series is still in progress, @ref saadct_get
 * returns @c -EAGAIN.
 *
 * @param dev Pointer to the SAADCT device.
 * @param immediate Stop sampling immediately and discard any incomplete series.
 *
 * @retval 0 On success.
 * @retval -EINVAL Invalid argument.
 */
int saadct_stop(const struct device *dev, bool immediate);

/**
 * @brief Read a completed measurement series.
 *
 * For blocking operation use @c K_FOREVER for @p timeout parameter.
 * To poll use @c K_NO_WAIT.
 *
 * Call @ref saadct_put after processing the returned buffer.
 *
 * @param dev Pointer to the SAADCT device.
 * @param data Pointer receiving the sample buffer.
 * @param timeout Maximum time to wait for a completed series.
 *
 * @retval 0 On success.
 * @retval -EINVAL Invalid argument.
 * @retval -EAGAIN A series is still being captured but was not ready within @p timeout.
 * @retval -EIO No completed series is available.
 */
int saadct_get(const struct device *dev, nrf_saadc_value_t **data, k_timeout_t timeout);

/**
 * @brief Release a measurement buffer.
 *
 * @param dev Pointer to the SAADCT device.
 * @param data Buffer previously returned by @ref saadct_get.
 */
void saadct_put(const struct device *dev, nrf_saadc_value_t *data);

/**
 * @brief Get the number of completed series waiting to be read.
 *
 * @param dev Pointer to the SAADCT device.
 *
 * @return Number of series available via @ref saadct_get.
 */
uint32_t saadct_pending(const struct device *dev);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* SAADCT_H */
