/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _WIFI_RECAL_MONITOR_H_
#define _WIFI_RECAL_MONITOR_H_

#include <stddef.h>
#include <stdint.h>
#include <zephyr/sys/iterable_sections.h>

/**
 * @defgroup wifi_recal_monitor Wi-Fi recalibration monitor
 * @{
 * @brief Driver to capture operating-condition data that triggers Wi-Fi® recalibration.
 *
 */

/** Channel identifiers. Keep WIFI_RECAL_CH_COUNT last. */
enum wifi_recal_channel_id {
	WIFI_RECAL_CH_BATTERY_VOLTAGE,
	WIFI_RECAL_CH_DIE_TEMP,
	WIFI_RECAL_CH_COUNT,
};

/** Per-sample status reported back to consumers. */
enum wifi_recal_sample_status {
	/** Fresh reading from the live channel. */
	WIFI_RECAL_STATUS_OK,
	/** Live channel enabled but has not produced a reading yet. */
	WIFI_RECAL_STATUS_UNINITIALISED,
	/** Live channel produced an error on its last fetch. */
	WIFI_RECAL_STATUS_ERROR,
};

enum wifi_recal_sample_type {
	WIFI_RECAL_SAMPLE_TYPE_INT,
	WIFI_RECAL_SAMPLE_TYPE_UINT,
	WIFI_RECAL_SAMPLE_TYPE_FLOAT,
};

union wifi_recal_sample_value {
	int32_t i32;
	uint32_t u32;
	float f32;
};

/** A single channel reading. */
struct wifi_recal_sample {
	enum wifi_recal_sample_type type;
	union wifi_recal_sample_value value;
	uint64_t timestamp_ms;
	enum wifi_recal_sample_status status;
};

struct wifi_recal_channel {
	/** Channel id this descriptor implements. */
	enum wifi_recal_channel_id id;
	/** Type tag for the default value below. */
	enum wifi_recal_sample_type default_type;
	/** Default value returned with ``WIFI_RECAL_STATUS_UNINITIALISED`` until the
	 *  first live reading lands.
	 */
	union wifi_recal_sample_value default_value;
	/**
	 * Fetch the channel's latest sample.
	 *
	 * Must be non-NULL. Returns 0 on success or a negative errno;
	 * on error, the aggregator marks the snapshot entry with
	 * ``WIFI_RECAL_STATUS_ERROR`` and keeps the previous value.
	 */
	int (*sample)(struct wifi_recal_sample *out);
	/**
	 * Optional channel bring-up. Called once from
	 * ``wifi_recal_monitor_init()``. May be NULL.
	 */
	int (*init)(void);
};

/**
 * Register a provider for a channel with the Wi-Fi recalibration monitor.
 *
 * Expands to a ``static const struct wifi_recal_channel`` placed in the
 * ``wifi_recal_channel`` iterable section. This is the sole registration
 * contract for providers, built-in or customer-supplied: a board
 * registers at most one provider per channel. The ``unique_id_##_id``
 * symbol below has external linkage on purpose, so a second provider
 * registered for the same ``_id`` fails the link with "multiple
 * definition" instead of silently overwriting or racing at runtime.
 *
 * @param _sym         C symbol name for the descriptor.
 * @param _id          ``enum wifi_recal_channel_id`` this channel handles.
 * @param _sample_fn   Per-channel sample callback. Must be non-NULL.
 * @param _init_fn     Optional init callback (NULL if not needed).
 * @param _type        Default value's ``enum wifi_recal_sample_type``.
 * @param _field       Member of ``union wifi_recal_sample_value`` to assign
 *                     the default into (for example, ``i32``).
 * @param _default     Default reading value.
 */
#define WIFI_RECAL_CHANNEL_DEFINE(_sym, _id, _sample_fn, _init_fn, _type, _field, _default)               \
	const int unique_id_##_id = 0;                                                             \
	static const STRUCT_SECTION_ITERABLE(wifi_recal_channel, _sym) = {                                \
		.id = (_id),                                                                       \
		.sample = (_sample_fn),                                                            \
		.init = (_init_fn),                                                                \
		.default_type = (_type),                                                           \
		.default_value._field = (_default),                                                \
	}

/**
 * Fetch the latest snapshot value for a channel.
 *
 * Returns the most recent value captured by the aggregator (or the
 * channel's default value when the channel has no live update). The
 * value is returned untyped; the caller is expected to know the channel's
 * ``enum wifi_recal_sample_type`` and read the matching union member.
 *
 * @param id   Channel to read.
 * @param out  Destination for the snapshot value. Must be non-NULL.
 *
 * @retval 0        On success.
 * @retval -EINVAL  If @p out is NULL or @p id is out of range.
 */
int wifi_recal_monitor_sample_get(enum wifi_recal_channel_id id, union wifi_recal_sample_value *out);

/**
 * @}
 */

#endif /* _WIFI_RECAL_MONITOR_H_ */
