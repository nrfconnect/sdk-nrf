/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/net/socket_ncs.h>

#include <net/lwm2m_client_utils.h>
#include "stubs.h"

#define LIFETIME 120
#define TAU 60

static void setup(void *data)
{
	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);

	/* reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

static int lte_lc_psm_disabled(int *tau, int *active_time)
{
	*tau = TAU;
	*active_time = -1;

	return 0;
}

static int lte_lc_psm_ok(int *tau, int *active_time)
{
	*tau = TAU;
	*active_time = 30;

	return 0;
}

static int get_lifetime(const struct lwm2m_obj_path *path, uint32_t *value)
{
	*value = LIFETIME;

	return 0;
}

static int get_lifetime_and_tau_match(const struct lwm2m_obj_path *path, uint32_t *value)
{
	*value = TAU;

	return 0;
}

static int setsockopt_save(int sock, int level, int optname, const void *optval, socklen_t optlen)
{
	zassert_equal(optname, TLS_DTLS_CONN_SAVE, "Incorrect socket option");

	return 0;
}

static int setsockopt_load(int sock, int level, int optname, const void *optval, socklen_t optlen)
{
	zassert_equal(optname, TLS_DTLS_CONN_LOAD, "Incorrect socket option");

	return 0;
}

ZTEST_SUITE(lwm2m_client_utils_con_management, NULL, NULL, setup, NULL, NULL);

ZTEST(lwm2m_client_utils_con_management, test_dtls_save_load)
{
	struct lwm2m_ctx ctx;
	enum lwm2m_rd_client_event event = LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF;

	z_impl_zsock_setsockopt_fake.custom_fake = setsockopt_save;
	lwm2m_utils_connection_manage(&ctx, &event);

	event = LWM2M_RD_CLIENT_EVENT_REG_UPDATE;
	z_impl_zsock_setsockopt_fake.custom_fake = setsockopt_load;
	lwm2m_utils_connection_manage(&ctx, &event);

	event = LWM2M_RD_CLIENT_EVENT_DISCONNECT;
	lwm2m_utils_connection_manage(&ctx, &event);
}

ZTEST(lwm2m_client_utils_con_management, test_psm_disabled)
{
	struct lwm2m_ctx ctx;
	enum lwm2m_rd_client_event event = LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE;

	lte_lc_psm_get_fake.custom_fake = lte_lc_psm_disabled;
	lwm2m_utils_connection_manage(&ctx, &event);

	zassert_equal(lwm2m_get_u32_fake.call_count, 0,
		      "Incorrect number of lwm2m_get_u32() calls");
}

ZTEST(lwm2m_client_utils_con_management, test_request_tau)
{
	struct lwm2m_ctx ctx;
	enum lwm2m_rd_client_event event = LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE;

	lte_lc_psm_get_fake.custom_fake = lte_lc_psm_ok;
	lwm2m_get_u32_fake.custom_fake = get_lifetime;
	lwm2m_utils_connection_manage(&ctx, &event);

	zassert_equal(lte_lc_psm_param_set_seconds_fake.call_count, 1,
		      "Incorrect number of lte_lc_psm_param_set_seconds() calls");
	zassert_equal(lte_lc_psm_param_set_seconds_fake.arg0_val, LIFETIME, "Incorrect lifetime");
}

ZTEST(lwm2m_client_utils_con_management, test_tau_and_lifetime_match)
{
	struct lwm2m_ctx ctx;
	enum lwm2m_rd_client_event event = LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE;

	lte_lc_psm_get_fake.custom_fake = lte_lc_psm_ok;
	lwm2m_get_u32_fake.custom_fake = get_lifetime_and_tau_match;
	lwm2m_utils_connection_manage(&ctx, &event);

	zassert_equal(lte_lc_psm_param_set_seconds_fake.call_count, 0,
		      "Incorrect number of lte_lc_psm_param_set_seconds() calls");
}
