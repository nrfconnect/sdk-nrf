/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <nrf_rpc/nrf_rpc_ipc.h>
#include <nrf_rpc_cbor.h>
#include <mpsl/flash_sync_rpc_host.h>

#include "soc_flash_nrf.h"

#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_sync_rpc_host);

#define FLASH_SYNC_EXE_MAX_ENCODED_LENGTH (1 + sizeof(uint32_t))
#define FLASH_SYNC_EXE_RPC_COMMAND 0

#define FLASH_SYNC_EXE_MUTEX_TIMEOUT_MS 1000

/* In perfect condition the time was approximately 70us */
#define TIMESLOT_STARTED_RPC_LATENCY_MARGIN_US 250

#if DT_HAS_CHOSEN(zephyr_bt_hci_ipc)
#define IPC_DEVICE DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_hci_ipc))
#else
#define IPC_DEVICE DEVICE_DT_GET(DT_NODELABEL(ipc0))
#endif

static void rpc_bound_handler(const struct nrf_rpc_group *group);

NRF_RPC_IPC_TRANSPORT(flash_sync_rpc_api_tr, IPC_DEVICE, "flash_sync_rpc_ept");
NRF_RPC_GROUP_DEFINE_NOWAIT(flash_sync_rpc_api_grp, "flash_sync_rpc_api", &flash_sync_rpc_api_tr,
			    NULL, NULL, NULL, rpc_bound_handler, true);

struct flash_sync_rpc_context {
	/* Whether the module was initialized. */
	bool initialized;
	/* Whether the synchronization is enabled. */
	bool sync_enabled;
	/* Whether the RPC endpoint was bound (RPC init successful) */
	bool bound;
	/* Only one rpc host flash_sync_exe operation is allowed at a time. */
	struct k_mutex lock;
	/* The flash operation may be split into multiple requests.
	 * This represents the length of such a request.
	 */
	uint32_t request_length_us;
	/* Time since the MPSL timeslot on the remote core has started. */
	uint32_t timeslot_started_time;
	/* Argument passed to nrf_flash_sync_exe(). */
	struct flash_op_desc *op_desc;
	int status; /* Return value for nrf_flash_sync_exe(). */
};

static struct flash_sync_rpc_context _context = {
	.initialized = false,
	.bound = false,
#if CONFIG_BT
	.sync_enabled = true,
#else
	.sync_enabled = false,
#endif
};

static void rpc_bound_handler(const struct nrf_rpc_group *group)
{
	ARG_UNUSED(group);

	LOG_DBG("nrf_flash_sync_rpc RPC endpoint bound");
	_context.bound = true;
}

static void high_prio_handler(void)
{
	uint32_t now = k_cycle_get_32();
	uint32_t elapsed = k_cyc_to_us_floor64(now - _context.timeslot_started_time);
	int rc;

	/* There is no way to make the time since the timeslot start constant, as there
	 * might have been interrupts on both cores while the communication was ongoing.
	 * Also, it is not possible to influence the RPC thread priority from the host side.
	 * Thus, the best we can do is to check that the time since the timeslot start does
	 * not exceed the margin and fail if it does.
	 */
	if (elapsed > TIMESLOT_STARTED_RPC_LATENCY_MARGIN_US) {
		LOG_ERR("Time since timeslot start exceeds the maximum time! Time ellapsed %u",
			elapsed);
		_context.status = -ETIME;
		return;
	}

	rc = _context.op_desc->handler(_context.op_desc->context);
	_context.status = (rc == FLASH_OP_DONE) ? 0 : rc;
}

static void call_handler_from_high_prio(void)
{
	/* In the general case this should trigger a high priority interrupt
	 * which would call the high_prio_handler making this resemble what is
	 * happening on the radio core, where the handler is called from a high
	 * priority interrupt by MPSL. Alternatively, it is possible to simply
	 * disable interrupts and call the handler directly (probably a worse solution).

	 * However, for the current only use case (Flash IPUC) + PoC calling the handler
	 * from a thread with the highest priority is sufficient.
	 * Additionally, for this case it could be difficult to achieve the goal
	 * using an interrupt handler, as the Flash IPUC implementation needs
	 * to perform calls via IPC.
	 */
	high_prio_handler();
}

int nrf_flash_sync_init(void)
{
	if (_context.initialized) {
		return 0;
	}

	k_mutex_init(&_context.lock);
	_context.initialized = true;

	return 0;
}

void nrf_flash_sync_set_context(uint32_t duration)
{
	/* Do not forward this immediately via RPC,
	 * As this is always used together with nrf_flash_sync_exe.
	 * Thus, forwarding this separately would just be a waste of time
	 */
	_context.request_length_us = duration;
}

bool nrf_flash_sync_is_required(void)
{
	return _context.sync_enabled && _context.bound;
}

void nrf_flash_sync_get_timestamp_begin(void)
{
	/* Not needed for this driver. */
}

int nrf_flash_sync_exe(struct flash_op_desc *op_desc)
{
	struct nrf_rpc_cbor_ctx ctx;
	int32_t result = 0;

	_context.timeslot_started_time = 0;
	_context.op_desc = op_desc;
	_context.status = -ETIMEDOUT;

	if (!_context.bound) {
		return -ENXIO;
	}

	if (k_mutex_lock(&_context.lock, K_MSEC(FLASH_SYNC_EXE_MUTEX_TIMEOUT_MS)) != 0) {
		return -EBUSY;
	}

	_context.timeslot_started_time = 0;
	_context.op_desc = op_desc;
	_context.status = -ETIMEDOUT;

	NRF_RPC_CBOR_ALLOC(&flash_sync_rpc_api_grp, ctx, FLASH_SYNC_EXE_MAX_ENCODED_LENGTH);

	/* _context.request_length_us specifies the required timeslot duration. The timeslot
	 * is extended to account for the time needed for RPC communication to signal
	 * the start of the timeslot.
	 */
	zcbor_uint32_put(ctx.zs,
			 _context.request_length_us + TIMESLOT_STARTED_RPC_LATENCY_MARGIN_US);

	/* Elevate the thread priority, so that the handler is processed as quick as
	 * possible after MPSL timeslot is granted.
	 */
	k_tid_t current_thread = k_current_get();
	int orig_thread_prio = k_thread_priority_get(current_thread);

	k_thread_priority_set(current_thread, K_HIGHEST_THREAD_PRIO);

	do {
		LOG_DBG("Sending flash sync exe RPC command");
		result = nrf_rpc_cbor_cmd_rsp(&flash_sync_rpc_api_grp, FLASH_SYNC_EXE_RPC_COMMAND,
					      &ctx);

		if (result != 0) {
			LOG_ERR("RPC communication failure!: %d", result);
		}

		if (result == 0) {
			if (!zcbor_int32_decode(ctx.zs, &result)) {
				result = -EINVAL;
			} else if (result != 0) {
				LOG_ERR("Failed to allocate timeslot: %d", result);
			}
		}

		if (result == 0) {
			if (!zcbor_uint32_decode(ctx.zs, &_context.timeslot_started_time)) {
				result = -EINVAL;
			}
		}

		nrf_rpc_cbor_decoding_done(&flash_sync_rpc_api_grp, &ctx);

		if (result == 0) {
			call_handler_from_high_prio();
			result = _context.status;
		}
	} while (result == FLASH_OP_ONGOING);

	k_thread_priority_set(current_thread, orig_thread_prio);

	k_mutex_unlock(&_context.lock);

	return result;
}

bool nrf_flash_sync_check_time_limit(uint32_t iteration)
{
	(void)iteration;

	/* Not used by this driver */
	return true;
}

void flash_sync_rpc_host_sync_enable(bool enable)
{
	_context.sync_enabled = enable;
}
