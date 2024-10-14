/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common_mocks.h"

DEFINE_FAKE_VALUE_FUNC(void *, nrf_rpc_cbkproxy_out_get, int, void *);
