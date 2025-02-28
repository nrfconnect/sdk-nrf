/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __FUEL_GAUGE_H__
#define __FUEL_GAUGE_H__

enum battery_type {
	/* Cylindrical non-rechargeable Alkaline AA */
	BATTERY_TYPE_ALKALINE_AA,
	/* Cylindrical non-rechargeable Alkaline AAA */
	BATTERY_TYPE_ALKALINE_AAA,
	/* Alkaline coin cell LR44 */
	BATTERY_TYPE_ALKALINE_LR44,
	/* Lithium-manganese dioxide coin cell CR2032 */
	BATTERY_TYPE_LITHIUM_CR2032,
};

int fuel_gauge_init(const struct device *vbat, enum battery_type battery);

int fuel_gauge_update(const struct device *vbat);

#endif /* __FUEL_GAUGE_H__ */
