/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common_fakes.h"

DEFINE_FAKE_VALUE_FUNC(uint16_t, otMessageGetLength, const otMessage *);
DEFINE_FAKE_VALUE_FUNC(uint16_t, otMessageGetOffset, const otMessage *);
DEFINE_FAKE_VALUE_FUNC(otError, otMessageGetThreadLinkInfo, const otMessage *, otThreadLinkInfo *);
DEFINE_FAKE_VALUE_FUNC(uint16_t, otMessageRead, const otMessage *, uint16_t, void *, uint16_t);
DEFINE_FAKE_VOID_FUNC(otMessageFree, otMessage *);
DEFINE_FAKE_VALUE_FUNC(otError, otMessageAppend, otMessage *, const void *, uint16_t);
DEFINE_FAKE_VALUE_FUNC(otMessage *, otUdpNewMessage, otInstance *, const otMessageSettings *);
DEFINE_FAKE_VALUE_FUNC(void *, nrf_rpc_cbkproxy_out_get, int, void *);
DEFINE_FAKE_VOID_FUNC(otCliInit, otInstance *, otCliOutputCallback, void *);
DEFINE_FAKE_VOID_FUNC(otCliInputLine, char *);
