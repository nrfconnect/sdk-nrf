/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PULSE_MEAS_H
#define PULSE_MEAS_H

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <hal/nrf_gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup pulse_meas Pulse width measurement
 * @brief GPIOTE + TIMER + GPPI pulse width measurement on Nordic nRF devices.
 *
 * The driver measures pulse widths on two GPIO inputs. A TIMER instance is
 * cleared on the pulse start edge and captured on the end edge. Results are
 * delivered as series of pulse widths stored in memory slabs. Each value is in
 * microseconds.
 *
 * @{
 */

/** Size of the FIFO reservation field in a measurement block. */
#define PULSE_MEAS_BLOCK_HEADER_SIZE sizeof(void *)

/** Size of a memory slab block for one pulse width measurement series. */
#define PULSE_MEAS_BLOCK_SIZE(num_meas)                                                        \
	(PULSE_MEAS_BLOCK_HEADER_SIZE + (num_meas) * sizeof(uint32_t))

typedef enum {
	/** Rising edge clears the timer, falling edge latches the timer. */
	PULSE_MEAS_PULSE_POSITIVE,
	/** Falling edge clears the timer, rising edge latches the timer. */
	PULSE_MEAS_PULSE_NEGATIVE,
} pulse_meas_pulse_t;

typedef enum {
	/** One series is captured and the measurement stops automatically. */
	PULSE_MEAS_MODE_ONE_SHOT,
	/** Series are captured continuously until @ref pulse_meas_stop is called. */
	PULSE_MEAS_MODE_CONTINUOUS,
} pulse_meas_mode_t;

struct pulse_meas_config {
	/** Number of pulse widths in one series. */
	uint32_t num_of_meas;
	/** Pulse polarity. See @ref pulse_meas_pulse_t. */
	pulse_meas_pulse_t pulse_type;
	/** Operation mode. See @ref pulse_meas_mode_t. */
	pulse_meas_mode_t mode;
	/** GPIO pull configuration applied to both input pins. */
	nrf_gpio_pin_pull_t pull_config;
	/**
	 * Optional callback invoked when a measurement series completes.
	 *
	 * Called from the GPIOTE interrupt context.
	 */
	void (*user_handler)(void *context);
	/** User context passed to @p user_handler. */
	void *user_context;
};

/**
 * @brief Configure pulse width measurement.
 *
 * Must be called before @ref pulse_meas_start.
 *
 * @param dev Pointer to the pulse width measurement device.
 * @param cfg Pointer to the configuration structure.
 *
 * @retval 0 On success.
 * @retval -EINVAL Invalid argument.
 */
int pulse_meas_configure(const struct device *dev, const struct pulse_meas_config *cfg);

/**
 * @brief Start pulse width measurement.
 *
 * @p slab must point to a memory slab created with @p K_MEM_SLAB_DEFINE.
 * Each slab block must be at least @ref PULSE_MEAS_BLOCK_SIZE bytes large for
 * the configured number of measurements.
 *
 * @param dev Pointer to the pulse width measurement device.
 * @param slab Memory slab used for measurement buffers.
 *
 * @retval 0 On success.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOMEM Not enough memory in @p slab.
 */
int pulse_meas_start(const struct device *dev, struct k_mem_slab *slab);

/**
 * @brief Stop pulse width measurement.
 *
 * The function is non-blocking. When @p immediate is true, the function stops sampling immediately
 * and discards any incomplete series.
 *
 * When @p immediate is false, the driver completes the current series before stopping hardware.
 * While that series is still in progress, @ref pulse_meas_get returns @c -EAGAIN.
 *
 * @param dev Pointer to the pulse width measurement device.
 * @param immediate Stop sampling immediately and discard any incomplete series.
 *
 * @retval 0 On success.
 * @retval -EINVAL Invalid argument.
 */
int pulse_meas_stop(const struct device *dev, bool immediate);

/**
 * @brief Read a completed measurement series.
 *
 * Call @ref pulse_meas_put after processing the returned buffer.
 *
 * @param dev Pointer to the pulse width measurement device.
 * @param data Pointer receiving the pulse width buffer.
 *
 * @retval 0 On success.
 * @retval -EINVAL Invalid argument.
 * @retval -EAGAIN A series is still being captured.
 * @retval -EIO No completed series is available.
 */
int pulse_meas_get(const struct device *dev, uint32_t **data);

/**
 * @brief Release a measurement buffer.
 *
 * @param dev Pointer to the pulse width measurement device.
 * @param data Buffer previously returned by @ref pulse_meas_get.
 */
void pulse_meas_put(const struct device *dev, uint32_t *data);

/**
 * @brief Get the number of completed series waiting to be read.
 *
 * @param dev Pointer to the pulse width measurement device.
 *
 * @return Number of series available via @ref pulse_meas_get.
 */
uint32_t pulse_meas_pending(const struct device *dev);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* PULSE_MEAS_H */
