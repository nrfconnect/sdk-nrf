/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/ztest.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#define DEVICE_DT_GET_AND_COMMA(node_id) DEVICE_DT_GET(node_id),
/* Generate a list of devices for all instances of the "compat" */
#define DEVS_FOR_DT_COMPAT(compat) \
	DT_FOREACH_STATUS_OKAY(compat, DEVICE_DT_GET_AND_COMMA)

static const struct device *const devices[] = {
#ifdef CONFIG_NRFX_SPIM
	DEVS_FOR_DT_COMPAT(nordic_nrf_spim)
#endif
};

typedef void (*test_func_t)(const struct device *dev);
typedef bool (*capability_func_t)(const struct device *dev);


#define DATA_FIELD (16)

#define MEMORY_SECTION(node)                                                                       \
	COND_CODE_1(DT_NODE_HAS_PROP(node, memory_regions),                                        \
		    (__attribute__((__section__(                                                   \
			    LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(node, memory_regions)))))),      \
		    ())

static struct spi_config default_spi_cfg = {
	.frequency = 4000000,
	.operation = (SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_WORD_SET(8) | SPI_LINES_SINGLE),
};

static void setup_instance(const struct device *dev)
{
	/* Left for future test expansion. */
}

static void tear_down_instance(const struct device *dev)
{
	/* Left for future test expansion. */
}

static void test_all_instances(test_func_t func, capability_func_t capability_check)
{
	int devices_skipped = 0;

	zassert_true(ARRAY_SIZE(devices) > 0, "No device found");
	for (int i = 0; i < ARRAY_SIZE(devices); i++) {
		setup_instance(devices[i]);
		TC_PRINT("\nInstance %u: ", i + 1);
		if ((capability_check == NULL) ||
		     capability_check(devices[i])) {
			TC_PRINT("Testing %s\n", devices[i]->name);
			func(devices[i]);
		} else {
			TC_PRINT("Skipped for %s\n", devices[i]->name);
			devices_skipped++;
		}
		tear_down_instance(devices[i]);
		/* Allow logs to be printed. */
		k_sleep(K_MSEC(100));
	}
	if (devices_skipped == ARRAY_SIZE(devices)) {
		ztest_test_skip();
	}
}


/**
 * Test validates if instance can transceive data.
 */
static void test_transceive_data_instance(const struct device *dev)
{
	uint8_t spim_buffer[2 * DATA_FIELD] MEMORY_SECTION(DT_BUS(dev));
	int ret = 0;

	/* SPI buffer sets */
	struct spi_buf tx_spi_buf = {.buf = &spim_buffer[0], .len = DATA_FIELD};
	struct spi_buf_set tx_spi_buf_set = {.buffers = &tx_spi_buf, .count = 1};

	struct spi_buf rx_spi_buf = {.buf = &spim_buffer[DATA_FIELD],
				     .len = DATA_FIELD};
	struct spi_buf_set rx_spi_buf_set = {.buffers = &rx_spi_buf, .count = 1};

	ret = spi_transceive(dev, &default_spi_cfg, &tx_spi_buf_set, &rx_spi_buf_set);

	zassert_equal(0, ret, "%s: Unexpected error", dev->name);
}

static bool test_transceive_data_capable(const struct device *dev)
{
	return true;
}

ZTEST(spim_instances, test_transceive_data)
{
	test_all_instances(test_transceive_data_instance, test_transceive_data_capable);
}

static void *suite_setup(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(devices); i++) {
		zassert_true(device_is_ready(devices[i]),
			     "Device %s is not ready", devices[i]->name);
		k_object_access_grant(devices[i], k_current_get());
	}

	return NULL;
}

ZTEST_SUITE(spim_instances, NULL, suite_setup, NULL, NULL, NULL);
