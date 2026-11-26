/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/partitions.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <bl_mpc.h>

#include "mcuboot_config/mcuboot_config.h"
#include "bootutil/bootutil_log.h"
#include "bootutil/bootutil_public.h"
#include "bootutil/fault_injection_hardening.h"
#include "bootutil/image.h"
#include "bootutil/mcuboot_status.h"
#if defined(MCUBOOT_DIRECT_XIP) || defined(MCUBOOT_RAM_LOAD)
#include "bootutil/bootutil.h"
#include <../../bootutil/src/bootutil_priv.h>
#endif

#if defined(MCUBOOT_SINGLE_APPLICATION_SLOT) || \
	defined(MCUBOOT_SINGLE_APPLICATION_SLOT_RAM_LOAD) || defined(MCUBOOT_FIRMWARE_LOADER)
#error "bl_mpc_mcuboot: MCUboot mode not supported"
#endif

/* The image can reach the trailer page, which then could not be written to confirm the image. */
#if defined(MCUBOOT_DIRECT_XIP_REVERT)
#error "bl_mpc_mcuboot: Direct-XIP with revert not supported"
#endif

BOOT_LOG_MODULE_DECLARE(mcuboot);

/* MPC00 covers only partitions of the memory-mapped zephyr,flash memory. */
#define PART_IN_FLASH(node)                                                                        \
	COND_CODE_1(DT_NODE_EXISTS(DT_MEM_FROM_PARTITION(node)),                                   \
		    (IS_EQ(DT_DEP_ORD(DT_MEM_FROM_PARTITION(node)),                               \
			   DT_DEP_ORD(DT_CHOSEN(zephyr_flash)))),                                  \
		    (0))
#define PART_COVERED(label)                                                                        \
	COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(label)), (PART_IN_FLASH(DT_NODELABEL(label))), (0))
#define PART_START(label)                                                                          \
	COND_CODE_1(PART_COVERED(label), (DT_PARTITION_ADDR(DT_NODELABEL(label))), (0))
#define PART_END(label)                                                                            \
	COND_CODE_1(PART_COVERED(label),                                                           \
		    ((DT_PARTITION_ADDR(DT_NODELABEL(label)) + DT_REG_SIZE(DT_NODELABEL(label)))), \
		    (0))
#define PART_ALIGNED(label)                                                                        \
	(((PART_START(label) | PART_END(label)) % BL_MPC_GRANULARITY_BYTES) == 0)
#define PART_RANGE(label) {.start = PART_START(label), .end = PART_END(label)}

/* Selected slot: code and trailing read-write part. Other slot: read-write. */
#define REGIONS_PER_IMAGE 3
#define REGION_COUNT_MAX                                                                           \
	(REGIONS_PER_IMAGE * MCUBOOT_IMAGE_NUMBER + PART_COVERED(storage_partition))

#if defined(MCUBOOT_RAM_LOAD)
#define CODE_PERM BL_MPC_PERM_R
#else
#define CODE_PERM BL_MPC_PERM_RX
#endif

BUILD_ASSERT(MCUBOOT_IMAGE_NUMBER <= 3, "bl_mpc_mcuboot supports up to 3 images");
BUILD_ASSERT(REGION_COUNT_MAX <= BL_MPC_REGION_COUNT_MAX, "Too many MPC regions for this SoC");
BUILD_ASSERT(PART_ALIGNED(slot0_partition) && PART_ALIGNED(slot1_partition) &&
		     PART_ALIGNED(slot2_partition) && PART_ALIGNED(slot3_partition) &&
		     PART_ALIGNED(slot4_partition) && PART_ALIGNED(slot5_partition) &&
		     PART_ALIGNED(storage_partition),
	     "MPC-covered partitions must be aligned to BL_MPC_GRANULARITY_BYTES");

struct part_range {
	uint32_t start;
	uint32_t end;
};

/* An empty range marks a partition not covered by MPC00. */
static const struct part_range slots[MCUBOOT_IMAGE_NUMBER][BOOT_SLOT_COUNT] = {
	{PART_RANGE(slot0_partition), PART_RANGE(slot1_partition)},
#if MCUBOOT_IMAGE_NUMBER > 1
	{PART_RANGE(slot2_partition), PART_RANGE(slot3_partition)},
#endif
#if MCUBOOT_IMAGE_NUMBER > 2
	{PART_RANGE(slot4_partition), PART_RANGE(slot5_partition)},
#endif
};

static const struct part_range storage = PART_RANGE(storage_partition);

static int active_slot_get(uint8_t image, uint32_t *slot)
{
#if defined(MCUBOOT_DIRECT_XIP) || defined(MCUBOOT_RAM_LOAD)
	uint32_t active = boot_get_loader_state()->slot_usage[image].active_slot;

	if (active >= BOOT_SLOT_COUNT) {
		return -ENOENT;
	}

	*slot = active;
#else
	ARG_UNUSED(image);
	*slot = BOOT_SLOT_PRIMARY;
#endif
	return 0;
}

/* The trailer after the image and its TLVs must stay writable for image confirmation.
 * In RAM-load mode the image can reach the last page, which holds the trailer, but the
 * slot is not executed from, so that page is always left read-write.
 */
static int code_end_get(const struct part_range *slot, uint32_t *code_end)
{
	const struct image_header *hdr = (const struct image_header *)slot->start;
	uint32_t slot_size = slot->end - slot->start;

	if (hdr->ih_magic != IMAGE_MAGIC || hdr->ih_img_size > slot_size ||
	    hdr->ih_hdr_size > slot_size - hdr->ih_img_size) {
		return -EINVAL;
	}

	*code_end = ROUND_UP(slot->start + hdr->ih_hdr_size + hdr->ih_img_size,
			     BL_MPC_GRANULARITY_BYTES);
#if defined(MCUBOOT_RAM_LOAD)
	*code_end = MIN(*code_end, slot->end - BL_MPC_GRANULARITY_BYTES);
#endif
	return 0;
}

static void region_add(struct bl_mpc_region *regions, size_t *count, uint32_t start,
		       uint32_t end, uint32_t permissions)
{
	if (start < end) {
		__ASSERT_NO_MSG(*count < REGION_COUNT_MAX);
		regions[*count] = (struct bl_mpc_region){
			.start = start,
			.end = end,
			.permissions = permissions,
		};
		(*count)++;
	}
}

static int regions_build(struct bl_mpc_region *regions, size_t *count)
{
	*count = 0;

	for (uint8_t image = 0; image < MCUBOOT_IMAGE_NUMBER; image++) {
		uint32_t active;
		int rc = active_slot_get(image, &active);

		if (rc) {
			return rc;
		}

		for (uint32_t slot = 0; slot < BOOT_SLOT_COUNT; slot++) {
			const struct part_range *part = &slots[image][slot];
			uint32_t rw_start = part->start;

			if (part->start == part->end) {
				continue;
			}

			if (slot == active) {
				rc = code_end_get(part, &rw_start);
				if (rc) {
					return rc;
				}

				region_add(regions, count, part->start, rw_start, CODE_PERM);
			}

			region_add(regions, count, rw_start, part->end, BL_MPC_PERM_RW);
		}
	}

	region_add(regions, count, storage.start, storage.end, BL_MPC_PERM_RW);

	return 0;
}

static int regions_merge(struct bl_mpc_region *regions, size_t *count)
{
	size_t last = 0;

	if (*count == 0) {
		return 0;
	}

	for (size_t i = 1; i < *count; i++) {
		struct bl_mpc_region region = regions[i];
		size_t j = i;

		for (; j > 0 && regions[j - 1].start > region.start; j--) {
			regions[j] = regions[j - 1];
		}
		regions[j] = region;
	}

	for (size_t i = 1; i < *count; i++) {
		if (regions[last].end > regions[i].start) {
			return -EINVAL;
		}

		if (regions[last].end == regions[i].start &&
		    regions[last].permissions == regions[i].permissions) {
			regions[last].end = regions[i].end;
		} else {
			regions[++last] = regions[i];
		}
	}

	*count = last + 1;
	return 0;
}

void mcuboot_status_change(mcuboot_status_type_t status)
{
	struct bl_mpc_region regions[REGION_COUNT_MAX];
	size_t count = 0;
	int rc;

	if (status != MCUBOOT_STATUS_BOOTABLE_IMAGE_FOUND) {
		return;
	}

	rc = regions_build(regions, &count);
	if (!rc) {
		rc = regions_merge(regions, &count);
	}
	if (!rc) {
		rc = bl_mpc_protect_and_lock(regions, count);
	}
	if (rc) {
		BOOT_LOG_ERR("MPC protection failed (err: %d)", rc);
		FIH_PANIC;
	}

	BOOT_LOG_INF("MPC regions locked: %zu", count);
}
