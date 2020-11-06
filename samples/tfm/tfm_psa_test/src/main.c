/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>

/**
 * \brief This symbol is the entry point provided by the PSA API compliance
 *        test libraries
 */
extern void val_entry(void);

extern void TIMER1_Handler(void);

__attribute__((noreturn))
void main(void)
{
	IRQ_CONNECT(TIMER1_IRQn, NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
		TIMER1_Handler, NULL, 0);

#if !defined(CONFIG_TFM_PSA_TEST_CRYPTO) \
	&& !defined(CONFIG_TFM_PSA_TEST_PROTECTED_STORAGE) \
	&& !defined(CONFIG_TFM_PSA_TEST_INTERNAL_TRUSTED_STORAGE) \
	&& !defined(CONFIG_TFM_PSA_TEST_STORAGE) \
	&& !defined(CONFIG_TFM_PSA_TEST_INITIAL_ATTESTATION)
	printk("No PSA test suite set. Use Kconfig to enable a test suite.\n");
#else
	val_entry();
#endif

	for (;;) {
	}
}
