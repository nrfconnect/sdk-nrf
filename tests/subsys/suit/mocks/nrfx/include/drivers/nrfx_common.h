/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRFX_COMMON_H__
#define NRFX_COMMON_H__

/* nRF54H20 processor IDs */
typedef enum {
	NRF_PROCESSOR_APPLICATION = 2,  /* Application Core Processor */
	NRF_PROCESSOR_RADIOCORE   = 3,  /* Radio Core Processor */
	NRF_PROCESSOR_SYSCTRL     = 12, /* System Controller Processor */
	NRF_PROCESSOR_PPR         = 13, /* Peripheral Processor */
	NRF_PROCESSOR_FLPR        = 14, /* Fast Lightweight Processor */
} NRF_PROCESSORID_Type;

#endif /* NRFX_COMMON_H__ */
