/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdint.h>
#include <stddef.h>

/* Forward decleration for dummy handler */
void dummy_handler(void);

/* weak assign all interrupt handlers to dummy_handler */
#define ALIAS(name) __attribute__((weak, alias(name)))
void __nmi(void) ALIAS("dummy_handler");
void __hard_fault(void) ALIAS("dummy_handler");
#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
void __mpu_fault(void) ALIAS("dummy_handler");
void __bus_fault(void) ALIAS("dummy_handler");
void __usage_fault(void) ALIAS("dummy_handler");
void __debug_monitor(void) ALIAS("dummy_handler");
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
void __secure_fault(void) ALIAS("dummy_handler");
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE */
void __svc(void) ALIAS("dummy_handler");
void __pendsv(void) ALIAS("dummy_handler");
void __sys_tick(void) ALIAS("dummy_handler");
void z_clock_isr(void) ALIAS("dummy_handler");
void *__reserved;

/* TODO: Remove when multi image is supported */
void assert_post_action(void) ALIAS("dummy_func");
void dummy_func(void)
{
}

void dummy_handler(void)
{
	/* Hang on unexpected faults and interrupts as it's considered
	 * a bug in the program..
	 */
	while (1) {
		;
	}

}
