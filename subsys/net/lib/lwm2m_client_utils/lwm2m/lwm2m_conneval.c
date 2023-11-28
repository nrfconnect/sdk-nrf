/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define LOG_MODULE_NAME net_lwm2m_conneval
#define LOG_LEVEL CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <modem/lte_lc.h>
#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>
#include "lwm2m_engine.h"
#include "nrf_modem_at.h"
#include "nrf_errno.h"

#define MIN_POLL_PERIOD_MS 500
#define AT_CONEVAL_READ "AT%%CONEVAL"
#define AT_CONEVAL_PARAMS_MAX 19
#define AT_CONEVAL_RESULT_INDEX 1
#define AT_CONEVAL_ENERGY_ESTIMATE_INDEX 3

static struct k_work conneval_work;
static struct k_work conneval_estimate_work;
static struct k_work_delayable conneval_work_delayable;

static struct lte_lc_conn_eval_params params;
static enum lte_lc_energy_estimate energy_estimate;
static uint64_t maximum_delay; /* [s] */
static uint64_t poll_period; /* [ms] */
static bool initialized;
static uint64_t pause_start_time; /* [ms] */
static bool engine_paused;
static struct at_param_list at_list;

static void pause_engine_work(struct k_work *work)
{
	lwm2m_engine_pause();
	engine_paused = true;
}

static int read_energy_estimate(uint16_t *energy_estimate)
{
	int ret;
	int conneval_result;

	ret = at_params_int_get(&at_list, AT_CONEVAL_RESULT_INDEX, &conneval_result);
	if (ret) {
		LOG_ERR("AT params error : %d", ret);
		return ret;
	}

	/* Check that coneval estimation is valid */
	if (conneval_result == 0) {
		ret = at_params_unsigned_short_get(&at_list, AT_CONEVAL_ENERGY_ESTIMATE_INDEX,
						   energy_estimate);
		if (ret) {
			LOG_ERR("AT params error : %d", ret);
			return ret;
		}
		LOG_INF("Energy estimate: %hu", *energy_estimate);
	}

	return 0;
}

static void conneval_cb(const char *resp)
{
	int ret;

	LOG_DBG("Conneval response: %s", resp);

	ret = at_parser_max_params_from_str(resp, NULL, &at_list, AT_CONEVAL_PARAMS_MAX);
	if (ret) {
		LOG_ERR("AT parser error : %d", ret);
		return;
	}
	ret = k_work_submit(&conneval_estimate_work);
	if (ret < 0) {
		LOG_ERR("Work item was not submitted to system queue, error %d", ret);
	}
}

static void estimate_work(struct k_work *work)
{
	int ret;
	uint16_t estimate = 0;

	ret = read_energy_estimate(&estimate);
	if (ret) {
		LOG_WRN("Read energy estimate error : %d", ret);
	}

	if ((estimate >= energy_estimate) ||
	    ((k_uptime_get() - pause_start_time) / 1000 >= maximum_delay)) {
		lwm2m_engine_resume();
		engine_paused = false;
	}
}

static void conneval_update_work(struct k_work *work)
{
	int ret;
	uint16_t estimate = 0;

	if (!engine_paused) {
		return;
	}

	if ((estimate >= energy_estimate) ||
	    ((k_uptime_get() - pause_start_time) / 1000 >= maximum_delay)) {
		lwm2m_engine_resume();
		engine_paused = false;
		return;
	}

	ret = nrf_modem_at_cmd_async(conneval_cb, AT_CONEVAL_READ);
	if (ret < 0 && ret != -NRF_EINPROGRESS) {
		LOG_ERR("AT cmd async error : %d", ret);
	}

	ret = k_work_reschedule(&conneval_work_delayable, K_MSEC(poll_period));
	if (ret < 0) {
		LOG_ERR("Work item was not submitted to system queue, error %d", ret);
		lwm2m_engine_resume();
		engine_paused = false;
	}
}

int lwm2m_utils_enable_conneval(enum lte_lc_energy_estimate min_energy_estimate,
				uint64_t maximum_delay_s, uint64_t poll_period_ms)
{
	int ret;

	LOG_INF("Min energy estimate %d", min_energy_estimate);
	LOG_INF("Max delay %" PRIu64 " s", maximum_delay_s);
	LOG_INF("Poll period %" PRIu64 " ms", poll_period_ms);

	if (poll_period_ms < MIN_POLL_PERIOD_MS) {
		LOG_ERR("Minimum poll period is %d ms", MIN_POLL_PERIOD_MS);
		return -EINVAL;
	}

	if (!initialized) {
		k_work_init_delayable(&conneval_work_delayable, conneval_update_work);
		k_work_init(&conneval_work, pause_engine_work);
		k_work_init(&conneval_estimate_work, estimate_work);
		ret = at_params_list_init(&at_list, AT_CONEVAL_PARAMS_MAX);
		if (ret < 0) {
			LOG_ERR("Failed to initialize parameter list: %d", ret);
			return ret;
		}
		initialized = true;
	}

	energy_estimate = min_energy_estimate;
	maximum_delay = maximum_delay_s;
	poll_period = poll_period_ms;

	return 0;
}

void lwm2m_utils_disable_conneval(void)
{
	maximum_delay = 0;
}

int lwm2m_utils_conneval(struct lwm2m_ctx *client, enum lwm2m_rd_client_event *client_event)
{
	int ret;

	if (!client || !client_event) {
		return -EINVAL;
	}

	if (maximum_delay && *client_event == LWM2M_RD_CLIENT_EVENT_REG_UPDATE) {
		ret = lte_lc_conn_eval_params_get(&params);
		if (ret < 0) {
			LOG_ERR("Get connection evaluation parameters failed, error: %d", ret);
			return ret;
		}

		LOG_INF("Energy estimate %d", params.energy_estimate);
		if (params.energy_estimate < energy_estimate) {
			ret = k_work_submit(&conneval_work);
			if (ret < 0) {
				LOG_ERR("Work item was not submitted to system queue, error %d",
					ret);
				return ret;
			}
			pause_start_time = k_uptime_get();
			ret = k_work_reschedule(&conneval_work_delayable, K_MSEC(poll_period));
			if (ret < 0) {
				LOG_ERR("Work item was not submitted to system queue, error %d",
					ret);
				return ret;
			}
			*client_event = LWM2M_RD_CLIENT_EVENT_NONE;
		}
	}

	return 0;
}
