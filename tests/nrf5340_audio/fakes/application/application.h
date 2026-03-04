/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _APPLICATION_FAKES_H_
#define _APPLICATION_FAKES_H_

#include <zephyr/fff.h>
#include <zephyr/zbus/zbus.h>

DECLARE_FAKE_VALUE_FUNC(int, audio_sync_timer_capture);

#endif /* _APPLICATION_FAKES_H_ */
