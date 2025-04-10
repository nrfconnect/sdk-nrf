/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __CPUCONF_H__
#define __CPUCONF_H__

#include <zephyr/drivers/firmware/nrf_ironside/call.h>

#define IRONSIDE_SE_CPUCONF_SERVICE_WRONG_CPU 0x5eb2
#define IRONSIDE_SE_CPUCONF_SERVICE_STARTING_CPU_FAILED 0x5eb0
#define IRONSIDE_SE_CPUCONF_SERVICE_MEM_ACCESS_NOT_PERMITTED 0x5eb1

#define IRONSIDE_CALL_ID_CPUCONF_V0 2

enum {
	IRONSIDE_SE_CPUCONF_INDEX_CPU,
	IRONSIDE_SE_CPUCONF_INDEX_VECTOR_TABLE,
	IRONSIDE_SE_CPUCONF_INDEX_CPU_WAIT,
	IRONSIDE_SE_CPUCONF_INDEX_MSG,
	IRONSIDE_SE_CPUCONF_INDEX_MSG_SIZE,
	IRONSIDE_SE_CPUCONF_INDEX_ERR,
	/* The last enum value is reserved for the number of arguments */
	IRONSIDE_SE_CPUCONF_NUM_ARGS
};

BUILD_ASSERT(IRONSIDE_SE_CPUCONF_NUM_ARGS <= NRF_IRONSIDE_CALL_NUM_ARGS);

/**
 * @brief Boot a local domain CPU
 *
 * @param cpu The CPU to be booted
 * @param vector_table Pointer to the vector table used to boot the CPU.
 * @param cpu_wait When this is true, the CPU will WAIT even if the CPU has clock.
 * @param msg A message that can be placed in radiocore's boot report.
 * @param msg_size Size of the message in bytes.
 *
 * @note cpu_wait is only intended to be enabled for debug purposes
 * and it is only supported that a debugger resumes the CPU.
 *
 * @retval 0 on success or if the CPU has already booted.
 * @retval -IRONSIDE_SE_CPUCONF_SERVICE_WRONG_CPU if cpu is unrecognized
 * @retval -IRONSIDE_SE_CPUCONF_SERVICE_STARTING_CPU_FAILED if starting the CPU failed
 * @retval -IRONSIDE_SE_CPUCONF_SERVICE_MEM_ACCESS_NOT_PERMITTED
 * if the CPU does not have read access configured for the vector_table address
 */
int ironside_se_cpuconf_boot_core(NRF_PROCESSORID_Type cpu, void *vector_table, bool cpu_wait, uint8_t *msg, size_t msg_size);

#endif /* __CPUCONF_SERVICE_H__ */
