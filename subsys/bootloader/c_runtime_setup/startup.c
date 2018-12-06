/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/* General TODO: put stack after .bss and .data
 * +--------------+
 * | .bss + .data |
 * |--------------|
 * |   .stack     |
 * |      |       |
 * |      V       |
 * |--------------|
 * |     End      |
 * +--------------+
 * And upwards growing heap if heap is present ontop of .bss and .data
 * This gives you MMU less stack protection for both .heap and .stack
 * +--------------+
 * |--------------|
 * |      ^       |
 * |      |       |
 * |    .heap     |
 * |--------------|
 * | .bss + .data |
 * |--------------|
 * |    .stack    |
 * |      |       |
 * |      V       |
 * |--------------|
 * |     End      |
 * +--------------+
 * MMU Less stack and heap protection, but limited heap size
 * You can't resize the stack to account for more heap etc. so this
 * should be configurable. As you may  want the heap to grow towards
 * the stack for for small devices with low memory.
 */
#include <toolchain.h>
#include <stdint.h>

/* Linker-defined symbols for addresses of flash and ram */

/* Block started by symbol, data container for statically allocated objects
 * Such as uninitialized objects both variables and constatns declared in
 * file scope and uninitialized static local variables
 * Short name BETTER SAVE SPACE(BSS)
 */
extern u32_t __bss_start__;
extern u32_t __bss_end__;
extern u32_t __etext;
extern u32_t __data_start__;
/* C main function to be called from the reset_handler */
extern int main_bl(void);
/* int main(void); */
/* Device specific intialization functions for erratas and system clock setup */
extern void SystemInit(void);
/* Forward decleration for dummy handler */
void dummy_handler(void);
void Reset_Handler(void);
void _bss_zero(u32_t *dest, u32_t *end);
void _data_copy(u32_t *src, u32_t *dest, u32_t *end);
/* weak assign all interrupt handlers to dummy_handler */
#define ALIAS(name) __attribute__((weak, alias(name)))
void Reset_Handler(void) __attribute((alias("reset_handler")));
void reset_handler(void)
{
	_bss_zero(&__bss_start__, &__bss_end__);
	_data_copy(&__etext, &__data_start__, &__bss_start__);
#if defined(CONFIG_SB_VENDOR_SYSTEM_INIT)
	SystemInit(); /* Create define for system INIT */
#endif /* CONFIG_SECURE_BOOT TODO: Find a way to use select defines? */
	main_bl();
	CODE_UNREACHABLE;
}
void nmi_handler(void) ALIAS("dummy_handler");
void hard_fault_handler(void) ALIAS("dummy_handler");
#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
void mpu_fault_handler(void) ALIAS("dummy_handler");
void bus_fault_handler(void) ALIAS("dummy_handler");
void usage_fault_handler(void) ALIAS("dummy_handler");
void debug_monitor_handler(void) ALIAS("dummy_handler");
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
void secure_fault_handler(void) ALIAS("dummy_handler");
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE */
void svc_handler(void) ALIAS("dummy_handler");
void pend_sv_handler(void) ALIAS("dummy_handler");
void sys_tick_handler(void) ALIAS("dummy_handler");

extern u32_t __StackTop;
/* TODO: Add vendor specific vectors to vector table */
void *core_vector_table[16] __attribute__((section(".isr_vector"))) = {
	&__StackTop,
	reset_handler,      /*__reset */
	nmi_handler,	/* __nmi */
	hard_fault_handler, /*__hard_fault */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	0,
	/* reserved */ /*__reserved */
	0,	     /* reserved */
	0,	     /* reserved */
	0,	     /* reserved */
	0,	     /* reserved */
	0,	     /* reserved */
	0,	     /* reserved */
	svc_handler,   /*__svc */
	0,	     /* reserved */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	mpu_fault_handler,     /*__mpu_fault */
	bus_fault_handler,     /*__bus_fault */
	usage_fault_handler,   /*__usage_fault */
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	secure_fault_handler,  /*__secure_fault */
#else
	0, /* reserved */
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
	0,		       /* reserved */
	0,		       /* reserved */
	0,		       /* reserved */
	svc_handler,	   /*__svc */
	debug_monitor_handler, /* debug_monitor */
#else  /* ARM_ARCHITECTURE */
#error Unknown ARM architecture
#endif			 /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
	0,		 /* reserved */
	pend_sv_handler, /*__pendsv */
#if defined(CONFIG_CORTEX_M_SYSTICK)
	sys_tick_handler, /*_timer_int_handler */
#else
	0,		       /* reserved */
#endif /* CONFIG_CORTEX_M_SYSTICK */
};

void _bss_zero(u32_t *dest, u32_t *end)
{
	while (dest < (u32_t *)&end) {
		*dest++ = 0;
	}
}

void _data_copy(u32_t *src, u32_t *dest, u32_t *end)
{
	while (dest < (u32_t *)&end) {
		*dest++ = *src++;
	}
}

void dummy_handler(void)
{
	/* Hang on unexpected interrupts as it's considered a bug in the program
	*/
	while (1) {
		;
	}
}
