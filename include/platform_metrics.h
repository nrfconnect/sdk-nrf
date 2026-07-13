/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _PLATFORM_METRICS_H_
#define _PLATFORM_METRICS_H_

#include <stddef.h>
#include <stdint.h>
#include <zephyr/sys/iterable_sections.h>

/**
 * @defgroup platform_metrics Platform metrics
 * @{
 * @brief Driver to capture platform operating-condition data, such as battery
 *        voltage and die temperature.
 *
 */

/** Channel identifiers. Keep PLATFORM_METRICS_CH_COUNT last. */
enum platform_metrics_channel_id {
	PLATFORM_METRICS_CH_BATTERY_VOLTAGE,
	PLATFORM_METRICS_CH_DIE_TEMP,
	PLATFORM_METRICS_CH_COUNT,
};

/** Per-sample status reported back to consumers. */
enum platform_metrics_sample_status {
	/** Fresh reading from the live channel. */
	PLATFORM_METRICS_STATUS_OK,
	/** Live channel enabled but has not produced a reading yet. */
	PLATFORM_METRICS_STATUS_UNINITIALISED,
	/** Live channel produced an error on its last fetch. */
	PLATFORM_METRICS_STATUS_ERROR,
};

enum platform_metrics_sample_type {
	PLATFORM_METRICS_SAMPLE_TYPE_INT,
	PLATFORM_METRICS_SAMPLE_TYPE_UINT,
	PLATFORM_METRICS_SAMPLE_TYPE_FLOAT,
};

union platform_metrics_sample_value {
	int32_t i32;
	uint32_t u32;
	float f32;
};

/** A single channel reading. */
struct platform_metrics_sample {
	enum platform_metrics_sample_type type;
	union platform_metrics_sample_value value;
	uint64_t timestamp_ms;
	enum platform_metrics_sample_status status;
};

struct platform_metrics_channel {
	/** Channel id this descriptor implements. */
	enum platform_metrics_channel_id id;
	/** Type tag for the default value below. */
	enum platform_metrics_sample_type default_type;
	/** Default value returned with ``PLATFORM_METRICS_STATUS_UNINITIALISED`` until the
	 *  first live reading lands.
	 */
	union platform_metrics_sample_value default_value;
	/**
	 * Fetch the channel's latest sample.
	 *
	 * Must be non-NULL. Returns 0 on success or a negative errno;
	 * on error, the aggregator marks the snapshot entry with
	 * ``PLATFORM_METRICS_STATUS_ERROR`` and keeps the previous value.
	 */
	int (*sample)(struct platform_metrics_sample *out);
	/**
	 * Optional channel bring-up. Called once from
	 * ``platform_metrics_init()``. May be NULL.
	 */
	int (*init)(void);
};

/**
 * Register a provider for a channel with the platform metrics library.
 *
 * Expands to a ``static const struct platform_metrics_channel`` placed in the
 * ``platform_metrics_channel`` iterable section. This is the sole registration
 * contract for providers, built-in or customer-supplied: a board
 * registers at most one provider per channel. The ``unique_id_##_id``
 * symbol below has external linkage on purpose, so a second provider
 * registered for the same ``_id`` fails the link with "multiple
 * definition" instead of silently overwriting or racing at runtime.
 *
 * @param _sym         C symbol name for the descriptor.
 * @param _id          ``enum platform_metrics_channel_id`` this channel handles.
 * @param _sample_fn   Per-channel sample callback. Must be non-NULL.
 * @param _init_fn     Optional init callback (NULL if not needed).
 * @param _type        Default value's ``enum platform_metrics_sample_type``.
 * @param _field       Member of ``union platform_metrics_sample_value`` to assign
 *                     the default into (for example, ``i32``).
 * @param _default     Default reading value.
 */
#define PLATFORM_METRICS_CHANNEL_DEFINE(_sym, _id, _sample_fn, _init_fn, _type, _field, _default) \
	const int unique_id_##_id = 0; \
	static const STRUCT_SECTION_ITERABLE(platform_metrics_channel, _sym) = { \
		.id = (_id), \
		.sample = (_sample_fn), \
		.init = (_init_fn), \
		.default_type = (_type), \
		.default_value._field = (_default), \
	}

/**
 * Fetch the latest snapshot value for a channel.
 *
 * Returns the most recent value captured by the aggregator (or the
 * channel's default value when the channel has no live update). The
 * value is returned untyped; the caller is expected to know the channel's
 * ``enum platform_metrics_sample_type`` and read the matching union member.
 *
 * @param id   Channel to read.
 * @param out  Destination for the snapshot value. Must be non-NULL.
 *
 * @retval 0        On success.
 * @retval -EINVAL  If @p out is NULL or @p id is out of range.
 */
int platform_metrics_sample_get(enum platform_metrics_channel_id id,
				   union platform_metrics_sample_value *out);

/**
 * @}
 */

#endif /* _PLATFORM_METRICS_H_ */
