/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/ztest.h>

struct test_data {
	struct k_work_delayable test_work;
	struct k_sem sem;
	int spim_alloc_idx;
	int spis_alloc_idx;
	struct spi_buf_set sets[4];
	struct spi_buf_set *mtx_set;
	struct spi_buf_set *mrx_set;
	struct spi_buf_set *stx_set;
	struct spi_buf_set *srx_set;
	struct spi_buf bufs[8];
};

/** Basic test where slave and master have RX and TX sets which contains only one
 *  same size buffer.
 */
void test_basic(bool async);

/** Basic test with zero length buffers.
 */
void test_basic_zero_len(bool async);

/** Setup a transfer where RX buffer on master and slave are shorter than
 *  TX buffers. RX buffers shall contain beginning of TX data and last TX
 *  bytes that did not fit in the RX buffers shall be lost.
 */
void test_short_rx(bool async);

/** Test where only master transmits. */
void test_only_tx(bool async);

/** Test where only master transmits and slave receives in chunks. */
void test_only_tx_in_chunks(bool async);

/** Test where only slave transmits. */
void test_only_rx(bool async);

/** Test where only slave transmits in chunks. */
void test_only_rx_in_chunks(bool async);

void before(void *not_used);

void *suite_setup(void);
