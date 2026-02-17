/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <helpers/nrfx_reset_reason.h>
#include <nrf_modem.h>

#include <sys/types.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/dfu/mcuboot.h>

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#include <modem/nrf_modem_lib.h>
#include <modem/nrf_modem_lib_trace.h>

#include <dk_buttons_and_leds.h>

#if defined(CONFIG_AT_SHELL)
#include <modem/at_shell.h>
#endif

#include "desh_defines.h"
#include "desh_print.h"
#include <net/dect/dect_net_l2_shell_util.h>
#include <stdarg.h>

BUILD_ASSERT(IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL),
	     "CONFIG_SHELL_BACKEND_SERIAL shell backend must be enabled");

/* Global variables */
const struct shell *desh_shell;
struct k_poll_signal desh_signal;

#define DESH_COMMON_WORKQUEUE_STACK_SIZE CONFIG_SAMPLE_DESH_COMMON_WORKQ_STACK_SIZE
#define DESH_COMMON_WORKQ_PRIORITY	 5
K_THREAD_STACK_DEFINE(desh_common_workq_stack, DESH_COMMON_WORKQUEUE_STACK_SIZE);
struct k_work_q desh_common_work_q;

static const char *modem_crash_reason_get(uint32_t reason)
{
	switch (reason) {
	case NRF_MODEM_FAULT_UNDEFINED:
		return "Undefined fault";

	case NRF_MODEM_FAULT_HW_WD_RESET:
		return "HW WD reset";

	case NRF_MODEM_FAULT_HARDFAULT:
		return "Hard fault";

	case NRF_MODEM_FAULT_MEM_MANAGE:
		return "Memory management fault";

	case NRF_MODEM_FAULT_BUS:
		return "Bus fault";

	case NRF_MODEM_FAULT_USAGE:
		return "Usage fault";

	case NRF_MODEM_FAULT_SECURE_RESET:
		return "Secure control reset";

	case NRF_MODEM_FAULT_PANIC_DOUBLE:
		return "Error handler crash";

	case NRF_MODEM_FAULT_PANIC_RESET_LOOP:
		return "Reset loop";

	case NRF_MODEM_FAULT_ASSERT:
		return "Assert";

	case NRF_MODEM_FAULT_PANIC:
		return "Unconditional SW reset";

	case NRF_MODEM_FAULT_FLASH_ERASE:
		return "Flash erase fault";

	case NRF_MODEM_FAULT_FLASH_WRITE:
		return "Flash write fault";

	case NRF_MODEM_FAULT_POFWARN:
		return "Undervoltage fault";

	case NRF_MODEM_FAULT_THWARN:
		return "Overtemperature fault";

	default:
		return "Unknown reason";
	}
}

void nrf_modem_fault_handler(struct nrf_modem_fault_info *fault_info)
{
	printk("Modem crash reason: 0x%x (%s), PC: 0x%x\n", fault_info->reason,
	       modem_crash_reason_get(fault_info->reason), fault_info->program_counter);

	__ASSERT(false, "Modem crash detected, halting application execution");
}

static void reset_reason_str_get(char *str, uint32_t reason)
{
	size_t len;

	*str = '\0';

	if (reason & NRFX_RESET_REASON_RESETPIN_MASK) {
		(void)strcat(str, "PIN reset | ");
	}
	if (reason & NRFX_RESET_REASON_DOG_MASK) {
		(void)strcat(str, "watchdog | ");
	}
	if (reason & NRFX_RESET_REASON_OFF_MASK) {
		(void)strcat(str, "wakeup from power-off | ");
	}
	if (reason & NRFX_RESET_REASON_DIF_MASK) {
		(void)strcat(str, "debug interface wakeup | ");
	}
	if (reason & NRFX_RESET_REASON_SREQ_MASK) {
		(void)strcat(str, "software | ");
	}
	if (reason & NRFX_RESET_REASON_LOCKUP_MASK) {
		(void)strcat(str, "CPU lockup | ");
	}
	if (reason & NRFX_RESET_REASON_CTRLAP_MASK) {
		(void)strcat(str, "control access port | ");
	}

	len = strlen(str);
	if (len == 0) {
		(void)strcpy(str, "power-on reset");
	} else {
		str[len - 3] = '\0';
	}
}

static void desh_print_reset_reason(void)
{
	uint32_t reset_reason;
	char reset_reason_str[128];

	/* Read RESETREAS register value and clear current reset reason(s). */
	reset_reason = nrfx_reset_reason_get();
	nrfx_reset_reason_clear(reset_reason);

	reset_reason_str_get(reset_reason_str, reset_reason);

	printk("\nReset reason: %s\n", reset_reason_str);
}

#if defined(CONFIG_DK_LIBRARY)
static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & DK_BTN1_MSK) {
		desh_print("Button 1 pressed - raising a kill signal");
		k_poll_signal_raise(&desh_signal, DESH_SIGNAL_KILL);
	} else if (has_changed & ~button_states & DK_BTN1_MSK) {
		desh_print("Button 1 released - resetting a kill signal");
		k_poll_signal_reset(&desh_signal);
	}
}
#endif

/* Wrapper functions to adapt desh_print to shell_print signature */
static void dect_l2_shell_print_wrapper(const struct shell *shell, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	desh_fprintf_valist(DESH_PRINT_LEVEL_PRINT, fmt, args);
	va_end(args);
}

static void dect_l2_shell_error_wrapper(const struct shell *shell, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	desh_fprintf_valist(DESH_PRINT_LEVEL_ERROR, fmt, args);
	va_end(args);
}

static void dect_l2_shell_warn_wrapper(const struct shell *shell, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	desh_fprintf_valist(DESH_PRINT_LEVEL_WARN, fmt, args);
	va_end(args);
}

int main(void)
{
	int err;
	struct k_work_queue_config cfg = {
		.name = "desh_common_workq",
	};

	desh_shell = shell_backend_uart_get_ptr();

	__ASSERT(desh_shell != NULL, "Failed to get shell backend");

	/* Initialize l2_shell library with custom print functions for timestamping */
	struct dect_net_l2_shell_print_fns print_fns = {
		.print_fn = dect_l2_shell_print_wrapper,
		.error_fn = dect_l2_shell_error_wrapper,
		.warn_fn = dect_l2_shell_warn_wrapper,
	};

	dect_net_l2_shell_init(&print_fns);

	/* Reset reason can only be read once, because the register needs to be cleared after
	 * reading.
	 */
	desh_print_reset_reason();

	k_work_queue_start(&desh_common_work_q, desh_common_workq_stack,
			   K_THREAD_STACK_SIZEOF(desh_common_workq_stack),
			   DESH_COMMON_WORKQ_PRIORITY, &cfg);

#if defined(CONFIG_AT_SHELL)
	/* Initialize AT shell (optional work queue for cmd mode). */
	struct at_shell_config at_cfg = {
		.at_cmd_mode_work_q = &desh_common_work_q,
	};
	(void)at_shell_init(&at_cfg);
#endif

	err = nrf_modem_lib_init();
	if (err) {
		/* Modem library initialization failed. */
		printk("Could not initialize nrf_modem_lib, err %d\n", err);
		printk("Fatal error\n");
		return 0;
	}

#if defined(CONFIG_DK_LIBRARY)
	err = dk_buttons_init(button_handler);
	if (err) {
		printk("Failed to initialize DK buttons library, error: %d\n", err);
	}
	err = dk_leds_init();
	if (err) {
		printk("Cannot initialize LEDs (err: %d)\n", err);
	}
#endif
	k_poll_signal_init(&desh_signal);

	/* Resize terminal width and height of the shell to have proper command editing. */
	shell_execute_cmd(desh_shell, "resize");

	return 0;
}
