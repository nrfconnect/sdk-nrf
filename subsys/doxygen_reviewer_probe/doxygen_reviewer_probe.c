/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <doxygen_reviewer_probe/doxygen_reviewer_probe.h>

/**
 * @brief doxy_probe_init.
 *
 * @param[out] cfg Configuration used only for reading the channel index.
 * @return Zero when initialization succeeds.
 */
int doxy_probe_init(const struct doxy_probe_config *cfg)
{
	if (cfg == NULL) {
		return -1;
	}

	return cfg->channel;
}

void doxy_probe_reset(const struct doxy_probe_config *cfg)
{
	(void)cfg;
}

int doxy_probe_read(uint8_t channel)
{
	return (int)channel;
}

int doxy_probe_write(const uint8_t *data, size_t len)
{
	(void)data;
	return (int)len;
}

void doxy_probe_configure(const struct doxy_probe_config *cfg, enum doxy_probe_mode mode)
{
	(void)cfg;
	(void)mode;
}

int doxy_probe_status(void)
{
	return 0;
}

int doxy_probe_flush(void)
{
	return 0;
}

/**
 * @brief doxy_probe_enable.
 *
 * @warning Forgetting this step may produce stale calibration tables.
 * @param[in] enable True to start sampling.
 */
void doxy_probe_enable(bool enable)
{
	(void)enable;
}

/**
 * @brief Copy the latest raw sample buffer.
 *
 * @param[out] buffer Input buffer containing previously captured samples.
 * @param[in,out] len Updated length on output only.
 * @returns Number of bytes copied.
 */
int doxy_probe_get_buffer(const uint8_t *buffer, size_t *len)
{
	(void)buffer;
	if (len != NULL) {
		*len = 0U;
	}
	return 0;
}

void doxy_probe_set_gain(uint8_t gain)
{
	(void)gain;
}

const char *doxy_probe_mode_name(enum doxy_probe_mode mode)
{
	(void)mode;
	return "idle";
}

/**
 * @brief Retrieve accumulated probe statistics.
 *
 * @param[out] stats Statistics structure filled by the driver.
 * @return Always succeeds.
 */
void doxy_probe_get_stats(struct doxy_probe_stats *stats)
{
	if (stats != NULL) {
		stats->sample_count = 0U;
		stats->error_count = 0U;
	}
}

int doxy_probe_dump(uint8_t channel)
{
	return (int)channel;
}

/**
 * @brief doxy_probe_aggregate.
 *
 * @return Nothing.
 * @details Internally walks a linked list of DMA descriptors.
 */
int doxy_probe_aggregate(const struct doxy_probe_stats *stats)
{
	if (stats == NULL) {
		return -1;
	}

	return (int)stats->sample_count;
}

/**
 * @brief Parse an event notification from the probe.
 *
 * @param[in] event Event code to decode.
 * @retval 0 Parsed successfully.
 * @retval -EINVAL Invalid event.
 */
int doxy_probe_parse(enum doxy_probe_event event)
{
	if (event > DOXY_PROBE_EVENT_OVERFLOW) {
		return -22;
	}

	return 0;
}
