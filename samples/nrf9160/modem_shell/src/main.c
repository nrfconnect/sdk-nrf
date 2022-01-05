/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>

#include <zephyr.h>
#include <init.h>
#include <nrf_modem.h>

#include <sys/types.h>
#include <logging/log_ctrl.h>
#include <power/reboot.h>
#include <dfu/mcuboot.h>

#include <shell/shell.h>
#include <shell/shell_uart.h>

#include <modem/nrf_modem_lib.h>

#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include <modem/at_monitor.h>

#include <modem/modem_info.h>
#include <modem/lte_lc.h>

#include <dk_buttons_and_leds.h>
#include "uart/uart_shell.h"

#if defined(CONFIG_MOSH_PPP)
#include "ppp_ctrl.h"
#endif

#if defined(CONFIG_MOSH_LINK)
#include "link.h"
#endif

#if defined(CONFIG_MOSH_GNSS)
#include "gnss.h"
#endif

#if defined(CONFIG_MOSH_FOTA)
#include "fota.h"
#endif
#if defined(CONFIG_MOSH_WORKER_THREADS)
#include "th/th_ctrl.h"
#endif
#include "mosh_defines.h"
#include "mosh_print.h"

#if defined(CONFIG_MOSH_LOCATION)
#include "location_shell.h"
#endif

/* Global variables */
struct modem_param_info modem_param;
struct k_poll_signal mosh_signal;

K_SEM_DEFINE(nrf_modem_lib_initialized, 0, 1);

static void mosh_print_version_info(void)
{
#if defined(APP_VERSION)
	printk("\nMOSH version:       %s", STRINGIFY(APP_VERSION));
#else
	printk("\nMOSH version:       unknown");
#endif

#if defined(BUILD_ID)
	printk("\nMOSH build id:      v%s", STRINGIFY(BUILD_ID));
#else
	printk("\nMOSH build id:      custom");
#endif

#if defined(BUILD_VARIANT)
#if defined(BRANCH_NAME)
	printk("\nMOSH build variant: %s/%s\n\n", STRINGIFY(BRANCH_NAME), STRINGIFY(BUILD_VARIANT));
#else
	printk("\nMOSH build variant: %s\n\n", STRINGIFY(BUILD_VARIANT));
#endif
#else
	printk("\nMOSH build variant: dev\n\n");
#endif
}

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & DK_BTN1_MSK) {
		mosh_print("Button 1 pressed - raising a kill signal");
		k_poll_signal_raise(&mosh_signal, MOSH_SIGNAL_KILL);
#if defined(CONFIG_MOSH_WORKER_THREADS)
		th_ctrl_kill_em_all();
#endif
	} else if (has_changed & ~button_states & DK_BTN1_MSK) {
		mosh_print("Button 1 released - resetting a kill signal");
		k_poll_signal_reset(&mosh_signal);
	}

	if (has_changed & button_states & DK_BTN2_MSK) {
		mosh_print("Button 2 pressed, toggling UART power state");
		uart_toggle_power_state();
	}
}

void main(void)
{
	int err;
	const struct shell *shell = shell_backend_uart_get_ptr();

	mosh_print_version_info();

#if !defined(CONFIG_LWM2M_CARRIER)
	/* Get Modem library initialization return value. */
	err = nrf_modem_lib_get_init_ret();
	switch (err) {
	case 0:
		/* Modem library was initialized successfully. */
		break;
	case MODEM_DFU_RESULT_OK:
		printk("Modem firmware update successful!\n");
		printk("Modem will run the new firmware after reboot\n");
		sys_reboot(SYS_REBOOT_WARM);
		return;
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		printk("Modem firmware update failed!\n");
		printk("Modem will run non-updated firmware on reboot.\n");
		sys_reboot(SYS_REBOOT_WARM);
		return;
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		printk("Modem firmware update failed!\n");
		printk("Fatal error.\n");
		sys_reboot(SYS_REBOOT_WARM);
		return;
	default:
		/* Modem library initialization failed. */
		printk("Could not initialize modemlib.\n");
		printk("Fatal error.\n");
		return;
	}

	at_cmd_init();
#if !defined(CONFIG_AT_NOTIF_SYS_INIT)
	at_notif_init();
#endif
#if !defined(CONFIG_AT_MONITOR_SYS_INIT)
	at_monitor_init();
#endif
	lte_lc_init();
#else
	/* Wait until modemlib has been initialized. */
	k_sem_take(&nrf_modem_lib_initialized, K_FOREVER);

#endif
#if defined(CONFIG_MOSH_PPP)
	ppp_ctrl_init();
#endif
#if defined(CONFIG_MOSH_WORKER_THREADS)
	th_ctrl_init();
#endif
#if defined(CONFIG_MOSH_GNSS)
	gnss_configure_lna();
#endif
#if defined(CONFIG_MOSH_FOTA)
	err = fota_init();
	if (err) {
		printk("Could not initialize FOTA: %d\n", err);
	}
#endif

#if defined(CONFIG_MOSH_LOCATION)
	/* Location library should be initialized before LTE normal mode */
	location_ctrl_init();
#endif
#if defined(CONFIG_LTE_LINK_CONTROL) && defined(CONFIG_MOSH_LINK)
	link_init();
#endif

#if defined(CONFIG_MODEM_INFO)
	err = modem_info_init();
	if (err) {
		printk("Modem info could not be established: %d\n", err);
		return;
	}
	modem_info_params_init(&modem_param);
#endif

	err = dk_buttons_init(button_handler);
	if (err) {
		printk("Failed to initialize DK buttons library, error: %d", err);
	}

	/* Application started successfully, mark image as OK to prevent
	 * revert at next reboot.
	 */
#if defined(CONFIG_BOOTLOADER_MCUBOOT)
	boot_write_img_confirmed();
#endif
	k_poll_signal_init(&mosh_signal);

	/* Resize terminal width and height of the shell to have proper command editing. */
	shell_execute_cmd(shell, "resize");
	/* Run empty command because otherwise "resize" would be set to the command line. */
	shell_execute_cmd(shell, "");
}
