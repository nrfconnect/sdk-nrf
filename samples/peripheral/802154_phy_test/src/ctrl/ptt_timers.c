/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: timers implementation */

#include <stddef.h>

#include "ptt.h"

#include "ctrl/ptt_trace.h"

#include "ptt_timers_internal.h"

#ifdef TESTS
#include "test_timers_conf.h"
#else
#include "ptt_ctrl_internal.h"
#endif

/* Main idea is to keep timeouts bounded to current time and refresh them every
 *	time when a new timer added or any timer is ended. The nearest timeout is
 *   selected from all timeouts in the pool and the application timer is called
 *   with this timeout. It helps to charge the application timer actually only
 *   when a timer which is closer than already charged one is added and allows
 *   to keep a pool of timers based on a single application timer.
 */

static void charge_app_timer(void);
static ptt_time_t get_min_timeout(void);
static void update_timers(ptt_time_t current_time);

static inline void app_timer_update(ptt_time_t timeout);
static inline ptt_time_t update_timeout(ptt_time_t timeout, ptt_time_t value);
static inline ptt_time_t time_diff(ptt_time_t prev_time, ptt_time_t new_time);

void ptt_timers_init(void)
{
	PTT_TRACE("%s ->\n", __func__);

	struct ptt_timer_ctx_s *timer_ctx = ptt_ctrl_get_timer_ctx();

	*timer_ctx = (struct ptt_timer_ctx_s){ 0 };
	timer_ctx->free_slots = PTT_TIMER_POOL_N;
	timer_ctx->last_timeout = ptt_ctrl_get_max_time();
	timer_ctx->last_update_time = ptt_ctrl_get_max_time();

	PTT_TRACE("%s -<\n", __func__);
}

void ptt_timers_uninit(void)
{
	PTT_TRACE("%s\n", __func__);
}

void ptt_timers_reset_all(void)
{
	PTT_TRACE("%s ->\n", __func__);
	ptt_timers_uninit();
	ptt_timers_init();
	PTT_TRACE("%s -<\n", __func__);
}

static inline ptt_time_t time_diff(ptt_time_t prev_time, ptt_time_t new_time)
{
	if (prev_time <= new_time) {
		return (new_time - prev_time);
	} else {
		return ((ptt_ctrl_get_max_time() - prev_time) + new_time);
	}
}

static inline void app_timer_update(ptt_time_t timeout)
{
	PTT_TRACE("%s -> %d\n", __func__, timeout);

	struct ptt_timer_ctx_s *timer_ctx = ptt_ctrl_get_timer_ctx();

	timer_ctx->last_timeout = timeout;
	ptt_ctrl_call_me_cb(timeout);

	PTT_TRACE("%s -<\n", __func__);
}

static inline ptt_time_t update_timeout(ptt_time_t timeout, ptt_time_t value)
{
	if (value > timeout) {
		return 0;
	} else {
		return (timeout - value);
	}
}

void ptt_timers_process(ptt_time_t current_time)
{
	PTT_TRACE("%s -> %d\n", __func__, current_time);

	uint8_t i;
	struct ptt_timer_ctx_s *timer_ctx;

	update_timers(current_time);

	timer_ctx = ptt_ctrl_get_timer_ctx();

	for (i = 0; i < PTT_TIMER_POOL_N; ++i) {
		if ((true == timer_ctx->timer_pool[i].use) &&
		    (timer_ctx->timer_pool[i].timeout == 0)) {
			if (timer_ctx->timer_pool[i].cb != NULL) {
				timer_ctx->timer_pool[i].cb(timer_ctx->timer_pool[i].evt_id);
			}
			timer_ctx->timer_pool[i] = (struct ptt_timer_s){ 0 };
			timer_ctx->free_slots++;
		}
	}

	charge_app_timer();

	PTT_TRACE("%s -<\n", __func__);
}

enum ptt_ret ptt_timer_add(ptt_time_t timeout, ptt_timer_cb cb, ptt_evt_id_t evt_id)
{
	PTT_TRACE("%s -> tm %d cb %p evt %d\n", __func__, timeout, cb, evt_id);

	enum ptt_ret ret = PTT_RET_SUCCESS;
	struct ptt_timer_s timer = { 0 };

	if (timeout > ptt_ctrl_get_max_time()) {
		ret = PTT_RET_INVALID_VALUE;
		PTT_TRACE("%s -< ret %d\n", __func__, ret);
		return ret;
	}

	if (cb != NULL) {
		timer.cb = cb;
	} else {
		ret = PTT_RET_INVALID_VALUE;
		PTT_TRACE("%s -< ret %d\n", __func__, ret);
		return ret;
	}

	if (evt_id < PTT_EVENT_POOL_N) {
		timer.evt_id = evt_id;
	} else {
		ret = PTT_RET_INVALID_VALUE;
		PTT_TRACE("%s -< ret %d\n", __func__, ret);
		return ret;
	}

	timer.timeout = timeout;
	timer.use = true;

	struct ptt_timer_ctx_s *timer_ctx = ptt_ctrl_get_timer_ctx();

	if (timer_ctx->free_slots == 0) {
		ret = PTT_RET_NO_FREE_SLOT;
		PTT_TRACE("%s -< ret %d\n", __func__, ret);
		return ret;
	}

	ptt_time_t current_time = ptt_get_current_time();

	/* refresh last update time each time when the first timer is added to empty pool
	 * to have actual time after large delay of update_timers calls
	 */
	if (timer_ctx->free_slots == PTT_TIMER_POOL_N) {
		timer_ctx->last_update_time = current_time;

		/* as pool is empty */
		timer_ctx->timer_pool[0] = timer;
		timer_ctx->free_slots--;

		app_timer_update(timer.timeout);
	} else {
		/* need to bind all previously placed timers to current time to align them with
		 * timer of adding new timer
		 */
		/* legend:
		 *	o - time of last binding timers to absolute time (last_update_time)
		 *	c - current time (time of adding the new timer)
		 *	a - time when the application timer will shoot
		 *	t0, t1 - previously added timers
		 *	t2 - new timer
		 *	x0, x1, x2 - timeouts
		 */
		/*    o----c-----a
		 * |-------------x0    t0
		 * |----------x1       t1
		 * .....|..........    t2(new)
		 */
		update_timers(current_time);
		/*    o----c-----a
		 * .....|--------x0    t0
		 * .....|-----x1       t1
		 * .....|..........    t2(new)
		 */

		/* add timer to pool */
		for (uint8_t i = 0; i < PTT_TIMER_POOL_N; ++i) {
			if (false == timer_ctx->timer_pool[i].use) {
				timer_ctx->timer_pool[i] = timer;
				timer_ctx->free_slots--;
				break;
			}
		}
		/*    o----c-----a
		 *    .....|--------x0    t0
		 *    .....|-----x1       t1
		 *    .....|----------x2  t2(new)
		 */

		/* check is new timer closer than already charged in the application timer */
		/* note: timer.timeout = x2 - c */
		/* note: timer_ctx->last_timeout = x1 - c */
		if (timer_ctx->last_timeout > timer.timeout) {
			app_timer_update(timer.timeout);
		}
	}

	PTT_TRACE("%s -< ret %d\n", __func__, ret);
	return ret;
}

/* if removing timer was going to expire then one of next calls from an application
 * will be ptt_timers_process, where timer pool will be updated and new application
 * timer will be charged if any timers are running
 */
void ptt_timer_remove(ptt_evt_id_t evt_id)
{
	PTT_TRACE("%s -> evt %d\n", __func__, evt_id);

	uint8_t i;
	struct ptt_timer_ctx_s *timer_ctx = ptt_ctrl_get_timer_ctx();

	for (i = 0; i < PTT_TIMER_POOL_N; ++i) {
		if (evt_id == timer_ctx->timer_pool[i].evt_id) {
			timer_ctx->timer_pool[i] = (struct ptt_timer_s){ 0 };
			timer_ctx->free_slots += 1;
		}
	}

	PTT_TRACE("%s -<\n", __func__);
}

static void charge_app_timer(void)
{
	PTT_TRACE("%s ->\n", __func__);

	ptt_time_t current_time = ptt_get_current_time();

	update_timers(current_time);
	ptt_time_t nearest_timeout = get_min_timeout();

	app_timer_update(nearest_timeout);

	PTT_TRACE("%s -<\n", __func__);
}

static ptt_time_t get_min_timeout(void)
{
	PTT_TRACE("%s ->\n", __func__);

	/* @fixme: division by 4 is workaround to update application timer variables
	 * min_timeout = ptt_ctrl_get_max_time() should be after bug is fixed
	 */
	ptt_time_t min_timeout = ptt_ctrl_get_max_time() / 4;
	struct ptt_timer_ctx_s *timer_ctx = ptt_ctrl_get_timer_ctx();
	uint8_t i;

	for (i = 0; i < PTT_TIMER_POOL_N; ++i) {
		if ((true == timer_ctx->timer_pool[i].use) &&
		    (min_timeout > timer_ctx->timer_pool[i].timeout)) {
			min_timeout = timer_ctx->timer_pool[i].timeout;
		}
	}

	PTT_TRACE("%s -< tm %d\n", __func__, min_timeout);
	return min_timeout;
}

static void update_timers(ptt_time_t current_time)
{
	PTT_TRACE("%s -> ct %d\n", __func__, current_time);

	uint8_t i;
	struct ptt_timer_ctx_s *timer_ctx = ptt_ctrl_get_timer_ctx();

	/* calculate time difference */
	ptt_time_t diff = time_diff(timer_ctx->last_update_time, current_time);

	for (i = 0; i < PTT_TIMER_POOL_N; ++i) {
		if (true == timer_ctx->timer_pool[i].use) {
			timer_ctx->timer_pool[i].timeout =
				update_timeout(timer_ctx->timer_pool[i].timeout, diff);
		}
	}

	timer_ctx->last_timeout = update_timeout(timer_ctx->last_timeout, diff);
	timer_ctx->last_update_time = current_time;

	PTT_TRACE("%s -<\n", __func__);
}
