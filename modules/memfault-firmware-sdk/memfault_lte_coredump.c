/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/smf.h>

#include <memfault/components.h>
#include <memfault/ports/zephyr/http.h>
#include <memfault/metrics/metrics.h>
#include <memfault/panics/coredump.h>

#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <modem/pdn.h>
#include <nrf_modem_at.h>

#include "memfault_lte_coredump_modem_trace.h"

LOG_MODULE_DECLARE(memfault_ncs, CONFIG_MEMFAULT_NCS_LOG_LEVEL);

NRF_MODEM_LIB_ON_INIT(memfault_ncs_lte_coredump_init_hook, on_modem_lib_init, NULL);
NRF_MODEM_LIB_ON_SHUTDOWN(memfault_ncs_lte_coredump_shutdown_hook, on_modem_lib_shutdown, NULL);

#define MSG_QUEUE_ENTRY_COUNT		4
#define MSG_QUEUE_BYTE_ALIGNMENT	4

enum library_state {
	STATE_RUNNING,
	STATE_WAITING_FOR_NRF_MODEM_LIB_INIT,
	STATE_WAITING_FOR_NETWORK_CONNECTION,
	STATE_NETWORK_CONNECTED,
	STATE_COREDUMP_SEND_ATTEMPT,
	STATE_COREDUMP_SEND_BACKOFF,
	STATE_FINISHED,
	STATE_ERROR
};

enum library_event {
	EVENT_NRF_MODEM_LIB_INITED,
	EVENT_NRF_MODEM_LIB_SHUTDOWN,
	EVENT_LTE_REGISTERED,
	EVENT_LTE_DEREGISTERED,
	EVENT_PDN_ACTIVATED,
	EVENT_PDN_DEACTIVATED,
	EVENT_NETWORK_CONNECTED,
	EVENT_NETWORK_DISCONNECTED,
	EVENT_BACKOFF_TIMER_EXPIRED,
	EVENT_COREDUMP_SEND_FAIL,
	EVENT_COREDUMP_SEND_SUCCESS,
	EVENT_ERROR
};

K_MSGQ_DEFINE(mflt_lte_coredump_queue,
	      sizeof(enum library_event),
	      MSG_QUEUE_ENTRY_COUNT,
	      MSG_QUEUE_BYTE_ALIGNMENT);

struct fsm_state_object {
	/* This must be first */
	struct smf_ctx ctx;

	enum library_event event;

	uint8_t retry_count;

	struct k_work_delayable backoff_work;

	bool lte_registered;
	bool pdn_active;
};

/* Forward declarations: Local functions */
static void pdn_event_handler(uint8_t cid, enum pdn_event event, int reason);
static void lte_handler(const struct lte_lc_evt *const evt);
static void on_modem_lib_init(int ret, void *ctx);
static void on_modem_lib_shutdown(void *ctx);
static void event_send(enum library_event event);
static void schedule_send_backoff(struct fsm_state_object *state_object);

/* Forward declarations: State Machine Functions */
static void state_running_entry(void *o);
static enum smf_state_result state_running_run(void *o);
static void state_running_exit(void *o);
static void state_waiting_for_nrf_modem_lib_init_entry(void *o);
static enum smf_state_result state_waiting_for_nrf_modem_lib_init_run(void *o);
static enum smf_state_result state_waiting_for_network_connection_run(void *o);
static void state_network_connected_entry(void *o);
static enum smf_state_result state_network_connected_run(void *o);
static void state_coredump_send_attempt_entry(void *o);
static enum smf_state_result state_coredump_send_attempt_run(void *o);
static void state_coredump_send_backoff_entry(void *o);
static enum smf_state_result state_coredump_send_backoff_run(void *o);
static void state_finished_entry(void *o);
static void state_error_entry(void *o);

static const struct smf_state states[] = {
	[STATE_WAITING_FOR_NRF_MODEM_LIB_INIT] =
		SMF_CREATE_STATE(state_waiting_for_nrf_modem_lib_init_entry,
				 state_waiting_for_nrf_modem_lib_init_run,
				 NULL,
				 NULL,
				 NULL),
	[STATE_RUNNING] =
		SMF_CREATE_STATE(state_running_entry,
				 state_running_run,
				 state_running_exit,
				 NULL,
				 &states[STATE_WAITING_FOR_NETWORK_CONNECTION]),
	[STATE_WAITING_FOR_NETWORK_CONNECTION] =
		SMF_CREATE_STATE(NULL,
				 state_waiting_for_network_connection_run,
				 NULL,
				 &states[STATE_RUNNING],
				 NULL),
	[STATE_NETWORK_CONNECTED] =
		SMF_CREATE_STATE(state_network_connected_entry,
				 state_network_connected_run,
				 NULL,
				 &states[STATE_RUNNING],
#if defined(CONFIG_MEMFAULT_NCS_POST_COREDUMP_AFTER_INITIAL_DELAY)
				 &states[STATE_COREDUMP_SEND_BACKOFF]),
#else
				 &states[STATE_COREDUMP_SEND_ATTEMPT]),
#endif /* CONFIG_MEMFAULT_NCS_POST_COREDUMP_AFTER_INITIAL_DELAY */
	[STATE_COREDUMP_SEND_ATTEMPT] =
		SMF_CREATE_STATE(state_coredump_send_attempt_entry,
				 state_coredump_send_attempt_run,
				 NULL,
				 &states[STATE_NETWORK_CONNECTED],
				 NULL),
	[STATE_COREDUMP_SEND_BACKOFF] =
		SMF_CREATE_STATE(state_coredump_send_backoff_entry,
				 state_coredump_send_backoff_run,
				 NULL,
				 &states[STATE_NETWORK_CONNECTED],
				 NULL),
	[STATE_FINISHED] =
		SMF_CREATE_STATE(state_finished_entry,
				 NULL,
				 NULL,
				 NULL,
				 NULL),
	[STATE_ERROR] =
		SMF_CREATE_STATE(state_error_entry,
				 NULL,
				 NULL,
				 NULL,
				 NULL)
};

static void timer_work_fn(struct k_work *work)
{
	event_send(EVENT_BACKOFF_TIMER_EXPIRED);
}

static struct fsm_state_object state_object = { 0 };

/* nRF Modem Library Handlers */

static void on_modem_lib_init(int ret, void *ctx)
{
	if (ret != 0) {
		LOG_ERR("Modem library did not initialize: %d", ret);
		return;
	}

	event_send(EVENT_NRF_MODEM_LIB_INITED);
}

static void on_modem_lib_shutdown(void *ctx)
{
	event_send(EVENT_NRF_MODEM_LIB_SHUTDOWN);
}

/* Callback Handlers for Link Controller and PDN Libraries */

static void lte_handler(const struct lte_lc_evt *const evt)
{
	if (evt->type == LTE_LC_EVT_NW_REG_STATUS) {
		switch (evt->nw_reg_status) {
		case LTE_LC_NW_REG_REGISTERED_HOME:
			__fallthrough;
		case LTE_LC_NW_REG_REGISTERED_ROAMING:
			event_send(EVENT_LTE_REGISTERED);
			break;
		case LTE_LC_NW_REG_UNKNOWN:
			__fallthrough;
		case LTE_LC_NW_REG_UICC_FAIL:
			__fallthrough;
		case LTE_LC_NW_REG_NOT_REGISTERED:
			 __fallthrough;
		case LTE_LC_NW_REG_REGISTRATION_DENIED:
			event_send(EVENT_LTE_DEREGISTERED);
		default:
			/* Don't care */
			break;
		}
	}
}

static void pdn_event_handler(uint8_t cid, enum pdn_event event, int reason)
{
	switch (event) {
	case PDN_EVENT_ACTIVATED:
		event_send(EVENT_PDN_ACTIVATED);
		break;
	case PDN_EVENT_NETWORK_DETACH:
		__fallthrough;
	case PDN_EVENT_DEACTIVATED:
		event_send(EVENT_PDN_DEACTIVATED);
		break;
	default:
		/* Don't care */
		break;
	}
}

/* SMF State Handlers */

static void state_waiting_for_nrf_modem_lib_init_entry(void *o)
{
	struct fsm_state_object *state_object = o;

	LOG_DBG("state_waiting_for_nrf_modem_lib_init_entry");

	state_object->lte_registered = false;
	state_object->pdn_active = false;
	state_object->retry_count = 0;
}

static enum smf_state_result state_waiting_for_nrf_modem_lib_init_run(void *o)
{
	struct fsm_state_object *state_object = o;

	LOG_DBG("state_waiting_for_nrf_modem_lib_init_run");

	if (state_object->event == EVENT_NRF_MODEM_LIB_INITED) {
		smf_set_state(SMF_CTX(state_object), &states[STATE_RUNNING]);
		return SMF_EVENT_HANDLED;
	}

	return SMF_EVENT_PROPAGATE;
}

static void state_running_entry(void *o)
{
	int err;

	ARG_UNUSED(o);

	LOG_DBG("state_running_entry");

	/* Subscribe to +CEREG notifications, level 5 */
	err = nrf_modem_at_printf("AT+CEREG=5");
	if (err) {
		LOG_ERR("nrf_modem_at_printf, error: %d", err);
		event_send(EVENT_ERROR);
		return;
	}

	lte_lc_register_handler(lte_handler);

	err = pdn_default_ctx_cb_reg(pdn_event_handler);
	if (err) {
		LOG_ERR("pdn_default_ctx_cb_reg, error: %d", err);
		event_send(EVENT_ERROR);
		return;
	}
}

static enum smf_state_result state_running_run(void *o)
{
	struct fsm_state_object *state_object = o;

	LOG_DBG("state_running_run");

	if (state_object->event == EVENT_NRF_MODEM_LIB_SHUTDOWN) {
		smf_set_state(SMF_CTX(state_object), &states[STATE_WAITING_FOR_NRF_MODEM_LIB_INIT]);
		return SMF_EVENT_HANDLED;
	} else if (state_object->event == EVENT_ERROR) {
		smf_set_state(SMF_CTX(state_object), &states[STATE_ERROR]);
		return SMF_EVENT_HANDLED;
	}

	return SMF_EVENT_PROPAGATE;
}

static void state_running_exit(void *o)
{
	int err;

	struct fsm_state_object *state_object = o;

	LOG_DBG("state_running_exit");

	err = lte_lc_deregister_handler(lte_handler);
	if (err) {
		LOG_ERR("lte_lc_deregister_handler, error: %d", err);
		event_send(EVENT_ERROR);
		return;
	}

	err = pdn_default_ctx_cb_dereg(pdn_event_handler);
	if (err) {
		LOG_ERR("pdn_default_ctx_cb_dereg, error: %d", err);
		event_send(EVENT_ERROR);
		return;
	}

	(void)k_work_cancel_delayable(&state_object->backoff_work);
}

static enum smf_state_result state_waiting_for_network_connection_run(void *o)
{
	struct fsm_state_object *state_object = o;

	LOG_DBG("state_waiting_for_network_connection_run");

	switch (state_object->event) {
	case EVENT_LTE_REGISTERED:
		state_object->lte_registered = true;
		break;
	case EVENT_LTE_DEREGISTERED:
		state_object->lte_registered = false;
		break;
	case EVENT_PDN_ACTIVATED:
		state_object->pdn_active = true;
		break;
	case EVENT_PDN_DEACTIVATED:
		state_object->pdn_active = false;
		break;
	default:
		/* Don't care */
		break;
	}

	if (state_object->lte_registered && state_object->pdn_active) {
		smf_set_state(SMF_CTX(state_object), &states[STATE_NETWORK_CONNECTED]);
		return SMF_EVENT_HANDLED;
	}

	return SMF_EVENT_PROPAGATE;
}

static void state_network_connected_entry(void *o)
{
	struct fsm_state_object *state_object = o;

	LOG_DBG("state_network_connected_entry");

	state_object->retry_count = 0;
}

static enum smf_state_result state_network_connected_run(void *o)
{
	struct fsm_state_object *state_object = o;

	LOG_DBG("state_network_connected_run");

	if (state_object->event == EVENT_LTE_DEREGISTERED) {
		state_object->lte_registered = false;

		smf_set_state(SMF_CTX(state_object), &states[STATE_WAITING_FOR_NETWORK_CONNECTION]);
		return SMF_EVENT_HANDLED;
	} else if (state_object->event == EVENT_PDN_DEACTIVATED) {
		state_object->pdn_active = false;

		smf_set_state(SMF_CTX(state_object), &states[STATE_WAITING_FOR_NETWORK_CONNECTION]);
		return SMF_EVENT_HANDLED;
	}

	return SMF_EVENT_PROPAGATE;
}

static void state_coredump_send_attempt_entry(void *o)
{
	int err;

	ARG_UNUSED(o);

	LOG_DBG("state_coredump_send_attempt_entry");
	LOG_DBG("Triggering heartbeat");
	LOG_DBG("Attempting to send coredump");

	memfault_metrics_heartbeat_debug_trigger();

	if (IS_ENABLED(CONFIG_MEMFAULT_NCS_POST_MODEM_TRACE_ON_COREDUMP)) {
		err = memfault_lte_coredump_modem_trace_init();
		if (err == -EIO) {
			LOG_ERR("memfault_lte_coredump_modem_trace_init, error: %d", err);
			__ASSERT_NO_MSG(false);
			return;
		}

		err = memfault_lte_coredump_modem_trace_prepare_for_upload();
		if (err == -ENOTSUP) {
			__ASSERT_NO_MSG(false);
			return;
		}
	}

	err = memfault_zephyr_port_post_data();
	if (err) {
		LOG_DBG("Failed to post data to Memfault");
		event_send(EVENT_COREDUMP_SEND_FAIL);
	} else {
		LOG_DBG("Succeeded posting data to Memfault");
		event_send(EVENT_COREDUMP_SEND_SUCCESS);
	}
}

static enum smf_state_result state_coredump_send_attempt_run(void *o)
{
	struct fsm_state_object *state_object = o;

	LOG_DBG("state_coredump_send_attempt_run");

	if (state_object->event == EVENT_COREDUMP_SEND_FAIL) {
		smf_set_state(SMF_CTX(state_object), &states[STATE_COREDUMP_SEND_BACKOFF]);
		return SMF_EVENT_HANDLED;
	} else if (state_object->event == EVENT_COREDUMP_SEND_SUCCESS) {
		smf_set_state(SMF_CTX(state_object), &states[STATE_FINISHED]);
		return SMF_EVENT_HANDLED;
	}

	return SMF_EVENT_PROPAGATE;
}

static void state_coredump_send_backoff_entry(void *o)
{
	struct fsm_state_object *state_object = o;

	LOG_DBG("state_coredump_send_backoff_entry");

	schedule_send_backoff(state_object);
}

static enum smf_state_result state_coredump_send_backoff_run(void *o)
{
	struct fsm_state_object *state_object = o;

	LOG_DBG("state_coredump_send_backoff_run");

	if (state_object->event == EVENT_BACKOFF_TIMER_EXPIRED) {
		smf_set_state(SMF_CTX(state_object), &states[STATE_COREDUMP_SEND_ATTEMPT]);
		return SMF_EVENT_HANDLED;
	}

	return SMF_EVENT_PROPAGATE;
}

static void state_finished_entry(void *o)
{
	ARG_UNUSED(o);

	LOG_DBG("state_finished_entry");
}

static void state_error_entry(void *o)
{
	ARG_UNUSED(o);

	LOG_DBG("state_error_entry");

	__ASSERT_NO_MSG(false);
}

/* Local Convenience Functions */

static void event_send(enum library_event event)
{
	int ret;
	enum library_event event_new = event;

	ret = k_msgq_put(&mflt_lte_coredump_queue, &event_new, K_NO_WAIT);
	if (ret) {
		LOG_ERR("k_msgq_put, error: %d", ret);
		__ASSERT_NO_MSG(false);
		return;
	}
}

static void schedule_send_backoff(struct fsm_state_object *state_object)
{
	state_object->retry_count++;

	if (state_object->retry_count >= CONFIG_MEMFAULT_NCS_POST_COREDUMP_RETRIES_MAX) {
		event_send(STATE_FINISHED);
		return;
	}

	LOG_DBG("Retry number: %d", state_object->retry_count);
	LOG_DBG("Scheduling backoff, next attempt in %d seconds",
		CONFIG_MEMFAULT_NCS_POST_COREDUMP_RETRY_INTERVAL_SECONDS);

	(void)k_work_schedule(&state_object->backoff_work,
			      K_SECONDS(CONFIG_MEMFAULT_NCS_POST_COREDUMP_RETRY_INTERVAL_SECONDS));
}

/* Library thread */

static void mflt_lte_coredump_fn(void)
{
	int err;

	k_work_init_delayable(&state_object.backoff_work, timer_work_fn);

	bool has_coredump = memfault_coredump_has_valid_coredump(NULL);

	if (has_coredump) {
		LOG_DBG("Coredump found");
		smf_set_initial(SMF_CTX(&state_object),
				&states[STATE_WAITING_FOR_NRF_MODEM_LIB_INIT]);
	} else {
		LOG_DBG("No coredump found");
		smf_set_initial(SMF_CTX(&state_object),
				&states[STATE_FINISHED]);

		return;
	}

	while (1) {
		err = k_msgq_get(&mflt_lte_coredump_queue, &state_object.event, K_FOREVER);
		if (err) {
			LOG_ERR("k_msgq_get, error: %d", err);
			__ASSERT_NO_MSG(false);
			return;
		}

		err = smf_run_state(SMF_CTX(&state_object));
		if (err) {
			LOG_ERR("smf_run_state, error: %d", err);
			__ASSERT_NO_MSG(false);
			return;
		}
	}
}

K_THREAD_DEFINE(mflt_lte_coredump_thread_id, 4096,
		mflt_lte_coredump_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
