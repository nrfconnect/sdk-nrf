/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _PASSKEY_BUTTONS_H_
#define _PASSKEY_BUTTONS_H_

#include <caf/key_id.h>

#define DIGIT_CNT 10

struct passkey_input_config {
	const uint16_t key_ids[DIGIT_CNT];
};

#endif /* _PASSKEY_BUTTONS_H_ */
