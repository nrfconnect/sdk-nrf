/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(idle_spim_loopback, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/pm/device_runtime.h>

#define	DELTA			(1)

#define SPI_MODE_DEFAULT (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_LINES_SINGLE \
				| SPI_TRANSFER_MSB | SPI_MODE_CPHA | SPI_MODE_CPOL)

#if defined(CONFIG_TEST_SPI_HOLD_ON_CS) && defined(CONFIG_TEST_SPI_LOCK_ON)
#define SPI_MODE (SPI_MODE_DEFAULT | SPI_HOLD_ON_CS | SPI_LOCK_ON)
#elif defined(CONFIG_TEST_SPI_HOLD_ON_CS) && !defined(CONFIG_TEST_SPI_LOCK_ON)
#define SPI_MODE (SPI_MODE_DEFAULT | SPI_HOLD_ON_CS)
#elif !defined(CONFIG_TEST_SPI_HOLD_ON_CS) && defined(CONFIG_TEST_SPI_LOCK_ON)
#define SPI_MODE (SPI_MODE_DEFAULT | SPI_LOCK_ON)
#else
#define SPI_MODE (SPI_MODE_DEFAULT)
#endif

static struct spi_dt_spec spim_spec = SPI_DT_SPEC_GET(DT_NODELABEL(dut_spi_dt), SPI_MODE, 0);

static const struct gpio_dt_spec pin_in = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), test_gpios);

#define MEMORY_SECTION(node)                                                           \
	COND_CODE_1(DT_NODE_HAS_PROP(node, memory_regions),                                \
		    (__attribute__((__section__(                                               \
			    LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(node, memory_regions)))))),      \
		    ())

static uint8_t spim_buffer[2 * CONFIG_DATA_FIELD]
	MEMORY_SECTION(DT_BUS(DT_NODELABEL(dut_spi_dt)));

/* Variables used to count edges on SPI CS */
static struct gpio_callback gpio_input_cb_data;
static volatile uint32_t high, low;

/* Variables used to make SPI active for ~1 second */
static struct k_timer my_timer;
static bool timer_expired;


void my_timer_handler(struct k_timer *dummy)
{
	timer_expired = true;
}

void gpio_input_edge_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	uint32_t pin = 0;

	while (pins >>= 1) {
		pin++;
	}

	/* Get pin RAW to confirm pin polarity */
	if (gpio_pin_get_raw(dev, pin)) {
		high++;
	} else {
		low++;
	}
}

int main(void)
{
	int ret;
	int counter = 0;
	uint8_t acc = 0;
	bool test_pass;

	/* SPI buffer sets */
	struct spi_buf tx_spi_buf = {
		.buf = &spim_buffer[0],
		.len = CONFIG_DATA_FIELD
	};
	struct spi_buf_set tx_spi_buf_set = {
		.buffers = &tx_spi_buf,
		.count = 1
	};

	struct spi_buf rx_spi_buf = {
		.buf = &spim_buffer[CONFIG_DATA_FIELD],
		.len = CONFIG_DATA_FIELD
	};
	struct spi_buf_set rx_spi_buf_set = {
		.buffers = &rx_spi_buf,
		.count = 1
	};

	LOG_INF("Multicore idle_spi_loopback test on %s", CONFIG_BOARD_TARGET);
	LOG_INF("Core will sleep for %u ms", CONFIG_TEST_SLEEP_DURATION_MS);
	LOG_INF("Testing SPIM device %s", spim_spec.bus->name);
	LOG_INF("GPIO loopback at %s, pin %d", pin_in.port->name, pin_in.pin);
	LOG_INF("%d bytes of data exchanged at once", CONFIG_DATA_FIELD);
#if defined(CONFIG_TEST_SPI_HOLD_ON_CS)
	LOG_INF("SPI CS lock enabled");
#endif
#if defined(CONFIG_TEST_SPI_LOCK_ON)
	LOG_INF("SPI LOCK enabled");
#endif
#if !defined(CONFIG_TEST_SPI_RELEASE_BEFORE_SLEEP)
	LOG_INF("SPI is never released");
#endif
	LOG_INF("===================================================================");

	ret = spi_is_ready_dt(&spim_spec);
	if (!ret) {
		LOG_ERR("Error: SPI device is not ready");
	}
	__ASSERT_NO_MSG(ret);

	ret = gpio_is_ready_dt(&pin_in);
	if (!ret) {
		LOG_ERR("Device %s is not ready.", pin_in.port->name);
	}
	__ASSERT_NO_MSG(ret);

	/* Configure gpio and init GPIOTE callback. */
	ret = gpio_pin_configure_dt(&pin_in, GPIO_INPUT);
	if (ret) {
		LOG_ERR("gpio_pin_configure_dt() has failed (%d)", ret);
	}
	__ASSERT_NO_MSG(ret == 0);

	gpio_init_callback(&gpio_input_cb_data, gpio_input_edge_callback, BIT(pin_in.pin));

	k_timer_init(&my_timer, my_timer_handler, NULL);

	/* Run test forever. */
	while (1) {
		test_pass = true;
		timer_expired = false;

		/* Clear edge counters. */
		high = 0;
		low = 0;

		/* Start a one-shot timer that expires after 1 second. */
		k_timer_start(&my_timer, K_MSEC(1000), K_NO_WAIT);

		/* Configure and enable GPIOTE. */
		ret = gpio_pin_interrupt_configure_dt(&pin_in, GPIO_INT_EDGE_BOTH);
		if (ret) {
			LOG_ERR("gpio_pin_interrupt_configure_dt() has failed (%d)", ret);
		}
		__ASSERT_NO_MSG(ret == 0);

		ret = gpio_add_callback(pin_in.port, &gpio_input_cb_data);
		if (ret) {
			LOG_ERR("gpio_add_callback() has failed (%d)", ret);
		}
		__ASSERT_NO_MSG(ret == 0);

		/* SPI active transmissions for ~ 1 second. */
		while (!timer_expired) {
			/* Generate pseudo random tx_data for current test. */
			for (int i = 0; i < CONFIG_DATA_FIELD; i++) {
				*((uint8_t *) tx_spi_buf.buf + i) = acc;
				acc += DELTA;
			}

			/* Transmit data. */
			ret = spi_transceive_dt(&spim_spec, &tx_spi_buf_set, &rx_spi_buf_set);
			if (ret != 0) {
				LOG_ERR("spi_transceive_dt, err: %d", ret);
			}
			__ASSERT(ret == 0, "Error: spi_transceive_dt, err: %d\n", ret);

			/* Check if the received data is consistent with the data sent. */
			for (int i = 0; i < CONFIG_DATA_FIELD; i++) {
				uint8_t received = *((uint8_t *) rx_spi_buf.buf + i);
				uint8_t transmitted = *((uint8_t *) tx_spi_buf.buf + i);

				if (received != transmitted) {
					LOG_ERR("FAIL: rx[%d] = %d, expected %d",
						i, received, transmitted);
					test_pass = false;
					__ASSERT(false, "Run %d - FAILED\n", counter);
				}
			}

#if defined(CONFIG_TEST_SPI_HOLD_ON_CS)
			/* At least one SPI transmission has already happened.
			 * When SPI is configured with SPI_HOLD_ON_CS
			 * then SPI CS signal shall be active here.
			 */
			int temp_pin_value = gpio_pin_get_raw(pin_in.port, pin_in.pin);
			int expected_pin_value;

			if (pin_in.dt_flags & GPIO_ACTIVE_LOW) {
				/* GPIO_ACTIVE_LOW */
				expected_pin_value = 0;
			} else {
				/* GPIO_ACTIVE_HIGH */
				expected_pin_value = 1;
			}

			if (temp_pin_value != expected_pin_value) {
				LOG_ERR("SPI CS signal is %d, while expected is %d",
					temp_pin_value, expected_pin_value);
			}
			__ASSERT_NO_MSG(temp_pin_value == expected_pin_value);
#endif
		} /* while (!timer_expired) */

#if defined(CONFIG_TEST_SPI_RELEASE_BEFORE_SLEEP)
		/* Release SPI Chip Select / SPI device. */
		ret = spi_release_dt(&spim_spec);
		if (ret != 0) {
			LOG_ERR("spi_release_dt(), err: %d", ret);
		}
		__ASSERT_NO_MSG(ret == 0);
#endif

		/* Disable GPIOTE interrupt. */
		ret = gpio_remove_callback(pin_in.port, &gpio_input_cb_data);
		if (ret) {
			LOG_ERR("gpio_remove_callback() has failed (%d)", ret);
		}
		__ASSERT_NO_MSG(ret == 0);

		ret = gpio_pin_interrupt_configure_dt(&pin_in, GPIO_INT_DISABLE |
				GPIO_INT_LEVELS_LOGICAL | GPIO_INT_HIGH_1);
		if (ret) {
			LOG_ERR("gpio_pin_interrupt_configure_dt() has failed (%d)", ret);
		}
		__ASSERT_NO_MSG(ret == 0);

		/* Report if communication was successful. */
		if (test_pass) {
			LOG_INF("Run %d - PASS; rising: %u, falling %u", counter, high, low);
		} else {
			LOG_INF("Run %d - FAILED; rising: %u, falling %u", counter, high, low);
		}

#if defined(CONFIG_TEST_SPI_HOLD_ON_CS)
		/* When SPI is configured with SPI_HOLD_ON_CS:
		 * - first spi_transceive_dt() activates SPI CS signal;
		 * - following spi_transceive_dt() calls don't change SPI CS state;
		 * - SPI CS signal is deactivated by spi_release_dt().
		 */
#if defined(CONFIG_TEST_SPI_RELEASE_BEFORE_SLEEP)
		/* Every test iteration will see one SPI CS activation and one deactivation. */
		__ASSERT_NO_MSG(high == 1);
		__ASSERT_NO_MSG(low == 1);
#else /* defined(CONFIG_TEST_SPI_RELEASE_BEFORE_SLEEP) */
		/* SPI CS gets activated in the first iteration and stays active forever. */
		if (counter == 0) {
			/* Observed edge depends on GPIO polarity. */
			if (pin_in.dt_flags & GPIO_ACTIVE_LOW) {
				/* GPIO_ACTIVE_LOW */
				__ASSERT_NO_MSG(high == 0);
				__ASSERT_NO_MSG(low == 1);
			} else {
				/* GPIO_ACTIVE_HIGH */
				__ASSERT_NO_MSG(high == 1);
				__ASSERT_NO_MSG(low == 0);
			}
		} else {
			__ASSERT_NO_MSG(high == 0);
			__ASSERT_NO_MSG(low == 0);
		}
#endif /* defined(CONFIG_TEST_SPI_RELEASE_BEFORE_SLEEP) */
#else
		/* SPI was active for ~1 second with separate SPI CS activations
		 * for each spi_transceive_dt() call.
		 */
		__ASSERT_NO_MSG(high >= 1000);
		__ASSERT_NO_MSG(low >= 1000);
		__ASSERT_NO_MSG(low == high);
#endif
		counter++;

		/* Sleep / enter low power state. */
		k_msleep(CONFIG_TEST_SLEEP_DURATION_MS);
	}

	return 0;
}
