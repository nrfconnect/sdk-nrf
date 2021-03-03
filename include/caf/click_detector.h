/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CLICK_DETECTOR_H_
#define _CLICK_DETECTOR_H_

#ifdef __cplusplus
extern "C" {
#endif

struct click_detector_config {
	uint16_t key_id;
	bool consume_button_event;
};

#ifdef __cplusplus
}
#endif

#endif /* _CLICK_DETECTOR_H_ */
