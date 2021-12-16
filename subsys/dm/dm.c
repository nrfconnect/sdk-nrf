/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <inttypes.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>

#include <mpsl_timeslot.h>
#include <mpsl.h>

#include <nrf_dm.h>
#include "dm.h"
#include "time.h"
#include "timeslot_queue.h"
#include "dm_io.h"

#if defined(DPPI_PRESENT)
#include <nrfx_dppi.h>
#define gppi_channel_t uint8_t
#define gppi_channel_alloc nrfx_dppi_channel_alloc
#else
#include <nrfx_ppi.h>
#define gppi_channel_t nrf_ppi_channel_t
#define gppi_channel_alloc nrfx_ppi_channel_alloc
#endif

#include <logging/log.h>

LOG_MODULE_REGISTER(nrf_dm, CONFIG_NRF_DM_LOG_LEVEL);

#ifdef CONFIG_NRF_DM_TIMER0
#define NRF_DM_TIMER NRF_TIMER0
#endif
#ifdef CONFIG_NRF_DM_TIMER1
#define NRF_DM_TIMER NRF_TIMER1
#endif
#ifdef CONFIG_NRF_DM_TIMER2
#define NRF_DM_TIMER NRF_TIMER2
#endif
#ifdef CONFIG_NRF_DM_TIMER3
#define NRF_DM_TIMER NRF_TIMER3
#endif
#ifdef CONFIG_NRF_DM_TIMER4
#define NRF_DM_TIMER NRF_TIMER4
#endif

#define PPI_CH_COUNT                 2

#define MPSL_THREAD_PRIO             CONFIG_MPSL_THREAD_COOP_PRIO
#define STACKSIZE                    CONFIG_MAIN_STACK_SIZE
#define DM_THREAD_PRIORITY           K_HIGHEST_APPLICATION_THREAD_PRIO
#define TIMESLOT_TIMEOUT_STEP_MS     120000

static K_MUTEX_DEFINE(ranging_mtx);
static K_TIMER_DEFINE(timer, NULL, NULL);

enum mpsl_timeslot_call {
	OPEN_SESSION,
	MAKE_REQUEST_EARLY,
	MAKE_REQUEST_NORMAL,
	CLOSE_SESSION,
};

enum dm_call {
	INITIATOR_START,
	REFLECTOR_START,
	TIMESLOT_EARLY_END,
	TIMESLOT_NORMAL_END,
	TIMESLOT_RESCHEDULE,
};

enum timeslot_state {
	TIMESLOT_STATE_INIT,
	TIMESLOT_STATE_NEED_EARLY,
	TIMESLOT_STATE_EARLY_PENDING,
	TIMESLOT_STATE_IDLE,
	TIMESLOT_STATE_PENDING
};

K_MSGQ_DEFINE(mpsl_api_msgq, sizeof(enum mpsl_timeslot_call), 8, 4);
K_MSGQ_DEFINE(dm_api_msgq, sizeof(enum dm_call), 8, 4);

struct {
	bool ranging_status;
	struct dm_cb *cb;
} static dm_context;

struct {
	struct timeslot_request curr_req;
	atomic_val_t state;
	uint32_t last_start;
} static timeslot_ctx = {
	.state = ATOMIC_INIT(TIMESLOT_STATE_INIT),
};

/* Timeslot request */
static mpsl_timeslot_request_t timeslot_request_earliest = {
	.request_type = MPSL_TIMESLOT_REQ_TYPE_EARLIEST,
	.params.earliest.hfclk = MPSL_TIMESLOT_HFCLK_CFG_NO_GUARANTEE,
	.params.earliest.priority = MPSL_TIMESLOT_PRIORITY_HIGH,
	.params.earliest.length_us = MPSL_TIMESLOT_LENGTH_MIN_US,
	.params.earliest.timeout_us = 70000,
};

static mpsl_timeslot_request_t timeslot_request_normal = {
	.request_type = MPSL_TIMESLOT_REQ_TYPE_NORMAL,
	.params.normal.hfclk = MPSL_TIMESLOT_HFCLK_CFG_XTAL_GUARANTEED,
	.params.normal.priority = MPSL_TIMESLOT_PRIORITY_HIGH,
	.params.normal.length_us = TIMESLOT_LENGTH_US,
};

struct dm_result result;
static mpsl_timeslot_signal_return_param_t signal_callback_return_param;

static int dm_configure(void)
{
	static nrf_dm_config_t dm_config;

	dm_config = NRF_DM_DEFAULT_CONFIG;
	dm_config.access_address = timeslot_ctx.curr_req.dm_req.access_address;

	if (timeslot_ctx.curr_req.dm_req.role == DM_ROLE_INITIATOR) {
		dm_config.role = NRF_DM_ROLE_INITIATOR;
	} else if (timeslot_ctx.curr_req.dm_req.role == DM_ROLE_REFLECTOR) {
		dm_config.role = NRF_DM_ROLE_REFLECTOR;
	} else {
		return NRF_DM_STATUS_CONF_INVALID;
	}

	if (timeslot_ctx.curr_req.dm_req.ranging_mode == DM_RANGING_MODE_RTT) {
		dm_config.ranging_mode = NRF_DM_RANGING_MODE_RTT;
	} else if (timeslot_ctx.curr_req.dm_req.ranging_mode == DM_RANGING_MODE_MCPD) {
		dm_config.ranging_mode = NRF_DM_RANGING_MODE_MCPD;
	} else {
		return NRF_DM_STATUS_CONF_INVALID;
	}

	return nrf_dm_configure(&dm_config);
}

static mpsl_timeslot_signal_return_param_t *mpsl_timeslot_callback(
				mpsl_timeslot_session_id_t session_id, uint32_t signal_type)
{
	ARG_UNUSED(session_id);
	mpsl_timeslot_signal_return_param_t *p_ret_val = NULL;
	nrf_dm_status_t nrf_dm_status;
	enum dm_call dm_api_call;

	switch (signal_type) {
	case MPSL_TIMESLOT_SIGNAL_START:
		timeslot_ctx.last_start = time_now();
		signal_callback_return_param.callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_END;
		p_ret_val = &signal_callback_return_param;

		dm_io_set(DM_IO_RANGING);

		if (atomic_get(&timeslot_ctx.state) == TIMESLOT_STATE_EARLY_PENDING) {
			return p_ret_val;
		}

		nrf_dm_status = dm_configure();

		if (nrf_dm_status == NRF_DM_STATUS_SUCCESS) {
			uint32_t timeout = timeslot_ctx.curr_req.dm_req.role == DM_ROLE_INITIATOR ?
					 INITIATOR_RANGING_WINDOW_US : REFLECTOR_RANGING_WINDOW_US;
			nrf_dm_status = nrf_dm_proc_execute(timeout);
		}

		dm_context.ranging_status = (nrf_dm_status == NRF_DM_STATUS_SUCCESS);

		dm_io_clear(DM_IO_RANGING);

		break;
	case MPSL_TIMESLOT_SIGNAL_SESSION_IDLE:
		if (atomic_get(&timeslot_ctx.state) == TIMESLOT_STATE_EARLY_PENDING) {
			dm_api_call = TIMESLOT_EARLY_END;
		} else {
			dm_api_call = TIMESLOT_NORMAL_END;
		}

		k_msgq_put(&dm_api_msgq, &dm_api_call, K_NO_WAIT);
		break;
	case MPSL_TIMESLOT_SIGNAL_BLOCKED:
	case MPSL_TIMESLOT_SIGNAL_CANCELLED:
	case MPSL_TIMESLOT_SIGNAL_INVALID_RETURN:
		dm_api_call = TIMESLOT_RESCHEDULE;
		k_msgq_put(&dm_api_msgq, &dm_api_call, K_NO_WAIT);
		break;
	default:
		break;
	}

	return p_ret_val;
}

/* To ensure thread safe operation, call all MPSL APIs from a non-preemptible thread. */
static void mpsl_nonpreemptible_thread(void)
{
	int err;
	enum mpsl_timeslot_call api_call;

	/* Initialize to invalid session id */
	mpsl_timeslot_session_id_t session_id = 0xFFu;

	while (1) {
		if (k_msgq_get(&mpsl_api_msgq, &api_call, K_FOREVER) == 0) {
			switch (api_call) {
			case OPEN_SESSION:
				err = mpsl_timeslot_session_open(mpsl_timeslot_callback,
								 &session_id);
				if (err) {
					LOG_DBG("MPSL session open failed (err %d)", err);
				} else {
					atomic_set(&timeslot_ctx.state, TIMESLOT_STATE_NEED_EARLY);
				}
				break;
			case MAKE_REQUEST_EARLY:
				err = mpsl_timeslot_request(session_id, &timeslot_request_earliest);
				if (err) {
					LOG_DBG("MPSL timeslot request failed (err %d)", err);
					atomic_set(&timeslot_ctx.state, TIMESLOT_STATE_IDLE);
				}
				break;
			case MAKE_REQUEST_NORMAL:
				k_timer_start(&timer, K_MSEC(TIMESLOT_TIMEOUT_STEP_MS), K_NO_WAIT);
				err = mpsl_timeslot_request(session_id, &timeslot_request_normal);
				if (err) {
					LOG_DBG("MPSL timeslot request failed (err %d)", err);
					atomic_set(&timeslot_ctx.state, TIMESLOT_STATE_IDLE);
				}
				break;
			case CLOSE_SESSION:
				break;
			default:
				break;
			}
		}
	}
}

static void process_data(const nrf_dm_report_t *data)
{
	if (!data) {
		result.status = false;
		return;
	}
	result.status = dm_context.ranging_status;
	bt_addr_le_copy(&result.bt_addr, &timeslot_ctx.curr_req.dm_req.bt_addr);

	result.quality = DM_QUALITY_NONE;
	if (data->quality == NRF_DM_QUALITY_OK) {
		result.quality = DM_QUALITY_OK;
	} else if (data->quality == NRF_DM_QUALITY_POOR) {
		result.quality = DM_QUALITY_POOR;
	} else if (data->quality == NRF_DM_QUALITY_DO_NOT_USE) {
		result.quality = DM_QUALITY_DO_NOT_USE;
	} else if (data->quality == NRF_DM_QUALITY_CRC_FAIL) {
		result.quality = DM_QUALITY_CRC_FAIL;
	}

	result.ranging_mode = timeslot_ctx.curr_req.dm_req.ranging_mode;
	if (result.ranging_mode == DM_RANGING_MODE_RTT) {
		result.dist_estimates.rtt.rtt = data->distance_estimates.rtt.rtt;
	} else {
		result.dist_estimates.mcpd.ifft = data->distance_estimates.mcpd.ifft;
		result.dist_estimates.mcpd.phase_slope = data->distance_estimates.mcpd.phase_slope;
		result.dist_estimates.mcpd.best = data->distance_estimates.mcpd.best;
		result.dist_estimates.mcpd.rssi_openspace =
						      data->distance_estimates.mcpd.rssi_openspace;
	}
}

static int timeslot_request_early(void)
{
	int err;
	enum mpsl_timeslot_call mpsl_api_call;

	mpsl_api_call = MAKE_REQUEST_EARLY;
	err = k_msgq_put(&mpsl_api_msgq, &mpsl_api_call, K_FOREVER);
	if (err) {
		LOG_ERR("k_msgq_put MAKE REQUEST EARLY failed (err %d)", err);
	}
	return err;
}

static int timeslot_request(uint32_t distance_from_last)
{
	int err;
	enum mpsl_timeslot_call mpsl_api_call;

	timeslot_request_normal.params.normal.distance_us = distance_from_last;
	mpsl_api_call = MAKE_REQUEST_NORMAL;

	err = k_msgq_put(&mpsl_api_msgq, &mpsl_api_call, K_FOREVER);
	if (err) {
		LOG_ERR("k_msgq_put MAKE REQUEST NORMAL failed (err %d)", err);
	}
	return err;
}

static void dm_start_ranging(void)
{
	struct timeslot_request *req;
	int err;

	k_mutex_lock(&ranging_mtx, K_FOREVER);
	if (k_timer_status_get(&timer) > 0) {
		atomic_set(&timeslot_ctx.state, TIMESLOT_STATE_NEED_EARLY);
	}

	if (atomic_cas(&timeslot_ctx.state,
			TIMESLOT_STATE_NEED_EARLY, TIMESLOT_STATE_EARLY_PENDING)) {
		err = timeslot_request_early();
		if (err) {
			atomic_set(&timeslot_ctx.state, TIMESLOT_STATE_NEED_EARLY);
		}
		goto out;
	}
	if (atomic_get(&timeslot_ctx.state) != TIMESLOT_STATE_IDLE) {
		goto out;
	}

	req = timeslot_queue_peek();
	if (!req) {
		goto out;
	}

	memcpy(&timeslot_ctx.curr_req, req, sizeof(timeslot_ctx.curr_req));
	timeslot_queue_remove_first();

	uint32_t distance = time_distance_get(timeslot_ctx.last_start, req->start_time);
	uint32_t distance_now = time_distance_get(timeslot_ctx.last_start, time_now());

	if (distance_now > distance) {
		goto out;
	}

	atomic_set(&timeslot_ctx.state, TIMESLOT_STATE_PENDING);
	err = timeslot_request(TICKS_TO_US(distance));
	if (err) {
		atomic_set(&timeslot_ctx.state, TIMESLOT_STATE_IDLE);
	}
out:
	k_mutex_unlock(&ranging_mtx);
}

static void dm_reschedule(void)
{
	if (IS_ENABLED(CONFIG_DM_TIMESLOT_RESCHEDULE)) {
		int err;

		if (dm_context.ranging_status) {
			err = timeslot_queue_append(&timeslot_ctx.curr_req.dm_req,
						timeslot_ctx.last_start);
			if (err) {
				LOG_DBG("Timeslot allocator failed (err %d)", err);
			}
		}
		dm_start_ranging();
	}
}

static void dm_thread(void)
{
	int err;
	enum mpsl_timeslot_call mpsl_api_call;
	enum dm_call dm_api_call;

	mpsl_api_call = OPEN_SESSION;
	err = k_msgq_put(&mpsl_api_msgq, &mpsl_api_call, K_FOREVER);
	if (err) {
		LOG_ERR("k_msgq_put OPEN_SESSION failed (err %d)", err);
		return;
	}
	while (1) {
		if (k_msgq_get(&dm_api_msgq, &dm_api_call, K_FOREVER) == 0) {
			switch (dm_api_call) {
			case TIMESLOT_EARLY_END:
				atomic_set(&timeslot_ctx.state, TIMESLOT_STATE_IDLE);
				dm_start_ranging();
				break;
			case TIMESLOT_NORMAL_END:
				dm_reschedule();
				if (dm_context.ranging_status) {
					nrf_dm_calc();
				}

				const nrf_dm_report_t *dm_proc_data = nrf_dm_report_get();

				process_data(dm_proc_data);
				if (dm_context.cb->data_ready != NULL) {
					dm_context.cb->data_ready(&result);
				}
				atomic_set(&timeslot_ctx.state, TIMESLOT_STATE_IDLE);
				dm_start_ranging();
				break;
			case TIMESLOT_RESCHEDULE:
				atomic_set(&timeslot_ctx.state, TIMESLOT_STATE_IDLE);
				dm_start_ranging();
				break;
			default:
				break;
			}
		}
	}
}

int dm_request_add(struct dm_request *req)
{
	int err;

	if (req->role == DM_ROLE_INITIATOR) {
		req->start_delay_us += INITIATOR_DELAY_US;
	} else {
		req->start_delay_us += REFLECTOR_DELAY_US;
	}

	dm_io_set(DM_IO_ADD_REQUEST);
	err = timeslot_queue_append(req, time_now());
	if (err) {
		LOG_DBG("Timeslot allocation failed (err %d)", err);
	}

	dm_start_ranging();
	dm_io_clear(DM_IO_ADD_REQUEST);

	return err;
}


int dm_init(struct dm_init_param *init_param)
{
	int err;
	const char *ver;

	if (!init_param) {
		return -EINVAL;
	}

	uint8_t ppi_ch[PPI_CH_COUNT];
	nrf_dm_ppi_config_t ppi_conf = {
		.ppi_chan_count = PPI_CH_COUNT,
		.ppi_chan = ppi_ch,
	};
	nrf_dm_antenna_config_t ant_conf = NRF_DM_DEFAULT_SINGLE_ANTENNA_CONFIG;

	for (size_t i = 0; i < PPI_CH_COUNT; i++) {
		gppi_channel_t channel;

		err = gppi_channel_alloc(&channel);
		if (err != NRFX_SUCCESS) {
			LOG_ERR("(D)PPI channel allocation error: %08x", err);
			return -ENOMEM;
		}

		ppi_ch[i] = (uint8_t)channel;
	}

	nrf_dm_init(&ppi_conf, &ant_conf, NRF_DM_TIMER);

	ver = nrf_dm_version_string_get();
	LOG_DBG("Initialized NRF_DM version %s", ver);

	dm_context.cb = init_param->cb;

	err = dm_io_init();
	if (err) {
		LOG_ERR("IO init failed (err %d)", err);
		return err;
	}

	return 0;
}

K_THREAD_DEFINE(dm_thread_id, STACKSIZE, dm_thread, NULL, NULL, NULL, DM_THREAD_PRIORITY, 0, 0);

K_THREAD_DEFINE(mpsl_nonpreemptible_thread_id, STACKSIZE,
		mpsl_nonpreemptible_thread, NULL, NULL, NULL, K_PRIO_COOP(MPSL_THREAD_PRIO), 0, 0);
