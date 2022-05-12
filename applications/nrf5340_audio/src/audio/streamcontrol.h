/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _STREAMCONTROL_H_
#define _STREAMCONTROL_H_

#include <zephyr.h>

void le_audio_rx_data_handler(uint8_t const *const p_data, size_t data_size, bool bad_frame,
			      uint32_t sdu_ref);

#endif /* _STREAMCONTROL_H_ */
