/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <debug/cpu_load.h>
#include <zephyr/shell/shell.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#ifdef RTC_PRESENT
#include <hal/nrf_rtc.h>
#endif
#include <hal/nrf_power.h>
#include <debug/ppi_trace.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cpu_load, CONFIG_NRF_CPU_LOAD_LOG_LEVEL);

/* Convert event address to associated publish register */
#define PUBLISH_ADDR(evt) (volatile uint32_t *)(evt + 0x80)

/* Indicates that channel is not allocated. */
#define CH_INVALID 0xFF

/* Define to please compiler when periodic logging is disabled. */
#ifdef CONFIG_NRF_CPU_LOAD_LOG_INTERVAL
#define CPU_LOAD_LOG_INTERVAL CONFIG_NRF_CPU_LOAD_LOG_INTERVAL
#else
#define CPU_LOAD_LOG_INTERVAL 0
#endif

static nrfx_timer_t timer =
	NRFX_TIMER_INSTANCE(NRF_TIMER_INST_GET(CONFIG_NRF_CPU_LOAD_TIMER_INSTANCE));
static bool ready;
static struct k_work_delayable cpu_load_log;
static uint32_t cycle_ref;

static void cpu_load_log_fn(struct k_work *item)
{
	int load = cpu_load_get();
	uint32_t percent = load / 1000;
	uint32_t fraction = load % 1000;

	if (load < 0) {
		LOG_ERR("Module failed to initialize.");
		return;
	}

	cpu_load_reset();
	LOG_INF("Load:%d,%03d%%", percent, fraction);
	k_work_schedule(&cpu_load_log, K_MSEC(CPU_LOAD_LOG_INTERVAL));
}

static int cpu_load_log_init(void)
{
	k_work_init_delayable(&cpu_load_log, cpu_load_log_fn);
	return k_work_schedule(&cpu_load_log, K_MSEC(CPU_LOAD_LOG_INTERVAL));
}

static void timer_handler(nrf_timer_event_t event_type, void *context)
{
	/*empty*/
}

static int ppi_handle_get(uint32_t evt)
{
#ifdef DPPI_PRESENT
	if (*PUBLISH_ADDR(evt) != 0) {
		if (!IS_ENABLED(CONFIG_NRF_CPU_LOAD_USE_SHARED_DPPI_CHANNELS)) {
			return -ENOTSUP;
		}
		return *PUBLISH_ADDR(evt) & DPPIC_SUBSCRIBE_CHG_EN_CHIDX_Msk;
	}
#endif
	return -ENOTSUP;
}
static int ppi_setup(uint32_t eep, uint32_t tep)
{
	nrfx_gppi_handle_t handle;
	int err;

	err = ppi_handle_get(eep);
	if (err >= 0) {
		/* It works only on single domain DPPI. */
		handle = (nrfx_gppi_handle_t)err;
		nrfx_gppi_ep_attach(handle, tep);
		return 0;
	}

	err = nrfx_gppi_conn_alloc(eep, tep, &handle);
	if (err < 0) {
		LOG_ERR("Failed to allocate PPI resources");
		return err;
	}

	nrfx_gppi_conn_enable(handle);
	return 0;
}

int cpu_load_init_internal(void)
{
	int err;
	uint32_t base_frequency = NRF_TIMER_BASE_FREQUENCY_GET(timer.p_reg);
	nrfx_timer_config_t config = NRFX_TIMER_DEFAULT_CONFIG(base_frequency);
	int ret = 0;

	if (ready) {
		return 0;
	}

	config.frequency = NRFX_MHZ_TO_HZ(1);
	config.bit_width = NRF_TIMER_BIT_WIDTH_32;

#ifdef CONFIG_NRF_CPU_LOAD_ALIGNED_CLOCKS
	/* It's assumed that RTC1 is driving system clock. */
	config.mode = NRF_TIMER_MODE_COUNTER;
	ret = ppi_setup(nrf_rtc_event_address_get(NRF_RTC1, NRF_RTC_EVENT_TICK),
			nrfx_timer_task_address_get(&timer, NRF_TIMER_TASK_COUNT));
	if (ret < 0) {
		return ret;
	}
	nrf_rtc_event_enable(NRF_RTC1, NRF_RTC_INT_TICK_MASK);
#endif

	ret = ppi_setup(nrf_power_event_address_get(NRF_POWER, NRF_POWER_EVENT_SLEEPENTER),
			nrfx_timer_task_address_get(&timer, NRF_TIMER_TASK_START));
	if (ret < 0) {
		return ret;
	}

	ret = ppi_setup(nrf_power_event_address_get(NRF_POWER, NRF_POWER_EVENT_SLEEPEXIT),
			nrfx_timer_task_address_get(&timer, NRF_TIMER_TASK_STOP));
	if (ret < 0) {
		return ret;
	}

	err = nrfx_timer_init(&timer, &config, timer_handler);
	if (err != 0) {
		return -EBUSY;
	}

	cpu_load_reset();

	if (IS_ENABLED(CONFIG_NRF_CPU_LOAD_LOG_PERIODIC)) {
		ret = cpu_load_log_init();
		if (ret >= 0) {
			ret = 0;
		}
	}

	ready = true;

	return ret;
}

int cpu_load_init(void)
{
	return cpu_load_init_internal();
}

void cpu_load_reset(void)
{
	nrfx_timer_clear(&timer);
	cycle_ref = k_cycle_get_32();
}

static uint32_t sleep_ticks_to_us(uint32_t ticks)
{
	return IS_ENABLED(CONFIG_NRF_CPU_LOAD_ALIGNED_CLOCKS) ?
	   (uint32_t)(((uint64_t)ticks * 1000000) / sys_clock_hw_cycles_per_sec()) :
	   ticks;
}

int cpu_load_get(void)
{
	uint32_t sleep_us;
	uint32_t total_cyc;
	uint64_t total_us;
	uint64_t load;

	if (!ready) {
		return -ENODEV;
	}

	sleep_us = sleep_ticks_to_us(nrfx_timer_capture(&timer, 0));
	total_cyc = k_cycle_get_32() - cycle_ref;

	total_us = ((uint64_t)total_cyc * 1000000) /
		    sys_clock_hw_cycles_per_sec();
	__ASSERT(total_us < UINT32_MAX, "Measurement is limited.");

	/* Because of different clock sources for system clock and TIMER it
	 * is possible that sleep time is bigger than total measured time.
	 *
	 * Note that for simplicity reasons module does not handle TIMER
	 * overflow.
	 */
	load = (total_us > (uint64_t)sleep_us) ?
			(100000 * (total_us - (uint64_t)sleep_us)) / total_us : 0;

	return (int)load;
}

static int cmd_cpu_load_get(const struct shell *shell, size_t argc, char **argv)
{
	int load;
	uint32_t percent;
	uint32_t fraction;

	load = cpu_load_get();
	if (load < 0) {
		shell_error(shell, "Not initialized.");
		return 0;
	}
	percent = load / 1000;
	fraction = load % 1000;

	shell_print(shell, "CPU load:%d,%03d%%", percent, fraction);

	return 0;
}

static int cmd_cpu_load_reset(const struct shell *shell,
				size_t argc, char **argv)
{
	cpu_load_reset();

	return 0;
}

SYS_INIT(cpu_load_init_internal, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_cmd_cpu_load,
	SHELL_CMD_ARG(get, NULL, "Get load", cmd_cpu_load_get, 1, 0),
	SHELL_CMD_ARG(reset, NULL, "Reset measurement",
			cmd_cpu_load_reset, 1, 0),
	SHELL_CMD_ARG(init, NULL, "Init",
			cmd_cpu_load_reset, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_COND_CMD_ARG_REGISTER(CONFIG_NRF_CPU_LOAD_CMDS, cpu_load, &sub_cmd_cpu_load,
			"CPU load", cmd_cpu_load_get, 1, 1);
