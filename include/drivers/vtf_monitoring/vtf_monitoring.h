/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _VTF_MONITORING_H_
#define _VTF_MONITORING_H_

#include <stddef.h>
#include <stdint.h>
#include <zephyr/sys/iterable_sections.h>

/**
 * @defgroup vtf_monitoring VTF monitoring
 * @{
 * @brief Driver to capture data to help ensure optimal performance of the Wi-Fi® system.
 *
 */

/** Channel identifiers. Keep VTF_CH_COUNT last. */
enum vtf_channel_id {
	VTF_CH_BATTERY_VOLTAGE,
	VTF_CH_DIE_TEMP,
	VTF_CH_FREQ_OFFSET,
	VTF_CH_COUNT,
};

/** Per-sample status reported back to consumers. */
enum vtf_sample_status {
	/** Fresh reading from the live channel. */
	VTF_STATUS_OK,
	/** Live channel enabled but has not produced a reading yet. */
	VTF_STATUS_UNINITIALISED,
	/** Live channel produced an error on its last fetch. */
	VTF_STATUS_ERROR,
};

enum vtf_sample_type {
	VTF_SAMPLE_TYPE_INT,
	VTF_SAMPLE_TYPE_UINT,
	VTF_SAMPLE_TYPE_FLOAT,
};

union vtf_sample_value {
	int32_t i32;
	uint32_t u32;
	float f32;
};

/** A single channel reading. */
struct vtf_sample {
	enum vtf_sample_type type;
	union vtf_sample_value value;
	uint64_t timestamp_ms;
	enum vtf_sample_status status;
};

struct vtf_channel {
	/** Channel id this descriptor implements. */
	enum vtf_channel_id id;
	/** Type tag for the default value below. */
	enum vtf_sample_type default_type;
	/** Default value returned with ``VTF_STATUS_UNINITIALISED`` until the
	 *  first live reading lands.
	 */
	union vtf_sample_value default_value;
	/**
	 * Fetch the channel's latest sample.
	 *
	 * Must be non-NULL. Returns 0 on success or a negative errno;
	 * on error, the aggregator marks the snapshot entry with
	 * ``VTF_STATUS_ERROR`` and keeps the previous value.
	 */
	int (*sample)(struct vtf_sample *out);
	/**
	 * Optional channel bring-up. Called once from
	 * ``vtf_monitoring_init()``. May be NULL.
	 */
	int (*init)(void);
};

/**
 * Register a channel with the VTF subsystem.
 *
 * Expands to a ``static const struct vtf_channel`` placed in the
 * ``vtf_channel`` iterable section.
 *
 * @param _sym         C symbol name for the descriptor.
 * @param _id          ``enum vtf_channel_id`` this channel handles.
 * @param _sample_fn   Per-channel sample callback. Must be non-NULL.
 * @param _init_fn     Optional init callback (NULL if not needed).
 * @param _type        Default value's ``enum vtf_sample_type``.
 * @param _field       Member of ``union vtf_sample_value`` to assign
 *                     the default into (for example, ``i32``).
 * @param _default     Default reading value.
 */
#define VTF_CHANNEL_DEFINE(_sym, _id, _sample_fn, _init_fn, _type, _field, _default)               \
	const int unique_id_##_id = 0;                                                             \
	static const STRUCT_SECTION_ITERABLE(vtf_channel, _sym) = {                                \
		.id = (_id),                                                                       \
		.sample = (_sample_fn),                                                            \
		.init = (_init_fn),                                                                \
		.default_type = (_type),                                                           \
		.default_value._field = (_default),                                                \
	}

/**
 * @}
 */

#endif /* _VTF_MONITORING_H_ */
