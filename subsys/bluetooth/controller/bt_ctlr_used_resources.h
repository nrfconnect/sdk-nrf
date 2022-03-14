/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_CTLR_USED_RESOURCES_H__
#define BT_CTLR_USED_RESOURCES_H__

/* As per the section "SoftDevice Controller/Integration with applications"
 * in the nrfxlib documentation, the controller uses the following channels:
 */
#if defined(PPI_PRESENT)
	/* PPI channels 17 - 31, for the nRF52 Series */
	#define PPI_CHANNELS_USED_BY_CTLR (BIT_MASK(15) << 17)
#else
	/* DPPI channels 0 - 13, for the nRF53 Series */
	#define PPI_CHANNELS_USED_BY_CTLR BIT_MASK(14)
#endif

/* Additionally, MPSL requires several PPI channels (as per the section
 * "Multiprotocol Service Layer/Integration notes").
 */
#include <mpsl.h>

/* The following macros are used in nrfx_glue.h for marking these PPI channels
 * and groups and GPIOTE channels as occupied and thus unavailable to other
 * modules.
 */
#define BT_CTLR_USED_PPI_CHANNELS    (PPI_CHANNELS_USED_BY_CTLR | MPSL_RESERVED_PPI_CHANNELS)
#define BT_CTLR_USED_PPI_GROUPS	     0
#define BT_CTLR_USED_GPIOTE_CHANNELS 0

#endif /* BT_CTLR_USED_RESOURCES_H__ */
