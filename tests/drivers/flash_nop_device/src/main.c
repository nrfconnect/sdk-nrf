/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <drivers/flash.h>
#include <device.h>

/* configuration derived from native_posix DT */
#define SOC_NV_FLASH_NODE DT_CHILD(DT_INST(0, zephyr_sim_flash), flash_0)

#define FLASH_NOP_DEVICE_BASE_OFFSET DT_REG_ADDR(SOC_NV_FLASH_NODE)
#define FLASH_NOP_DEVICE_ERASE_UNIT DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)
#define FLASH_NOP_DEVICE_PROG_UNIT DT_PROP(SOC_NV_FLASH_NODE, write_block_size)
#define FLASH_NOP_DEVICE_FLASH_SIZE DT_REG_SIZE(SOC_NV_FLASH_NODE)

#define FLASH_NOP_DEVICE_ERASE_VALUE \
		DT_PROP(DT_PARENT(SOC_NV_FLASH_NODE), erase_value)

#define TEST_NOP_FLASH_SIZE FLASH_NOP_DEVICE_FLASH_SIZE

#define TEST_NOP_FLASH_END (TEST_NOP_FLASH_SIZE +\
			   FLASH_NOP_DEVICE_BASE_OFFSET)

#define PATTERN8TO32BIT(pat) \
		(((((((0xff & pat) << 8) | (0xff & pat)) << 8) | \
		   (0xff & pat)) << 8) | (0xff & pat))

static const struct device *flash_dev;
static uint8_t test_read_buf[TEST_NOP_FLASH_SIZE];

static void test_init(void)
{
	flash_dev = device_get_binding(DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);

	zassert_true(flash_dev != NULL,
		     "Sim flash driver to nop was not found!");
}

static void test_read(void)
{
	off_t i;
	int rc;

	rc = flash_read(flash_dev, FLASH_NOP_DEVICE_BASE_OFFSET,
			test_read_buf, sizeof(test_read_buf));
	zassert_equal(0, rc, "flash_read should succeed");

	for (i = 0; i < sizeof(test_read_buf); i++) {
		zassert_equal(FLASH_NOP_DEVICE_ERASE_VALUE,
			     test_read_buf[i],
			     "nop flash byte at offset 0x%x has value 0x%02x",
			     i, test_read_buf[i]);
	}
}

static void test_write_read(void)
{
	off_t off;
	uint32_t val32 = 0, r_val32;
	int rc;

	for (off = 0; off < TEST_NOP_FLASH_SIZE; off += 4) {
		rc = flash_write(flash_dev, FLASH_NOP_DEVICE_BASE_OFFSET +
					    off,
				 &val32, sizeof(val32));
		zassert_ok(rc, "flash_write (%d) should succeed at off 0x%x", rc,
			   FLASH_NOP_DEVICE_BASE_OFFSET + off);
		val32++;
	}


	for (off = 0; off < TEST_NOP_FLASH_SIZE; off += 4) {
		rc = flash_read(flash_dev, FLASH_NOP_DEVICE_BASE_OFFSET +
					   off,
				&r_val32, sizeof(r_val32));
		zassert_ok(rc, "flash_read should succeed");
		zassert_equal(PATTERN8TO32BIT(FLASH_NOP_DEVICE_ERASE_VALUE), r_val32,
			"flash byte at offset 0x%x has value 0x%08x, expected  0x%08x",
			off, r_val32, PATTERN8TO32BIT(FLASH_NOP_DEVICE_ERASE_VALUE));
	}
}

static void test_erase(void)
{
	int rc;

	rc = flash_erase(flash_dev, FLASH_NOP_DEVICE_BASE_OFFSET +
			 FLASH_NOP_DEVICE_ERASE_UNIT,
			 FLASH_NOP_DEVICE_ERASE_UNIT);
	zassert_ok(rc, "flash_erase should succeed");
}

static void test_out_of_bounds(void)
{
	int rc;
	uint8_t data[8] = {0};

	rc = flash_write(flash_dev, FLASH_NOP_DEVICE_BASE_OFFSET - 4,
				 data, 4);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_write(flash_dev, FLASH_NOP_DEVICE_BASE_OFFSET - 4,
				 data, 8);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_write(flash_dev, TEST_NOP_FLASH_END,
				 data, 4);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_write(flash_dev, TEST_NOP_FLASH_END - 4,
				 data, 8);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, FLASH_NOP_DEVICE_BASE_OFFSET -
			 FLASH_NOP_DEVICE_ERASE_UNIT,
			 FLASH_NOP_DEVICE_ERASE_UNIT);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, TEST_NOP_FLASH_END,
			 FLASH_NOP_DEVICE_ERASE_UNIT);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, FLASH_NOP_DEVICE_BASE_OFFSET -
			 FLASH_NOP_DEVICE_ERASE_UNIT * 2,
			 FLASH_NOP_DEVICE_ERASE_UNIT * 2);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, TEST_NOP_FLASH_END -
			 FLASH_NOP_DEVICE_ERASE_UNIT,
			 FLASH_NOP_DEVICE_ERASE_UNIT * 2);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_read(flash_dev, FLASH_NOP_DEVICE_BASE_OFFSET - 4,
				 data, 4);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_read(flash_dev, FLASH_NOP_DEVICE_BASE_OFFSET - 4,
				 data, 8);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_read(flash_dev, TEST_NOP_FLASH_END,
				 data, 4);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_read(flash_dev, TEST_NOP_FLASH_END - 4, data, 8);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);
}

static void test_align(void)
{
#if FLASH_NOP_DEVICE_PROG_UNIT > 1
	int rc;
	uint8_t data[4] = {0};

	rc = flash_write(flash_dev, FLASH_NOP_DEVICE_BASE_OFFSET + 1,
				 data, 4);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_write(flash_dev, FLASH_NOP_DEVICE_BASE_OFFSET,
				 data, 3);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, FLASH_NOP_DEVICE_BASE_OFFSET + 1,
			 FLASH_NOP_DEVICE_ERASE_UNIT);

	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, FLASH_NOP_DEVICE_BASE_OFFSET,
			 FLASH_NOP_DEVICE_ERASE_UNIT + 1);

	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);
#endif
}

static void test_get_erase_value(void)
{
	const struct flash_parameters *fp = flash_get_parameters(flash_dev);

	zassert_equal(fp->erase_value, FLASH_NOP_DEVICE_ERASE_VALUE,
		      "Expected erase value %x",
		      FLASH_NOP_DEVICE_ERASE_VALUE);
}

void test_main(void)
{
	ztest_test_suite(flash_nop_device,
			 ztest_unit_test(test_init),
			 ztest_unit_test(test_read),
			 ztest_unit_test(test_write_read),
			 ztest_unit_test(test_erase),
			 ztest_unit_test(test_out_of_bounds),
			 ztest_unit_test(test_align),
			 ztest_unit_test(test_get_erase_value));

	ztest_run_test_suite(flash_nop_device);
}
