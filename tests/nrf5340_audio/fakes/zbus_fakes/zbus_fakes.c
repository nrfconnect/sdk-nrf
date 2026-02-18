/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "zbus_fakes.h"

DEFINE_FAKE_VALUE_FUNC(int, zbus_chan_pub, const struct zbus_channel *, const void *, k_timeout_t);
