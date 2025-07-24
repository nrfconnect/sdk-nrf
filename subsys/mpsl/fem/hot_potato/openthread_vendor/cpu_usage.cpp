/* Copyright (c) 2025, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <hal/nrf_power.h>
#include <hal/nrf_timer.h>
#include <nrf_802154_sl_timer.h>
#include <nrfx_dppi.h>
#include <nrfx_ppib.h>
#include <nrfx_timer.h>

#include <openthread/cli.h>
#include <openthread/error.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

#define CPU_USAGE_HISTOGRAM_LEN 10
#define CPU_USAGE_AGG_PERIOD_US (100ULL * 1000ULL)

static const nrfx_timer_t timer_cpu = NRFX_TIMER_INSTANCE(20); /* CPU duty cycle */
static const nrfx_dppi_t dppi_peri = NRFX_DPPI_INSTANCE(20);   /* TIMER20 DPPI - PERI DPPI */
static const nrfx_dppi_t dppi_lp = NRFX_DPPI_INSTANCE(30);     /* POWER DPPI - LP DPPI */
static const nrfx_ppib_interconnect_t ppib = NRFX_PPIB_INTERCONNECT_INSTANCE(22, 30);

static volatile bool initialized;

static uint8_t dppi_lp_channel_sleep_enter;
static uint8_t dppi_peri_channel_sleep_enter;
static uint8_t ppib_channel_sleep_enter;

static uint8_t dppi_lp_channel_sleep_exit;
static uint8_t dppi_peri_channel_sleep_exit;
static uint8_t ppib_channel_sleep_exit;

static volatile bool meas_started; /* Measurement started? */
static uint32_t max_cpu_usage;
static uint32_t cpu_usage_histogram[CPU_USAGE_HISTOGRAM_LEN];

static void VendorUsageCpuMeasure(struct k_timer *timer);
static K_TIMER_DEFINE(timer_meas, VendorUsageCpuMeasure, NULL);

static void VendorUsageCpuInit()
{
	nrfx_timer_config_t timer_cfg = NRFX_TIMER_DEFAULT_CONFIG(NRFX_MHZ_TO_HZ(1));
	nrfx_err_t err;

	if (initialized) {
		return;
	}

	/* === Initialize timer === */
	timer_cfg.bit_width = NRF_TIMER_BIT_WIDTH_32;
	err = nrfx_timer_init(&timer_cpu, &timer_cfg, NULL);
	__ASSERT_NO_MSG(err == NRFX_SUCCESS);

	nrfx_timer_enable(&timer_cpu);

	/* === Stop timer on SLEEPENTER === */
	err = nrfx_dppi_channel_alloc(&dppi_lp, &dppi_lp_channel_sleep_enter);
	__ASSERT_NO_MSG(err == NRFX_SUCCESS);

	err = nrfx_dppi_channel_alloc(&dppi_peri, &dppi_peri_channel_sleep_enter);
	__ASSERT_NO_MSG(err == NRFX_SUCCESS);

	err = nrfx_ppib_channel_alloc(&ppib, &ppib_channel_sleep_enter);
	__ASSERT_NO_MSG(err == NRFX_SUCCESS);

	/* POWER.EVENT_SLEEPENTER -> DPPI30 */
	nrf_power_publish_set(NRF_POWER, NRF_POWER_EVENT_SLEEPENTER, dppi_lp_channel_sleep_enter);
	/* DPPI30 -> PPIB30 */
	nrf_ppib_subscribe_set(ppib.right.p_reg, nrf_ppib_send_task_get(ppib_channel_sleep_enter),
			       dppi_lp_channel_sleep_enter);
	/* PPIB30 -> PPIB22 (implicit)*/
	/* PPIB22 -> DPPI20 */
	nrf_ppib_publish_set(ppib.left.p_reg, nrf_ppib_receive_event_get(ppib_channel_sleep_enter),
			     dppi_peri_channel_sleep_enter);
	/* DPPI20 -> TIMER.TASK_STOP */
	nrf_timer_subscribe_set(timer_cpu.p_reg, NRF_TIMER_TASK_STOP,
				dppi_peri_channel_sleep_enter);

	err = nrfx_dppi_channel_enable(&dppi_lp, dppi_lp_channel_sleep_enter);
	__ASSERT_NO_MSG(err == NRFX_SUCCESS);

	err = nrfx_dppi_channel_enable(&dppi_peri, dppi_peri_channel_sleep_enter);
	__ASSERT_NO_MSG(err == NRFX_SUCCESS);

	/* === Start timer on SLEEPEXIT === */
	err = nrfx_dppi_channel_alloc(&dppi_lp, &dppi_lp_channel_sleep_exit);
	__ASSERT_NO_MSG(err == NRFX_SUCCESS);

	err = nrfx_dppi_channel_alloc(&dppi_peri, &dppi_peri_channel_sleep_exit);
	__ASSERT_NO_MSG(err == NRFX_SUCCESS);

	err = nrfx_ppib_channel_alloc(&ppib, &ppib_channel_sleep_exit);
	__ASSERT_NO_MSG(err == NRFX_SUCCESS);

	/* POWER.EVENT_SLEEPEXIT -> DPPI30 */
	nrf_power_publish_set(NRF_POWER, NRF_POWER_EVENT_SLEEPEXIT, dppi_lp_channel_sleep_exit);
	/* DPPI30 -> PPIB30 */
	nrf_ppib_subscribe_set(ppib.right.p_reg, nrf_ppib_send_task_get(ppib_channel_sleep_exit),
			       dppi_lp_channel_sleep_exit);
	/* PPIB30 -> PPIB22 (implicit) */
	/* PPIB22 -> DPPI20 */
	nrf_ppib_publish_set(ppib.left.p_reg, nrf_ppib_receive_event_get(ppib_channel_sleep_exit),
			     dppi_peri_channel_sleep_exit);
	/* DPPI20 -> TIMER.TASK_START */
	nrf_timer_subscribe_set(timer_cpu.p_reg, NRF_TIMER_TASK_START,
				dppi_peri_channel_sleep_exit);

	err = nrfx_dppi_channel_enable(&dppi_lp, dppi_lp_channel_sleep_exit);
	__ASSERT_NO_MSG(err == NRFX_SUCCESS);

	err = nrfx_dppi_channel_enable(&dppi_peri, dppi_peri_channel_sleep_exit);
	__ASSERT_NO_MSG(err == NRFX_SUCCESS);

	k_timer_start(&timer_meas, K_NO_WAIT, K_USEC(CPU_USAGE_AGG_PERIOD_US));

	initialized = true;
}

static void VendorUsageCpuReset()
{
	unsigned key = irq_lock();

	meas_started = false;
	max_cpu_usage = 0;
	memset(cpu_usage_histogram, 0, sizeof(cpu_usage_histogram));
	irq_unlock(key);
}

static void VendorUsageCpuUpdate(uint32_t usage)
{
	size_t histogram_index;

	usage = MIN(usage, 100);
	histogram_index = (size_t)(usage * CPU_USAGE_HISTOGRAM_LEN / 100);

	if (histogram_index == CPU_USAGE_HISTOGRAM_LEN) {
		histogram_index--;
	}

	cpu_usage_histogram[histogram_index]++;
	max_cpu_usage = MAX(usage, max_cpu_usage);
}

static void VendorUsageCpuMeasureBegin(void)
{
	if (!initialized) {
		return;
	}

	nrf_timer_task_trigger(timer_cpu.p_reg, NRF_TIMER_TASK_CLEAR);
	meas_started = true;
}

static void VendorUsageCpuMeasureEnd(void)
{
	uint32_t cpu_time;
	uint32_t cpu_usage;

	if (!meas_started) {
		return;
	}

	cpu_time = nrfx_timer_capture(&timer_cpu, NRF_TIMER_CC_CHANNEL0);
	cpu_usage = (uint32_t)((cpu_time * 100ULL + CPU_USAGE_AGG_PERIOD_US / 2) /
			       CPU_USAGE_AGG_PERIOD_US);
	VendorUsageCpuUpdate(cpu_usage);
	meas_started = false;
}

static void VendorUsageCpuMeasure(struct k_timer *timer)
{
	VendorUsageCpuMeasureEnd();
	VendorUsageCpuMeasureBegin();
}

otError VendorUsageCpu(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	uint32_t max_usage;
	uint32_t hist[CPU_USAGE_HISTOGRAM_LEN];
	unsigned key;

	if (aArgsLength >= 1) {
		if (strcmp(aArgs[0], "init") == 0) {
			VendorUsageCpuInit();
		} else if (strcmp(aArgs[0], "reset") == 0) {
			VendorUsageCpuReset();
		} else {
			return OT_ERROR_INVALID_ARGS;
		}
		return OT_ERROR_NONE;
	}

	key = irq_lock();
	max_usage = max_cpu_usage;
	memcpy(hist, cpu_usage_histogram, sizeof(hist));
	irq_unlock(key);

	if (max_cpu_usage == 0) {
		otCliOutputFormat("Max CPU usage: unknown\r\n");
		otCliOutputFormat("CPU usage/100ms histogram: unknown\r\n");
	} else {
		otCliOutputFormat("Max CPU usage: %u%\r\n", max_usage);
		otCliOutputFormat("CPU usage/100ms histogram: %u %u %u %u %u %u %u %u %u %u\r\n",
				  hist[0], hist[1], hist[2], hist[3], hist[4], hist[5], hist[6],
				  hist[7], hist[8], hist[9]);
	}

	return OT_ERROR_NONE;
}
