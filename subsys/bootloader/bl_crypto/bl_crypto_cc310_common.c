/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <misc/util.h>
#include <toolchain.h>
#include <nrf.h>
#include <nrf_cc310_bl_init.h>

#ifdef CONFIG_SOC_SERIES_NRF91X
#define NRF_CRYPTOCELL NRF_CRYPTOCELL_S
#endif

/* These are needed by the cc310_bl code, but not
 * (always) included in the bootloader.
 */
void *__weak memset(void *buf, int c, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		((u8_t *)buf)[i] = c;
	}
	return buf;
}

void *__weak memcpy(void *restrict d, const void *restrict s, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		((u8_t *)d)[i] = ((u8_t *)s)[i];
	}
	return d;
}

void *memcpy32(void *restrict d, const void *restrict s, size_t n)
{
	size_t len_words = ROUND_DOWN(n, 4) / 4;
	for (size_t i = 0; i < len_words; i++) {
		((u32_t *)d)[i] = ((u32_t *)s)[i];
	}
	return d;
}

void cc310_bl_backend_enable(void)
{
	/* Enable the cryptocell hardware */
	NRF_CRYPTOCELL->ENABLE = 1;
}

void cc310_bl_backend_disable(void)
{
	/* Disable the cryptocell hardware */
	NRF_CRYPTOCELL->ENABLE = 0;
}

bool cc310_bl_init(void)
{
	static bool initialized;

	if (!initialized) {
		cc310_bl_backend_enable();
		if (nrf_cc310_bl_init() != CRYS_OK) {
			return false;
		}
		initialized = true;
		cc310_bl_backend_disable();
	}

	return true;
}
