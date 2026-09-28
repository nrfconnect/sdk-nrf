/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @defgroup cracen_kmu_layout CRACEN KMU slot layout
 * @{
 * @brief KMU slot layout used by the CRACEN driver.
 *
 * The layout is defined in cracen_kmu_layout_default.h unless the CRACEN_KMU_LAYOUT_FILE
 * CMake variable gives the absolute path of a header that replaces it. The variable is read
 * from sysbuild, so setting it once applies the layout to all images, for example:
 *
 *     west build -b <board> <app> -- -DCRACEN_KMU_LAYOUT_FILE=<path>/kmu_layout.h
 *
 * See cracen_kmu_layout_default.h for the macros a layout must define.
 */

#ifndef CRACEN_KMU_LAYOUT_H
#define CRACEN_KMU_LAYOUT_H

/* Copy of the selected layout header, placed in the build tree by CMake. */
#include <cracen_kmu_layout_config.h>

#if !defined(CRACEN_KMU_PROVISIONING_SLOT) || !defined(CRACEN_KMU_PROT_RAM_INV_SLOT) ||        \
	!defined(CRACEN_KMU_IKG_SEED_SLOT) || !defined(CRACEN_KMU_NS_SLOT_FIRST) ||                \
	!defined(CRACEN_KMU_NS_SLOT_LAST)
#error "The KMU slot layout must define all macros in cracen_kmu_layout_default.h"
#endif

#if CRACEN_KMU_NS_SLOT_FIRST > CRACEN_KMU_NS_SLOT_LAST || CRACEN_KMU_NS_SLOT_LAST > 255
#error "Invalid non-secure KMU slot range"
#endif

#if CRACEN_KMU_PROVISIONING_SLOT > 255 || CRACEN_KMU_PROT_RAM_INV_SLOT > 254 ||                   \
	CRACEN_KMU_IKG_SEED_SLOT > 253
#error "KMU slot reserved by the CRACEN driver is out of range"
#endif

/* Whether the inclusive slot ranges [a_first, a_last] and [b_first, b_last] overlap. */
#define CRACEN_KMU_SLOTS_OVERLAP(a_first, a_last, b_first, b_last)                                \
	((a_first) <= (b_last) && (b_first) <= (a_last))

#if CRACEN_KMU_SLOTS_OVERLAP(CRACEN_KMU_PROVISIONING_SLOT, CRACEN_KMU_PROVISIONING_SLOT,          \
			     CRACEN_KMU_PROT_RAM_INV_SLOT, CRACEN_KMU_PROT_RAM_INV_SLOT + 1) ||   \
	CRACEN_KMU_SLOTS_OVERLAP(CRACEN_KMU_PROVISIONING_SLOT, CRACEN_KMU_PROVISIONING_SLOT,      \
				 CRACEN_KMU_IKG_SEED_SLOT, CRACEN_KMU_IKG_SEED_SLOT + 2) ||       \
	CRACEN_KMU_SLOTS_OVERLAP(CRACEN_KMU_PROT_RAM_INV_SLOT, CRACEN_KMU_PROT_RAM_INV_SLOT + 1,  \
				 CRACEN_KMU_IKG_SEED_SLOT, CRACEN_KMU_IKG_SEED_SLOT + 2)
#error "KMU slots reserved by the CRACEN driver overlap"
#endif

#if CRACEN_KMU_SLOTS_OVERLAP(CRACEN_KMU_PROVISIONING_SLOT, CRACEN_KMU_PROVISIONING_SLOT,          \
			     CRACEN_KMU_NS_SLOT_FIRST, CRACEN_KMU_NS_SLOT_LAST) ||                \
	CRACEN_KMU_SLOTS_OVERLAP(CRACEN_KMU_PROT_RAM_INV_SLOT, CRACEN_KMU_PROT_RAM_INV_SLOT + 1,  \
				 CRACEN_KMU_NS_SLOT_FIRST, CRACEN_KMU_NS_SLOT_LAST)
#error "Non-secure KMU slot range includes a slot reserved by the CRACEN driver"
#endif

#undef CRACEN_KMU_SLOTS_OVERLAP

#endif /* CRACEN_KMU_LAYOUT_H */

/** @} */
