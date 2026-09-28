/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @addtogroup cracen_kmu_layout
 * @{
 * @brief Default KMU slot layout used by the CRACEN driver.
 *
 * This layout is used unless the CRACEN_KMU_LAYOUT_FILE CMake variable is set.
 * A replacement layout header must define every macro in this file.
 */

#ifndef CRACEN_KMU_LAYOUT_DEFAULT_H
#define CRACEN_KMU_LAYOUT_DEFAULT_H

/** @brief Slot used to record whether provisioning is in progress for a set of slots.
 *
 * Only the metadata field of this slot is used.
 */
#define CRACEN_KMU_PROVISIONING_SLOT 186

/** @brief First of the two consecutive slots holding the protected RAM invalidation data. */
#define CRACEN_KMU_PROT_RAM_INV_SLOT 248

/** @brief First of the three consecutive slots holding the IKG seed. */
#define CRACEN_KMU_IKG_SEED_SLOT CONFIG_CRACEN_IKG_SEED_KMU_SLOT

/** @brief First KMU slot that the TF-M non-secure agent may access (inclusive). */
#define CRACEN_KMU_NS_SLOT_FIRST 0

/** @brief Last KMU slot that the TF-M non-secure agent may access (inclusive).
 *
 * KMU slots >= 180 are reserved for Nordic use.
 */
#define CRACEN_KMU_NS_SLOT_LAST 179

#endif /* CRACEN_KMU_LAYOUT_DEFAULT_H */

/** @} */
