/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/ztest.h>
#include <zephyr/debug/cpu_load.h>
#include <zephyr/irq.h>
#include <nrfx_timer.h>

#if IS_ENABLED(CONFIG_TEST_BENCHMARK_SCK_DELAY)
#include <helpers/nrfx_gppi.h>
#include <gpiote_nrfx.h>
#include <nrfx_gpiote.h>
#endif

#define MSPI_DEV_NODE DT_NODELABEL(dut_mspi_dev)
#define MSPI_BUS_NODE DT_BUS(MSPI_DEV_NODE)

#define MSPI_FREQ_HZ DT_PROP(MSPI_DEV_NODE, mspi_max_frequency)

static struct mspi_dev_cfg mspi_device_cfg = MSPI_DEVICE_CONFIG_DT(MSPI_DEV_NODE);
static const struct mspi_dev_id mspi_device_id = MSPI_DEVICE_ID_DT(MSPI_DEV_NODE);

static const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

#define TEST_TIMER_NODE DT_ALIAS(test_timer)
#define TEST_TIMER_FREQUENCY MHZ(16)

#define TEST_MAX_PACKET_COUNT 5
#define TEST_MAX_PACKET_SIZE 1024
#define TEST_MEASUREMENT_COUNT 10
#define TEST_XFER_DELAY_MS 10
#define TEST_XFER_TIMEOUT_MS 100

/* Max accepted SCK delay value used in the test (microseconds). */
#define TEST_ALLOWED_SCK_DELAY_US 100U

/*
 * Factor used to calculate the maximum duration of an MSPI transfer
 * based on the time required to physically transmit the data on the bus.
 *
 * This isn't an ideal method - relative overhead is much greater in short transfers.
 */
#define TEST_ALLOWED_XFER_TIME_FACTOR 6

static nrfx_timer_t timer = NRFX_TIMER_INSTANCE(DT_REG_ADDR(TEST_TIMER_NODE));

#if IS_ENABLED(CONFIG_TEST_BENCHMARK_SCK_DELAY)
#define TEST_GPIO_PIN NRF_DT_GPIOS_TO_PSEL(DT_PATH(zephyr_user), test_gpios)
#define TEST_GPIOTE_INST NRF_DT_GPIOTE_INST(DT_PATH(zephyr_user), test_gpios)
#define TEST_GPIOTE_NODE DT_NODELABEL(CONCAT(gpiote, TEST_GPIOTE_INST))

static nrfx_gpiote_t *gpiote = &GPIOTE_NRFX_INST_BY_NODE(TEST_GPIOTE_NODE);
#endif

static const int test_packet_sizes[] = { 64, 256, 1024 };

static uint8_t data_buf[TEST_MAX_PACKET_SIZE * TEST_MAX_PACKET_COUNT];
static struct mspi_xfer_packet packets[TEST_MAX_PACKET_COUNT];

/*
 * Set the direction and size on every packet in the shared "packets" array and
 * fill the shared data buffer with a fixed pattern. The remaining packet
 * fields (cmd, address, cb_mask, data_buf) are configured once in setup() and
 * are left untouched here, since they are the same for every test case.
 */
static void prepare_packets(enum mspi_xfer_direction dir, uint32_t packet_size)
{
	zassert_true((packet_size > 0) && (packet_size <= TEST_MAX_PACKET_SIZE),
		     "Invalid packet size: %u", packet_size);

	for (size_t i = 0; i < ARRAY_SIZE(packets); i++) {
		packets[i].dir = dir;
		packets[i].num_bytes = packet_size;
	}

	/* Fill the buffer with dummy data for TX transfer. */
	if (dir == MSPI_TX) {
		for (size_t i = 0; i < ARRAY_SIZE(data_buf); i++) {
			data_buf[i] = (uint8_t)i;
		}
	}
}

static void *setup(void)
{
	int ret;

	/* Configure MSPI */
	zassert_true(device_is_ready(mspi_bus), "MSPI bus is not ready");

	ret = mspi_dev_config(mspi_bus, &mspi_device_id, MSPI_DEVICE_CONFIG_ALL, &mspi_device_cfg);
	zassert_equal(ret, 0, "mspi_dev_config failed: %d", ret);

	/* Connect TIMER instance IRQ to irq handler */
	IRQ_CONNECT(DT_IRQN(TEST_TIMER_NODE), DT_IRQ(TEST_TIMER_NODE, priority),
		    nrfx_timer_irq_handler, &timer, 0);

#if IS_ENABLED(CONFIG_TEST_BENCHMARK_SCK_DELAY)
	/* Connect GPIOTE instance IRQ to irq handler */
	IRQ_CONNECT(DT_IRQN(TEST_GPIOTE_NODE), DT_IRQ(TEST_GPIOTE_NODE, priority),
		    nrfx_gpiote_irq_handler, &GPIOTE_NRFX_INST_BY_NODE(TEST_GPIOTE_NODE), 0);
#endif
	/*
	 * Packet template shared by every test case, .dir and .num_bytes
	 * are set later by prepare_packets() inside each test. .cmd and
	 * .address are arbitrary fixed values - the benchmark needs a non-zero
	 * command/address phase (see .addr_length in the xfer configs below).
	 */
	struct mspi_xfer_packet packet = {
		.cb_mask = MSPI_BUS_NO_CB,
		.cmd = 0x55,
		.address = 0xAA,
	};

	for (size_t i = 0; i < ARRAY_SIZE(packets); i++) {
		packet.data_buf = data_buf + i * TEST_MAX_PACKET_SIZE;
		packets[i] = packet;
	}
	return NULL;
}

#if IS_ENABLED(CONFIG_TEST_BENCHMARK_SCK_DELAY)
/*
 * Measure the delay between issuing mspi_transceive() and the
 * first clock edge actually appearing on the bus, i.e. the software/driver
 * call overhead that precedes the on-wire transfer.
 *
 * This requires the MSPI clock line to be jumpered externally to the GPIO configured
 * via "test-gpios". A GPIOTE input on that pin detects the rising edge and
 * triggers a hardware timer capture.
 */
static uint32_t measure_sck_delay_us(struct mspi_xfer *xfer)
{
	uint8_t gpiote_channel;
	nrfx_gppi_handle_t ppi_handle;
	int ret = nrfx_gpiote_init(gpiote, 0);

	zassert_equal(ret, 0, "nrfx_gpiote_init: %d", ret);

	ret = nrfx_gpiote_channel_alloc(gpiote, &gpiote_channel);
	zassert_equal(ret, 0, "nrfx_gpiote_channel_alloc: %d", ret);

	nrf_gpio_pin_pull_t pull_config = NRF_GPIO_PIN_NOPULL;

	nrfx_gpiote_trigger_config_t trigger_config = {
		.trigger = NRFX_GPIOTE_TRIGGER_LOTOHI,
		.p_in_channel = &gpiote_channel,
	};

	nrfx_gpiote_handler_config_t handler_config = {
		.handler = NULL,
	};

	nrfx_gpiote_input_pin_config_t input_config = {
		.p_pull_config = &pull_config,
		.p_trigger_config = &trigger_config,
		.p_handler_config = &handler_config
	};

	ret = nrfx_gpiote_input_configure(gpiote, TEST_GPIO_PIN, &input_config);
	zassert_equal(ret, 0, "nrfx_gpiote_input_configure failed: %d", ret);

	nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(TEST_TIMER_FREQUENCY);

	timer_config.bit_width = NRF_TIMER_BIT_WIDTH_32;

	ret = nrfx_timer_init(&timer, &timer_config, NULL);
	zassert_equal(ret, 0, "nrfx_timer_init failed: %d", ret);

	ret = nrfx_gppi_conn_alloc(nrfx_gpiote_in_event_address_get(gpiote, TEST_GPIO_PIN),
				   nrfx_timer_task_address_get(&timer, NRF_TIMER_TASK_CAPTURE0),
				   &ppi_handle);
	zassert_equal(ret, 0, "nrfx_gppi_conn_alloc failed: %d", ret);

	/* Stop the timer on the same edge so it does not keep running past the first capture. */
	nrfx_gppi_ep_attach(nrfx_timer_task_address_get(&timer, NRF_TIMER_TASK_STOP), ppi_handle);
	nrfx_gppi_conn_enable(ppi_handle);
	nrfx_gpiote_trigger_enable(gpiote, TEST_GPIO_PIN, true);

	uint32_t total_ticks = 0;
	uint32_t ticks = 0;
	int xfer_ret = 0;

	for (int i = 0; i < TEST_MEASUREMENT_COUNT; i++) {
		nrfx_timer_clear(&timer);
		/* Zero CC0 register to be able to know when an edge was not detected */
		nrfx_timer_capture(&timer, NRF_TIMER_CC_CHANNEL0);
		nrfx_timer_enable(&timer);

		xfer_ret = mspi_transceive(mspi_bus, &mspi_device_id, xfer);

		/*
		 * Read the value of CC register which was latched on the first rising edge
		 * of the clock signal.
		 */
		ticks = nrfx_timer_capture_get(&timer, NRF_TIMER_CC_CHANNEL0);

		nrfx_timer_disable(&timer);

		if (xfer_ret != 0 || ticks == 0) {
			/* Stop early, the failure is asserted after cleanup. */
			break;
		}

		total_ticks += ticks;

		k_msleep(TEST_XFER_DELAY_MS);
	}
	nrfx_gppi_conn_disable(ppi_handle);
	nrfx_gppi_ep_clear(nrfx_gpiote_in_event_address_get(gpiote, TEST_GPIO_PIN));
	nrfx_gppi_ep_clear(nrfx_timer_task_address_get(&timer, NRF_TIMER_TASK_CAPTURE0));
	nrfx_gppi_ep_clear(nrfx_timer_task_address_get(&timer, NRF_TIMER_TASK_STOP));
	nrfx_gppi_domain_conn_free(ppi_handle);

	nrfx_gpiote_channel_free(gpiote, gpiote_channel);
	nrfx_gpiote_uninit(gpiote);
	nrfx_timer_uninit(&timer);

	zassert_equal(xfer_ret, 0, "mspi_transceive failed: %d", xfer_ret);
	zassert_true(ticks > 0, "No SCK edge was detected, loopback not connected?");

	/*
	 * Convert the summed ticks (clocked at TEST_TIMER_FREQUENCY)
	 * into an average delay in microseconds.
	 */
	uint32_t avg_sck_delay_us = (uint64_t)total_ticks * (uint64_t)MHZ(1) /
				    (uint64_t)(TEST_MEASUREMENT_COUNT * TEST_TIMER_FREQUENCY);

	return avg_sck_delay_us;
}

/*
 * For a fixed packet size, sweep the packet count from 1 to
 * TEST_MAX_PACKET_COUNT and print the average SCK delay measured for each,
 * so the effect of batching more packets into one transfer can be seen.
 */
static void run_sck_delay_sweep(enum mspi_xfer_direction dir, uint32_t packet_size)
{
	prepare_packets(dir, packet_size);

	struct mspi_xfer xfer = {
		.async = false,
		.xfer_mode = MSPI_PIO,
		.tx_dummy = 0,
		.rx_dummy = 0,
		.cmd_length = 0,
		/*
		 * sQSPI requires a non-zero command/address phase to start clocking an
		 * RX-only transfer; apply it to TX as well so both directions carry the
		 * same fixed overhead.
		 */
		.addr_length = 1,
		.hold_ce = false,
		/* Only one xfer is ever in flight in this benchmark; the value is arbitrary. */
		.priority = 1,
		.packets = packets,
		.timeout = TEST_XFER_TIMEOUT_MS,
	};

	TC_PRINT(" Packets | Total Size [B] | Delay [us]\n");
	for (uint32_t i = 1; i <= TEST_MAX_PACKET_COUNT; i++) {
		xfer.num_packet = i;

		uint32_t sck_delay_us = measure_sck_delay_us(&xfer);

		TC_PRINT(" %7u | %14u | %10u\n", i, i * packet_size, sck_delay_us);

		zexpect_true(sck_delay_us <= TEST_ALLOWED_SCK_DELAY_US,
			     "SCK delay was %u us, expected at most %u us",
			     sck_delay_us, TEST_ALLOWED_SCK_DELAY_US);
	}
}

/*
 * Measure the SCK delay - the software/driver overhead between calling
 * mspi_transceive() and the first clock edge appearing on the bus - for TX
 * transfers, across the range of packet sizes and packet counts defined
 * above.
 */
ZTEST(xspi_performance_mspi_api, test_sck_delay_tx)
{
	for (int i = 0; i < ARRAY_SIZE(test_packet_sizes); i++) {
		TC_PRINT("Packet size: %d B\n", test_packet_sizes[i]);
		run_sck_delay_sweep(MSPI_TX, test_packet_sizes[i]);
	}
}

/* Same as test_sck_delay_tx, but for RX transfers. */
ZTEST(xspi_performance_mspi_api, test_sck_delay_rx)
{
	for (int i = 0; i < ARRAY_SIZE(test_packet_sizes); i++) {
		TC_PRINT("Packet size: %d B\n", test_packet_sizes[i]);
		run_sck_delay_sweep(MSPI_RX, test_packet_sizes[i]);
	}
}
#endif /* IS_ENABLED(CONFIG_TEST_BENCHMARK_SCK_DELAY) */

/*
 * Measure the total duration of a synchronous mspi_transceive()
 * and the average CPU load while it was in progress.
 */
static void measure_total_xfer_time_us_cpu_load(struct mspi_xfer *xfer,
						uint32_t *out_time,
						int *out_cpu_load)
{
	int ret;
	nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(TEST_TIMER_FREQUENCY);

	timer_config.bit_width = NRF_TIMER_BIT_WIDTH_32;

	ret = nrfx_timer_init(&timer, &timer_config, NULL);
	zassert_equal(ret, 0, "nrfx_timer_init failed: %d", ret);

	uint32_t total_ticks = 0;
	int total_load = 0;
	int xfer_ret = 0;

	for (int i = 0; i < TEST_MEASUREMENT_COUNT; i++) {
		nrfx_timer_clear(&timer);

		/* Reset the CPU load accounting so the next read reflects only this transfer. */
		(void)cpu_load_get(true);
		nrfx_timer_enable(&timer);

		xfer_ret = mspi_transceive(mspi_bus, &mspi_device_id, xfer);

		/* Read the elapsed time and the CPU load right after the call returns. */
		uint32_t ticks = nrfx_timer_capture(&timer, NRF_TIMER_CC_CHANNEL0);
		int load = cpu_load_get(false);

		nrfx_timer_disable(&timer);

		if (xfer_ret != 0) {
			/* Stop early, the failure is asserted after cleanup. */
			break;
		}

		total_ticks += ticks;
		total_load += load;

		k_msleep(TEST_XFER_DELAY_MS);
	}
	nrfx_timer_uninit(&timer);

	zassert_equal(xfer_ret, 0, "mspi_transceive failed: %d", xfer_ret);

	/*
	 * Convert the summed ticks (clocked at TEST_TIMER_FREQUENCY)
	 * into an average time in microseconds.
	 */
	*out_time = (uint64_t)total_ticks * (uint64_t)MHZ(1) /
		    (uint64_t)(TEST_MEASUREMENT_COUNT * TEST_TIMER_FREQUENCY);

	*out_cpu_load = total_load / TEST_MEASUREMENT_COUNT;
}

/*
 * For a fixed packet size, sweep the packet count from 1 to
 * TEST_MAX_PACKET_COUNT and print the average total transfer time and CPU
 * load measured for each.
 */
static void run_total_time_sweep(enum mspi_xfer_direction dir, uint32_t packet_size)
{
	prepare_packets(dir, packet_size);

	struct mspi_xfer xfer = {
		.async = false,
		.xfer_mode = MSPI_PIO,
		.tx_dummy = 0,
		.rx_dummy = 0,
		.cmd_length = 0,
		.addr_length = 1,
		.hold_ce = false,
		/* Only one xfer is ever in flight in this benchmark; the value is arbitrary. */
		.priority = 1,
		.packets = packets,
		.timeout = TEST_XFER_TIMEOUT_MS,
	};

	TC_PRINT(" Packets | Total Size [B] | Time [us] | CPU Load [1/1000]\n");
	for (uint32_t i = 1; i <= TEST_MAX_PACKET_COUNT; i++) {
		xfer.num_packet = i;

		uint32_t xfer_time_us;
		int cpu_load;

		measure_total_xfer_time_us_cpu_load(&xfer, &xfer_time_us, &cpu_load);

		TC_PRINT(" %7u | %14u | %9u | %17u\n", i, i * packet_size, xfer_time_us, cpu_load);

		/* In 4-lane SPI every byte needs 2 clock cycles, hence the 2 in fomula below */
		uint32_t bus_xfer_time_us = (i * packet_size + xfer.addr_length) * 2 /
					    (MSPI_FREQ_HZ / MHZ(1));
		uint32_t allowed_xfer_time_us = TEST_ALLOWED_XFER_TIME_FACTOR * bus_xfer_time_us;

		zexpect_true(xfer_time_us <= allowed_xfer_time_us,
			     "Transfer took %u us, expected at most %u us",
			     xfer_time_us, allowed_xfer_time_us);
	}
}

/*
 * Measure the total duration of the synchronous mspi_transceive() call and
 * the average CPU load while it runs, for TX transfers.
 */
ZTEST(xspi_performance_mspi_api, test_total_time_tx)
{
	for (int i = 0; i < ARRAY_SIZE(test_packet_sizes); i++) {
		TC_PRINT("Packet size: %d B\n", test_packet_sizes[i]);
		run_total_time_sweep(MSPI_TX, test_packet_sizes[i]);
	}
}

/* Same as test_total_time_tx, but for RX transfers. */
ZTEST(xspi_performance_mspi_api, test_total_time_rx)
{
	for (int i = 0; i < ARRAY_SIZE(test_packet_sizes); i++) {
		TC_PRINT("Packet size: %d B\n", test_packet_sizes[i]);
		run_total_time_sweep(MSPI_RX, test_packet_sizes[i]);
	}
}

ZTEST_SUITE(xspi_performance_mspi_api, NULL, setup, NULL, NULL, NULL);
