/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __FUEL_GAUGE_H__
#define __FUEL_GAUGE_H__

int fuel_gauge_init(const struct device *charger);

int fuel_gauge_update(const struct device *charger);

#endif /* __FUEL_GAUGE_H__ */
