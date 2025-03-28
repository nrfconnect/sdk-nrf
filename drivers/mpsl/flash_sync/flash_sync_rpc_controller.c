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

#include "soc_flash_nrf.h"

#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_sync_rpc_remote);

#if DT_HAS_CHOSEN(zephyr_bt_hci_ipc)
#define IPC_DEVICE DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_hci_ipc))
#else
#define IPC_DEVICE DEVICE_DT_GET(DT_NODELABEL(ipc0))
#endif

NRF_RPC_IPC_TRANSPORT(flash_sync_rpc_api_tr, IPC_DEVICE, "flash_sync_rpc_ept");
NRF_RPC_GROUP_DEFINE(flash_sync_rpc_api_grp, "flash_sync_rpc_api", &flash_sync_rpc_api_tr,
		     NULL, NULL, NULL);

#define _SWI_IRQN(num) SWI##num##_IRQn
#define SWI_IRQN(num) _SWI_IRQN(num)

#define SWI_NUM CONFIG_SOC_FLASH_NRF_RADIO_SYNC_RPC_CONTROLLER_SWI_NUM
#define TIMESLOT_STARTED_OFFLOAD_SWI_IRQN SWI_IRQN(SWI_NUM)
#define TIMESLOT_STARTED_OFFLOAD_SWI_IRQ_PRIO  1
#define TIMESLOT_STARTED_OFFLOAD_SWI_IRQ_FLAGS 0

#define FLASH_SYNC_EXE_RPC_COMMAND 0

#define MAX_FLASH_SYNC_EXE_RET_ENCODED_LENGTH (1 + sizeof(int32_t) +  1 + sizeof(uint32_t))

#define FLASH_OP_ONGOING_NONBLOCKING 0x266641F

struct flash_sync_rpc_controller_context {
	bool initialized;
	struct flash_op_desc op_desc;
	uint32_t timeslot_start_time;
	struct k_work flash_sync_exe_work;
	struct k_sem timeslot_started_sem;
	struct k_sem session_active_sem;
	atomic_t operation_success;
	int flash_sync_exe_ret;
};

static struct flash_sync_rpc_controller_context _context = {
	.initialized = false,
	.timeslot_start_time = 0,
};

static int timeslot_started_handler(void *context)
{
	(void) context;
	_context.timeslot_start_time = k_cycle_get_32();
	atomic_set(&_context.operation_success, 1);

	/* Zephyr primitives cannot be used directly from the MPSL rtc0
	 * interrupt context, which is where we are currently executing.
	 * Specifically, giving semaphores from this interrupt is prohibited,
	 * as it could prevent the unblocked thread from being executed.
	 *
	 * To address this, offload the semaphore operation to a software-triggered
	 * interrupt.
	 */
	NVIC_SetPendingIRQ(TIMESLOT_STARTED_OFFLOAD_SWI_IRQN);

	return FLASH_OP_ONGOING_NONBLOCKING;
}

ISR_DIRECT_DECLARE(timeslot_handler_offload_irq)
{
	k_sem_give(&_context.timeslot_started_sem);
	ISR_DIRECT_PM();
	return 1;
}

static void flash_sync_exe_worker(struct k_work *work)
{
	(void) work;

	_context.flash_sync_exe_ret = nrf_flash_sync_exe(&_context.op_desc);

	if (!atomic_get(&_context.operation_success)) {
		/* The operation success flag has not been set, meaning the
		 * timeslot was not granted and the semaphor has not been given.
		 * Give it here to unblock the RPC thread to send the error response.
		 */
		k_sem_give(&_context.timeslot_started_sem);
	}

	/* nrf_flash_sync_exe will not exit until the MPSL timeslot has ended */
	k_sem_give(&_context.session_active_sem);
}

static void remote_handler_nrf_flash_sync_exe(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx,
						  void *handler_data)
{
	uint32_t request_length_us;

	if (!zcbor_uint32_decode(ctx->zs, &request_length_us)) {
		nrf_rpc_cbor_decoding_done(group, ctx);
		return;
	}
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!_context.initialized) {
		nrf_flash_sync_init();
		k_sem_init(&_context.timeslot_started_sem, 0, 1);
		k_sem_init(&_context.session_active_sem, 1, 1);
		k_work_init(&_context.flash_sync_exe_work, flash_sync_exe_worker);
		_context.op_desc.handler = timeslot_started_handler;
		_context.op_desc.context = NULL;
		IRQ_DIRECT_CONNECT(TIMESLOT_STARTED_OFFLOAD_SWI_IRQN,
				   TIMESLOT_STARTED_OFFLOAD_SWI_IRQ_PRIO,
				   timeslot_handler_offload_irq,
				   TIMESLOT_STARTED_OFFLOAD_SWI_IRQ_FLAGS);
		irq_enable(TIMESLOT_STARTED_OFFLOAD_SWI_IRQN);
		_context.initialized = true;
	}

	/* Prevent a situation where the MPSL timeslot is still ongoing,
	 * but another request has come;
	 * A better solution might be another RPC command to close
	 * the session, but there is a problem with getting
	 * the session_id.
	 */
	k_sem_take(&_context.session_active_sem, K_FOREVER);

	/* Reset the semaphore to mitigate an unlikely case where it was given
	 * both from the interrupt and from the worker thread.
	 */
	k_sem_reset(&_context.timeslot_started_sem);
	_context.timeslot_start_time = 0;
	atomic_clear(&_context.operation_success);

	nrf_flash_sync_set_context(request_length_us);

	/* Elevate the thread priority, so that the RPC response is sent as quick as
	 * possible after MPSL timeslot is granted.
	 */
	k_tid_t rpc_thread = k_current_get();
	int orig_thread_prio = k_thread_priority_get(rpc_thread);

	k_thread_priority_set(rpc_thread, K_HIGHEST_THREAD_PRIO);

	k_work_submit(&_context.flash_sync_exe_work);
	k_sem_take(&_context.timeslot_started_sem, K_FOREVER);


	if (atomic_get(&_context.operation_success)) {
		/* If the timeslot was granted, return success,
		 * otherwise return the error code returned by
		 * nrf_flash_sync_exe
		 */
		_context.flash_sync_exe_ret = 0;
	} else {
		/* Timeslot allocation failed, no need to send the response
		 * with high priority
		 */
		LOG_ERR("Failed to allocate timeslot with duration %u, error: %d",
			request_length_us, _context.flash_sync_exe_ret);
		k_thread_priority_set(rpc_thread, orig_thread_prio);
	}

	/* Send the response */
	struct nrf_rpc_cbor_ctx nctx;

	NRF_RPC_CBOR_ALLOC(group, nctx, MAX_FLASH_SYNC_EXE_RET_ENCODED_LENGTH);

	if (!zcbor_int32_put(nctx.zs, _context.flash_sync_exe_ret)) {
		return;
	}

	if (!zcbor_uint32_put(nctx.zs, _context.timeslot_start_time)) {
		return;
	}

	nrf_rpc_cbor_rsp_no_err(group, &nctx);

	k_thread_priority_set(rpc_thread, orig_thread_prio);
}

NRF_RPC_CBOR_CMD_DECODER(flash_sync_rpc_api_grp,
			 remote_handler_nrf_flash_sync_exe,
			 FLASH_SYNC_EXE_RPC_COMMAND,
			 remote_handler_nrf_flash_sync_exe,
			 NULL);
