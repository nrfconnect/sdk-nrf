/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <debug/rram_wear_test.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rram_wear_test, CONFIG_RRAM_WEAR_TEST_LOG_LEVEL);

#define CHUNK_SIZE 256U


#define RWT_DTS_PARTITION_ENTRY(node) \
	{ \
		.name = DT_PROP_OR(node, label, ""), \
		.flash_area_id = DT_PARTITION_ID(node), \
	},

#define RWT_DTS_PARTITIONS_FOREACH(table) \
	DT_FOREACH_CHILD(table, RWT_DTS_PARTITION_ENTRY)
#define RWT_DTS_SUBPARTITIONS_FOREACH(table) \
	DT_FOREACH_CHILD(table, RWT_DTS_PARTITION_ENTRY)

struct rwt_partition_entry {
	const char *name;
	uint8_t flash_area_id;
};

static const struct rwt_partition_entry rwt_partitions[] = {
	{
		.name = "custom",
		.flash_area_id = 255,
	},
#if DT_HAS_COMPAT_STATUS_OKAY(fixed_partitions)
	DT_FOREACH_STATUS_OKAY(fixed_partitions, RWT_DTS_PARTITIONS_FOREACH)
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(fixed_subpartitions)
	DT_FOREACH_STATUS_OKAY(fixed_subpartitions, RWT_DTS_SUBPARTITIONS_FOREACH)
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_mapped_partition)
	DT_FOREACH_STATUS_OKAY(zephyr_mapped_partition, RWT_DTS_PARTITION_ENTRY)
#endif
};

static int test_pattern(const struct flash_area *fa, off_t test_off, size_t test_size,
			uint8_t pattern)
{
	off_t off;
	int rc;
	uint8_t buf[CHUNK_SIZE];

	LOG_INF("Writing 0x%02X pattern...", pattern);
	memset(buf, pattern, CHUNK_SIZE);
	for (off = 0; off < (off_t)test_size; off += CHUNK_SIZE) {
		size_t len = (off + CHUNK_SIZE <= (off_t)test_size) ?
			     CHUNK_SIZE : (test_size - off);

		rc = flash_area_write(fa, test_off + off, buf, len);
		if (rc != 0) {
			LOG_ERR("Failed to write 0x%02X at offset 0x%lx (err: %d)", pattern,
				(unsigned long)(test_off + off), rc);
			return rc;
		}
	}

	LOG_INF("Verifying 0x%02X pattern...", pattern);
	for (off = 0; off < (off_t)test_size; off += CHUNK_SIZE) {
		size_t len = (off + CHUNK_SIZE <= (off_t)test_size) ?
			     CHUNK_SIZE : (test_size - off);

		rc = flash_area_read(fa, test_off + off, buf, len);
		if (rc != 0) {
			LOG_ERR("Failed to read at offset 0x%lx (err: %d)",
				(unsigned long)(test_off + off), rc);
			return rc;
		}
		for (size_t i = 0; i < len; i++) {
			if (buf[i] != pattern) {
				LOG_ERR("Verification failed for 0x%02X at offset 0x%lx", pattern,
					(unsigned long)(test_off + off + i));
				return -EIO;
			}
		}
	}

	return 0;
}

static int run_pattern_verify_on_fa(const struct flash_area *fa, off_t test_off, size_t test_size)
{
	int rc;

	LOG_INF("Starting pattern verify on flash area %u (offset: 0x%lx, size: %zu)",
		fa->fa_id, (unsigned long)(fa->fa_off + test_off), test_size);

	rc = test_pattern(fa, test_off, test_size, 0x00);
	if (rc != 0) {
		return rc;
	}

	rc = test_pattern(fa, test_off, test_size, 0xFF);
	if (rc != 0) {
		return rc;
	}

	LOG_INF("Pattern verify completed successfully.");
	return 0;
}

static const struct device *infer_internal_flash_device(void)
{
#if DT_HAS_CHOSEN(zephyr_flash_controller)
	const struct device *d = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

	if (device_is_ready(d)) {
		return d;
	}
#endif

	return NULL;
}

static struct flash_area s_custom_fa;

static bool is_firmware_partition(const char *name)
{
	if (name == NULL || name[0] == '\0') {
		return false;
	}

	if (strstr(name, "image-") != NULL ||
	    strstr(name, "slot") != NULL ||
	    strstr(name, "app") != NULL ||
	    strstr(name, "mcuboot") != NULL ||
	    strcmp(name, "b0") == 0 ||
	    strcmp(name, "s0") == 0 ||
	    strcmp(name, "s1") == 0) {
		return true;
	}

	return false;
}

extern char __rom_region_start[];
extern char _flash_used[];

static bool overlaps_with_running_firmware(uint64_t start, uint64_t end)
{
#if DT_HAS_CHOSEN(zephyr_flash)
	uintptr_t rom_start = (uintptr_t)__rom_region_start;
	uintptr_t flash_base = DT_REG_ADDR(DT_CHOSEN(zephyr_flash));
	uintptr_t flash_used = (uintptr_t)_flash_used;

	uint64_t fw_start = (uint64_t)(rom_start - flash_base);
	uint64_t fw_end = fw_start + (uint64_t)flash_used;

	/* Check for overlap: start1 < end2 && start2 < end1 */
	if (start < fw_end && fw_start < end) {
		return true;
	}
#endif
	return false;
}

	/* run_custom_region removed */

size_t rram_wear_test_partition_count(void)
{
	return ARRAY_SIZE(rwt_partitions);
}

int rram_wear_test_partition_get(size_t index, struct rram_wear_test_partition *out)
{
	const struct flash_area *fa;
	int rc;

	if (out == NULL) {
		return -EINVAL;
	}

	if (index >= ARRAY_SIZE(rwt_partitions)) {
		return -ENOENT;
	}

	out->name = rwt_partitions[index].name;

	if (rwt_partitions[index].flash_area_id == 255) {
		out->addr_start = 0;
		const struct device *dev = infer_internal_flash_device();

		if (dev != NULL) {
			uint64_t size;

			rc = flash_get_size(dev, &size);
			out->addr_end = (rc == 0) ? size : 0;
		} else {
			out->addr_end = 0;
		}
	} else {
		rc = flash_area_open(rwt_partitions[index].flash_area_id, &fa);
		if (rc == 0) {
			out->addr_start = fa->fa_off;
			out->addr_end = fa->fa_off + fa->fa_size;
			flash_area_close(fa);
		} else {
			out->addr_start = 0;
			out->addr_end = 0;
		}
	}

	return 0;
}

int rram_wear_test(size_t index, uint64_t addr_start, uint64_t addr_end, bool force)
{
	const struct flash_area *fa = NULL;
	uint64_t test_start;
	uint64_t test_end;
	off_t test_off;
	size_t test_size;
	int rc;

	LOG_INF("rram_wear_test: index=%zu, offset_start=0x%llx, offset_end=0x%llx, force=%d",
		index, (unsigned long long)addr_start, (unsigned long long)addr_end, force);

	if (index >= ARRAY_SIZE(rwt_partitions)) {
		LOG_ERR("Invalid partition index %zu", index);
		return -ENOENT;
	}

	struct rram_wear_test_partition part;

	rc = rram_wear_test_partition_get(index, &part);
	if (rc != 0) {
		return rc;
	}

	if (addr_end > addr_start) {
		test_start = part.addr_start + addr_start;
		test_end = part.addr_start + addr_end;

		if (test_end > part.addr_end) {
			LOG_ERR("Requested range exceeds partition size");
			return -EINVAL;
		}

		test_off = (off_t)addr_start;
		test_size = (size_t)(addr_end - addr_start);
	} else {
		test_start = part.addr_start;
		test_end = part.addr_end;
		test_off = 0;
		test_size = (size_t)(part.addr_end - part.addr_start);
	}

	if (overlaps_with_running_firmware(test_start, test_end)) {
		LOG_ERR(
			"Requested range [0x%llx, 0x%llx) overlaps with active firmware. "
			"Operation strictly forbidden.",
			(unsigned long long)test_start, (unsigned long long)test_end);
		return -EPERM;
	}

	if (!force && index == 0) {
		LOG_ERR("Custom region test requires force flag");
		return -EPERM;
	}

	if (!force && index != 0 && is_firmware_partition(rwt_partitions[index].name)) {
		LOG_ERR(
			"Partition '%s' looks to be a firmware partition. "
			"Set force flag to overwrite.",
			rwt_partitions[index].name ? rwt_partitions[index].name : "");
		return -EPERM;
	}

	if (index == 0) {
		const struct device *dev = infer_internal_flash_device();

		if (dev == NULL) {
			LOG_ERR("Failed to infer internal flash device");
			return -ENODEV;
		}

		(void)memset(&s_custom_fa, 0, sizeof(s_custom_fa));
		s_custom_fa.fa_id = 255;
		s_custom_fa.fa_off = 0;
		s_custom_fa.fa_size = part.addr_end;
		s_custom_fa.fa_dev = dev;
		fa = &s_custom_fa;
	} else {
		rc = flash_area_open(rwt_partitions[index].flash_area_id, &fa);
		if (rc != 0) {
			LOG_ERR("Failed to open flash area for partition %zu (id: %u, err: %d)",
				index, rwt_partitions[index].flash_area_id, rc);
			return rc;
		}
	}

	rc = run_pattern_verify_on_fa(fa, test_off, test_size);

	if (index != 0) {
		flash_area_close(fa);
	}

	return rc;
}
