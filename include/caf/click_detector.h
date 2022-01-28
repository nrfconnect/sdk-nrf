/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * The RST file for this library can be found in doc/nrf/libraries/caf/click_detector.rst.
 * Rendered documentation is available at
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/caf/click_detector.html.
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
