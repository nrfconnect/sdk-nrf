/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <debug/ppi_trace.h>
#include <drivers/counter.h>
#include <hal/nrf_rtc.h>
#include <hal/nrf_clock.h>
#include <device.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(app);

#define ALARM_PERIOD_US 50000

#if IS_ENABLED(CONFIG_USE_RTC2)
#define RTC       NRF_RTC2
#define RTC_LABEL DT_LABEL(DT_NODELABEL(rtc2))
#else
#define RTC       NRF_RTC0
#define RTC_LABEL DT_LABEL(DT_NODELABEL(rtc0))
#endif

static void alarm_callback(struct device *dev, u8_t chan_id, u32_t ticks,
			   void *user_data);

static struct counter_alarm_cfg alarm_cfg = {
	.callback = alarm_callback,
	.flags = COUNTER_ALARM_CFG_ABSOLUTE,
};

extern void bluetooth_enable(void);

static void ppi_trace_pin_setup(u32_t pin, u32_t evt)
{
	void *handle;

	handle = ppi_trace_config(pin, evt);
	__ASSERT(handle != NULL,
		"Failed to initialize trace pin, no PPI or GPIOTE resources?");

	ppi_trace_enable(handle);
}

static void ppi_trace_setup(void)
{
	ppi_trace_pin_setup(CONFIG_PPI_TRACE_PIN_RTC_COMPARE_EVT,
		nrf_rtc_event_address_get(RTC, NRF_RTC_EVENT_COMPARE_0));

	/* Due to low power domain events must be explicitly enabled in RTC. */
	nrf_rtc_event_enable(RTC, NRF_RTC_INT_TICK_MASK);
	ppi_trace_pin_setup(CONFIG_PPI_TRACE_PIN_RTC_TICK_EVT,
		nrf_rtc_event_address_get(RTC, NRF_RTC_EVENT_TICK));

	ppi_trace_pin_setup(CONFIG_PPI_TRACE_PIN_LFCLOCK_STARTED_EVT,
		nrf_clock_event_address_get(NRF_CLOCK,
					    NRF_CLOCK_EVENT_LFCLKSTARTED));

	LOG_INF("PPI trace setup done.");
}

static void alarm_callback(struct device *dev, u8_t chan_id, u32_t ticks,
			   void *user_data)
{
	int err;
	u32_t alarm_cnt = (u32_t)user_data + 1;

	alarm_cfg.ticks = ticks + counter_us_to_ticks(dev, ALARM_PERIOD_US);
	alarm_cfg.user_data = (void *)alarm_cnt;

	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	__ASSERT_NO_MSG(err == 0);
	(void)err;
}

static void counter_setup(void)
{
	int err;
	struct device *dev = device_get_binding(RTC_LABEL);

	__ASSERT(dev, "Sample cannot run on this board.");

	alarm_cfg.ticks = counter_us_to_ticks(dev, ALARM_PERIOD_US);
	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	__ASSERT_NO_MSG(err == 0);

	err = counter_start(dev);
	__ASSERT_NO_MSG(err == 0);
}

void main(void)
{
	ppi_trace_setup();
	counter_setup();

	if (IS_ENABLED(CONFIG_BT)) {
		bluetooth_enable();
	}
}
