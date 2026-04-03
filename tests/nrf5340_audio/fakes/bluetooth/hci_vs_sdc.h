/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HCI_VS_SDC_FAKE_INCLUDE_H__
#define HCI_VS_SDC_FAKE_INCLUDE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint16_t conn_handle;
} sdc_hci_cmd_vs_iso_read_tx_timestamp_t;

typedef struct {
	uint32_t tx_time_stamp;
	uint16_t packet_sequence_number;
} sdc_hci_cmd_vs_iso_read_tx_timestamp_return_t;

int hci_vs_sdc_iso_read_tx_timestamp(const sdc_hci_cmd_vs_iso_read_tx_timestamp_t *params,
				     sdc_hci_cmd_vs_iso_read_tx_timestamp_return_t *return_params);

#ifdef __cplusplus
}
#endif

#endif /* HCI_VS_SDC_FAKE_INCLUDE_H__ */
