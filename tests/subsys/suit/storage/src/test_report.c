/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_storage.h>
#include <suit_plat_mem_util.h>

#if DT_NODE_EXISTS(DT_NODELABEL(cpusec_suit_storage))
#define SUIT_STORAGE_OFFSET	      FIXED_PARTITION_OFFSET(cpusec_suit_storage)
#define SUIT_STORAGE_SIZE	      FIXED_PARTITION_SIZE(cpusec_suit_storage)
#define SUIT_STORAGE_REPORT_SLOTS     1
#define SUIT_STORAGE_REPORT_SLOT_SIZE (160 - SUIT_STORAGE_REPORT_FLAG_SIZE)
#else
#define SUIT_STORAGE_OFFSET	      FIXED_PARTITION_OFFSET(suit_storage)
#define SUIT_STORAGE_SIZE	      FIXED_PARTITION_SIZE(suit_storage)
#define SUIT_STORAGE_REPORT_SLOTS     2
#define SUIT_STORAGE_REPORT_SLOT_SIZE (0x1000 - SUIT_STORAGE_REPORT_FLAG_SIZE)
#endif
#define SUIT_STORAGE_ADDRESS	      suit_plat_mem_nvm_ptr_get(SUIT_STORAGE_OFFSET)
#define SUIT_STORAGE_REPORT_FLAG_SIZE sizeof(uint32_t)

static uint8_t sample_report[SUIT_STORAGE_REPORT_SLOT_SIZE + 1];

static void test_suite_before(void *f)
{
	/* Execute SUIT storage init, so the MPI area is copied into a backup region. */
	int err = suit_storage_init();

	zassert_equal(SUIT_PLAT_SUCCESS, err, "Failed to init and backup suit storage (%d)", err);

	/* Clear the whole nordic area */
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	zassert_not_null(fdev, "Unable to find a driver to erase storage area");

	err = flash_erase(fdev, SUIT_STORAGE_OFFSET, SUIT_STORAGE_SIZE);
	zassert_equal(0, err, "Unable to erase storage before test execution");

	err = suit_storage_init();
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Failed to initialize SUIT storage module (%d).",
		      err);
}

ZTEST_SUITE(suit_storage_report_tests, NULL, NULL, test_suite_before, NULL, NULL);

ZTEST(suit_storage_report_tests, test_empty_report_read)
{
	const uint8_t *buf = NULL;
	size_t len = 0;

	for (size_t i = 0; i < SUIT_STORAGE_REPORT_SLOTS; i++) {
		suit_plat_err_t ret = suit_storage_report_read(i, &buf, &len);

		zassert_equal(ret, SUIT_PLAT_ERR_NOT_FOUND,
			      "Storage is empty, but report availability is reported (0x%x, %d).",
			      buf, len);
	}
}

ZTEST(suit_storage_report_tests, test_empty_report_index_range)
{
	const uint8_t *buf = NULL;
	size_t len = 0;

	suit_plat_err_t ret = suit_storage_report_clear(SUIT_STORAGE_REPORT_SLOTS + 1);

	zassert_equal(ret, SUIT_PLAT_ERR_OUT_OF_BOUNDS,
		      "Out of bounds report clear not handled correctly (%d).", ret);

	ret = suit_storage_report_save(SUIT_STORAGE_REPORT_SLOTS + 1, NULL, 0);

	zassert_equal(ret, SUIT_PLAT_ERR_OUT_OF_BOUNDS,
		      "Out of bounds report save not handled correctly (%d).", ret);

	ret = suit_storage_report_read(SUIT_STORAGE_REPORT_SLOTS + 1, &buf, &len);

	zassert_equal(ret, SUIT_PLAT_ERR_OUT_OF_BOUNDS,
		      "Out of bounds report read not handled correctly (%d).", ret);
}

ZTEST(suit_storage_report_tests, test_empty_report_read_invalid_arg)
{
	const uint8_t *buf = NULL;
	size_t len = 0;

	suit_plat_err_t ret = suit_storage_report_read(0, NULL, NULL);

	zassert_equal(ret, SUIT_PLAT_ERR_INVAL,
		      "Invalid arguments (0, NULL, NULL) not detected (%d).", ret);

	ret = suit_storage_report_read(0, &buf, NULL);

	zassert_equal(ret, SUIT_PLAT_ERR_INVAL, "Invalid arguments (0, _, NULL) not detected (%d).",
		      ret);

	ret = suit_storage_report_read(0, NULL, &len);

	zassert_equal(ret, SUIT_PLAT_ERR_INVAL, "Invalid arguments (0, NULL, _) not detected (%d).",
		      ret);
}

ZTEST(suit_storage_report_tests, test_empty_report_readback_flag)
{
	const uint8_t *buf = NULL;
	size_t len = 0;

	for (size_t i = 0; i < SUIT_STORAGE_REPORT_SLOTS; i++) {
		suit_plat_err_t ret = suit_storage_report_clear(i);

		zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to clear report slot %d (%d).", i,
			      ret);

		ret = suit_storage_report_read(i, &buf, &len);

		zassert_equal(ret, SUIT_PLAT_ERR_NOT_FOUND, "Failed to clear slot %d (%d).", i,
			      ret);

		ret = suit_storage_report_save(i, NULL, 0);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to set empty report flag for slot %d (%d).", i, ret);

		ret = suit_storage_report_read(i, &buf, &len);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to read empty report flag for slot %d (%d).", i, ret);
		zassert_equal(buf, NULL, "Invalid buffer value for slot %d returned (0x%lx).", i,
			      buf);
		zassert_equal(len, 0, "Invalid buffer length for slot %d returned (0x%lx).", i,
			      len);
	}
}

ZTEST(suit_storage_report_tests, test_empty_report_readback_data)
{
	const uint8_t *buf = NULL;
	size_t len = 0;

	/* Fill buffer with sample data */
	for (size_t i = 0; i < SUIT_STORAGE_REPORT_SLOT_SIZE; i++) {
		sample_report[i] = i % 0x100;
	}

	for (size_t i = 0; i < SUIT_STORAGE_REPORT_SLOTS; i++) {
		suit_plat_err_t ret = suit_storage_report_clear(i);

		zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to clear report slot %d (%d).", i,
			      ret);

		ret = suit_storage_report_read(i, &buf, &len);

		zassert_equal(ret, SUIT_PLAT_ERR_NOT_FOUND, "Failed to clear slot %d (%d).", i,
			      ret);

		ret = suit_storage_report_save(i, sample_report, SUIT_STORAGE_REPORT_SLOT_SIZE);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to set report with data for slot %d (%d).", i, ret);

		ret = suit_storage_report_read(i, &buf, &len);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to read report with data for slot %d (%d).", i, ret);

		zassert_mem_equal(sample_report, buf, SUIT_STORAGE_REPORT_SLOT_SIZE,
				  "Corrupted report data received");
		zassert_not_equal(buf, NULL, "Invalid buffer value for slot %d returned (0x%lx).",
				  i, buf);
		zassert_equal(len, SUIT_STORAGE_REPORT_SLOT_SIZE,
			      "Invalid buffer length for slot %d returned (0x%lx).", i, len);
	}
}

ZTEST(suit_storage_report_tests, test_empty_report_readback_short_data)
{
	const uint8_t *buf = NULL;
	size_t len = 0;

	/* Set the buffer with default values */
	memset(sample_report, 0xff, SUIT_STORAGE_REPORT_SLOT_SIZE);

	/* Fill buffer with sample data */
	for (size_t i = 0; i < 0x16; i++) {
		sample_report[i] = i % 0x100;
	}

	for (size_t i = 0; i < SUIT_STORAGE_REPORT_SLOTS; i++) {
		suit_plat_err_t ret = suit_storage_report_clear(i);

		zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to clear report slot %d (%d).", i,
			      ret);

		ret = suit_storage_report_read(i, &buf, &len);

		zassert_equal(ret, SUIT_PLAT_ERR_NOT_FOUND, "Failed to clear slot %d (%d).", i,
			      ret);

		ret = suit_storage_report_save(i, sample_report, 0x16);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to set report with data for slot %d (%d).", i, ret);

		ret = suit_storage_report_read(i, &buf, &len);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to read report with data for slot %d (%d).", i, ret);

		zassert_mem_equal(sample_report, buf, SUIT_STORAGE_REPORT_SLOT_SIZE,
				  "Corrupted report data received");
		zassert_not_equal(buf, NULL, "Invalid buffer value for slot %d returned (0x%lx).",
				  i, buf);
		zassert_equal(len, SUIT_STORAGE_REPORT_SLOT_SIZE,
			      "Invalid buffer length for slot %d returned (0x%lx).", i, len);
	}
}

ZTEST(suit_storage_report_tests, test_empty_report_save_overflow)
{
	for (size_t i = 0; i < SUIT_STORAGE_REPORT_SLOTS; i++) {
		suit_plat_err_t ret =
			suit_storage_report_save(i, sample_report, sizeof(sample_report));

		zassert_equal(ret, SUIT_PLAT_ERR_INVAL, "Failed to detect slot %d overflow (%d).",
			      i, ret);
	}
}

ZTEST(suit_storage_report_tests, test_empty_report_overwrite_flag)
{
	const uint8_t *buf = NULL;
	size_t len = 0;

	/* Fill buffer with sample data */
	for (size_t i = 0; i < SUIT_STORAGE_REPORT_SLOT_SIZE; i++) {
		sample_report[i] = i % 0x100;
	}

	for (size_t i = 0; i < SUIT_STORAGE_REPORT_SLOTS; i++) {
		suit_plat_err_t ret =
			suit_storage_report_save(i, sample_report, SUIT_STORAGE_REPORT_SLOT_SIZE);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to set report with data for slot %d (%d).", i, ret);

		ret = suit_storage_report_read(i, &buf, &len);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to read report with data for slot %d (%d).", i, ret);
		zassert_mem_equal(sample_report, buf, SUIT_STORAGE_REPORT_SLOT_SIZE,
				  "Corrupted report data received");
		zassert_not_equal(buf, NULL, "Invalid buffer value for slot %d returned (0x%lx).",
				  i, buf);
		zassert_equal(len, SUIT_STORAGE_REPORT_SLOT_SIZE,
			      "Invalid buffer length for slot %d returned (0x%lx).", i, len);

		ret = suit_storage_report_save(i, NULL, 0);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to set empty report flag for slot %d (%d).", i, ret);

		ret = suit_storage_report_read(i, &buf, &len);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to read empty report flag for slot %d (%d).", i, ret);
		zassert_equal(buf, NULL, "Invalid buffer value for slot %d returned (0x%lx).", i,
			      buf);
		zassert_equal(len, 0, "Invalid buffer length for slot %d returned (0x%lx).", i,
			      len);
	}
}

ZTEST(suit_storage_report_tests, test_empty_report_overwrite_data)
{
	const uint8_t *buf = NULL;
	size_t len = 0;

	/* Fill buffer with sample data */
	for (size_t i = 0; i < SUIT_STORAGE_REPORT_SLOT_SIZE; i++) {
		sample_report[i] = i % 0x100;
	}

	for (size_t i = 0; i < SUIT_STORAGE_REPORT_SLOTS; i++) {
		suit_plat_err_t ret = suit_storage_report_save(i, NULL, 0);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to set empty report flag for slot %d (%d).", i, ret);

		ret = suit_storage_report_read(i, &buf, &len);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to read empty report flag for slot %d (%d).", i, ret);
		zassert_equal(buf, NULL, "Invalid buffer value for slot %d returned (0x%lx).", i,
			      buf);
		zassert_equal(len, 0, "Invalid buffer length for slot %d returned (0x%lx).", i,
			      len);

		ret = suit_storage_report_save(i, sample_report, SUIT_STORAGE_REPORT_SLOT_SIZE);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to set report with data for slot %d (%d).", i, ret);

		ret = suit_storage_report_read(i, &buf, &len);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to read report with data for slot %d (%d).", i, ret);
		zassert_mem_equal(sample_report, buf, SUIT_STORAGE_REPORT_SLOT_SIZE,
				  "Corrupted report data received");
		zassert_not_equal(buf, NULL, "Invalid buffer value for slot %d returned (0x%lx).",
				  i, buf);
		zassert_equal(len, SUIT_STORAGE_REPORT_SLOT_SIZE,
			      "Invalid buffer length for slot %d returned (0x%lx).", i, len);
	}
}

ZTEST(suit_storage_report_tests, test_empty_report_overwrite_short_data)
{
	const uint8_t *buf = NULL;
	size_t len = 0;

	/* Fill buffer with sample data */
	for (size_t i = 0; i < SUIT_STORAGE_REPORT_SLOT_SIZE; i++) {
		sample_report[i] = i % 0x100;
	}

	for (size_t i = 0; i < SUIT_STORAGE_REPORT_SLOTS; i++) {
		suit_plat_err_t ret =
			suit_storage_report_save(i, sample_report, SUIT_STORAGE_REPORT_SLOT_SIZE);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to set report with data for slot %d (%d).", i, ret);

		ret = suit_storage_report_read(i, &buf, &len);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to read report with data for slot %d (%d).", i, ret);
		zassert_mem_equal(sample_report, buf, SUIT_STORAGE_REPORT_SLOT_SIZE,
				  "Corrupted report data received");
		zassert_not_equal(buf, NULL, "Invalid buffer value for slot %d returned (0x%lx).",
				  i, buf);
		zassert_equal(len, SUIT_STORAGE_REPORT_SLOT_SIZE,
			      "Invalid buffer length for slot %d returned (0x%lx).", i, len);

		/* Set the buffer with default values */
		memset(sample_report, 0xff, SUIT_STORAGE_REPORT_SLOT_SIZE);

		/* Fill buffer with sample short data */
		for (size_t i = 0; i < 0x16; i++) {
			sample_report[i] = i % 0x100;
		}

		/* Overwrite the buffer partially to verify that the remaining part gets erased. */
		ret = suit_storage_report_save(i, sample_report, 0x16);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to set report with data for slot %d (%d).", i, ret);

		ret = suit_storage_report_read(i, &buf, &len);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to read report with data for slot %d (%d).", i, ret);
		zassert_mem_equal(sample_report, buf, SUIT_STORAGE_REPORT_SLOT_SIZE,
				  "Corrupted report data received");
		zassert_not_equal(buf, NULL, "Invalid buffer value for slot %d returned (0x%lx).",
				  i, buf);
		zassert_equal(len, SUIT_STORAGE_REPORT_SLOT_SIZE,
			      "Invalid buffer length for slot %d returned (0x%lx).", i, len);
	}
}

ZTEST(suit_storage_report_tests, test_empty_report_clear_flag)
{
	const uint8_t *buf = NULL;
	size_t len = 0;

	for (size_t i = 0; i < SUIT_STORAGE_REPORT_SLOTS; i++) {
		suit_plat_err_t ret = suit_storage_report_save(i, NULL, 0);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to set empty report flag for slot %d (%d).", i, ret);

		ret = suit_storage_report_read(i, &buf, &len);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to read empty report flag for slot %d (%d).", i, ret);
		zassert_equal(buf, NULL, "Invalid buffer value for slot %d returned (0x%lx).", i,
			      buf);
		zassert_equal(len, 0, "Invalid buffer length for slot %d returned (0x%lx).", i,
			      len);

		ret = suit_storage_report_clear(i);

		zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to clear report slot %d (%d).", i,
			      ret);

		ret = suit_storage_report_read(i, &buf, &len);

		zassert_equal(ret, SUIT_PLAT_ERR_NOT_FOUND, "Failed to clear slot %d (%d).", i,
			      ret);
	}
}

ZTEST(suit_storage_report_tests, test_empty_report_clear_data)
{
	const uint8_t *buf = NULL;
	size_t len = 0;

	/* Fill buffer with sample data */
	for (size_t i = 0; i < SUIT_STORAGE_REPORT_SLOT_SIZE; i++) {
		sample_report[i] = i % 0x100;
	}

	for (size_t i = 0; i < SUIT_STORAGE_REPORT_SLOTS; i++) {
		suit_plat_err_t ret =
			suit_storage_report_save(i, sample_report, SUIT_STORAGE_REPORT_SLOT_SIZE);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to set report with data for slot %d (%d).", i, ret);

		ret = suit_storage_report_read(i, &buf, &len);

		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "Failed to read report with data for slot %d (%d).", i, ret);

		zassert_mem_equal(sample_report, buf, SUIT_STORAGE_REPORT_SLOT_SIZE,
				  "Corrupted report data received");
		zassert_not_equal(buf, NULL, "Invalid buffer value for slot %d returned (0x%lx).",
				  i, buf);
		zassert_equal(len, SUIT_STORAGE_REPORT_SLOT_SIZE,
			      "Invalid buffer length for slot %d returned (0x%lx).", i, len);

		ret = suit_storage_report_clear(i);

		zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to clear report slot %d (%d).", i,
			      ret);

		ret = suit_storage_report_read(i, &buf, &len);

		zassert_equal(ret, SUIT_PLAT_ERR_NOT_FOUND, "Failed to clear slot %d (%d).", i,
			      ret);
	}
}
