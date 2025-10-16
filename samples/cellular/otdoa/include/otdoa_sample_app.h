/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

 #include <stdint.h>

int otdoa_sample_main(void);

/* Default timeout for OTDOA in milliseconds */
#define OTDOA_SAMPLE_DEFAULT_TIMEOUT_MS (2 * 60 * 1000)

/* API functions called from the shell */
void otdoa_sample_start(uint32_t session_length, uint32_t capture_flags, uint32_t timeout_msec);
void otdoa_sample_cancel(void);
void otdoa_sample_ubsa_dl_test(uint32_t ecgi, uint32_t dlearfcn,
	uint32_t radius, uint32_t max_cells, uint16_t mcc, uint16_t mnc);
