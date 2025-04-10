/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/sys/util.h>

#include <nrfx.h>
#include <ironside/se/cpuconf.h> // Will be moved to Zephyr

#include <zephyr/drivers/firmware/nrf_ironside/call.h>

int ironside_se_cpuconf_boot_core(NRF_PROCESSORID_Type cpu, void *vector_table, bool cpu_wait, uint8_t *msg, size_t msg_size)
{
	ARG_UNUSED(msg);
	ARG_UNUSED(msg_size);

	/* NCSDK-32393: msg is unused until the boot report is available  */

	struct ironside_call_buf *const buf = ironside_call_alloc();

	buf->id = IRONSIDE_CALL_ID_CPUCONF_V0;

	buf->args[IRONSIDE_SE_CPUCONF_INDEX_CPU] = cpu;
	buf->args[IRONSIDE_SE_CPUCONF_INDEX_VECTOR_TABLE] = (uint32_t)vector_table;
	buf->args[IRONSIDE_SE_CPUCONF_INDEX_CPU_WAIT] = cpu_wait;
	buf->args[IRONSIDE_SE_CPUCONF_INDEX_MSG] = (uint32_t)msg;
	buf->args[IRONSIDE_SE_CPUCONF_INDEX_MSG_SIZE] = msg_size;

	ironside_call_dispatch(buf);

	int err = -1; // TODO COMM_FAILURE

	if (buf->status == IRONSIDE_CALL_STATUS_RSP_SUCCESS) {
		err = buf->args[IRONSIDE_SE_CPUCONF_INDEX_ERR];
	}

	ironside_call_release(buf);

	return err;
}
