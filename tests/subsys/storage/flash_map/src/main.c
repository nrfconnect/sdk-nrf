/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>

#define DUMMY_FLASH_OFFSET  0x1000
#define DUMMY_NRANGE_OFFSET 0x11000

ZTEST(partition_macros, test_dt_reg_addr)
{
	/* Test regular fixed partitions */
	zassert_equal(DUMMY_FLASH_OFFSET + 0x0, DT_REG_ADDR(DT_NODELABEL(dummy_0x0)),
		      "Expected 0x1000, got 0x%x", DT_REG_ADDR(DT_NODELABEL(dummy_0x0)));
	zassert_equal(DUMMY_FLASH_OFFSET + 0x4000, DT_REG_ADDR(DT_NODELABEL(dummy_0x4000)),
		      "Expected 0x5000, got 0x%x", DT_REG_ADDR(DT_NODELABEL(dummy_0x4000)));

	/* Test regular fixed subpartitions */
	zassert_equal(DUMMY_FLASH_OFFSET + 0x1000 + 0x0,
		      DT_REG_ADDR(DT_NODELABEL(dummy_0x1000_0x0)), "Expected 0x2000, got 0x%x",
		      DT_REG_ADDR(DT_NODELABEL(dummy_0x1000_0x0)));
	zassert_equal(DUMMY_FLASH_OFFSET + 0x1000 + 0x1000,
		      DT_REG_ADDR(DT_NODELABEL(dummy_0x1000_0x1000)), "Expected 0x3000, got 0x%x",
		      DT_REG_ADDR(DT_NODELABEL(dummy_0x1000_0x1000)));

	/* Test regular fixed sub-subpartitions */
	zassert_equal(DUMMY_FLASH_OFFSET + 0x1000 + 0x800 + 0x0,
		      DT_REG_ADDR(DT_NODELABEL(dummy_0x1000_0x800_0x0)),
		      "Expected 0x2800, got 0x%x",
		      DT_REG_ADDR(DT_NODELABEL(dummy_0x1000_0x800_0x0)));
	zassert_equal(DUMMY_FLASH_OFFSET + 0x1000 + 0x800 + 0x400,
		      DT_REG_ADDR(DT_NODELABEL(dummy_0x1000_0x800_0x400)),
		      "Expected 0x2c00, got 0x%x",
		      DT_REG_ADDR(DT_NODELABEL(dummy_0x1000_0x800_0x400)));

	/* Test subpartitions that do not define "ranges"*/
	zassert_equal(0x0, DT_REG_ADDR(DT_NODELABEL(dummy_0x3000_0x0)), "Expected 0x0, got 0x%x",
		      DT_REG_ADDR(DT_NODELABEL(dummy_0x3000_0x0)));
	zassert_equal(0x1000, DT_REG_ADDR(DT_NODELABEL(dummy_0x3000_0x1000)),
		      "Expected 0x1000, got 0x%x", DT_REG_ADDR(DT_NODELABEL(dummy_0x3000_0x1000)));

	/* Test sub-subpartitions that do not define "ranges"*/
	zassert_equal(0x0, DT_REG_ADDR(DT_NODELABEL(dummy_0x3000_0x800_0x0)),
		      "Expected 0x0, got 0x%x", DT_REG_ADDR(DT_NODELABEL(dummy_0x3000_0x800_0x0)));
	zassert_equal(0x400, DT_REG_ADDR(DT_NODELABEL(dummy_0x3000_0x800_0x400)),
		      "Expected 0x400, got 0x%x",
		      DT_REG_ADDR(DT_NODELABEL(dummy_0x3000_0x800_0x400)));

	/* Test fixed partition that contains subpartitions */
	zassert_equal(DUMMY_FLASH_OFFSET + 0x1000, DT_REG_ADDR(DT_NODELABEL(dummy_0x1000)),
		      "Expected 0x2000, got 0x%x", DT_REG_ADDR(DT_NODELABEL(dummy_0x1000)));

	/* Test fixed partition that contains subpartitions and do not define "ranges" */
	zassert_equal(DUMMY_FLASH_OFFSET + 0x3000, DT_REG_ADDR(DT_NODELABEL(dummy_0x3000)),
		      "Expected 0x4000, got 0x%x", DT_REG_ADDR(DT_NODELABEL(dummy_0x3000)));
}

ZTEST(partition_macros, test_dt_reg_addr_nrange)
{
	/* Test regular fixed partitions */
	zassert_equal(0x0, DT_REG_ADDR(DT_NODELABEL(dummy_nrange_0x0)), "Expected 0x0, got 0x%x",
		      DT_REG_ADDR(DT_NODELABEL(dummy_nrange_0x0)));
	zassert_equal(0x4000, DT_REG_ADDR(DT_NODELABEL(dummy_nrange_0x4000)),
		      "Expected 0x4000, got 0x%x", DT_REG_ADDR(DT_NODELABEL(dummy_nrange_0x4000)));

	/* Test regular fixed subpartitions */
	zassert_equal(0x0, DT_REG_ADDR(DT_NODELABEL(dummy_nrange_0x1000_0x0)),
		      "Expected 0x0, got 0x%x", DT_REG_ADDR(DT_NODELABEL(dummy_nrange_0x1000_0x0)));
	zassert_equal(0x1000, DT_REG_ADDR(DT_NODELABEL(dummy_nrange_0x1000_0x1000)),
		      "Expected 0x1000, got 0x%x",
		      DT_REG_ADDR(DT_NODELABEL(dummy_nrange_0x1000_0x1000)));

	/* Test regular fixed sub-subpartitions */
	zassert_equal(0x0, DT_REG_ADDR(DT_NODELABEL(dummy_nrange_0x1000_0x800_0x0)),
		      "Expected 0x0, got 0x%x",
		      DT_REG_ADDR(DT_NODELABEL(dummy_nrange_0x1000_0x800_0x0)));
	zassert_equal(0x400, DT_REG_ADDR(DT_NODELABEL(dummy_nrange_0x1000_0x800_0x400)),
		      "Expected 0x400, got 0x%x",
		      DT_REG_ADDR(DT_NODELABEL(dummy_nrange_0x1000_0x800_0x400)));

	/* Test fixed partition that contains subpartitions */
	zassert_equal(0x1000, DT_REG_ADDR(DT_NODELABEL(dummy_nrange_0x1000)),
		      "Expected 0x1000, got 0x%x", DT_REG_ADDR(DT_NODELABEL(dummy_nrange_0x1000)));
}

ZTEST(partition_macros, test_dt_fixed_partition_addr)
{
	/* Test regular fixed partitions */
	zassert_equal(DUMMY_FLASH_OFFSET + 0x0, DT_FIXED_PARTITION_ADDR(DT_NODELABEL(dummy_0x0)),
		      "Expected 0x1000, got 0x%x",
		      DT_FIXED_PARTITION_ADDR(DT_NODELABEL(dummy_0x0)));
	zassert_equal(
		DUMMY_FLASH_OFFSET + 0x4000, DT_FIXED_PARTITION_ADDR(DT_NODELABEL(dummy_0x4000)),
		"Expected 0x5000, got 0x%x", DT_FIXED_PARTITION_ADDR(DT_NODELABEL(dummy_0x4000)));

	/* Test fixed partition that contains subpartitions */
	zassert_equal(
		DUMMY_FLASH_OFFSET + 0x1000, DT_FIXED_PARTITION_ADDR(DT_NODELABEL(dummy_0x1000)),
		"Expected 0x2000, got 0x%x", DT_FIXED_PARTITION_ADDR(DT_NODELABEL(dummy_0x1000)));

	/* Test fixed partition that contains subpartitions and do not define "ranges" */
	zassert_equal(
		DUMMY_FLASH_OFFSET + 0x3000, DT_FIXED_PARTITION_ADDR(DT_NODELABEL(dummy_0x3000)),
		"Expected 0x4000, got 0x%x", DT_FIXED_PARTITION_ADDR(DT_NODELABEL(dummy_0x3000)));
}

ZTEST(partition_macros, test_dt_fixed_partition_addr_nrange)
{
	/* Test regular fixed partitions */
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x0,
		      DT_FIXED_PARTITION_ADDR(DT_NODELABEL(dummy_nrange_0x0)),
		      "Expected 0x11000, got 0x%x",
		      DT_FIXED_PARTITION_ADDR(DT_NODELABEL(dummy_nrange_0x0)));
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x4000,
		      DT_FIXED_PARTITION_ADDR(DT_NODELABEL(dummy_nrange_0x4000)),
		      "Expected 0x15000, got 0x%x",
		      DT_FIXED_PARTITION_ADDR(DT_NODELABEL(dummy_nrange_0x4000)));

	/* Test fixed partition that contains subpartitions */
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x1000,
		      DT_FIXED_PARTITION_ADDR(DT_NODELABEL(dummy_nrange_0x1000)),
		      "Expected 0x12000, got 0x%x",
		      DT_FIXED_PARTITION_ADDR(DT_NODELABEL(dummy_nrange_0x1000)));
}

ZTEST(partition_macros, test_dt_fixed_subpartition_addr)
{
	/* Test regular fixed subpartitions */
	zassert_equal(DUMMY_FLASH_OFFSET + 0x1000 + 0x0,
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_0x1000_0x0)),
		      "Expected 0x2000, got 0x%x",
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_0x1000_0x0)));
	zassert_equal(DUMMY_FLASH_OFFSET + 0x1000 + 0x1000,
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_0x1000_0x1000)),
		      "Expected 0x3000, got 0x%x",
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_0x1000_0x1000)));

	/* Test regular fixed sub-subpartitions */
	zassert_equal(DUMMY_FLASH_OFFSET + 0x1000 + 0x800 + 0x0,
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_0x1000_0x800_0x0)),
		      "Expected 0x2800, got 0x%x",
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_0x1000_0x800_0x0)));
	zassert_equal(DUMMY_FLASH_OFFSET + 0x1000 + 0x800 + 0x400,
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_0x1000_0x800_0x400)),
		      "Expected 0x2c00, got 0x%x",
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_0x1000_0x800_0x400)));

	/* Test subpartitions that do not define "ranges"*/
	zassert_equal(DUMMY_FLASH_OFFSET + 0x3000 + 0x0,
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_0x3000_0x0)),
		      "Expected 0x4000, got 0x%x",
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_0x3000_0x0)));
	zassert_equal(DUMMY_FLASH_OFFSET + 0x3000 + 0x1000,
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_0x3000_0x1000)),
		      "Expected 0x5000, got 0x%x",
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_0x3000_0x1000)));

	/* Test sub-subpartitions that do not define "ranges"*/
	zassert_equal(DUMMY_FLASH_OFFSET + 0x3000 + 0x800 + 0x0,
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_0x3000_0x800_0x0)),
		      "Expected 0x4800, got 0x%x",
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_0x3000_0x800_0x0)));
	zassert_equal(DUMMY_FLASH_OFFSET + 0x3000 + 0x800 + 0x400,
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_0x3000_0x800_0x400)),
		      "Expected 0x4c00, got 0x%x",
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_0x3000_0x800_0x400)));
}

ZTEST(partition_macros, test_dt_fixed_subpartition_addr_nrange)
{
	/* Test regular fixed subpartitions */
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x1000 + 0x0,
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_nrange_0x1000_0x0)),
		      "Expected 0x12000, got 0x%x",
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_nrange_0x1000_0x0)));
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x1000 + 0x1000,
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_nrange_0x1000_0x1000)),
		      "Expected 0x13000, got 0x%x",
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_nrange_0x1000_0x1000)));

	/* Test regular fixed sub-subpartitions */
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x1000 + 0x800 + 0x0,
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_nrange_0x1000_0x800_0x0)),
		      "Expected 0x12800, got 0x%x",
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_nrange_0x1000_0x800_0x0)));
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x1000 + 0x800 + 0x400,
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_nrange_0x1000_0x800_0x400)),
		      "Expected 0x12c00, got 0x%x",
		      DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(dummy_nrange_0x1000_0x800_0x400)));
}

ZTEST(partition_macros, test_fixed_partition_address)
{
	/* Test regular fixed partitions */
	zassert_equal(DUMMY_FLASH_OFFSET + 0x0, FIXED_PARTITION_ADDRESS(dummy_0x0),
		      "Expected 0x1000, got 0x%x", FIXED_PARTITION_ADDRESS(dummy_0x0));
	zassert_equal(DUMMY_FLASH_OFFSET + 0x4000, FIXED_PARTITION_ADDRESS(dummy_0x4000),
		      "Expected 0x5000, got 0x%x", FIXED_PARTITION_ADDRESS(dummy_0x4000));

	/* Test regular fixed subpartitions */
	zassert_equal(DUMMY_FLASH_OFFSET + 0x1000 + 0x0, FIXED_PARTITION_ADDRESS(dummy_0x1000_0x0),
		      "Expected 0x2000, got 0x%x", FIXED_PARTITION_ADDRESS(dummy_0x1000_0x0));
	zassert_equal(DUMMY_FLASH_OFFSET + 0x1000 + 0x1000,
		      FIXED_PARTITION_ADDRESS(dummy_0x1000_0x1000), "Expected 0x3000, got 0x%x",
		      FIXED_PARTITION_ADDRESS(dummy_0x1000_0x1000));

	/* Test regular fixed sub-subpartitions */
	zassert_equal(DUMMY_FLASH_OFFSET + 0x1000 + 0x800 + 0x0,
		      FIXED_PARTITION_ADDRESS(dummy_0x1000_0x800_0x0), "Expected 0x2800, got 0x%x",
		      FIXED_PARTITION_ADDRESS(dummy_0x1000_0x800_0x0));
	zassert_equal(DUMMY_FLASH_OFFSET + 0x1000 + 0x800 + 0x400,
		      FIXED_PARTITION_ADDRESS(dummy_0x1000_0x800_0x400),
		      "Expected 0x2c00, got 0x%x",
		      FIXED_PARTITION_ADDRESS(dummy_0x1000_0x800_0x400));

	/* Test subpartitions that do not define "ranges"*/
	zassert_equal(DUMMY_FLASH_OFFSET + 0x3000 + 0x0, FIXED_PARTITION_ADDRESS(dummy_0x3000_0x0),
		      "Expected 0x4000, got 0x%x", FIXED_PARTITION_ADDRESS(dummy_0x3000_0x0));
	zassert_equal(DUMMY_FLASH_OFFSET + 0x3000 + 0x1000,
		      FIXED_PARTITION_ADDRESS(dummy_0x3000_0x1000), "Expected 0x5000, got 0x%x",
		      FIXED_PARTITION_ADDRESS(dummy_0x3000_0x1000));

	/* Test sub-subpartitions that do not define "ranges"*/
	zassert_equal(DUMMY_FLASH_OFFSET + 0x3000 + 0x800 + 0x0,
		      FIXED_PARTITION_ADDRESS(dummy_0x3000_0x800_0x0), "Expected 0x4800, got 0x%x",
		      FIXED_PARTITION_ADDRESS(dummy_0x3000_0x800_0x0));
	zassert_equal(DUMMY_FLASH_OFFSET + 0x3000 + 0x800 + 0x400,
		      FIXED_PARTITION_ADDRESS(dummy_0x3000_0x800_0x400),
		      "Expected 0x4c00, got 0x%x",
		      FIXED_PARTITION_ADDRESS(dummy_0x3000_0x800_0x400));

	/* Test fixed partition that contains subpartitions */
	zassert_equal(DUMMY_FLASH_OFFSET + 0x1000, FIXED_PARTITION_ADDRESS(dummy_0x1000),
		      "Expected 0x2000, got 0x%x", FIXED_PARTITION_ADDRESS(dummy_0x1000));

	/* Test fixed partition that contains subpartitions and do not define "ranges" */
	zassert_equal(DUMMY_FLASH_OFFSET + 0x3000, FIXED_PARTITION_ADDRESS(dummy_0x3000),
		      "Expected 0x4000, got 0x%x", FIXED_PARTITION_ADDRESS(dummy_0x3000));
}

ZTEST(partition_macros, test_fixed_partition_node_address)
{
	/* Test regular fixed partitions */
	zassert_equal(DUMMY_FLASH_OFFSET + 0x0, FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x0)),
		      "Expected 0x1000, got 0x%x", FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x0)));
	zassert_equal(DUMMY_FLASH_OFFSET + 0x4000, FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x4000)),
		      "Expected 0x5000, got 0x%x", FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x4000)));

	/* Test regular fixed subpartitions */
	zassert_equal(DUMMY_FLASH_OFFSET + 0x1000 + 0x0, FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x1000_0x0)),
		      "Expected 0x2000, got 0x%x", FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x1000_0x0)));
	zassert_equal(DUMMY_FLASH_OFFSET + 0x1000 + 0x1000,
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x1000_0x1000)), "Expected 0x3000, got 0x%x",
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x1000_0x1000)));

	/* Test regular fixed sub-subpartitions */
	zassert_equal(DUMMY_FLASH_OFFSET + 0x1000 + 0x800 + 0x0,
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x1000_0x800_0x0)), "Expected 0x2800, got 0x%x",
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x1000_0x800_0x0)));
	zassert_equal(DUMMY_FLASH_OFFSET + 0x1000 + 0x800 + 0x400,
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x1000_0x800_0x400)),
		      "Expected 0x2c00, got 0x%x",
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x1000_0x800_0x400)));

	/* Test subpartitions that do not define "ranges"*/
	zassert_equal(DUMMY_FLASH_OFFSET + 0x3000 + 0x0, FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x3000_0x0)),
		      "Expected 0x4000, got 0x%x", FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x3000_0x0)));
	zassert_equal(DUMMY_FLASH_OFFSET + 0x3000 + 0x1000,
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x3000_0x1000)), "Expected 0x5000, got 0x%x",
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x3000_0x1000)));

	/* Test sub-subpartitions that do not define "ranges"*/
	zassert_equal(DUMMY_FLASH_OFFSET + 0x3000 + 0x800 + 0x0,
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x3000_0x800_0x0)), "Expected 0x4800, got 0x%x",
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x3000_0x800_0x0)));
	zassert_equal(DUMMY_FLASH_OFFSET + 0x3000 + 0x800 + 0x400,
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x3000_0x800_0x400)),
		      "Expected 0x4c00, got 0x%x",
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x3000_0x800_0x400)));

	/* Test fixed partition that contains subpartitions */
	zassert_equal(DUMMY_FLASH_OFFSET + 0x1000, FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x1000)),
		      "Expected 0x2000, got 0x%x", FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x1000)));

	/* Test fixed partition that contains subpartitions and do not define "ranges" */
	zassert_equal(DUMMY_FLASH_OFFSET + 0x3000, FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x3000)),
		      "Expected 0x4000, got 0x%x", FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_0x3000)));
}

ZTEST(partition_macros, test_fixed_partition_address_nrange)
{
	/* Test regular fixed partitions */
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x0, FIXED_PARTITION_ADDRESS(dummy_nrange_0x0),
		      "Expected 0x11000, got 0x%x", FIXED_PARTITION_ADDRESS(dummy_nrange_0x0));
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x4000, FIXED_PARTITION_ADDRESS(dummy_nrange_0x4000),
		      "Expected 0x15000, got 0x%x", FIXED_PARTITION_ADDRESS(dummy_nrange_0x4000));

	/* Test regular fixed subpartitions */
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x1000 + 0x0,
		      FIXED_PARTITION_ADDRESS(dummy_nrange_0x1000_0x0),
		      "Expected 0x12000, got 0x%x",
		      FIXED_PARTITION_ADDRESS(dummy_nrange_0x1000_0x0));
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x1000 + 0x1000,
		      FIXED_PARTITION_ADDRESS(dummy_nrange_0x1000_0x1000),
		      "Expected 0x13000, got 0x%x",
		      FIXED_PARTITION_ADDRESS(dummy_nrange_0x1000_0x1000));

	/* Test regular fixed sub-subpartitions */
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x1000 + 0x800 + 0x0,
		      FIXED_PARTITION_ADDRESS(dummy_nrange_0x1000_0x800_0x0),
		      "Expected 0x12800, got 0x%x",
		      FIXED_PARTITION_ADDRESS(dummy_nrange_0x1000_0x800_0x0));
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x1000 + 0x800 + 0x400,
		      FIXED_PARTITION_ADDRESS(dummy_nrange_0x1000_0x800_0x400),
		      "Expected 0x12c00, got 0x%x",
		      FIXED_PARTITION_ADDRESS(dummy_nrange_0x1000_0x800_0x400));

	/* Test fixed partition that contains subpartitions */
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x1000, FIXED_PARTITION_ADDRESS(dummy_nrange_0x1000),
		      "Expected 0x2000, got 0x%x", FIXED_PARTITION_ADDRESS(dummy_nrange_0x1000));
}

ZTEST(partition_macros, test_fixed_partition_node_address_nrange)
{
	/* Test regular fixed partitions */
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x0, FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_nrange_0x0)),
		      "Expected 0x11000, got 0x%x", FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_nrange_0x0)));
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x4000, FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_nrange_0x4000)),
		      "Expected 0x15000, got 0x%x", FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_nrange_0x4000)));

	/* Test regular fixed subpartitions */
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x1000 + 0x0,
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_nrange_0x1000_0x0)),
		      "Expected 0x12000, got 0x%x",
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_nrange_0x1000_0x0)));
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x1000 + 0x1000,
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_nrange_0x1000_0x1000)),
		      "Expected 0x13000, got 0x%x",
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_nrange_0x1000_0x1000)));

	/* Test regular fixed sub-subpartitions */
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x1000 + 0x800 + 0x0,
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_nrange_0x1000_0x800_0x0)),
		      "Expected 0x12800, got 0x%x",
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_nrange_0x1000_0x800_0x0)));
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x1000 + 0x800 + 0x400,
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_nrange_0x1000_0x800_0x400)),
		      "Expected 0x12c00, got 0x%x",
		      FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_nrange_0x1000_0x800_0x400)));

	/* Test fixed partition that contains subpartitions */
	zassert_equal(DUMMY_NRANGE_OFFSET + 0x1000, FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_nrange_0x1000)),
		      "Expected 0x2000, got 0x%x", FIXED_PARTITION_NODE_ADDRESS(DT_NODELABEL(dummy_nrange_0x1000)));
}

ZTEST(partition_macros, test_fixed_partition_offset)
{
	/* Test regular fixed partitions */
	zassert_equal(0x0, FIXED_PARTITION_OFFSET(dummy_0x0), "Expected 0x0, got 0x%x",
		      FIXED_PARTITION_OFFSET(dummy_0x0));
	zassert_equal(0x4000, FIXED_PARTITION_OFFSET(dummy_0x4000), "Expected 0x4000, got 0x%x",
		      FIXED_PARTITION_OFFSET(dummy_0x4000));

	/* Test regular fixed subpartitions */
	zassert_equal(0x1000 + 0x0, FIXED_PARTITION_OFFSET(dummy_0x1000_0x0),
		      "Expected 0x1000, got 0x%x", FIXED_PARTITION_OFFSET(dummy_0x1000_0x0));
	zassert_equal(0x1000 + 0x1000, FIXED_PARTITION_OFFSET(dummy_0x1000_0x1000),
		      "Expected 0x2000, got 0x%x", FIXED_PARTITION_OFFSET(dummy_0x1000_0x1000));

	/* Test subpartitions that do not define "ranges"*/
	zassert_equal(0x3000 + 0x0, FIXED_PARTITION_OFFSET(dummy_0x3000_0x0),
		      "Expected 0x3000, got 0x%x", FIXED_PARTITION_OFFSET(dummy_0x3000_0x0));
	zassert_equal(0x3000 + 0x1000, FIXED_PARTITION_OFFSET(dummy_0x3000_0x1000),
		      "Expected 0x4000, got 0x%x", FIXED_PARTITION_OFFSET(dummy_0x3000_0x1000));

	/* Test fixed partition that contains subpartitions */
	zassert_equal(0x1000, FIXED_PARTITION_OFFSET(dummy_0x1000), "Expected 0x1000, got 0x%x",
		      FIXED_PARTITION_OFFSET(dummy_0x1000));

	/* Test fixed partition that contains subpartitions and do not define "ranges" */
	zassert_equal(0x3000, FIXED_PARTITION_OFFSET(dummy_0x3000), "Expected 0x3000, got 0x%x",
		      FIXED_PARTITION_OFFSET(dummy_0x3000));
}

ZTEST(partition_macros, test_fixed_partition_node_offset)
{
	/* Test regular fixed partitions */
	zassert_equal(0x0, FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_0x0)), "Expected 0x0, got 0x%x",
		      FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_0x0)));
	zassert_equal(0x4000, FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_0x4000)), "Expected 0x4000, got 0x%x",
		      FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_0x4000)));

	/* Test regular fixed subpartitions */
	zassert_equal(0x1000 + 0x0, FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_0x1000_0x0)),
		      "Expected 0x1000, got 0x%x", FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_0x1000_0x0)));
	zassert_equal(0x1000 + 0x1000, FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_0x1000_0x1000)),
		      "Expected 0x2000, got 0x%x", FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_0x1000_0x1000)));

	/* Test subpartitions that do not define "ranges"*/
	zassert_equal(0x3000 + 0x0, FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_0x3000_0x0)),
		      "Expected 0x3000, got 0x%x", FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_0x3000_0x0)));
	zassert_equal(0x3000 + 0x1000, FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_0x3000_0x1000)),
		      "Expected 0x4000, got 0x%x", FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_0x3000_0x1000)));

	/* Test fixed partition that contains subpartitions */
	zassert_equal(0x1000, FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_0x1000)), "Expected 0x1000, got 0x%x",
		      FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_0x1000)));

	/* Test fixed partition that contains subpartitions and do not define "ranges" */
	zassert_equal(0x3000, FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_0x3000)), "Expected 0x3000, got 0x%x",
		      FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_0x3000)));
}

ZTEST(partition_macros, test_fixed_partition_offset_nrange)
{
	/* Test regular fixed partitions */
	zassert_equal(0x0, FIXED_PARTITION_OFFSET(dummy_nrange_0x0), "Expected 0x0, got 0x%x",
		      FIXED_PARTITION_OFFSET(dummy_nrange_0x0));
	zassert_equal(0x4000, FIXED_PARTITION_OFFSET(dummy_nrange_0x4000),
		      "Expected 0x4000, got 0x%x", FIXED_PARTITION_OFFSET(dummy_nrange_0x4000));

	/* Test regular fixed subpartitions */
	zassert_equal(0x1000 + 0x0, FIXED_PARTITION_OFFSET(dummy_nrange_0x1000_0x0),
		      "Expected 0x1000, got 0x%x", FIXED_PARTITION_OFFSET(dummy_nrange_0x1000_0x0));
	zassert_equal(0x1000 + 0x1000, FIXED_PARTITION_OFFSET(dummy_nrange_0x1000_0x1000),
		      "Expected 0x2000, got 0x%x",
		      FIXED_PARTITION_OFFSET(dummy_nrange_0x1000_0x1000));

	/* Test fixed partition that contains subpartitions */
	zassert_equal(0x1000, FIXED_PARTITION_OFFSET(dummy_nrange_0x1000),
		      "Expected 0x1000, got 0x%x", FIXED_PARTITION_OFFSET(dummy_nrange_0x1000));
}

ZTEST(partition_macros, test_fixed_partition_node_offset_nrange)
{
	/* Test regular fixed partitions */
	zassert_equal(0x0, FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_nrange_0x0)), "Expected 0x0, got 0x%x",
		      FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_nrange_0x0)));
	zassert_equal(0x4000, FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_nrange_0x4000)),
		      "Expected 0x4000, got 0x%x", FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_nrange_0x4000)));

	/* Test regular fixed subpartitions */
	zassert_equal(0x1000 + 0x0, FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_nrange_0x1000_0x0)),
		      "Expected 0x1000, got 0x%x", FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_nrange_0x1000_0x0)));
	zassert_equal(0x1000 + 0x1000, FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_nrange_0x1000_0x1000)),
		      "Expected 0x2000, got 0x%x",
		      FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_nrange_0x1000_0x1000)));

	/* Test fixed partition that contains subpartitions */
	zassert_equal(0x1000, FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_nrange_0x1000)),
		      "Expected 0x1000, got 0x%x", FIXED_PARTITION_NODE_OFFSET(DT_NODELABEL(dummy_nrange_0x1000)));
}

ZTEST_SUITE(partition_macros, NULL, NULL, NULL, NULL, NULL);
