/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <string.h>
#include <hal/nrf_ecb.h>
#include "nrf_ecb_driver.h"

#define ECB_PERIPH NRF_ECB

/* ECB data structure for RNG peripheral to access. */
static u8_t ecb_data[48];

/* Key: Starts at ecb_data */
static u8_t *ecb_key;

/* Cleartext:  Starts at ecb_data + 16 bytes. */
static u8_t *ecb_cleartext;

/* Ciphertext: Starts at ecb_data + 32 bytes. */
static u8_t *ecb_ciphertext;

int nrf_ecb_driver_init(void)
{
	ecb_key = ecb_data;
	ecb_cleartext = ecb_data + 16;
	ecb_ciphertext = ecb_data + 32;

	nrf_ecb_data_pointer_set(ECB_PERIPH, ecb_data);
	return 0;
}

int nrf_ecb_driver_crypt(u8_t *dest_buf, const u8_t *src_buf)
{
	u32_t counter = 0x1000000;

	if (!(dest_buf && src_buf)) {
		return -1;
	}
	if (src_buf != ecb_cleartext) {
		memcpy(ecb_cleartext, src_buf, 16);
	}
	nrf_ecb_event_clear(ECB_PERIPH, NRF_ECB_EVENT_ENDECB);
	nrf_ecb_task_trigger(ECB_PERIPH, NRF_ECB_TASK_STARTECB);
	while (nrf_ecb_event_check(ECB_PERIPH, NRF_ECB_EVENT_ENDECB) == false) {
		counter--;
		if (counter == 0) {
			return -1;
		}
	}
	nrf_ecb_event_clear(ECB_PERIPH, NRF_ECB_EVENT_ENDECB);
	if (dest_buf != ecb_ciphertext) {
		memcpy(dest_buf, ecb_ciphertext, 16);
	}
	return 0;
}

void nrf_ecb_driver_set_key(const u8_t *key)
{
	if (!key) {
		return;
	}
	memcpy(ecb_key, key, 16);
}
