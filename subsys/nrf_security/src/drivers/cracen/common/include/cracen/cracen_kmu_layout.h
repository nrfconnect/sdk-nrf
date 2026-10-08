/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @addtogroup cracen_kmu
 * @{
 * @brief KMU slot layout of the CRACEN driver.
 *
 * This header defines the KMU slots that the CRACEN driver reserves and the slot range that
 * the TF-M non-secure application may access. The values come from Kconfig, which sysbuild
 * sets to the same layout in all images.
 */

#ifndef CRACEN_KMU_LAYOUT_H
#define CRACEN_KMU_LAYOUT_H

/** KMU slot whose metadata records that provisioning is in progress. */
#define CRACEN_KMU_PROVISIONING_SLOT CONFIG_CRACEN_KMU_PROVISIONING_SLOT
/** First of the 2 KMU slots that hold the protected RAM invalidation data. */
#define CRACEN_KMU_PROT_RAM_INV_SLOT CONFIG_CRACEN_KMU_PROT_RAM_INV_SLOT
/** First of the 3 KMU slots that hold the IKG seed. */
#define CRACEN_KMU_IKG_SEED_SLOT     CONFIG_CRACEN_IKG_SEED_KMU_SLOT

#if defined(CONFIG_BUILD_WITH_TFM)
/** First KMU slot the TF-M non-secure application may access. */
#define CRACEN_KMU_NS_SLOT_FIRST CONFIG_CRACEN_KMU_NS_SLOT_FIRST
/** Last KMU slot the TF-M non-secure application may access. */
#define CRACEN_KMU_NS_SLOT_LAST  CONFIG_CRACEN_KMU_NS_SLOT_LAST
#endif

/**
 * @brief Check whether two inclusive KMU slot ranges overlap.
 *
 * @param a_first First slot of range A.
 * @param a_last  Last slot of range A.
 * @param b_first First slot of range B.
 * @param b_last  Last slot of range B.
 *
 * @retval 1 The ranges share at least one slot.
 * @retval 0 The ranges do not overlap.
 */
#define CRACEN_KMU_SLOTS_OVERLAP(a_first, a_last, b_first, b_last)                                \
	((a_first) <= (b_last) && (b_first) <= (a_last))

/** @} */

#endif /* CRACEN_KMU_LAYOUT_H */
