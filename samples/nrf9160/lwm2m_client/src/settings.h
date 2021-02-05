/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FOTA_STORAGE_H__
#define FOTA_STORAGE_H__

struct update_counter {
	int current;
	int update;
};


enum counter_type {
	COUNTER_CURRENT = 0,
	COUNTER_UPDATE
};

int fota_update_counter_read(struct update_counter *update_counter);
int fota_update_counter_update(enum counter_type type, uint32_t new_value);
int fota_settings_init(void);

#endif	/* FOTA_STORAGE_H__ */
