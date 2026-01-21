/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/random/random.h>
#include <hal/nrf_timer.h>
#include <dk_buttons_and_leds.h>
#include <emds_flash.h>

#if defined(CONFIG_BT)
#include <mpsl.h>
#include <sdc.h>
#endif

#if defined CONFIG_SOC_FLASH_NRF_RRAM
#define RRAM DT_INST(0, soc_nv_flash)
#define EMDS_FLASH_BLOCK_SIZE DT_PROP(RRAM, write_block_size)
/* 32bits word in buffered stream is written typically 22us */
/* 16 bytes should take ideally 88us (really takes 62 - 128us) */
#define EXPECTED_STORE_TIME_BLOCK_SIZE (150)
/* 1024 bytes should take ideally 5632us (really takes 5664 - 5824us) */
#define EXPECTED_STORE_TIME_1024 (6200)
#else
#define FLASH DT_INST(0, soc_nv_flash)
#define EMDS_FLASH_BLOCK_SIZE DT_PROP(FLASH, write_block_size)
#if defined(CONFIG_SOC_NRF5340_CPUAPP)
#define EXPECTED_STORE_TIME_BLOCK_SIZE (48)
#define EXPECTED_STORE_TIME_1024 (11600)
#else
/* 32bits word is written maximum 41us (from datasheet) */
#define EXPECTED_STORE_TIME_BLOCK_SIZE (46)
/* 1024 bytes should take ideally 10496us (really takes about 11000us) */
#define EXPECTED_STORE_TIME_1024 (11500)
#endif
#endif

#define EMDS_SNAPSHOT_METADATA_MARKER 0x4D444553
#define PARTITIONS_NUM_MAX 2

static struct emds_partition partition[PARTITIONS_NUM_MAX];

/* Configure for 1MHz (1μs precision) */
#if defined(CONFIG_SOC_NRF54L15)
#define NORDIC_TIMER_INSTANCE  NRF_TIMER20
#else
#define NORDIC_TIMER_INSTANCE  NRF_TIMER2
#endif

static void microsecond_timer_init(void)
{
	/* Configure timer for 1 microsecond precision */
	nrf_timer_mode_set(NORDIC_TIMER_INSTANCE, NRF_TIMER_MODE_TIMER);
	nrf_timer_bit_width_set(NORDIC_TIMER_INSTANCE, NRF_TIMER_BIT_WIDTH_32);
	nrf_timer_prescaler_set(NORDIC_TIMER_INSTANCE, NRF_TIMER_FREQ_1MHz);

	nrf_timer_task_trigger(NORDIC_TIMER_INSTANCE, NRF_TIMER_TASK_CLEAR);
	nrf_timer_task_trigger(NORDIC_TIMER_INSTANCE, NRF_TIMER_TASK_START);

	printk("Timer initialized at 1MHz (1μs resolution)\n");
}

static uint32_t get_time_us(void)
{
	nrf_timer_task_trigger(NORDIC_TIMER_INSTANCE, NRF_TIMER_TASK_CAPTURE0);
	return nrf_timer_cc_get(NORDIC_TIMER_INSTANCE, NRF_TIMER_CC_CHANNEL0);
}

static void microsecond_timer_cleanup(void)
{
	/* Stop and disable timer to prevent system issues */
	nrf_timer_task_trigger(NORDIC_TIMER_INSTANCE, NRF_TIMER_TASK_STOP);
	nrf_timer_task_trigger(NORDIC_TIMER_INSTANCE, NRF_TIMER_TASK_CLEAR);
	printk("Timer stopped and cleaned up\n");
}

/** Local functions ***********************************************************/
static void *emds_flash_setup(void)
{
	const uint8_t id[] = {FIXED_PARTITION_ID(emds_partition_0),
			      FIXED_PARTITION_ID(emds_partition_1)};

	for (int i = 0; i < ARRAY_SIZE(id); i++) {
		zassert_ok(flash_area_open(id[i], &partition[i].fa), "Failed to open flash area %d",
			   id[i]);
		zassert_ok(emds_flash_init(&partition[i]), "Failed to initialize flash area %d",
			   id[i]);
	}

	return NULL;
}

static void emds_flash_partitions_erase(void *fixture)
{
	(void)fixture;

	for (int i = 0; i < PARTITIONS_NUM_MAX; i++) {
		zassert_ok(emds_flash_erase_partition(&partition[i]),
			   "Failed to erase partition %d", i);
	}
}

static struct emds_snapshot_metadata *
snapshot_make(const struct emds_partition *partition,
	      struct emds_snapshot_metadata *previous_snapshot, off_t metadata_off,
	      bool is_md_crc_correct, bool is_d_crc_correct, uint32_t fresh_cnt)
{
	static struct emds_snapshot_metadata metadata;
	uint8_t data[128 + sizeof(struct emds_data_entry)] = {0};
	uint8_t data_len = sys_rand32_get() % 128 + sizeof(struct emds_data_entry) + 1;

	for (int i = 0; i < data_len; i++) {
		data[i] = sys_rand32_get() % 256;
	}

	if (previous_snapshot) {
		metadata.data_instance_off = previous_snapshot->data_instance_off +
					     ROUND_UP(previous_snapshot->data_instance_len,
						      partition->fp->write_block_size);
	} else {
		memset(&metadata, 0, sizeof(metadata));
		metadata.marker = EMDS_SNAPSHOT_METADATA_MARKER;
	}
	metadata.data_instance_len = data_len;
	metadata.fresh_cnt = fresh_cnt;

	if (REGIONS_OVERLAP(metadata_off, sizeof(struct emds_snapshot_metadata),
			    metadata.data_instance_off,
			    ROUND_UP(data_len, partition->fp->write_block_size)) ||
	    metadata_off <= metadata.data_instance_off) {
		return NULL;
	}

	metadata.metadata_crc =
		crc32_k_4_2_update(0, (const unsigned char *)&metadata,
				   offsetof(struct emds_snapshot_metadata, metadata_crc));
	if (!is_md_crc_correct) {
		metadata.metadata_crc++;
	}

	metadata.snapshot_crc = crc32_k_4_2_update(0, data, data_len);
	if (!is_d_crc_correct) {
		metadata.snapshot_crc++;
	}

	zassert_ok(flash_area_write(partition->fa, metadata_off, &metadata, sizeof(metadata)),
		   "Failed to write metadata to flash area");

	data_len = ROUND_UP(data_len, sizeof(uint32_t));
	zassert_ok(flash_area_write(partition->fa, metadata.data_instance_off, data, data_len),
		   "Failed to write data to flash area");

	return &metadata;
}
/** End Local functions *******************************************************/

/* Test checks scanning the empty partition */
ZTEST(emds_flash, test_scanning_empty_partition)
{
	struct emds_snapshot_candidate candidate;

	candidate.metadata.fresh_cnt = 0;
	zassert_ok(emds_flash_scan_partition(&partition[0], &candidate),
		   "Failed to scan partition 0");
	zassert_equal(candidate.metadata.fresh_cnt, 0,
		      "Fresh count has been changed for empty partition");
}

/* Test checks that the partition scanning behaves equally for both partitions */
ZTEST(emds_flash, test_scanning_partitions_equality)
{
	struct emds_snapshot_candidate candidate;
	struct emds_snapshot_metadata *metadata = NULL;
	off_t metadata_off = partition[0].fa->fa_size;
	uint32_t fresh_cnt = 0;

	candidate.metadata.fresh_cnt = 0;
	for (int i = 0; i < CONFIG_EMDS_MAX_CANDIDATES; i++) {
		metadata_off -= sizeof(struct emds_snapshot_metadata);
		fresh_cnt++;
		metadata =
			snapshot_make(&partition[0], metadata, metadata_off, true, true, fresh_cnt);
		zassert_not_null(metadata, "Failed to create snapshot on partition 0");
	}

	metadata = NULL;
	metadata_off = partition[1].fa->fa_size;
	for (int i = 0; i < CONFIG_EMDS_MAX_CANDIDATES; i++) {
		metadata_off -= sizeof(struct emds_snapshot_metadata);
		fresh_cnt++;
		metadata =
			snapshot_make(&partition[1], metadata, metadata_off, true, true, fresh_cnt);
		zassert_not_null(metadata, "Failed to create snapshot on partition 1");
	}

	zassert_ok(emds_flash_scan_partition(&partition[0], &candidate),
		   "Failed to scan partition 0");
	zassert_equal(candidate.metadata_off,
		      partition[0].fa->fa_size -
			      CONFIG_EMDS_MAX_CANDIDATES * sizeof(struct emds_snapshot_metadata),
		      "Metadata offset mismatch for partition 0");
	zassert_equal(candidate.metadata.fresh_cnt, CONFIG_EMDS_MAX_CANDIDATES,
		      "Fresh count mismatch for partition 0");

	zassert_ok(emds_flash_scan_partition(&partition[1], &candidate),
		   "Failed to scan partition 1");
	zassert_equal(candidate.metadata_off,
		      partition[1].fa->fa_size -
			      CONFIG_EMDS_MAX_CANDIDATES * sizeof(struct emds_snapshot_metadata),
		      "Metadata offset mismatch for partition 1");
	zassert_equal(candidate.metadata.fresh_cnt, CONFIG_EMDS_MAX_CANDIDATES * 2,
		      "Fresh count mismatch for partition 1");
}

/* Test checks that emds scans partitions on full deepness. */
ZTEST(emds_flash, test_scanning_partitions_deepness)
{
	struct emds_snapshot_candidate candidate;
	int partition_index = sys_rand32_get() % PARTITIONS_NUM_MAX;
	struct emds_snapshot_metadata *metadata = NULL;
	off_t metadata_off = partition[partition_index].fa->fa_size;
	uint32_t fresh_cnt = 0;

	candidate.metadata.fresh_cnt = 0;
	do {
		metadata_off -= sizeof(struct emds_snapshot_metadata);
		fresh_cnt++;
		metadata = snapshot_make(&partition[partition_index], metadata, metadata_off, true,
					 true, fresh_cnt);
	} while (metadata != NULL);
	fresh_cnt--;

	zassert_ok(emds_flash_scan_partition(&partition[partition_index], &candidate),
		   "Failed to scan partition %d", partition_index);
	zassert_equal(candidate.metadata.fresh_cnt, fresh_cnt, "Fresh count mismatch");
}

/* Test checks that emds always finds the freshest snapshot. */
ZTEST(emds_flash, test_scanning_partitions_freshness)
{
	struct emds_snapshot_candidate candidate;
	int partition_index = sys_rand32_get() % PARTITIONS_NUM_MAX;
	struct emds_snapshot_metadata *metadata = NULL;
	off_t metadata_off = partition[partition_index].fa->fa_size;
	uint32_t fresh_cnt = 0;
	uint32_t fresh_cnt_max = 0;

	candidate.metadata.fresh_cnt = 0;
	do {
		metadata_off -= sizeof(struct emds_snapshot_metadata);
		fresh_cnt = sys_rand32_get() % 100 + 1;
		metadata = snapshot_make(&partition[partition_index], metadata, metadata_off, true,
					 true, fresh_cnt);
		if (fresh_cnt > fresh_cnt_max && metadata != NULL) {
			fresh_cnt_max = fresh_cnt;
		}
	} while (metadata != NULL);

	zassert_ok(emds_flash_scan_partition(&partition[partition_index], &candidate),
		   "Failed to scan partition %d", partition_index);
	zassert_equal(candidate.metadata.fresh_cnt, fresh_cnt_max, "Fresh count mismatch");
}

/* Test checks that emds ignores snapshot with wrong crc (both data and metadata). */
ZTEST(emds_flash, test_scanning_partitions_wrong_crc)
{
	struct emds_snapshot_candidate candidate;
	int partition_index = sys_rand32_get() % PARTITIONS_NUM_MAX;
	struct emds_snapshot_metadata *metadata = NULL;
	off_t metadata_off = partition[partition_index].fa->fa_size;
	uint32_t fresh_cnt = 1;

	candidate.metadata.fresh_cnt = 0;
	metadata_off -= sizeof(struct emds_snapshot_metadata);
	metadata = snapshot_make(&partition[partition_index], metadata, metadata_off, true, true,
				 fresh_cnt);
	zassert_not_null(metadata, "Failed to create snapshot on partition %d", partition_index);

	for (int i = 0; i < 2; i++) {
		metadata_off -= sizeof(struct emds_snapshot_metadata);
		fresh_cnt++;
		metadata = snapshot_make(&partition[partition_index], metadata, metadata_off, false,
					 true, fresh_cnt);
		zassert_not_null(metadata, "Failed to create snapshot on partition %d",
				 partition_index);
	}

	for (int i = 0; i < 2; i++) {
		metadata_off -= sizeof(struct emds_snapshot_metadata);
		fresh_cnt++;
		metadata = snapshot_make(&partition[partition_index], metadata, metadata_off, true,
					 false, fresh_cnt);
		zassert_not_null(metadata, "Failed to create snapshot on partition %d",
				 partition_index);
	}

	zassert_ok(emds_flash_scan_partition(&partition[partition_index], &candidate),
		   "Failed to scan partition %d", partition_index);
	zassert_equal(candidate.metadata.fresh_cnt, 1, "Fresh count mismatch");
}

/* Test checks that emds scanning has limitation according to KConfig options. */
ZTEST(emds_flash, test_scanning_limitations)
{
	struct emds_snapshot_candidate candidate;
	int partition_index = sys_rand32_get() % PARTITIONS_NUM_MAX;
	struct emds_snapshot_metadata *metadata = NULL;
	off_t metadata_off = partition[partition_index].fa->fa_size;
	uint32_t fresh_cnt = 1;

	candidate.metadata.fresh_cnt = 0;

	metadata_off -= sizeof(struct emds_snapshot_metadata);
	metadata = snapshot_make(&partition[partition_index], metadata, metadata_off, true, true,
				 fresh_cnt);
	zassert_not_null(metadata, "Failed to create snapshot on partition %d", partition_index);

	for (int i = 0; i < CONFIG_EMDS_MAX_CANDIDATES; i++) {
		metadata_off -= sizeof(struct emds_snapshot_metadata);
		fresh_cnt++;
		metadata = snapshot_make(&partition[partition_index], metadata, metadata_off, true,
					 false, fresh_cnt);
		zassert_not_null(metadata, "Failed to create snapshot on partition %d",
				 partition_index);
	}

	for (int i = 0; i <= CONFIG_EMDS_SCANNING_FAILURES; i++) {
		metadata_off -= sizeof(struct emds_snapshot_metadata);
		fresh_cnt++;
		metadata = snapshot_make(&partition[partition_index], metadata, metadata_off, false,
					 true, fresh_cnt);
		zassert_not_null(metadata, "Failed to create snapshot on partition %d",
				 partition_index);
	}

	fresh_cnt++;
	metadata_off -= sizeof(struct emds_snapshot_metadata);
	metadata = snapshot_make(&partition[partition_index], metadata, metadata_off, true, true,
				 fresh_cnt);
	zassert_not_null(metadata, "Failed to create snapshot on partition %d", partition_index);

	zassert_ok(emds_flash_scan_partition(&partition[partition_index], &candidate),
		   "Failed to scan partition %d", partition_index);
	zassert_equal(candidate.metadata.fresh_cnt, 0, "Fresh count mismatch");
}

/* Test checks allocation snapshot on the empty partition */
ZTEST(emds_flash, test_allocation_empty_partition)
{
	int partition_index = sys_rand32_get() % PARTITIONS_NUM_MAX;
	struct emds_snapshot_candidate allocated_snapshot = {
		.partition_index = partition_index,
		.metadata.fresh_cnt = 1
	};

	zassert_ok(emds_flash_allocate_snapshot(&partition[partition_index], NULL,
						&allocated_snapshot, 100),
		   "Failed to allocate snapshot on partition %d", partition_index);

	zassert_equal(allocated_snapshot.partition_index, partition_index,
		      "Partition index is not equal to the requested one");
	zassert_equal(allocated_snapshot.metadata_off,
		      partition[partition_index].fa->fa_size -
			      sizeof(struct emds_snapshot_metadata),
		      "Metadata offset is not equal to the end of the partition");
	zassert_equal(allocated_snapshot.metadata.data_instance_off, 0,
		      "Data instance offset is not equal to 0 for the empty partition");
	zassert_equal(allocated_snapshot.metadata.data_instance_len, 100,
		      "Data instance length is not equal to requested for the empty partition");
	zassert_equal(allocated_snapshot.metadata.fresh_cnt, 1,
		      "Fresh count is not equal to 1 for the empty partition");
	zassert_equal(allocated_snapshot.metadata.metadata_crc,
		      crc32_k_4_2_update(0, (const unsigned char *)&allocated_snapshot.metadata,
					 offsetof(struct emds_snapshot_metadata, metadata_crc)),
		      "Metadata CRC is not equal to calculated for the empty partition");
}

/* Test checks allocation snapshot on the same partition after the freshest one. */
ZTEST(emds_flash, test_allocation_after_freshest_same_partition)
{
	struct emds_snapshot_candidate freshest_snapshot;
	int partition_index = sys_rand32_get() % PARTITIONS_NUM_MAX;
	struct emds_snapshot_candidate allocated_snapshot = {
		.partition_index = partition_index,
		.metadata.fresh_cnt = 2
	};
	struct emds_snapshot_metadata *metadata = NULL;
	off_t metadata_off =
		partition[partition_index].fa->fa_size - sizeof(struct emds_snapshot_metadata);
	uint32_t fresh_cnt = 1;

	metadata = snapshot_make(&partition[partition_index], metadata, metadata_off, true, true,
				 fresh_cnt);
	zassert_not_null(metadata, "Failed to create snapshot on partition %d", partition_index);
	freshest_snapshot.metadata_off = metadata_off;
	freshest_snapshot.metadata = *metadata;

	zassert_ok(emds_flash_allocate_snapshot(&partition[partition_index], &freshest_snapshot,
						&allocated_snapshot, 100),
		   "Failed to allocate snapshot on partition %d", partition_index);

	zassert_equal(allocated_snapshot.partition_index, partition_index,
		      "Partition index is not equal to the requested one");
	zassert_equal(allocated_snapshot.metadata_off,
		      partition[partition_index].fa->fa_size -
			      2 * sizeof(struct emds_snapshot_metadata),
		      "Metadata offset is not equal to expected");
	zassert_equal(allocated_snapshot.metadata.data_instance_off,
		      ROUND_UP(metadata->data_instance_len,
			       partition[partition_index].fp->write_block_size),
		      "Data instance offset is not equal to expected");
	zassert_equal(allocated_snapshot.metadata.data_instance_len, 100,
		      "Data instance length is not equal to requested");
	zassert_equal(allocated_snapshot.metadata.fresh_cnt, 2, "Fresh count is not equal to 2");
	zassert_equal(allocated_snapshot.metadata.metadata_crc,
		      crc32_k_4_2_update(0, (const unsigned char *)&allocated_snapshot.metadata,
					 offsetof(struct emds_snapshot_metadata, metadata_crc)),
		      "Metadata CRC is not equal to calculated for the empty partition");
}

/* Test checks allocation snapshot with data size larger than partition size.
 */
ZTEST(emds_flash, test_allocation_data_larger_than_partition)
{
	int partition_index = sys_rand32_get() % PARTITIONS_NUM_MAX;
	size_t data_size = partition[partition_index].fa->fa_size + 100;
	struct emds_snapshot_candidate allocated_snapshot;

	zassert_equal(emds_flash_allocate_snapshot(&partition[partition_index], NULL,
						   &allocated_snapshot, data_size),
		      -EINVAL,
		      "Expected -EINVAL when trying to allocate snapshot with data size larger "
		      "than partition size");
}

/* Test checks allocation snapshot on partition if partition with the freshest snapshot is full.
 */
ZTEST(emds_flash, test_allocation_next_partition_if_freshest_full)
{
	struct emds_snapshot_candidate freshest_snapshot;
	int partition_index = sys_rand32_get() % PARTITIONS_NUM_MAX;
	struct emds_snapshot_candidate allocated_snapshot;
	struct emds_snapshot_metadata *metadata = NULL;
	struct emds_snapshot_metadata metadata_prev;
	off_t metadata_off = partition[partition_index].fa->fa_size;
	off_t metadata_off_prev;
	uint32_t fresh_cnt = 0;
	int rc;

	do {
		metadata_off_prev = metadata_off;
		metadata_off -= sizeof(struct emds_snapshot_metadata);
		fresh_cnt++;
		metadata_prev = metadata ? *metadata : (struct emds_snapshot_metadata){0};
		metadata = snapshot_make(&partition[partition_index], metadata, metadata_off, true,
					 true, fresh_cnt);
	} while (metadata != NULL);
	freshest_snapshot.metadata_off = metadata_off_prev;
	freshest_snapshot.metadata = metadata_prev;

	rc = emds_flash_allocate_snapshot(&partition[partition_index], &freshest_snapshot,
					  &allocated_snapshot, 200);
	zassert_equal(rc, -ENOMEM,
		      "Expected -ENOMEM when trying to allocate snapshot on full partition %d rc: %d",
		      partition_index, rc);
}

/* Test checks allocation snapshot if partition has garbage in metadata place. */
ZTEST(emds_flash, test_allocation_if_metadata_garbaged)
{
	int partition_index = sys_rand32_get() % PARTITIONS_NUM_MAX;
	struct emds_snapshot_candidate allocated_snapshot = {
		.partition_index = partition_index,
		.metadata.fresh_cnt = 1
	};
	const struct flash_area *fa = partition[partition_index].fa;
	const struct flash_parameters *fp = partition[partition_index].fp;
	uint8_t garbage[sizeof(struct emds_snapshot_metadata)];
	int rc;

	for (int i = 0; i < sizeof(garbage); i++) {
		garbage[i] = sys_rand32_get() % 256;
	}

	zassert_ok(flash_area_write(fa, fa->fa_size - sizeof(garbage), &garbage, sizeof(garbage)),
		   "Failed to write garbage into metadata flash area");

	rc = emds_flash_allocate_snapshot(&partition[partition_index], NULL, &allocated_snapshot,
					  100);
	if (flash_params_get_erase_cap(fp) & FLASH_ERASE_C_EXPLICIT) {
		zassert_equal(rc, -EADDRINUSE,
			      "Expected -EADDRINUSE when trying to allocate snapshot on partition "
			      "with garbage in metadata if memory has explicit erase");
		return;
	}

	zassert_ok(rc, "Expected 0 when trying to allocate snapshot on partition with "
		       "garbage in metadata if memory does not have explicit erase");

	zassert_equal(allocated_snapshot.partition_index, partition_index,
		      "Partition index is not equal to the requested one");
	zassert_equal(allocated_snapshot.metadata_off,
		      fa->fa_size - sizeof(struct emds_snapshot_metadata),
		      "Metadata offset is not equal to the end of the partition");
	zassert_equal(allocated_snapshot.metadata.data_instance_off, 0,
		      "Data instance offset is not equal to 0");
	zassert_equal(allocated_snapshot.metadata.data_instance_len, 100,
		      "Data instance length is not equal to requested one");
	zassert_equal(allocated_snapshot.metadata.fresh_cnt, 1, "Fresh count is not equal to 1");
	zassert_equal(allocated_snapshot.metadata.metadata_crc,
		      crc32_k_4_2_update(0, (const unsigned char *)&allocated_snapshot.metadata,
					 offsetof(struct emds_snapshot_metadata, metadata_crc)),
		      "Metadata CRC is not equal to calculated one");
}

/* Test checks allocation snapshot if partition has garbage in data place. */
ZTEST(emds_flash, test_allocation_if_data_garbaged)
{
	int partition_index = sys_rand32_get() % PARTITIONS_NUM_MAX;
	struct emds_snapshot_candidate allocated_snapshot = {
		.partition_index = partition_index,
		.metadata.fresh_cnt = 1
	};
	const struct flash_area *fa = partition[partition_index].fa;
	const struct flash_parameters *fp = partition[partition_index].fp;
	uint8_t garbage[sizeof(struct emds_data_entry)];
	int rc;

	for (int i = 0; i < sizeof(garbage); i++) {
		garbage[i] = sys_rand32_get() % 256;
	}

	zassert_ok(flash_area_write(fa, 0, &garbage, sizeof(garbage)),
		   "Failed to write garbage into metadata flash area");

	rc = emds_flash_allocate_snapshot(&partition[partition_index], NULL, &allocated_snapshot,
					  100);
	if (flash_params_get_erase_cap(fp) & FLASH_ERASE_C_EXPLICIT) {
		zassert_equal(rc, -EADDRINUSE,
			      "Expected -EADDRINUSE when trying to allocate snapshot on partition "
			      "with garbage in data area if memory has explicit erase");
		return;
	}

	zassert_ok(rc, "Expected 0 when trying to allocate snapshot on partition with "
		       "garbage in data area if memory does not have explicit erase");

	zassert_equal(allocated_snapshot.partition_index, partition_index,
		      "Partition index is not equal to the requested one");
	zassert_equal(allocated_snapshot.metadata_off,
		      fa->fa_size - sizeof(struct emds_snapshot_metadata),
		      "Metadata offset is not equal to the end of the partition");
	zassert_equal(allocated_snapshot.metadata.data_instance_off, 0,
		      "Data instance offset is not equal to 0");
	zassert_equal(allocated_snapshot.metadata.data_instance_len, 100,
		      "Data instance length is not equal to requested one");
	zassert_equal(allocated_snapshot.metadata.fresh_cnt, 1, "Fresh count is not equal to 1");
	zassert_equal(allocated_snapshot.metadata.metadata_crc,
		      crc32_k_4_2_update(0, (const unsigned char *)&allocated_snapshot.metadata,
					 offsetof(struct emds_snapshot_metadata, metadata_crc)),
		      "Metadata CRC is not equal to calculated one");
}

/* Test measures write timings. */
ZTEST(emds_flash, test_write_speed)
{
	int partition_index = sys_rand32_get() % PARTITIONS_NUM_MAX;
	const struct flash_area *fa = partition[partition_index].fa;
	static uint8_t data_in[EMDS_FLASH_BLOCK_SIZE];
	static uint8_t data_in_big[1024];
	static uint8_t read_back[1024] = {0};
	uint32_t start_us, end_us, elapsed_us;
	off_t data_off = 0;

	memset(data_in, 69, sizeof(data_in));
	memset(data_in_big, 42, sizeof(data_in_big));

	(void)dk_leds_init();
	(void)dk_set_led(0, false);

	/* Initialize microsecond timer */
	microsecond_timer_init();

	dk_set_led(0, true);
	start_us = get_time_us();
	emds_flash_write_data(&partition[partition_index], data_off, data_in, sizeof(data_in));
	end_us = get_time_us();
	dk_set_led(0, false);
	elapsed_us = end_us - start_us;
	printk("Storing %d bytes took %uus\n", sizeof(data_in), elapsed_us);

	zassert_true(elapsed_us < EXPECTED_STORE_TIME_BLOCK_SIZE,
		     "Storing %d bytes took too long time", sizeof(data_in));

	/* Verify data integrity */
	zassert_ok(flash_area_read(fa, data_off, read_back, sizeof(data_in)));
	zassert_mem_equal(read_back, data_in, sizeof(data_in));

	/* Large data test */
	dk_set_led(0, true);
	start_us = get_time_us();
	emds_flash_write_data(&partition[partition_index], data_off + sizeof(data_in), data_in_big,
			      sizeof(data_in_big));
	end_us = get_time_us();
	dk_set_led(0, false);
	elapsed_us = end_us - start_us;
	printk("Storing %d bytes took %uus\n", sizeof(data_in_big), elapsed_us);

	zassert_true(elapsed_us < EXPECTED_STORE_TIME_1024, "Storing %d bytes took too long time",
		     sizeof(data_in_big));

	/* Verify large data */
	zassert_ok(flash_area_read(fa, data_off + sizeof(data_in), read_back, sizeof(data_in_big)));
	zassert_mem_equal(read_back, data_in_big, sizeof(data_in_big));

	microsecond_timer_cleanup();
}

ZTEST_SUITE(emds_flash, NULL, emds_flash_setup, NULL, emds_flash_partitions_erase, NULL);
