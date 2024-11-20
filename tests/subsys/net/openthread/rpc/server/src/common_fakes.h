/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __COMMON_FAKES_H__
#define __COMMON_FAKES_H__
#include <zephyr/fff.h>
#include <openthread/message.h>

DECLARE_FAKE_VALUE_FUNC(otMessage *, otUdpNewMessage, otInstance *, const otMessageSettings *);
DECLARE_FAKE_VALUE_FUNC(uint16_t, otMessageGetLength, const otMessage *);
DECLARE_FAKE_VALUE_FUNC(uint16_t, otMessageGetOffset, const otMessage *);
DECLARE_FAKE_VALUE_FUNC(uint16_t, otMessageRead, const otMessage *, uint16_t, void *, uint16_t);
DECLARE_FAKE_VOID_FUNC(otMessageFree, otMessage *);
DECLARE_FAKE_VALUE_FUNC(otError, otMessageAppend, otMessage *, const void *, uint16_t);

#endif
