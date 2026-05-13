/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HCI_VS_SDC_FAKE_H__
#define HCI_VS_SDC_FAKE_H__

#include <zephyr/fff.h>
#include <bluetooth/hci_vs_sdc.h>

DECLARE_FAKE_VALUE_FUNC(int, hci_vs_sdc_iso_read_tx_timestamp,
			const sdc_hci_cmd_vs_iso_read_tx_timestamp_t *,
			sdc_hci_cmd_vs_iso_read_tx_timestamp_return_t *);

int hci_vs_sdc_iso_read_tx_timestamp_custom_fake(
	const sdc_hci_cmd_vs_iso_read_tx_timestamp_t *params,
	sdc_hci_cmd_vs_iso_read_tx_timestamp_return_t *return_params);

#endif /* HCI_VS_SDC_FAKE_H__ */
