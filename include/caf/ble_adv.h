/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BLE_ADV_H_
#define _BLE_ADV_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct caf_ble_adv_state {
	uint8_t bond_cnt;
	bool grace_period;
};

struct caf_ble_adv_data {
	const struct bt_data *ad;
	size_t ad_size;
	const struct bt_data *sd;
	size_t sd_size;
	bool grace_period_needed;
};

#ifdef __cplusplus
}
#endif

#endif /* _BLE_ADV_H_ */
