/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(conneval_test);

#include <net/lwm2m_client_utils.h>
#include "stubs.h"

static int counter_int;
static int counter_short;

static int resp_cb(nrf_modem_at_resp_handler_t cb, const char *fmt, char *p)
{
	cb("my_resp");

	return 0;
}

static void setup(void *data)
{
	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);

	/* reset common FFF internal structures */
	FFF_RESET_HISTORY();

	counter_int = 0;
	counter_short = 0;
	nrf_modem_at_cmd_async_fake.custom_fake = resp_cb;
}

static int energy_normal(struct lte_lc_conn_eval_params *params)
{
	params->energy_estimate = LTE_LC_ENERGY_CONSUMPTION_NORMAL;

	return 0;
}

static int energy_increased(struct lte_lc_conn_eval_params *params)
{
	params->energy_estimate = LTE_LC_ENERGY_CONSUMPTION_INCREASED;

	return 0;
}

static int at_params_int_get_custom_fake_error(const struct at_param_list *list, size_t index,
					       int32_t *value)
{
	if (counter_int <= 1) {
		*value = 1;
	} else {
		*value = 0;
	}
	counter_int++;

	return 0;
}

static int at_params_int_get_custom_fake_ok(const struct at_param_list *list, size_t index,
					    int32_t *value)
{
	*value = 0;

	return 0;
}

int at_params_unsigned_short_get_changing_energy(const struct at_param_list *list, size_t index,
						 uint16_t *value)
{
	if (counter_short > 2) {
		*value = LTE_LC_ENERGY_CONSUMPTION_NORMAL;
	} else {
		*value = LTE_LC_ENERGY_CONSUMPTION_INCREASED;
	}
	counter_short++;

	return 0;
}

int at_params_unsigned_short_get_normal_energy(const struct at_param_list *list, size_t index,
					       uint16_t *value)
{
	*value = LTE_LC_ENERGY_CONSUMPTION_NORMAL;

	return 0;
}

int at_params_unsigned_short_get_increased_energy(const struct at_param_list *list, size_t index,
						  uint16_t *value)
{
	*value = LTE_LC_ENERGY_CONSUMPTION_INCREASED;

	return 0;
}

ZTEST_SUITE(lwm2m_client_utils_conneval, NULL, NULL, setup, NULL, NULL);


ZTEST(lwm2m_client_utils_conneval, test_enable)
{
	int rc;

	rc = lwm2m_utils_enable_conneval(LTE_LC_ENERGY_CONSUMPTION_NORMAL, 60, 50);
	zassert_equal(rc, -EINVAL, "Wrong return value");

	rc = lwm2m_utils_enable_conneval(LTE_LC_ENERGY_CONSUMPTION_NORMAL, 60, 5000);
	zassert_equal(rc, 0, "Wrong return value");
}

ZTEST(lwm2m_client_utils_conneval, test_disable)
{
	int rc;
	struct lwm2m_ctx ctx;
	enum lwm2m_rd_client_event client_event;

	rc = lwm2m_utils_enable_conneval(LTE_LC_ENERGY_CONSUMPTION_NORMAL, 60, 5000);
	zassert_equal(rc, 0, "Wrong return value");

	lwm2m_utils_disable_conneval();

	client_event = LWM2M_RD_CLIENT_EVENT_REG_UPDATE;
	rc = lwm2m_utils_conneval(&ctx, &client_event);
	zassert_equal(rc, 0, "Wrong return value");

	zassert_equal(lte_lc_conn_eval_params_get_fake.call_count, 0,
		      "Incorrect number of lte_lc_conn_eval_params_get() calls");
}

ZTEST(lwm2m_client_utils_conneval, test_normal)
{
	int rc;
	struct lwm2m_ctx ctx;
	enum lwm2m_rd_client_event client_event;

	rc = lwm2m_utils_enable_conneval(LTE_LC_ENERGY_CONSUMPTION_NORMAL, 60, 5000);
	zassert_equal(rc, 0, "Wrong return value");

	lte_lc_conn_eval_params_get_fake.custom_fake = energy_normal;
	client_event = LWM2M_RD_CLIENT_EVENT_REG_UPDATE;

	rc = lwm2m_utils_conneval(&ctx, &client_event);
	zassert_equal(rc, 0, "Wrong return value");
	zassert_equal(lte_lc_conn_eval_params_get_fake.call_count, 1,
		      "Incorrect number of lte_lc_conn_eval_params_get() calls");
}

ZTEST(lwm2m_client_utils_conneval, test_max_delay)
{
	int rc;
	struct lwm2m_ctx ctx;
	enum lwm2m_rd_client_event client_event;

	rc = lwm2m_utils_enable_conneval(LTE_LC_ENERGY_CONSUMPTION_NORMAL, 20, 2000);
	zassert_equal(rc, 0, "Wrong return value");

	lte_lc_conn_eval_params_get_fake.custom_fake = energy_increased;
	at_params_int_get_fake.custom_fake = at_params_int_get_custom_fake_ok;
	at_params_unsigned_short_get_fake.custom_fake =
		at_params_unsigned_short_get_increased_energy;
	client_event = LWM2M_RD_CLIENT_EVENT_REG_UPDATE;

	rc = lwm2m_utils_conneval(&ctx, &client_event);
	zassert_equal(rc, 0, "Wrong return value");
	k_sleep(K_SECONDS(21)); /* Wait for timer */
	zassert_equal(lwm2m_engine_pause_fake.call_count, 1,
		      "Incorrect number of lte_lc_conn_eval_params_get() calls");
	zassert_equal(lte_lc_conn_eval_params_get_fake.call_count, 1,
		      "Incorrect number of lte_lc_conn_eval_params_get() calls");
	zassert_equal(lwm2m_engine_resume_fake.call_count, 1,
		      "Incorrect number of lte_lc_conn_eval_params_get() calls");
	zassert_equal(nrf_modem_at_cmd_async_fake.call_count, 9,
		      "Incorrect number of nrf_modem_at_cmd_async() calls");

	rc = lwm2m_utils_conneval(&ctx, &client_event);
	zassert_equal(rc, 0, "Wrong return value");
}

ZTEST(lwm2m_client_utils_conneval, test_energy_change)
{
	int rc;
	struct lwm2m_ctx ctx;
	enum lwm2m_rd_client_event client_event;

	rc = lwm2m_utils_enable_conneval(LTE_LC_ENERGY_CONSUMPTION_NORMAL, 20, 2000);
	zassert_equal(rc, 0, "Wrong return value");

	lte_lc_conn_eval_params_get_fake.custom_fake = energy_increased;
	at_params_int_get_fake.custom_fake = at_params_int_get_custom_fake_ok;
	at_params_unsigned_short_get_fake.custom_fake =
		at_params_unsigned_short_get_changing_energy;
	client_event = LWM2M_RD_CLIENT_EVENT_REG_UPDATE;

	rc = lwm2m_utils_conneval(&ctx, &client_event);
	zassert_equal(rc, 0, "Wrong return value");

	k_sleep(K_SECONDS(21)); /* Wait for timer */
	zassert_equal(lwm2m_engine_pause_fake.call_count, 1,
		      "Incorrect number of lte_lc_conn_eval_params_get() calls");
	zassert_equal(lte_lc_conn_eval_params_get_fake.call_count, 1,
		      "Incorrect number of lte_lc_conn_eval_params_get() calls");
	zassert_equal(lwm2m_engine_resume_fake.call_count, 1,
		      "Incorrect number of lte_lc_conn_eval_params_get() calls");
	zassert_equal(nrf_modem_at_cmd_async_fake.call_count, 4,
		      "Incorrect number of nrf_modem_at_cmd_async() calls");

	rc = lwm2m_utils_conneval(&ctx, &client_event);
	zassert_equal(rc, 0, "Wrong return value");
}

ZTEST(lwm2m_client_utils_conneval, test_evaluation_failed)
{
	int rc;
	struct lwm2m_ctx ctx;
	enum lwm2m_rd_client_event client_event;

	rc = lwm2m_utils_enable_conneval(LTE_LC_ENERGY_CONSUMPTION_NORMAL, 20, 2000);
	zassert_equal(rc, 0, "Wrong return value");

	lte_lc_conn_eval_params_get_fake.custom_fake = energy_increased;
	at_params_int_get_fake.custom_fake = at_params_int_get_custom_fake_error;
	at_params_unsigned_short_get_fake.custom_fake = at_params_unsigned_short_get_normal_energy;
	client_event = LWM2M_RD_CLIENT_EVENT_REG_UPDATE;

	rc = lwm2m_utils_conneval(&ctx, &client_event);
	zassert_equal(rc, 0, "Wrong return value");

	k_sleep(K_SECONDS(21)); /* Wait for timer */
	zassert_equal(lwm2m_engine_pause_fake.call_count, 1,
		      "Incorrect number of lte_lc_conn_eval_params_get() calls");
	zassert_equal(lte_lc_conn_eval_params_get_fake.call_count, 1,
		      "Incorrect number of lte_lc_conn_eval_params_get() calls");
	zassert_equal(lwm2m_engine_resume_fake.call_count, 1,
		      "Incorrect number of lte_lc_conn_eval_params_get() calls");
	zassert_equal(nrf_modem_at_cmd_async_fake.call_count, 3,
		      "Incorrect number of nrf_modem_at_cmd_async() calls");
	zassert_equal(at_params_unsigned_short_get_fake.call_count, 1,
		      "Incorrect number of at_params_unsigned_short_get() calls");
}

ZTEST(lwm2m_client_utils_conneval, test_incorrect_event)
{
	int rc;
	struct lwm2m_ctx ctx;
	enum lwm2m_rd_client_event client_event;

	rc = lwm2m_utils_enable_conneval(LTE_LC_ENERGY_CONSUMPTION_NORMAL, 60, 5000);
	zassert_equal(rc, 0, "Wrong return value");

	lte_lc_conn_eval_params_get_fake.custom_fake = energy_normal;
	client_event = LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE;

	rc = lwm2m_utils_conneval(&ctx, &client_event);
	zassert_equal(rc, 0, "Wrong return value");
	zassert_equal(lte_lc_conn_eval_params_get_fake.call_count, 0,
		      "Incorrect number of lte_lc_conn_eval_params_get() calls");
}

ZTEST(lwm2m_client_utils_conneval, test_conn_eval_params_get_fail)
{
	int rc;
	struct lwm2m_ctx ctx;
	enum lwm2m_rd_client_event client_event;

	rc = lwm2m_utils_enable_conneval(LTE_LC_ENERGY_CONSUMPTION_NORMAL, 60, 5000);
	zassert_equal(rc, 0, "Wrong return value");

	lte_lc_conn_eval_params_get_fake.return_val = -1;
	client_event = LWM2M_RD_CLIENT_EVENT_REG_UPDATE;

	rc = lwm2m_utils_conneval(&ctx, &client_event);
	zassert_equal(rc, -1, "Wrong return value");
}
