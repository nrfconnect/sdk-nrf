/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <toolchain.h>
#include <sys/util.h>
#include <pm_config.h>

#define SRAM_BASE 0x20000000
#define SHMEM_RANGE KB(128)
#define SHMEM_END (SRAM_BASE + SHMEM_RANGE)

#define SHMEM_IN_USE \
	(PM_NRF_MODEM_LIB_SRAM_ADDRESS + PM_NRF_MODEM_LIB_SRAM_SIZE)

BUILD_ASSERT(SHMEM_IN_USE < SHMEM_END,
	     "The libmodem shmem configuration exceeds the range of "
	     "memory readable by the Modem core");
