/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file bl_mpc.h
 * @brief Bootloader-side MPC protection for nRF54L Series.
 *
 * Programs MPC00 override regions from an explicit list of address ranges and
 * permissions, then locks every software-configurable override until the next
 * reset.
 *
 * The module is called once, by the last bootloader stage before hand-off
 * (MCUboot, or NSIB when MCUboot is not present). No later stage can configure
 * MPC overrides after this call (see @ref bl_mpc_protect_and_lock).
 *
 * All protected regions are Secure. The module is not compatible with a later
 * Secure stage that configures MPC overrides itself (for example TF-M).
 *
 * @note Region granularity is 4 KB.
 * @note Writable data within an otherwise read-only image, such as an MCUboot
 *       image trailer used for image confirmation, requires a separate aligned
 *       read-write region. The read-write region must not overlap a read-execute
 *       region because permissions in overlapping regions are combined.
 */

#ifndef BL_MPC_H_
#define BL_MPC_H_

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bl_mpc Bootloader MPC protection
 * @{
 */

/** Override region granularity. All region boundaries must be multiples of it. */
#define BL_MPC_GRANULARITY_BYTES 0x1000UL

/**
 * @brief Maximum number of regions accepted by this SoC.
 *
 * Equal to the number of general-purpose MPC00 overrides (overrides
 * 0 .. BL_MPC_REGION_COUNT_MAX - 1). Override BL_MPC_REGION_COUNT_MAX is the
 * access-blocking override: it never holds a region and is always locked disabled.
 * Overrides above it are reserved by the system and are not touched.
 *
 * @note MPC_OVERRIDE_MaxCount includes reserved overrides and cannot be used as
 *       the region capacity.
 */
#if defined(CONFIG_SOC_NRF54L05_CPUAPP) || defined(CONFIG_SOC_NRF54L10_CPUAPP) ||                  \
	defined(CONFIG_SOC_NRF54L15_CPUAPP) || defined(NRF54L05_XXAA) || defined(NRF54L10_XXAA) || \
	defined(NRF54L15_XXAA)
#define BL_MPC_REGION_COUNT_MAX 5
#elif defined(CONFIG_SOC_NRF54LM20A_CPUAPP) || defined(CONFIG_SOC_NRF54LM20B_CPUAPP) ||            \
	defined(CONFIG_SOC_NRF54LV10A_CPUAPP) || defined(CONFIG_SOC_NRF54LC10A_CPUAPP) ||          \
	defined(NRF54LM20A_XXAA) || defined(NRF54LM20B_XXAA) || defined(NRF54LV10A_XXAA) ||        \
	defined(NRF54LC10A_XXAA)
#define BL_MPC_REGION_COUNT_MAX 7
#else
#error "bl_mpc: MPC00 override layout not defined for this SoC"
#endif

/**
 * @name Permission flags for @ref bl_mpc_region.permissions
 * @{
 */
#define BL_MPC_PERM_READ    (1UL << 0)
#define BL_MPC_PERM_WRITE   (1UL << 1)
#define BL_MPC_PERM_EXECUTE (1UL << 2)

/** All valid permission bits. Any other bit set is rejected with -EINVAL. */
#define BL_MPC_PERM_ALL (BL_MPC_PERM_READ | BL_MPC_PERM_WRITE | BL_MPC_PERM_EXECUTE)
/** Readable and executable code without write access. */
#define BL_MPC_PERM_RX	(BL_MPC_PERM_READ | BL_MPC_PERM_EXECUTE)
/** Readable and writable data without execute access. */
#define BL_MPC_PERM_RW	(BL_MPC_PERM_READ | BL_MPC_PERM_WRITE)
/** Read-only, non-executable data. */
#define BL_MPC_PERM_R	BL_MPC_PERM_READ
/** @} */

/**
 * @brief Single protected address range.
 *
 * The region is mapped Secure. Permission combinations are not otherwise
 * restricted.
 */
struct bl_mpc_region {
	/** First address, aligned to BL_MPC_GRANULARITY_BYTES. */
	uint32_t start;
	/** First address after the region, aligned to BL_MPC_GRANULARITY_BYTES. */
	uint32_t end;
	/** OR of BL_MPC_PERM_* flags. Zero denies read, write, and execute access. */
	uint32_t permissions;
};

/**
 * @brief Program the given regions into MPC00 and lock all software-configurable
 *        overrides.
 *
 * Mapping is deterministic: regions[i] is assigned to general-purpose override i.
 * All unassigned general-purpose overrides and the access-blocking override are
 * locked disabled.
 *
 * Each region is programmed with PERM.SECATTR = Secure and PERMMASK covering
 * READ, WRITE, EXECUTE and SECATTR.
 *
 * The function validates its input before modifying MPC00. If any override is
 * already locked, the complete MPC00 state must match the requested target
 * state. Invalid input or a locked state mismatch leaves MPC00 unchanged. A
 * failure while programming or verifying the hardware may leave MPC00 partially
 * configured.
 *
 * The function is idempotent: a second call with the same input returns 0 and
 * performs no writes.
 *
 * The module does not check overlaps or adjacency between regions. Where regions
 * overlap, hardware combines their READ, WRITE and EXECUTE bits with logical OR,
 * so an overlap can only add access rights. The caller must provide
 * non-overlapping regions.
 *
 * Memory not covered by any region keeps the MPC default (after reset: Secure,
 * accessible by Secure code).
 *
 * The caller must not hand off control after an error.
 *
 * Input is invalid if:
 * - @p regions is NULL or @p region_count is zero.
 * - A region boundary is not a multiple of BL_MPC_GRANULARITY_BYTES.
 * - A region start address is not lower than its end address.
 * - A region permission value contains bits outside BL_MPC_PERM_ALL.
 *
 * @pre Called from Secure, privileged code, with no concurrent access to MPC00.
 * @pre Called once per boot by the last bootloader stage, after all bootloader
 *      writes to protected ranges are complete.
 * @pre The caller shall ensure that the executing code, stack, vector table,
 *      and handoff path remain accessible under the requested configuration.
 *
 * @param regions      Array of @p region_count regions.
 * @param region_count Number of regions, 1 .. BL_MPC_REGION_COUNT_MAX.
 *
 * @retval 0        All software-configurable overrides are in the target state,
 *                  locked and verified, including when they were already configured.
 * @retval -EINVAL  Invalid input. MPC00 is not modified.
 * @retval -ENOSPC  @p region_count exceeds BL_MPC_REGION_COUNT_MAX. MPC00 is not modified.
 * @retval -EACCES  An override is locked in a state that differs from the target
 *                  state. MPC00 is not modified.
 * @retval -EIO     Read-back mismatch after writing. MPC00 might be partially configured.
 */
int bl_mpc_protect_and_lock(const struct bl_mpc_region *regions, size_t region_count);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* BL_MPC_H_ */
