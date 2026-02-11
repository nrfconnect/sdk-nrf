/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ppi_seq/ppi_seq.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#include <nrfx_grtc.h>
#include <hal/nrf_dppi.h>
#ifdef RTC_PRESENT
#include <hal/nrf_rtc.h>
#endif
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ppi_seq);

static void timer_handler(struct ppi_seq *seq);
static void next_sys_timer_notifier(struct ppi_seq *seq);

static int ppi_alloc(struct ppi_seq *seq, uint32_t eep, uint32_t tep, bool enable)
{
	int rv;

	if (seq->ppi_cnt == ARRAY_SIZE(seq->ppi_pool)) {
		return -ENODEV;
	}

	if (seq->config->skip_gppi) {
		seq->ppi_pool[seq->ppi_cnt] = tep;
	} else if (IS_ENABLED(CONFIG_NRFX_GPPI)) {
		rv = nrfx_gppi_conn_alloc(eep, tep, &seq->ppi_pool[seq->ppi_cnt]);
		if (rv < 0) {
			return rv;
		}

		if (enable) {
			nrfx_gppi_conn_enable(seq->ppi_pool[seq->ppi_cnt]);
		}
	} else {
		return -ENOTSUP;
	}

	rv = seq->ppi_cnt;
	seq->ppi_cnt++;
	return rv;
}

static void ppi_enable(struct ppi_seq *seq)
{
	if (seq->config->skip_gppi) {
		for (uint32_t i = 0; i < seq->ppi_cnt; i++) {
			NRF_DPPI_ENDPOINT_ENABLE(seq->ppi_pool[i]);
		}
	} else if (IS_ENABLED(CONFIG_NRFX_GPPI)) {
		nrfx_gppi_conn_enable(seq->ppi_pool[0]);
	}
}

static void ppi_disable(struct ppi_seq *seq)
{
	if (seq->config->skip_gppi) {
		for (uint32_t i = 0; i < seq->ppi_cnt; i++) {
			NRF_DPPI_ENDPOINT_DISABLE(seq->ppi_pool[i]);
		}
	} else if (IS_ENABLED(CONFIG_NRFX_GPPI)) {
		nrfx_gppi_conn_disable(seq->ppi_pool[0]);
	}
}

static int eep_tep_connect(struct ppi_seq *seq, uint32_t eep, uint32_t tep)
{
	int ch = nrfx_gppi_ep_channel_get(tep);
	uint32_t domain_id;
	nrfx_gppi_handle_t handle;

	if (ch < 0) {
		return ppi_alloc(seq, eep, tep, true);
	}

	if (IS_ENABLED(CONFIG_NRFX_GPPI)) {
		domain_id = nrfx_gppi_domain_id_get(tep);
		for (int i = 0; i < seq->ppi_cnt; i++) {
			handle = seq->ppi_pool[i];
			if (nrfx_gppi_domain_channel_get(handle, domain_id) == ch) {
				return nrfx_gppi_ep_attach(eep, handle);
			}
		}
	}

	return -EINVAL;
}

#ifdef RTC_PRESENT
static inline uint64_t rtc_us_to_ticks(uint64_t us)
{
	return (uint64_t)((us * NRF_RTC_INPUT_FREQ) / MHZ(1));
}

static inline uint64_t rtc_ticks_to_us(uint64_t ticks)
{
	return (uint64_t)((ticks * MHZ(1)) / NRF_RTC_INPUT_FREQ);
}

static void rtc_cc_set(NRF_RTC_Type *rtc, uint32_t cc, uint32_t ticks)
{
	nrf_rtc_event_enable(rtc, NRF_RTC_CHANNEL_INT_MASK(cc));
	nrf_rtc_cc_set(rtc, cc, ticks);
}

static void rtc_init(NRF_RTC_Type *rtc, uint32_t ticks)
{
	nrf_rtc_prescaler_set(rtc, 0);
#ifdef RTC_SHORTS_COMPARE0_CLEAR_Msk
	/* HAL is currently missing support for shorts. */
	rtc->SHORTS = BIT(0); /* Compare0->Clear */
#endif
	rtc_cc_set(rtc, 0, ticks);
	rtc_cc_set(rtc, 1, 1);
}

static void rtc_start(NRF_RTC_Type *rtc)
{
	nrf_rtc_event_clear(rtc, NRF_RTC_EVENT_COMPARE_1);
	nrf_rtc_task_trigger(rtc, NRF_RTC_TASK_START);
	nrf_rtc_task_trigger(rtc, NRF_RTC_TASK_CLEAR);
}

static void rtc_stop(NRF_RTC_Type *rtc)
{
	nrf_rtc_task_trigger(rtc, NRF_RTC_TASK_STOP);
	nrf_rtc_task_trigger(rtc, NRF_RTC_TASK_CLEAR);
	nrf_rtc_event_disable(rtc, UINT32_MAX);
#ifdef RTC_SHORTS_COMPARE0_CLEAR_Msk
	rtc->SHORTS = 0;
#endif
}

static uint32_t rtc_evt_addr(NRF_RTC_Type *rtc, uint32_t cc)
{
	return nrf_rtc_event_address_get(rtc, nrf_rtc_compare_event_get(cc));
}

static int extra_ops_with_rtc_init(struct ppi_seq *seq)
{
	const struct ppi_seq_config *config = seq->config;
	uint32_t max_ops = 3;
	int rv;

	if (config->extra_ops_count > max_ops) {
		/* Not enough channels to handle that. */
		return -EINVAL;
	}

	for (size_t i = 0; i < config->extra_ops_count; i++) {
		uint32_t eep = rtc_evt_addr(config->rtc_reg, i + 2);

		rv = eep_tep_connect(seq, eep, config->extra_ops[i].task);
		if (rv < 0) {
			return rv;
		}
		rtc_cc_set(config->rtc_reg, i + 2,
			   1 + rtc_us_to_ticks(config->extra_ops[i].offset));
	}

	return 0;
}

static void extra_ops_with_rtc_uninit(struct ppi_seq *seq)
{
	for (size_t i = 0; i < seq->config->extra_ops_count; i++) {
		nrfx_gppi_ep_clear(rtc_evt_addr(seq->config->rtc_reg, i + 2));
		nrfx_gppi_ep_clear(seq->config->extra_ops[i].task);
	}
}

void ppi_seq_rtc_irq_handler(struct ppi_seq *seq)
{
	struct ppi_seq_notifier *notifier = seq->config->notifier;
	uint64_t next;
	uint64_t now;
	uint32_t period_us;

	nrf_rtc_event_clear(seq->config->rtc_reg, NRF_RTC_EVENT_COMPARE_1);
	nrf_rtc_int_disable(seq->config->rtc_reg, NRF_RTC_CHANNEL_INT_MASK(1));

	now = k_ticks_to_us_floor64(sys_clock_tick_get());
	period_us = rtc_ticks_to_us(notifier->sys_timer.period);

	/* Get next timeout in absolute microseconds. */
	next = now + (seq->batch_cnt - 1) * period_us + notifier->sys_timer.offset + period_us / 4;
	/* Convert it to RTC ticks. */
	next = rtc_us_to_ticks(next);
	notifier->sys_timer.period = seq->batch_cnt * notifier->sys_timer.period;
	notifier->sys_timer.timestamp = next;

	next_sys_timer_notifier(seq);
}
#endif /* RTC_PRESENT */

static void nrfx_timer_handler(nrf_timer_event_t event_type, void *context)
{
	timer_handler((struct ppi_seq *)context);
}

static void next_sys_timer_notifier(struct ppi_seq *seq)
{
	struct ppi_seq_notifier *notifier = seq->config->notifier;
	uint64_t next;
	k_timeout_t timeout;

#ifdef RTC_PRESENT
	if (seq->config->rtc_reg != NULL) {
		next = rtc_ticks_to_us(notifier->sys_timer.timestamp);
	} else {
		next = notifier->sys_timer.timestamp;
	}
#else
	next = notifier->sys_timer.timestamp;
#endif

	timeout = Z_TIMEOUT_TICKS(Z_TICK_ABS(K_USEC(next).ticks));
	k_timer_start(&notifier->sys_timer.timer, timeout, K_NO_WAIT);
	notifier->sys_timer.timestamp += notifier->sys_timer.period;
}

static void k_timer_handler(struct k_timer *timer)
{
	struct ppi_seq *seq = k_timer_user_data_get(timer);

	timer_handler(seq);
	if (seq->repeat != 0) {
		next_sys_timer_notifier(seq);
	}
}

/** @brief Initialize mechanism for notifying about sequence completion.
 *
 * There are 2 methods:
 *  - nrfx_timer in counter mode
 *  - system_timer
 */
static int ppi_seq_notifier_init(struct ppi_seq *seq)
{
	struct ppi_seq_notifier *notifier = seq->config->notifier;
	int rv;

	if (notifier->type == PPI_SEQ_NOTIFIER_SYS_TIMER) {
		k_timer_init(&notifier->sys_timer.timer, k_timer_handler, NULL);
		k_timer_user_data_set(&notifier->sys_timer.timer, seq);
		/* Calculate offset when system timer should expire. It should be after sequence
		 * is completed but enough time before next sequence starts.
		 */
		if (seq->config->extra_ops) {
			for (size_t i = 0; i < seq->config->extra_ops_count; i++) {
				notifier->sys_timer.offset += seq->config->extra_ops[i].offset;
			}
		}
		return 0;
	}

	nrfx_timer_config_t timer_config = {.frequency = MHZ(1),
					    .mode = NRF_TIMER_MODE_LOW_POWER_COUNTER,
					    .bit_width = NRF_TIMER_BIT_WIDTH_32,
					    .p_context = seq};

	uint32_t cnt_task =
		nrfx_timer_task_address_get(&notifier->nrfx_timer.timer, NRF_TIMER_TASK_COUNT);

	rv = nrfx_timer_init(&notifier->nrfx_timer.timer, &timer_config, nrfx_timer_handler);
	if (rv < 0) {
		return rv;
	}

	rv = ppi_alloc(seq, notifier->nrfx_timer.end_seq_event, cnt_task, true);
	if (rv < 0) {
		return rv;
	}

	return 0;
}

/** @brief Uninitialize mechanism for notifying about sequence completion. */
static void ppi_seq_notifier_uninit(struct ppi_seq *seq)
{
	struct ppi_seq_notifier *notifier = seq->config->notifier;
	uint32_t cnt_task;

	if (notifier->type == PPI_SEQ_NOTIFIER_SYS_TIMER) {
		return;
	}

	cnt_task = nrfx_timer_task_address_get(&notifier->nrfx_timer.timer, NRF_TIMER_TASK_COUNT);
	nrfx_timer_uninit(&notifier->nrfx_timer.timer);
	nrfx_gppi_ep_clear(notifier->nrfx_timer.end_seq_event);
	nrfx_gppi_ep_clear(cnt_task);
}

/** @brief Stop mechanism used for notifying completion of the sequence. */
static void ppi_seq_notifier_stop(struct ppi_seq *seq)
{
	struct ppi_seq_notifier *notifier = seq->config->notifier;

	if (notifier->type == PPI_SEQ_NOTIFIER_SYS_TIMER) {
		k_timer_stop(&notifier->sys_timer.timer);
		return;
	}

	nrfx_timer_disable(&notifier->nrfx_timer.timer);
}

/** @brief Start mechanism for notifying a completion of the sequence. */
static void ppi_seq_sys_notifier_start(struct ppi_seq *seq, size_t period, size_t batch_cnt)
{
	struct ppi_seq_notifier *notifier = seq->config->notifier;
	uint64_t now = k_ticks_to_us_floor64(sys_clock_tick_get());
	uint64_t next;

#ifdef RTC_PRESENT
	if (seq->config->rtc_reg != NULL) {
		notifier->sys_timer.period = period;
		seq->batch_cnt = (uint16_t)batch_cnt;
		nrf_rtc_int_enable(seq->config->rtc_reg, NRF_RTC_CHANNEL_INT_MASK(1));
		return;
	}
#endif
	next = now + (batch_cnt - 1) * period + notifier->sys_timer.offset + period / 4;
	notifier->sys_timer.period = batch_cnt * period;
	notifier->sys_timer.timestamp = next;
	next_sys_timer_notifier(seq);
}

static void ppi_seq_nrfx_notifier_start(struct ppi_seq *seq, size_t period, size_t batch_cnt)
{
	struct ppi_seq_notifier *notifier = seq->config->notifier;
	uint32_t val = batch_cnt * (1 + notifier->nrfx_timer.extra_main_ops);
	nrf_timer_cc_channel_t ch = NRF_TIMER_CC_CHANNEL0;
	uint32_t shorts = NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK;

	nrfx_timer_clear(&notifier->nrfx_timer.timer);
	nrfx_timer_enable(&notifier->nrfx_timer.timer);
	nrfx_timer_extended_compare(&notifier->nrfx_timer.timer, ch, val, shorts, true);
}

static void stop_ppi_seq(struct ppi_seq *seq)
{
	ppi_disable(seq);
	if (seq->grtc_chan != UINT8_MAX) {
		nrfx_grtc_syscounter_cc_disable((uint8_t)seq->grtc_chan);
	} else {
#ifdef RTC_PRESENT
		if (seq->config->rtc_reg != NULL) {
			rtc_stop(seq->config->rtc_reg);
		} else {
			nrfx_timer_disable(&seq->timer);
		}
#else
		nrfx_timer_disable(&seq->timer);
#endif
	}

	ppi_seq_notifier_stop(seq);
}

static void timer_handler(struct ppi_seq *seq)
{
	bool stop = false;

	if (seq->repeat != UINT32_MAX) {
		seq->repeat--;
		if (seq->repeat == 0) {
			stop_ppi_seq(seq);
			stop = true;
		}
	}

	seq->config->callback(seq, stop);
}

static int extra_ops_with_timer_init(struct ppi_seq *seq)
{
	const struct ppi_seq_config *config = seq->config;
	bool same_evt = config->task == config->extra_ops[config->extra_ops_count - 1].task;
	uint32_t max_ops = same_evt ? (NRF_TIMER_CC_COUNT_MAX - 1) : NRF_TIMER_CC_COUNT_MAX;
	uint32_t last_ch = same_evt ? config->extra_ops_count : (config->extra_ops_count - 1);
	int rv;

	if (config->extra_ops_count > max_ops) {
		/* Not enough channels to handle that. */
		return -EINVAL;
	}

	uint32_t tep = nrfx_timer_task_address_get(&seq->timer, NRF_TIMER_TASK_START);

	if (IS_ENABLED(CONFIG_NRFX_GPPI)) {
		rv = nrfx_gppi_ep_attach(tep, seq->ppi_pool[0]);
		if (rv < 0) {
			LOG_ERR("Task cannot be attached to the connection. "
				"Use TIMER from the same domain as the target task %d",
				rv);
			return rv;
		}
	}

	for (size_t i = 0; i < config->extra_ops_count; i++) {
		uint32_t eep;

		eep = nrfx_timer_event_address_get(&seq->timer, nrf_timer_compare_event_get(i));
		rv = eep_tep_connect(seq, eep, config->extra_ops[i].task);
		if (rv < 0) {
			return rv;
		}

		nrfx_timer_compare(&seq->timer, i, config->extra_ops[i].offset, false);
	}

	/* Add stop clear when extra events cycle ends. */
	if (same_evt) {
		nrfx_timer_compare(&seq->timer, config->extra_ops_count,
				   config->extra_ops[config->extra_ops_count - 1].offset + 1,
				   false);
	}
	nrf_timer_shorts_set(seq->timer.p_reg, nrf_timer_short_compare_stop_get(last_ch) |
					       nrf_timer_short_compare_clear_get(last_ch));

	return 0;
}

static void extra_ops_with_timer_uninit(struct ppi_seq *seq)
{
	nrfx_gppi_ep_clear(nrfx_timer_task_address_get(&seq->timer, NRF_TIMER_TASK_START));
	for (size_t i = 0; i < seq->config->extra_ops_count; i++) {
		uint32_t eep =
			nrfx_timer_event_address_get(&seq->timer, nrf_timer_compare_event_get(i));

		nrfx_gppi_ep_clear(eep);
		nrfx_gppi_ep_clear(seq->config->extra_ops[i].task);
	}
}

static int extra_ops_init(struct ppi_seq *seq)
{
	if (seq->config->extra_ops == NULL) {
		return 0;
	}

	if (seq->config->timer_reg) {
		return extra_ops_with_timer_init(seq);
	}

#ifdef RTC_PRESENT
	return extra_ops_with_rtc_init(seq);
#else
	return -ENOTSUP;
#endif
}

static void extra_ops_uninit(struct ppi_seq *seq)
{
	if (seq->config->timer_reg) {
		extra_ops_with_timer_uninit(seq);
		return;
	}

#ifdef RTC_PRESENT
	extra_ops_with_rtc_uninit(seq);
#endif
}

int ppi_seq_init(struct ppi_seq *seq, const struct ppi_seq_config *config)
{
	uint32_t periodic_evt = 0;
	int rv;
	bool has_timer = config->timer_reg != NULL;
#ifdef RTC_PRESENT
	bool has_rtc = config->rtc_reg != NULL;
#else
	bool has_rtc = false;
#endif
	bool use_grtc = (!has_timer && !has_rtc) || (has_timer && (config->extra_ops != NULL));

	seq->config = config;

	seq->grtc_chan = UINT8_MAX;
	if (use_grtc) {
		rv = z_nrf_grtc_timer_ext_chan_alloc();
		if (rv < 0) {
			return rv;
		}

		seq->grtc_chan = (uint8_t)rv;
		periodic_evt = z_nrf_grtc_timer_compare_evt_address_get(rv);
	}

#ifdef RTC_PRESENT
	if (has_rtc) {
		if (periodic_evt == 0) {
			periodic_evt = rtc_evt_addr(config->rtc_reg, 1);
		}
	}
#endif

	if (has_timer) {
		__ASSERT_NO_MSG(config->timer_reg != NULL);
		if (!use_grtc && (config->notifier->type == PPI_SEQ_NOTIFIER_SYS_TIMER)) {
			/* If TIMER is used for triggering then TIMER need to be used also for
			 * completion notifier. System timer cannot be used because it is driven
			 * from different source than TIMER (HFCLK vs LFCLK) and if clock source
			 * are not accurate it lead to undefined behavior.
			 */
			return -ENOTSUP;
		}

		static const nrfx_timer_config_t timer_config = {
			.frequency = MHZ(1),
			.mode = NRF_TIMER_MODE_TIMER,
			.bit_width = NRF_TIMER_BIT_WIDTH_32,
		};

		seq->timer.p_reg = config->timer_reg;
		/* Setup TIMER for repeating transactions. */
		rv = nrfx_timer_init(&seq->timer, &timer_config, NULL);
		if (rv < 0) {
			return rv;
		}

		if (periodic_evt == 0) {
			nrfx_timer_compare(&seq->timer, NRF_TIMER_CC_CHANNEL1, 1, false);
			periodic_evt = nrfx_timer_event_address_get(&seq->timer,
								    nrf_timer_compare_event_get(1));
		}
	}

	rv = ppi_alloc(seq, periodic_evt, config->task, false);
	if (rv < 0) {
		return rv;
	}

	rv = ppi_seq_notifier_init(seq);
	if (rv < 0) {
		return rv;
	}

	rv = extra_ops_init(seq);

	ppi_disable(seq);

	return rv;
}

void ppi_seq_uninit(struct ppi_seq *seq)
{
	uint32_t periodic_evt = 0;

	/* Sequencer is not initialized. */
	if (seq->ppi_cnt == 0) {
		return;
	}

	(void)ppi_seq_stop(seq, true);

	extra_ops_uninit(seq);

	if (seq->grtc_chan != UINT8_MAX) {
		periodic_evt = z_nrf_grtc_timer_compare_evt_address_get(seq->grtc_chan);
		nrfx_grtc_syscounter_cc_interval_reset(seq->grtc_chan);
		z_nrf_grtc_timer_chan_free(seq->grtc_chan);
	}

#ifdef RTC_PRESENT
	if (seq->config->rtc_reg != NULL) {
		periodic_evt = rtc_evt_addr(seq->config->rtc_reg, 1);
	}
#endif

	if (seq->config->timer_reg != NULL) {
		/* Uninit TIMER for repeating transactions. */
		if (periodic_evt == 0) {
			periodic_evt = nrfx_timer_event_address_get(&seq->timer,
								    nrf_timer_compare_event_get(0));
			nrfx_gppi_ep_clear(periodic_evt);
			periodic_evt = nrfx_timer_event_address_get(&seq->timer,
								    nrf_timer_compare_event_get(1));
		}
		nrfx_timer_uninit(&seq->timer);
	}

	if (!IS_ENABLED(CONFIG_NRFX_GPPI_V1)) {
		nrfx_gppi_ep_clear(periodic_evt);
		nrfx_gppi_ep_clear(seq->config->task);
	}

	ppi_seq_notifier_uninit(seq);

	if (seq->config->skip_gppi == false) {
		for (uint8_t i = 0; i < seq->ppi_cnt; i++) {
			nrfx_gppi_conn_disable(seq->ppi_pool[i]);
			if (IS_ENABLED(CONFIG_NRFX_GPPI_V1) && (i == 0)) {
				nrfx_gppi_conn_free(periodic_evt, seq->config->task,
						    seq->ppi_pool[i]);
			} else {
				nrfx_gppi_domain_conn_free(seq->ppi_pool[i]);
			}
		}
	}
	seq->ppi_cnt = 0;
}

int ppi_seq_start(struct ppi_seq *seq, size_t period, size_t batch_cnt, int repeat)
{
	bool nrfx_timer_notifier = seq->config->notifier->type == PPI_SEQ_NOTIFIER_NRFX_TIMER;

	if (atomic_cas(&seq->repeat, 0, repeat) == false) {
		return -EALREADY;
	}

	seq->repeat = repeat;
	if (nrfx_timer_notifier == true) {
		ppi_seq_nrfx_notifier_start(seq, period, batch_cnt);
	}
	ppi_enable(seq);

	if (seq->grtc_chan != UINT8_MAX) {
		nrfx_grtc_syscounter_cc_interval_set(seq->grtc_chan, 1, period);
#ifdef RTC_PRESENT
	} else if (seq->config->rtc_reg != NULL) {
		uint32_t t = rtc_us_to_ticks(period);

		period = t;
		rtc_init(seq->config->rtc_reg, t - 1);
		rtc_start(seq->config->rtc_reg);
#endif
	} else {
		nrfx_timer_extended_compare(&seq->timer, NRF_TIMER_CC_CHANNEL0, period,
					    NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);
		nrfx_timer_enable(&seq->timer);
	}

	if (nrfx_timer_notifier == false) {
		ppi_seq_sys_notifier_start(seq, period, batch_cnt);
	}

	return 0;
}

int ppi_seq_stop(struct ppi_seq *seq, bool immediate)
{
	if (seq->repeat == 0) {
		return -EALREADY;
	}

	if (!immediate) {
		seq->repeat = 1;
		return 0;
	}

	seq->repeat = 0;
	stop_ppi_seq(seq);

	return 0;
}
