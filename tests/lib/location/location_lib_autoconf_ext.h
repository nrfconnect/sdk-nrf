/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Cause nrf/lib/location to act as though CONFIG_NRF_MODEM_LIB is enabled, even though it isn't.
 * (It isn't enabled because we are mocking it)
 */
#define CONFIG_NRF_MODEM_LIB 1
