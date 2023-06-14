/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/crc.h>
#include <dk_buttons_and_leds.h>
#include <emds_flash.h>

#if defined(CONFIG_BT)
#include <mpsl.h>
#include <sdc.h>
#endif

#define ADDR_OFFS_MASK 0x0000FFFF

static struct {
	const struct device *fd;
	uint32_t offset;
	uint32_t size;
	uint32_t ate_idx_start;
	uint32_t data_wra_offset;
} m_test_fd;

/* Allocation Table Entry */
struct test_ate {
	uint16_t id;       /* data id */
	uint16_t offset;   /* data offset within sector */
	uint16_t len;      /* data len within sector */
	uint8_t crc8_data; /* crc8 check of the entry */
	uint8_t crc8;      /* crc8 check of the entry */
} __packed;

static struct emds_fs ctx;
static uint16_t m_sec_size;
static uint16_t m_sec_cnt;
static const struct flash_area *m_fa;

/** Local functions ***********************************************************/

static int flash_block_cmp(uint32_t addr, const void *data, size_t len)
{
	const uint8_t *data8 = (const uint8_t *)data;
	int rc;
	size_t bytes_to_cmp, block_size;
	uint8_t buf[32];

	block_size = sizeof(buf) & ~(4 - 1U);

	while (len) {
		bytes_to_cmp = MIN(block_size, len);
		rc = flash_read(m_test_fd.fd, addr, buf, bytes_to_cmp);
		if (rc) {
			return rc;
		}
		rc = memcmp(data8, buf, bytes_to_cmp);
		if (rc) {
			return -ENOTSUP;
		}

		len -= bytes_to_cmp;
		addr += bytes_to_cmp;
		data8 += bytes_to_cmp;
	}

	return 0;
}

static int flash_cmp_const(uint32_t addr, uint8_t value, size_t len)
{
	int rc;
	size_t bytes_to_cmp, block_size;
	uint8_t cmp[32];

	block_size = sizeof(cmp) & ~(4 - 1U);

	memset(cmp, value, block_size);
	while (len) {
		bytes_to_cmp = MIN(block_size, len);
		rc = flash_block_cmp(addr, cmp, bytes_to_cmp);

		if (rc) {
			return rc;
		}
		len -= bytes_to_cmp;
		addr += bytes_to_cmp;
	}
	return 0;
}

static inline size_t align_size(size_t len)
{
	return (len + (4 - 1U)) & ~(4 - 1U);
}

static int data_write(const void *data, size_t len)
{
	const uint8_t *data8 = (const uint8_t *)data;
	int rc = 0;
	off_t offset;
	size_t blen;
	size_t temp_len = len;
	uint8_t buf[32];

	if (!temp_len) {
		/* Nothing to write, avoid changing the flash protection */
		return 0;
	}

	offset = m_test_fd.offset;
	offset += m_test_fd.data_wra_offset & ADDR_OFFS_MASK;

	blen = temp_len & ~(4 - 1U);

	/* Writes multiples of 4 bytes to flash */
	if (blen > 0) {
		rc = flash_write(m_test_fd.fd, offset, data8, blen);
		if (rc) {
			/* flash write error */
			goto end;
		}
		temp_len -= blen;
		offset += blen;
		data8 += blen;
	}

	/* Writes remaining (len < 4) bytes to flash */
	/* Should NEVER be used for ATE!? */
	if (temp_len) {
		memcpy(buf, data8, temp_len);
		memset(buf + temp_len, 0, 4 - temp_len);

		rc = flash_write(m_test_fd.fd, offset, buf, 4);
	}

end:
	m_test_fd.data_wra_offset += align_size(len);
	return rc;

}

static void *fs_init(void)
{
	int rc;
	uint32_t sector_cnt = 1;
	struct flash_sector hw_flash_sector;
	size_t emds_flash_size = 0;
	uint16_t cnt = 0;

	rc = flash_area_open(FIXED_PARTITION_ID(emds_storage), &m_fa);
	__ASSERT(rc == 0, "Failed opening flash area (err:%d)", rc);

	rc = flash_area_get_sectors(FIXED_PARTITION_ID(emds_storage), &sector_cnt,
				    &hw_flash_sector);

	__ASSERT(rc == 0, "Failed when getting sector information (err:%d", rc);
	__ASSERT(hw_flash_sector.fs_size <= UINT16_MAX, "fs_size (%u) exceeds UINT16_MAX",
		 hw_flash_sector.fs_size);

	emds_flash_size += hw_flash_sector.fs_size;
	if (emds_flash_size <= m_fa->fa_size) {
		cnt++;
	}

	m_sec_size = hw_flash_sector.fs_size;
	m_sec_cnt = cnt;
	ctx.offset = m_fa->fa_off;
	ctx.sector_cnt = m_sec_cnt;
	ctx.sector_size = m_sec_size;

	m_test_fd.fd = m_fa->fa_dev;
	m_test_fd.offset = m_fa->fa_off;
	m_test_fd.size = hw_flash_sector.fs_size;
	m_test_fd.ate_idx_start = m_fa->fa_off + hw_flash_sector.fs_size - sizeof(struct test_ate);
	m_test_fd.data_wra_offset = 0;

	return NULL;
}

static int flash_clear(void)
{
	m_test_fd.data_wra_offset = 0;
	return flash_erase(m_test_fd.fd, m_test_fd.offset, m_test_fd.size);
}

static int entry_write(uint32_t ate_wra, uint16_t id, const void *data, size_t len)
{
	struct test_ate entry = {
		.id = id,
		.offset = m_test_fd.data_wra_offset,
		.len = (uint16_t)len,
		.crc8_data = crc8_ccitt(0xff, data, len),
		.crc8 = crc8_ccitt(0xff, &entry, offsetof(struct test_ate, crc8)),
	};

	data_write(data, len);
	return flash_write(m_test_fd.fd, ate_wra, &entry, sizeof(struct test_ate));
}

static int ate_invalidate_write(uint32_t ate_wra)
{
	uint8_t invalid[8];

	memset(invalid, 0, sizeof(invalid));

	return flash_write(m_test_fd.fd, ate_wra, invalid, sizeof(invalid));
}

static int ate_corrupt_write(uint32_t ate_wra)
{
	uint8_t corrupt[8];

	memset(corrupt, 69, sizeof(corrupt));

	return flash_write(m_test_fd.fd, ate_wra, corrupt, sizeof(corrupt));
}

static int corrupt_write_all(void)
{
	uint8_t corrupt[32];

	memset(corrupt, 69, sizeof(corrupt));

	for (uint32_t i = m_test_fd.offset; i < m_test_fd.offset + m_test_fd.size; i += 32) {
		flash_write(m_test_fd.fd, i, corrupt, sizeof(corrupt));
	}

	return 0;
}

static void device_reset(void)
{
	memset(&ctx, 0, sizeof(ctx));
	ctx.sector_cnt = m_sec_cnt;
	ctx.sector_size = m_sec_size;
	ctx.offset = m_fa->fa_off;
	ctx.flash_dev = m_fa->fa_dev;
}

/** End Local functions *******************************************************/

ZTEST(emds_flash_tests, test_initialize)
{
	/* Check that device inits on first attempt, and rejects init on consecutive attempts */
	flash_clear();
	device_reset();

	zassert_false(emds_flash_init(&ctx), "Error when initializing");
	zassert_true(emds_flash_init(&ctx), "Should not init more than once");
}

ZTEST(emds_flash_tests, test_rd_wr_simple)
{
	/* Init the device, performs prepare and does a write and read.
	 * Verifies that entry is valid
	 */
	uint8_t data_in[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	uint8_t data_out[8] = { 0 };

	flash_clear();
	device_reset();

	zassert_false(emds_flash_init(&ctx), "Error when initializing");
	zassert_false(emds_flash_prepare(&ctx, sizeof(data_in) + ctx.ate_size), "Prepare failed");
	zassert_false(emds_flash_write(&ctx, 1, data_in, sizeof(data_in)) < 0, "Error when write");
	zassert_false(emds_flash_read(&ctx, 1, data_out, sizeof(data_out)) < 0, "Error when read");
	zassert_false(memcmp(data_out, data_in, sizeof(data_out)), "Retrived wrong value");
}

ZTEST(emds_flash_tests, test_flash_recovery)
{
	char data_in1[9] = "Deadbeef";
	char data_in2[8] = "Beafded";
	char data_in3[13] = "Deadbeefface";
	char data_out[32] = {0};
	uint32_t idx;

	flash_clear();
	device_reset();

	/* Entries:
	 *    #1: Empty
	 *    #2: Empty
	 *    #3: Empty
	 *    #4: Empty
	 *    #5: Empty
	 *    #6: Empty
	 *    ...
	 * Expect: Normal behavior
	 */
	zassert_false(emds_flash_init(&ctx), "Error when initializing");
	zassert_false(ctx.force_erase, "Expected false");
	zassert_equal(m_test_fd.ate_idx_start, ctx.ate_wra, "%X not equal to %X",
		      m_test_fd.ate_idx_start, ctx.ate_wra);
	device_reset();

	/* Entries:
	 *     #1: Valid
	 *     #2: Empty
	 *     #3: Empty
	 *     #4: Empty
	 *     #5: Empty
	 *     #6: Empty
	 *     ...
	 * Expect: Normal behavior
	 */
	idx = m_test_fd.ate_idx_start;
	entry_write(idx, 1, data_in1, sizeof(data_in1));
	idx -= sizeof(struct test_ate);
	zassert_false(emds_flash_init(&ctx), "Error when initializing");
	zassert_true(emds_flash_read(&ctx, 1, data_out, sizeof(data_in1)) > 0, "Could not read");
	(void)emds_flash_read(&ctx, 1, data_out, sizeof(data_in1));
	zassert_false(ctx.force_erase, "Expected false");
	zassert_equal(idx, ctx.ate_wra, "Addr not equal");
	zassert_false(memcmp(data_in1, data_out, sizeof(data_in1)), "Not same data");
	device_reset();

	/* Entries:
	 *     #1: Inval
	 *     #2: Valid
	 *     #3: Empty
	 *     #4: Empty
	 *     #5: Empty
	 *     #6: Empty
	 *     ...
	 * Expect: Normal behavior
	 */
	idx = m_test_fd.ate_idx_start;
	ate_invalidate_write(idx);
	idx -= sizeof(struct test_ate);
	entry_write(idx, 2, data_in2, sizeof(data_in2));
	idx -= sizeof(struct test_ate);
	zassert_false(emds_flash_init(&ctx), "Error when initializing");
	zassert_false(ctx.force_erase, "Expected false");
	zassert_equal(idx, ctx.ate_wra, "Addr not equal");
	zassert_true(emds_flash_read(&ctx, 2, data_out, sizeof(data_in2)) > 0, "Could not read");
	zassert_false(memcmp(data_in2, data_out, sizeof(data_in2)), "Not same data");
	device_reset();

	/* Entries:
	 *     #1: Inval
	 *     #2: Valid
	 *     #3: Inval
	 *     #4: Valid
	 *     #5: Empty
	 *     #6: Empty
	 *     ...
	 * Expect:  Unexpected behavior
	 */
	ate_invalidate_write(idx);
	idx -= sizeof(struct test_ate);
	entry_write(idx, 3, data_in3, sizeof(data_in3));
	idx -= sizeof(struct test_ate);
	zassert_false(emds_flash_init(&ctx), "Error when initializing");
	zassert_true(ctx.force_erase, "Expected true");
	zassert_equal(idx, ctx.ate_wra, "Addr not equal");
	zassert_true(emds_flash_read(&ctx, 2, data_out, sizeof(data_in2)) > 0, "Could not read");
	zassert_false(memcmp(data_in2, data_out, sizeof(data_in2)), "Not same data");
	zassert_true(emds_flash_read(&ctx, 3, data_out, sizeof(data_in3)) > 0, "Could not read");
	zassert_false(memcmp(data_in3, data_out, sizeof(data_in3)), "Not same data");
	device_reset();

	/* Entries:
	 *     #1: Inval
	 *     #2: Inval
	 *     #3: Inval
	 *     #4: Valid
	 *     #5: Corrupt
	 *     #6: Empty
	 *     ...
	 * Expect:  Unexpected behavior.
	 *     - Erase flag raised.
	 *     - Should be able to recover valid entry
	 */
	idx = m_test_fd.ate_idx_start;
	for (size_t i = 0; i < 3; i++) {
		ate_invalidate_write(idx);
		idx -= sizeof(struct test_ate);
	}

	idx -= sizeof(struct test_ate);
	ate_corrupt_write(idx);
	idx -= sizeof(struct test_ate);
	zassert_false(emds_flash_init(&ctx), "Error when initializing");
	zassert_true(ctx.force_erase, "Expected true");
	zassert_equal(idx, ctx.ate_wra, "Addr not equal");
	zassert_true(emds_flash_read(&ctx, 3, data_out, sizeof(data_in3)) > 0, "Could not read");
	zassert_false(memcmp(data_in3, data_out, sizeof(data_in3)), "Not same data");
}

ZTEST(emds_flash_tests, test_flash_recovery_corner_case)
{
	flash_clear();
	device_reset();

	/* Fill flash with garbage. Expect 0 space left and erase on prepare */
	corrupt_write_all();
	zassert_false(emds_flash_init(&ctx), "Error when initializing");
	zassert_true(ctx.force_erase, "Force erase should be true");
	zassert_equal(0, emds_flash_free_space_get(&ctx), "Expected no free space");
	device_reset();
}


ZTEST(emds_flash_tests, test_invalidate_on_prepare)
{
	char data_in[9] = "Deadbeef";
	char data_out[9] = {0};

	flash_clear();
	device_reset();

	uint32_t idx = m_test_fd.ate_idx_start;

	for (size_t i = 0; i < 5; i++) {
		entry_write(idx, i, data_in, sizeof(data_in));
		idx -= sizeof(struct test_ate);
	}

	zassert_false(emds_flash_init(&ctx), "Error when initializing");
	zassert_false(ctx.force_erase, "Force erase should be false");
	zassert_equal(idx, ctx.ate_wra, "Addr not equal");

	for (size_t i = 0; i < 5; i++) {
		zassert_true(emds_flash_read(&ctx, i, data_out, sizeof(data_in)) > 0,
			     "Could not read");
		zassert_false(memcmp(data_in, data_out, sizeof(data_in)), "Not same data");
		memset(data_out, 0, sizeof(data_out));
	}

	zassert_false(emds_flash_prepare(&ctx, 0), "Error when preparing");

	device_reset();
	zassert_false(emds_flash_init(&ctx), "Error when initializing");

	/* Expect the free storage integrity to fail due to no valid ATE and invalid presence.
	 * This occurs only if the user does not store any new entry after emds_flash_prepare
	 */
	zassert_true(ctx.force_erase, "Expected true");
	for (size_t i = 0; i < 5; i++) {
		zassert_false(emds_flash_read(&ctx, i, data_out, sizeof(data_in)) > 0,
			      "Should not be able to read");
	}
	zassert_false(emds_flash_prepare(&ctx, 0), "Error when preparing");

	/* Reset again without writing */
	device_reset();

	/* If the device yet again runs emds_flash_prepare without writing the flash should
	 * be in a clean state
	 */
	zassert_false(ctx.force_erase, "Force erase should be false");
}

ZTEST(emds_flash_tests, test_clear_on_strange_flash)
{
	flash_clear();
	device_reset();

	/* Entries:
	 *     #1: Corrupt
	 *     #2: Empty
	 *     #3: Empty
	 *     ...
	 * Expect: Unexpected behavior
	 */
	ate_corrupt_write(m_test_fd.ate_idx_start);
	zassert_false(emds_flash_init(&ctx), "Error when initializing");
	zassert_true(ctx.force_erase, "Force erase should be true");
	zassert_false(emds_flash_prepare(&ctx, 0), "Error when preparing");
	zassert_false(flash_cmp_const(m_test_fd.offset, 0xff, m_test_fd.size), "Flash not cleared");
}

ZTEST(emds_flash_tests, test_permission)
{
	char data_in[8] = "Deadbeef";
	char data_out[8] = {0};

	flash_clear();
	device_reset();

	zassert_true(emds_flash_prepare(&ctx, sizeof(data_in) + ctx.ate_size) == -EACCES,
		     "Prepare did not fail");
	zassert_true(emds_flash_read(&ctx, 1, data_out, sizeof(data_in)) == -EACCES,
		     "Should not be able to read");
	zassert_true(emds_flash_write(&ctx, 1, data_in, sizeof(data_in)) == -EACCES,
		     "Should not be able to read");

	zassert_false(emds_flash_init(&ctx), "Error when initializing");

	zassert_true(emds_flash_write(&ctx, 1, data_in, sizeof(data_in)) == -EACCES,
		     "Should not be able to read");

	zassert_false(emds_flash_prepare(&ctx, sizeof(data_in) + ctx.ate_size), "Prepare failed");

	zassert_true(emds_flash_write(&ctx, 1, data_in, sizeof(data_in)) == sizeof(data_in),
				      "Should be able to write");
	zassert_true(emds_flash_read(&ctx, 1, data_out, sizeof(data_in)) == sizeof(data_in),
				     "Should be able to read");
	zassert_false(memcmp(data_out, data_in, sizeof(data_out)), "Retrived wrong value");

}

ZTEST(emds_flash_tests, test_overflow)
{
	char data_in[8] = "Deadbee";
	char data_out[8] = {0};

	flash_clear();
	device_reset();

	zassert_false(emds_flash_init(&ctx), "Error when initializing");
	zassert_false(emds_flash_prepare(&ctx, sizeof(data_in) + ctx.ate_size), "Prepare failed");

	uint16_t test_cnt = 0;

	while (emds_flash_free_space_get(&ctx)) {
		emds_flash_write(&ctx, test_cnt, data_in, sizeof(data_in));
		data_in[0]++;
		test_cnt++;
	}

	/* Try to write to full flash */
	zassert_true(emds_flash_write(&ctx, 1, data_in, sizeof(data_in)) < 0,
				      "Should not be able to write");

	device_reset();
	zassert_false(emds_flash_init(&ctx), "Error when initializing");

	data_in[0] = 'D';
	for (size_t i = 0; i < test_cnt; i++) {
		(void)emds_flash_read(&ctx, i, data_out, sizeof(data_out));
		zassert_false(memcmp(data_out, data_in, sizeof(data_out)), "Retrived wrong value");
		memset(data_out, 0, sizeof(data_out));
		data_in[0]++;
	}


	zassert_false(emds_flash_prepare(&ctx, sizeof(data_in) + ctx.ate_size), "Prepare failed");

	data_in[0] = 'D';
	emds_flash_write(&ctx, 0, data_in, sizeof(data_in));

	device_reset();
	zassert_false(emds_flash_init(&ctx), "Error when initializing");

	emds_flash_read(&ctx, 0, data_out, sizeof(data_out));
	zassert_false(memcmp(data_out, data_in, sizeof(data_out)), "Retrived wrong value");

	zassert_equal(emds_flash_free_space_get(&ctx),
		      m_test_fd.size - (sizeof(data_out) + sizeof(struct test_ate) * 2), "");
}


ZTEST(emds_flash_tests, test_prepare_overflow)
{
	flash_clear();
	device_reset();

	zassert_false(emds_flash_init(&ctx), "Error when initializing");
	zassert_true(emds_flash_prepare(&ctx, m_test_fd.size), "Prepare should return error");
	zassert_false(emds_flash_prepare(&ctx, m_test_fd.size - 16), "Prepare failed");
	zassert_false(emds_flash_prepare(&ctx, m_test_fd.size - 24), "Prepare failed");
}

ZTEST(emds_flash_tests, test_full_corrupt_recovery)
{
	flash_clear();
	device_reset();

	/* Fill flash with garbage. Expect 0 space left and erase on prepare */
	corrupt_write_all();
	zassert_false(emds_flash_init(&ctx), "Error when initializing");
	zassert_true(ctx.force_erase, "Force erase should be true");
	zassert_equal(0, emds_flash_free_space_get(&ctx), "Expected no free space");

	zassert_false(emds_flash_prepare(&ctx, 0), "Prepare failed");
	zassert_equal(m_test_fd.size - sizeof(struct test_ate), emds_flash_free_space_get(&ctx),
		      "Expected no free space");

	zassert_false(flash_cmp_const(m_test_fd.offset, 0xff, m_test_fd.size), "Flash not cleared");
	device_reset();
}

ZTEST(emds_flash_tests, test_corrupted_data)
{
	char corrupt[4] = {0};
	char data_in[8] = "Deadbee";
	char data_out[8] = {0};

	flash_clear();
	device_reset();

	zassert_false(emds_flash_init(&ctx), "Error when initializing");
	zassert_false(emds_flash_prepare(&ctx, sizeof(data_in) + ctx.ate_size), "Prepare failed");

	zassert_true(emds_flash_write(&ctx, 1, data_in, sizeof(data_in)) == sizeof(data_in),
				      "Should be able to write");

	zassert_true(emds_flash_read(&ctx, 1, data_out, sizeof(data_in)) == sizeof(data_in),
				     "Should be able to read");

	/* Corrupt data entry */
	flash_write(m_test_fd.fd, m_test_fd.data_wra_offset + m_test_fd.offset, corrupt,
		    sizeof(corrupt));

	/* Reset */
	device_reset();
	zassert_false(emds_flash_init(&ctx), "Error when initializing");

	zassert_true(emds_flash_read(&ctx, 1, data_out, sizeof(data_in)) < 0,
				     "Should not be able to read");
}

ZTEST(emds_flash_tests, test_write_speed)
{
	char data_in[4] = "bee";
	uint8_t data_in_big[1024];
	int64_t tic;
	int64_t toc;
	uint64_t store_time_us;

	memset(data_in_big, 69, sizeof(data_in_big));

	(void)dk_leds_init();
	(void)dk_set_led(0, false);
	(void)flash_clear();
	device_reset();

#if defined(CONFIG_BT)
	/* This is done to turn off mpsl scheduler to speed up storage time. */
	(void)sdc_disable();
	mpsl_uninit();
#endif
	(void)emds_flash_init(&ctx);
	(void)emds_flash_prepare(&ctx, sizeof(data_in) + ctx.ate_size);

	tic = k_uptime_ticks();
	dk_set_led(0, true);
	emds_flash_write(&ctx, 1, data_in, sizeof(data_in));
	dk_set_led(0, false);
	toc = k_uptime_ticks();
	store_time_us = k_ticks_to_us_ceil64(toc - tic);
	printk("Storing 4 bytes took: %lldus\n", store_time_us);
	zassert_true(store_time_us < 300, "Storing 4 bytes took to long time");

	dk_set_led(0, true);
	tic = k_uptime_ticks();
	emds_flash_write(&ctx, 2, data_in_big, sizeof(data_in_big));
	dk_set_led(0, false);
	toc = k_uptime_ticks();
	store_time_us = k_ticks_to_us_ceil64(toc - tic);
	printk("Storing 1024 bytes took: %lldus\n", store_time_us);
	zassert_true(store_time_us < 13000, "Storing 1024 bytes took to long time");
}

ZTEST_SUITE(emds_flash_tests, NULL, fs_init, NULL, NULL, NULL);
