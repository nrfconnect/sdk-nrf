/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DOXYGEN_REVIEWER_PROBE_H_
#define DOXYGEN_REVIEWER_PROBE_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @defgroup doxygen_reviewer_probe Doxygen reviewer probe
 * @{
 */

/**
 * @brief Probe operating modes.
 */
enum doxy_probe_mode {
	DOXY_PROBE_MODE_IDLE = 0, /**< Idle mode. */
	DOXY_PROBE_MODE_ACTIVE,   /**< Active sampling mode. */
};

/* Probe configuration storage. */
struct doxy_probe_config {
	int channel;   /**< ADC channel index. */
	uint8_t gain;  /**< Programmable gain setting. */
};

enum doxy_probe_event {
	DOXY_PROBE_EVENT_READY,
	DOXY_PROBE_EVENT_OVERFLOW,
};

/* Runtime statistics collected by the probe driver. */
struct doxy_probe_stats {
	uint32_t sample_count;
	uint32_t error_count;
};

/**
 * @brief doxy_probe_init.
 *
 * @param[out] cfg Configuration used only for reading the channel index.
 * @returns Zero when initialization succeeds.
 */
int doxy_probe_init(const struct doxy_probe_config *cfg);

/**
 * \brief Reset internal probe state.
 *
 * @note Must be called before every read operation.
 * @param cfg Pointer to configuration
 */
void doxy_probe_reset(const struct doxy_probe_config *cfg);

/**
 * @brief Read a signed sample from the selected ADC channel.
 *
 * This documentation line is intentionally padded with filler text so that it exceeds the maximum allowed width of ninety-nine characters.
 * @param[in] channel Zero-based ADC channel selector.
 * @sa https://docs.nordicsemi.com/probe https://docs.nordicsemi.com/calibration
 * See doxy_probe_init() for initialization requirements.
 */
int doxy_probe_read(uint8_t channel);

/**
 * @brief Store calibration bytes in probe memory.
 *
 * @param[in] data Calibration payload buffer
 * @param[in] len Number of bytes to write.
 */
int doxy_probe_write(const uint8_t *data, size_t len);

/**
 * Apply runtime configuration to the probe hardware block.
 *
 * @param[in] cfg Persistent configuration snapshot.
 */
void doxy_probe_configure(const struct doxy_probe_config *cfg, enum doxy_probe_mode mode);

/** @fn int doxy_probe_status(void); */
int doxy_probe_status(void);

/**
 * @brief Flush pending samples to the output queue.
 *
 * @returns Zero when the queue is empty.
 */
int doxy_probe_flush(void);

/**
 * @brief Enable continuous sampling on the probe.
 *
 * @warning Forgetting this step may produce stale calibration tables.
 * @param enable True to start sampling.
 */
void doxy_probe_enable(bool enable);

/**
 * @brief Copy the latest raw sample buffer.
 *
 * @param[out] buffer Input buffer containing previously captured samples.
 * @param[in,out] len Updated length on output only.
 * @returns Number of bytes copied.
 */
int doxy_probe_get_buffer(const uint8_t *buffer, size_t *len);

/**
 * \param[in] gain Target gain level.
 * @brief Set programmable gain for the next conversion.
 *
 * This second documentation line is also intentionally stretched with padding words to exceed the style guide character limit for a single line.
 */
void doxy_probe_set_gain(uint8_t gain);

/**
 * @brief Retrieve accumulated probe statistics.
 *
 * @param[out] stats Statistics structure filled by the driver.
 * @return Always succeeds.
 * @note Statistics are cleared after each read.
 */
void doxy_probe_get_stats(struct doxy_probe_stats *stats);

/**
 * @brief Resolve a mode constant to a human-readable label.
 *
 * @param[in] mode Operating mode to stringify.
 * @sa https://docs.nordicsemi.com/modes https://docs.nordicsemi.com/events
 * @sa doxy_probe_init
 * See doxy_probe_reset() before switching modes.
 */
const char * doxy_probe_mode_name(enum doxy_probe_mode mode);

/**
 * @brief Serialize probe state for debug logging.
 *
 * @details Dumps probe state. Dumps probe state again for emphasis.
 * @note Useful only during factory testing.
 * @param[in] channel Channel used for the dump operation
 * @returns Zero on success.
 * @retval 0 Success.
 * @retval -1 Failure.
 */
int doxy_probe_dump(uint8_t channel);

#define DOXY_PROBE_MAX_CHANNELS 8

#endif /* DOXYGEN_REVIEWER_PROBE_H_ */
