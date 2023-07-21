/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <debug/cpu_load.h>
#include <zephyr/shell/shell.h>
#ifdef DPPI_PRESENT
#include <nrfx_dppi.h>
#else
#include <nrfx_ppi.h>
#endif
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <hal/nrf_rtc.h>
#include <hal/nrf_power.h>
#include <debug/ppi_trace.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cpu_load, CONFIG_CPU_LOAD_LOG_LEVEL);

/* Convert event address to associated publish register */
#define PUBLISH_ADDR(evt) (volatile uint32_t *)(evt + 0x80)

/* Indicates that channel is not allocated. */
#define CH_INVALID 0xFF

/* Define to please compiler when periodic logging is disabled. */
#ifdef CONFIG_CPU_LOAD_LOG_INTERVAL
#define CPU_LOAD_LOG_INTERVAL CONFIG_CPU_LOAD_LOG_INTERVAL
#else
#define CPU_LOAD_LOG_INTERVAL 0
#endif

static nrfx_timer_t timer = NRFX_TIMER_INSTANCE(CONFIG_CPU_LOAD_TIMER_INSTANCE);
static bool ready;
static struct k_work_delayable cpu_load_log;
static uint32_t cycle_ref;
static uint32_t shared_ch_mask;

#define IS_CH_SHARED(ch) \
	(IS_ENABLED(CONFIG_CPU_LOAD_USE_SHARED_DPPI_CHANNELS) && \
	(BIT(ch) & shared_ch_mask))


/** @brief Allocate (D)PPI channel. */
static nrfx_err_t ppi_alloc(uint8_t *ch, uint32_t evt)
{
	nrfx_err_t err;
#ifdef DPPI_PRESENT
	if (*PUBLISH_ADDR(evt) != 0) {
		if (!IS_ENABLED(CONFIG_CPU_LOAD_USE_SHARED_DPPI_CHANNELS)) {
			return NRFX_ERROR_BUSY;
		}
		/* Use mask of one of subscribe registers in the system,
		 * assuming that all subscribe registers has the same mask for
		 * channel id.
		 */
		*ch = *PUBLISH_ADDR(evt) & DPPIC_SUBSCRIBE_CHG_EN_CHIDX_Msk;
		err = NRFX_SUCCESS;
		shared_ch_mask |= BIT(*ch);
	} else {
		err = nrfx_dppi_channel_alloc(ch);
	}
#else
	err = nrfx_ppi_channel_alloc((nrf_ppi_channel_t *)ch);
#endif
	return err;
}

static nrfx_err_t ppi_free(uint8_t ch)
{
#ifdef DPPI_PRESENT
	if (!IS_ENABLED(CONFIG_CPU_LOAD_USE_SHARED_DPPI_CHANNELS)
		|| ((BIT(ch) & shared_ch_mask) == 0)) {
		return nrfx_dppi_channel_free(ch);
	} else {
		return NRFX_SUCCESS;
	}
#else
	return nrfx_ppi_channel_free((nrf_ppi_channel_t)ch);
#endif
}

static void ppi_cleanup(uint8_t ch_tick, uint8_t ch_sleep, uint8_t ch_wakeup)
{
	nrfx_err_t err = NRFX_SUCCESS;

	if (IS_ENABLED(CONFIG_CPU_LOAD_ALIGNED_CLOCKS)) {
		err = ppi_free(ch_tick);
	}

	if ((err == NRFX_SUCCESS) && (ch_sleep != CH_INVALID)) {
		err = ppi_free(ch_sleep);
	}

	if ((err == NRFX_SUCCESS) && (ch_wakeup != CH_INVALID)) {
		err = ppi_free(ch_wakeup);
	}

	if (err != NRFX_SUCCESS) {
		LOG_ERR("PPI channel freeing failed (err:%d)", err);
	}
}

static void cpu_load_log_fn(struct k_work *item)
{
	uint32_t load = cpu_load_get();
	uint32_t percent = load / 1000;
	uint32_t fraction = load % 1000;

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


int cpu_load_init(void)
{
	uint8_t ch_sleep;
	uint8_t ch_wakeup;
	uint8_t ch_tick = 0;
	nrfx_err_t err;
	uint32_t base_frequency = NRF_TIMER_BASE_FREQUENCY_GET(timer.p_reg);
	nrfx_timer_config_t config = NRFX_TIMER_DEFAULT_CONFIG(base_frequency);
	int ret = 0;

	if (ready) {
		return 0;
	}

	config.frequency = NRFX_MHZ_TO_HZ(1);
	config.bit_width = NRF_TIMER_BIT_WIDTH_32;

	if (IS_ENABLED(CONFIG_CPU_LOAD_ALIGNED_CLOCKS)) {
		/* It's assumed that RTC1 is driving system clock. */
		config.mode = NRF_TIMER_MODE_COUNTER;
		err = ppi_alloc(&ch_tick,
		       nrf_rtc_event_address_get(NRF_RTC1, NRF_RTC_EVENT_TICK));
		if (err != NRFX_SUCCESS) {
			return -ENODEV;
		}
		nrfx_gppi_channel_endpoints_setup(ch_tick,
		     nrf_rtc_event_address_get(NRF_RTC1, NRF_RTC_EVENT_TICK),
		     nrfx_timer_task_address_get(&timer, NRF_TIMER_TASK_COUNT));
		nrf_rtc_event_enable(NRF_RTC1, NRF_RTC_INT_TICK_MASK);
	}

	err = ppi_alloc(&ch_sleep,
		       nrf_power_event_address_get(NRF_POWER,
						   NRF_POWER_EVENT_SLEEPENTER));
	if (err != NRFX_SUCCESS) {
		ppi_cleanup(ch_tick, CH_INVALID, CH_INVALID);
		return -ENODEV;
	}

	err = ppi_alloc(&ch_wakeup,
		       nrf_power_event_address_get(NRF_POWER,
						   NRF_POWER_EVENT_SLEEPEXIT));
	if (err != NRFX_SUCCESS) {
		ppi_cleanup(ch_tick, ch_sleep, CH_INVALID);
		return -ENODEV;
	}

	err = nrfx_timer_init(&timer, &config, timer_handler);
	if (err != NRFX_SUCCESS) {
		ppi_cleanup(ch_tick, ch_sleep, ch_wakeup);
		return -EBUSY;
	}

	nrfx_gppi_channel_endpoints_setup(ch_sleep,
		  nrf_power_event_address_get(NRF_POWER,
					      NRF_POWER_EVENT_SLEEPENTER),
		  nrfx_timer_task_address_get(&timer, NRF_TIMER_TASK_START));
	nrfx_gppi_channel_endpoints_setup(ch_wakeup,
		  nrf_power_event_address_get(NRF_POWER,
					      NRF_POWER_EVENT_SLEEPEXIT),
		  nrfx_timer_task_address_get(&timer, NRF_TIMER_TASK_STOP));

	/* In case of DPPI event can only be assigned to a single channel. In
	 * that case, cpu load can still subscribe to the channel but should
	 * not control it. It may result in cpu load not working. User must
	 * take care of that.
	 */
	nrfx_gppi_channels_enable((IS_CH_SHARED(ch_sleep) ? 0 : BIT(ch_sleep)) |
				(IS_CH_SHARED(ch_wakeup) ? 0 : BIT(ch_wakeup)) |
				(IS_CH_SHARED(ch_tick) ? 0 : BIT(ch_tick)));

	cpu_load_reset();

	if (IS_ENABLED(CONFIG_CPU_LOAD_LOG_PERIODIC)) {
		ret = cpu_load_log_init();
		if (ret >= 0) {
			ret = 0;
		}
	}

	ready = true;

	return ret;
}

void cpu_load_reset(void)
{
	nrfx_timer_clear(&timer);
	cycle_ref = k_cycle_get_32();
}

static uint32_t sleep_ticks_to_us(uint32_t ticks)
{
	return IS_ENABLED(CONFIG_CPU_LOAD_ALIGNED_CLOCKS) ?
	   (uint32_t)(((uint64_t)ticks * 1000000) / sys_clock_hw_cycles_per_sec()) :
	   ticks;
}

uint32_t cpu_load_get(void)
{
	uint32_t sleep_us;
	uint32_t total_cyc;
	uint64_t total_us;
	uint64_t load;

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

	return (uint32_t)load;
}

static int cmd_cpu_load_get(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t load;
	uint32_t percent;
	uint32_t fraction;

	if (!ready) {
		shell_error(shell, "Not initialized.");
		return 0;
	}

	load = cpu_load_get();
	percent = load / 1000;
	fraction = load % 1000;

	shell_print(shell, "CPU load:%d,%03d%%", percent, fraction);

	return 0;
}

static int cmd_cpu_load_reset(const struct shell *shell,
				size_t argc, char **argv)
{
	int err;

	err = cpu_load_init();
	if (err != 0) {
		shell_error(shell, "Init failed (err:%d)", err);
		return 0;
	}

	cpu_load_reset();

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_cmd_cpu_load,
	SHELL_CMD_ARG(get, NULL, "Get load", cmd_cpu_load_get, 1, 0),
	SHELL_CMD_ARG(reset, NULL, "Reset measurement",
			cmd_cpu_load_reset, 1, 0),
	SHELL_CMD_ARG(init, NULL, "Init",
			cmd_cpu_load_reset, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_COND_CMD_ARG_REGISTER(CONFIG_CPU_LOAD_CMDS, cpu_load, &sub_cmd_cpu_load,
			"CPU load", cmd_cpu_load_get, 1, 1);
