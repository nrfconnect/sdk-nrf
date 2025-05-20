/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *   This file implements the Software Coexistence interface.
 *
 */

#include <mpsl/mpsl_cx_software.h>

#include <mpsl_cx_abstract_interface.h>

#include <zephyr/init.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/toolchain.h>
#include <zephyr/kernel.h>

#define DEFAULT_OPS (MPSL_CX_OP_IDLE_LISTEN | MPSL_CX_OP_RX | MPSL_CX_OP_TX)

static mpsl_cx_cb_t callback;
static atomic_t granted_ops = DEFAULT_OPS;
static void granted_ops_reset(struct k_timer *timer);
static K_TIMER_DEFINE(granted_ops_reset_timer, granted_ops_reset, NULL);

static int32_t request(const mpsl_cx_request_t *req_params)
{
	ARG_UNUSED(req_params);

	return 0;
}

static int32_t release(void)
{
	return 0;
}

static uint32_t req_grant_delay_get(void)
{
	return 0u;
}

static int32_t granted_ops_get(mpsl_cx_op_map_t *ops)
{
	*ops = (mpsl_cx_op_map_t)atomic_get(&granted_ops);

	return 0;
}

static int32_t register_callback(mpsl_cx_cb_t cb)
{
	callback = cb;

	return 0;
}

static const mpsl_cx_interface_t m_mpsl_cx_methods = {
	.p_request = request,
	.p_release = release,
	.p_granted_ops_get = granted_ops_get,
	.p_req_grant_delay_get = req_grant_delay_get,
	.p_register_callback = register_callback,
};

static int mpsl_cx_init(void)
{
	int32_t ret;

	ret = mpsl_cx_interface_set(&m_mpsl_cx_methods);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

SYS_INIT(mpsl_cx_init, POST_KERNEL, CONFIG_MPSL_CX_INIT_PRIORITY);

int mpsl_cx_software_set_granted_ops(mpsl_cx_op_map_t ops, k_timeout_t timeout)
{
	mpsl_cx_op_map_t previous_ops;
	mpsl_cx_cb_t change_callback = callback;

	k_timer_stop(&granted_ops_reset_timer);

	previous_ops = (mpsl_cx_op_map_t)atomic_set(&granted_ops, ops);

	if (change_callback != NULL && ops != previous_ops) {
		change_callback(ops);
	}

	k_timer_start(&granted_ops_reset_timer, timeout, K_NO_WAIT);

	return 0;
}

static void granted_ops_reset(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	mpsl_cx_software_set_granted_ops(DEFAULT_OPS, K_FOREVER);
}
