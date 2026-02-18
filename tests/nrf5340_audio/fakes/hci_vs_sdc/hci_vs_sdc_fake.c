/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "hci_vs_sdc_fake.h"

DEFINE_FAKE_VALUE_FUNC(int, hci_vs_sdc_iso_read_tx_timestamp,
		       const sdc_hci_cmd_vs_iso_read_tx_timestamp_t *,
		       sdc_hci_cmd_vs_iso_read_tx_timestamp_return_t *);

int hci_vs_sdc_iso_read_tx_timestamp_custom_fake(
	const sdc_hci_cmd_vs_iso_read_tx_timestamp_t *params,
	sdc_hci_cmd_vs_iso_read_tx_timestamp_return_t *return_params)
{
	*return_params = (sdc_hci_cmd_vs_iso_read_tx_timestamp_return_t){0};
	return_params->tx_time_stamp = hci_vs_sdc_iso_read_tx_timestamp_fake.return_val;

	return 0;
}
