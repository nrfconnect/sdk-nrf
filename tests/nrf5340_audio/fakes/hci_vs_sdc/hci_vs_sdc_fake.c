/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "hci_vs_sdc_fake.h"

DEFINE_FAKE_VALUE_FUNC(int, hci_vs_sdc_iso_read_tx_timestamp,
		       const sdc_hci_cmd_vs_iso_read_tx_timestamp_t *,
		       sdc_hci_cmd_vs_iso_read_tx_timestamp_return_t *);
