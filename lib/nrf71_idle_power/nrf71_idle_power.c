/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include <nrf71_idle_power.h>

#if defined(CONFIG_RAM_POWER_DOWN_LIBRARY)
#include <ram_pwrdn.h>
#endif

LOG_MODULE_REGISTER(nrf71_idle_power, CONFIG_NRF71_IDLE_POWER_LOG_LEVEL);

#if defined(CONFIG_NRF71_IDLE_DIAGNOSTICS)

#include <zephyr/drivers/gpio.h>

/* Idle-phase marker: driven high while running, low while idling, so a logic
 * analyzer can line the trace up against the current measurement. Optional --
 * absent unless the application provides an "idle-phase" alias. Toggled from
 * nrf71_idle_power_suspend_console()/_resume_console(), the same points every
 * sample already calls right before/after an idle wait.
 */
static const struct gpio_dt_spec idle_marker =
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(idle_phase), gpios, {0});

static void idle_marker_set(bool running)
{
	if (!gpio_is_ready_dt(&idle_marker)) {
		return;
	}

	(void)gpio_pin_set_dt(&idle_marker, running ? 1 : 0);
}

#else

static inline void idle_marker_set(bool running)
{
	ARG_UNUSED(running);
}

#endif /* CONFIG_NRF71_IDLE_DIAGNOSTICS */

static void configure_ram_retention(void)
{
#if defined(CONFIG_NRF71_IDLE_POWER_RAM_RETAIN_UNUSED_ONLY)
	power_down_unused_ram();
#elif defined(CONFIG_NRF71_IDLE_POWER_RAM_RETAIN_64K) || \
	defined(CONFIG_NRF71_IDLE_POWER_RAM_RETAIN_128K) || \
	defined(CONFIG_NRF71_IDLE_POWER_RAM_RETAIN_256K) || \
	defined(CONFIG_NRF71_IDLE_POWER_RAM_RETAIN_512K)
	uintptr_t ram_start = DT_REG_ADDR(DT_CHOSEN(zephyr_sram));
	uintptr_t ram_end = ram_start + DT_REG_SIZE(DT_CHOSEN(zephyr_sram));

#if defined(CONFIG_NRF71_IDLE_POWER_RAM_RETAIN_64K)
	uintptr_t retained_size = 64U * 1024U;
#elif defined(CONFIG_NRF71_IDLE_POWER_RAM_RETAIN_128K)
	uintptr_t retained_size = 128U * 1024U;
#elif defined(CONFIG_NRF71_IDLE_POWER_RAM_RETAIN_256K)
	uintptr_t retained_size = 256U * 1024U;
#elif defined(CONFIG_NRF71_IDLE_POWER_RAM_RETAIN_512K)
	uintptr_t retained_size = 512U * 1024U;
#endif

	power_down_ram(ram_start + retained_size, ram_end);
#endif
	/* NRF71_IDLE_POWER_RAM_RETAIN_FULL (default): no power-down call,
	 * every byte of application RAM stays retained.
	 */
}

/* Runs before the kernel brings up device init or grows the heap, so the
 * unused range computed from the image end is still valid to power down.
 */
static int nrf71_idle_power_ram_init(void)
{
	configure_ram_retention();

	return 0;
}

SYS_INIT(nrf71_idle_power_ram_init, PRE_KERNEL_1, 0);

/* CONFIG_SERIAL=n ("quiet" builds) leaves the "zephyr,console" chosen node
 * without a compiled-in driver instance, so DEVICE_DT_GET_OR_NULL() would
 * reference a device ordinal that was never generated and fail to link.
 * Gate the whole console device lookup on CONFIG_SERIAL instead of relying
 * on the chosen-node check alone.
 */
#if defined(CONFIG_SERIAL)
static const struct device *const idle_power_console =
	DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_console));

static void run_console_action(enum pm_device_action action)
{
	int ret;

	if (!idle_power_console || !device_is_ready(idle_power_console)) {
		return;
	}

	ret = pm_device_action_run(idle_power_console, action);
	if (ret < 0 && ret != -EALREADY) {
		LOG_DBG("Console PM action %d failed: %d", action, ret);
	}
}

void nrf71_idle_power_suspend_console(void)
{
	run_console_action(PM_DEVICE_ACTION_SUSPEND);
	idle_marker_set(false);
}

void nrf71_idle_power_resume_console(void)
{
	idle_marker_set(true);
	run_console_action(PM_DEVICE_ACTION_RESUME);
}
#else
void nrf71_idle_power_suspend_console(void)
{
	idle_marker_set(false);
}

void nrf71_idle_power_resume_console(void)
{
	idle_marker_set(true);
}
#endif /* CONFIG_SERIAL */

#if defined(CONFIG_NRF71_IDLE_DIAGNOSTICS)

#include <hal/nrf_power.h>
#include <hal/nrf_memconf.h>
#include <hal/nrf_grtc.h>
#include <hal/nrf_lfxo.h>

/* Wi-Fi core local resource/clock controller (NRF_WIFICORE_LRCCONF_LRC0). */
#include <nrfx.h>

static int nrf71_idle_power_diag_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&idle_marker)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&idle_marker, GPIO_OUTPUT_HIGH);
	if (ret < 0) {
		LOG_ERR("Failed to configure idle marker GPIO: %d", ret);
		return ret;
	}

	return 0;
}

SYS_INIT(nrf71_idle_power_diag_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

void nrf71_idle_power_print_snapshot(const char *phase)
{
	bool lrc0_main_always_on;

	printk("\n%s: Power State Snapshot\n", phase);
	printk("  CONSTLATSTAT=0x%08x SYSCOUNTER_ACTIVE=0x%08x\n",
	       NRF_POWER->CONSTLATSTAT,
	       NRF_GRTC->SYSCOUNTER[0].ACTIVE);

	printk("  MEMCONF0: CONTROL=0x%08x RET=0x%08x RET2=0x%08x\n",
	       NRF_MEMCONF->POWER[0].CONTROL,
	       NRF_MEMCONF->POWER[0].RET,
	       NRF_MEMCONF->POWER[0].RET2);

	printk("  MEMCONF1: CONTROL=0x%08x RET=0x%08x RET2=0x%08x\n",
	       NRF_MEMCONF->POWER[1].CONTROL,
	       NRF_MEMCONF->POWER[1].RET,
	       NRF_MEMCONF->POWER[1].RET2);

	printk("  GRTC_MODE=0x%08x LFXO_STATUS=0x%08x LFXO_MODE=0x%08x\n",
	       NRF_GRTC->MODE,
	       NRF_LFXO->STATUS,
	       NRF_LFXO->MODE);

	/* LRC0 sits in the Wi-Fi core always-on management partition, so it stays
	 * readable from the app core even while the RPU power domain is collapsed.
	 * POWERON.MAIN reading AlwaysOn, RETAIN bits set, or any CLKSTAT.RUN bit
	 * still asserted points at a domain/clock the firmware never released --
	 * the usual suspect behind a flat idle-current floor.
	 */
	lrc0_main_always_on = NRF_WIFICORE_LRCCONF_LRC0->POWERON & LRCCONF_POWERON_MAIN_Msk;

	printk("  WIFICORE LRC0: POWERON=0x%08x (MAIN=%s) RETAIN=0x%08x CONSTLATSTAT=0x%08x\n",
	       NRF_WIFICORE_LRCCONF_LRC0->POWERON,
	       lrc0_main_always_on ? "AlwaysOn" : "Auto",
	       NRF_WIFICORE_LRCCONF_LRC0->RETAIN,
	       NRF_WIFICORE_LRCCONF_LRC0->CONSTLATSTAT);

	printk("  WIFICORE LRC0: HFXOSTARTED=0x%08x CLKSTAT.RUN[0..7]=",
	       NRF_WIFICORE_LRCCONF_LRC0->EVENTS_HFXOSTARTED);
	for (int i = 0; i < 8; i++) {
		printk("%u", (unsigned int)(NRF_WIFICORE_LRCCONF_LRC0->CLKSTAT[i].RUN & 0x1U));
	}
	printk("\n");
}

#if defined(CONFIG_SHELL)

#include <zephyr/shell/shell.h>

/* On-demand snapshot for interactive samples (e.g. wifi/shell): let the RPU
 * settle into idle, then dump the state without a build change.
 */
static int cmd_idle_snapshot(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);

	nrf71_idle_power_print_snapshot(argc > 1 ? argv[1] : "shell");

	return 0;
}

SHELL_CMD_REGISTER(idle_snapshot, NULL,
		   "Dump nRF71 idle-power register snapshot [phase]", cmd_idle_snapshot);

#endif /* CONFIG_SHELL */

#endif /* CONFIG_NRF71_IDLE_DIAGNOSTICS */
