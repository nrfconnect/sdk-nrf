/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* KMU slots reserved by the CRACEN driver, and the slot range the TF-M non-secure agent may
 * access. The values come from Kconfig, see SB_CONFIG_CRACEN_KMU_CUSTOM_LAYOUT for
 * applying one layout to all images.
 */

#ifndef CRACEN_KMU_LAYOUT_H
#define CRACEN_KMU_LAYOUT_H

#define CRACEN_KMU_PROVISIONING_SLOT CONFIG_CRACEN_KMU_PROVISIONING_SLOT
#define CRACEN_KMU_PROT_RAM_INV_SLOT CONFIG_CRACEN_KMU_PROT_RAM_INV_SLOT
#define CRACEN_KMU_IKG_SEED_SLOT     CONFIG_CRACEN_IKG_SEED_KMU_SLOT
#define CRACEN_KMU_NS_SLOT_FIRST     CONFIG_CRACEN_KMU_NS_SLOT_FIRST
#define CRACEN_KMU_NS_SLOT_LAST      CONFIG_CRACEN_KMU_NS_SLOT_LAST

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
				 CRACEN_KMU_NS_SLOT_FIRST, CRACEN_KMU_NS_SLOT_LAST) ||             \
	CRACEN_KMU_SLOTS_OVERLAP(CRACEN_KMU_IKG_SEED_SLOT, CRACEN_KMU_IKG_SEED_SLOT + 2,          \
				 CRACEN_KMU_NS_SLOT_FIRST, CRACEN_KMU_NS_SLOT_LAST)
#error "Non-secure KMU slot range includes a slot reserved by the CRACEN driver"
#endif

#undef CRACEN_KMU_SLOTS_OVERLAP

#endif /* CRACEN_KMU_LAYOUT_H */
