/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <ppi_seq/ppi_seq.h>
#include <ppi_seq/ppi_seq_i2c_spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <nrfx_twis.h>
#include <nrfx_spis.h>
#include <nrfx_timer.h>
#include <helpers/nrfx_gppi.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio/gpio_nrf.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <dmm.h>
LOG_MODULE_REGISTER(test);

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_ppi_seq_i2c)
#define TEST_I2C 1
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_ppi_seq_spi)
#define TEST_SPI 1
#else
#error "Wrong configuration"
#endif

/* Test is limited only to a basic scenario as ppi_seq functionality is verified by the SPI test. */
#define DUT_NODE    DT_NODELABEL(dut)
#define TESTER_NODE DT_NODELABEL(tester) /* TWIS SPIS node */
#define TIMER_NODE  DT_NODELABEL(test_timer)

#define XFER_TX_LEN 4
#define XFER_RX_LEN 8
#define XFER_LEN    MAX(XFER_RX_LEN, XFER_TX_LEN)
#define XFER_CNT    16
#define TWI_ADDR    0x3f

static uint8_t tester_rx_buf[XFER_LEN] DMM_MEMORY_SECTION(TESTER_NODE);
static uint8_t tester_tx_buf[XFER_LEN] DMM_MEMORY_SECTION(TESTER_NODE);
static uint8_t dut_rx_buf[XFER_CNT * XFER_LEN] DMM_MEMORY_SECTION(DUT_NODE);
static uint8_t dut_tx_buf[XFER_CNT * XFER_LEN] DMM_MEMORY_SECTION(DUT_NODE);

struct spis_twis_tester_data {
	nrfx_gppi_handle_t gppi_handle;
	nrfx_timer_t test_timer;
	nrfx_twis_t twis;
	nrfx_spis_t spis;
	uint32_t exp_time[2];
	uint32_t timestamp;
	uint32_t timestamp_idx;
	uint32_t timestamp_max;
	uint32_t tx_cnt;
	uint32_t rx_cnt;
	uint32_t rx_len;
	uint32_t tx_len;
	uint32_t cb_cnt;
	bool err;
	uint8_t *rx_buf;
	uint8_t *tx_buf;
};

struct seq_dut_data {
	nrfx_twim_t twim;
	uint8_t *tx_buf;
	uint8_t *rx_buf;
	uint32_t cb_cnt;
	uint32_t tx_cnt;
	uint32_t rx_cnt;
	uint32_t rpt;
	uint32_t t_cb;
	bool err;
	struct k_sem end_sem;
};

static struct spis_twis_tester_data tester_data;
static struct seq_dut_data dut_data;

static void test_timer_init(void)
{
	nrfx_timer_config_t timer_config = {
		.frequency = MHZ(1),
		.mode = NRF_TIMER_MODE_TIMER,
		.bit_width = NRF_TIMER_BIT_WIDTH_32,
	};
	int rv;

	tester_data.test_timer.p_reg = (NRF_TIMER_Type *)DT_REG_ADDR(TIMER_NODE);

	rv = nrfx_timer_init(&tester_data.test_timer, &timer_config, NULL);
	zassert_ok(rv);

	nrfx_timer_enable(&tester_data.test_timer);

	uint32_t task =
		nrfx_timer_task_address_get(&tester_data.test_timer, NRF_TIMER_TASK_CAPTURE0);
	uint32_t event = nrf_twis_event_address_get(tester_data.twis.p_reg, NRF_TWIS_EVENT_WRITE);

	event = IS_ENABLED(TEST_I2C)
			? nrf_twis_event_address_get((NRF_TWIS_Type *)DT_REG_ADDR(TESTER_NODE),
						     NRF_TWIS_EVENT_WRITE)
			: nrf_spis_event_address_get((NRF_SPIS_Type *)DT_REG_ADDR(TESTER_NODE),
						     NRF_SPIS_EVENT_RXSTARTED);

	rv = nrfx_gppi_conn_alloc(event, task, &tester_data.gppi_handle);
	zassert_ok(rv);

	nrfx_gppi_conn_enable(tester_data.gppi_handle);
}

static void check_time(void)
{
	uint32_t capt = nrfx_timer_capture_get(&tester_data.test_timer, NRF_TIMER_CC_CHANNEL0);

	if (tester_data.timestamp != 0) {
		uint32_t diff = capt - tester_data.timestamp;
		uint32_t exp_time = tester_data.exp_time[tester_data.timestamp_idx];

		zassert_within(diff, exp_time, exp_time / 10,
			       "Time between operations %d us not within:%d with delta:%d", diff,
			       exp_time, exp_time / 10);
		tester_data.timestamp_idx++;
		if (tester_data.timestamp_idx == tester_data.timestamp_max) {
			tester_data.timestamp_idx = 0;
		}
	}
	tester_data.timestamp = capt;
}

static void prepare_buf(uint8_t *buf, size_t cnt, uint32_t init_val)
{
	for (int i = 0; i < cnt; i++) {
		buf[i] = (uint8_t)(init_val + i);
	}
}

static bool check_buf(uint8_t *buf, size_t cnt, uint8_t exp_val, int line)
{
	for (size_t i = 0; i < cnt; i++) {
		if (buf[i] != exp_val) {
			zassert_equal(buf[i], exp_val,
				      "%d: Unexpected buffer content at %d, exp:%02x got:%02x",
				      line, i, exp_val, buf[i]);
			return false;
		}
		exp_val++;
	}

	return true;
}

static void i2c_slave_handler(nrfx_twis_event_t const *event)
{
	switch (event->type) {
	case NRFX_TWIS_EVT_READ_REQ:
		LOG_DBG("TWIS event: read request");
		prepare_buf(tester_data.tx_buf, XFER_RX_LEN, tester_data.tx_cnt);
		nrfx_twis_tx_prepare(&tester_data.twis, tester_data.tx_buf, XFER_RX_LEN);
		break;
	case NRFX_TWIS_EVT_READ_DONE:
		LOG_DBG("TWIS event: read done: %d", event->data.tx_amount);
		tester_data.tx_cnt += event->data.tx_amount;
		tester_data.cb_cnt++;
		zassert_equal(XFER_RX_LEN, event->data.tx_amount);
		break;
	case NRFX_TWIS_EVT_WRITE_REQ: {
		nrfx_twis_rx_prepare(&tester_data.twis, tester_data.rx_buf, XFER_TX_LEN);
		check_time();

		LOG_DBG("TWIS event: write request");
		break;
	}
	case NRFX_TWIS_EVT_WRITE_DONE:
		if (tester_data.err == false) {
			tester_data.err = check_buf(tester_data.rx_buf, event->data.rx_amount,
						    (uint8_t)tester_data.rx_cnt, __LINE__);
		}
		LOG_DBG("TWIS event: write done: %d", event->data.rx_amount);
		tester_data.rx_cnt += event->data.rx_amount;
		zassert_equal(XFER_TX_LEN, event->data.rx_amount);
		break;
	default:
		LOG_ERR("TWIS unexpected event: %d", event->type);
		break;
	}
}

static void next_spis(void)
{
	int rv;

	prepare_buf(tester_data.tx_buf, XFER_LEN, tester_data.tx_cnt);
	tester_data.tx_cnt += XFER_LEN;

	rv = nrfx_spis_buffers_set(&tester_data.spis, tester_data.tx_buf, XFER_LEN,
				   tester_data.rx_buf, XFER_LEN);
	zassert_ok(rv, "Unexpected error:%d", rv);
}

static void spis_event_handler(nrfx_spis_event_t const *event, void *context)
{
	if (event->evt_type == NRFX_SPIS_XFER_DONE) {
		zassert_equal(XFER_LEN, event->rx_amount);
		zassert_equal(XFER_LEN, event->tx_amount);

		check_time();

		if (tester_data.err == false) {
			tester_data.err = check_buf(tester_data.rx_buf, XFER_LEN,
						    tester_data.rx_cnt, __LINE__);
		}
		tester_data.rx_cnt += XFER_LEN;
		tester_data.cb_cnt++;
		next_spis();
	}
}

static void spis_init(void)
{
	int rv;
	nrfx_spis_config_t config = {
		.mode = NRF_SPIS_MODE_3,
		.bit_order = NRF_SPIS_BIT_ORDER_MSB_FIRST,
		.skip_gpio_cfg = true,
		.skip_psel_cfg = true,
	};

#ifdef TEST_SPI
	PINCTRL_DT_DEFINE(TESTER_NODE);

	rv = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(TESTER_NODE), PINCTRL_STATE_DEFAULT);
	zassert_ok(rv, "Unexpected error:%d", rv);

	IRQ_CONNECT(DT_IRQN(TESTER_NODE), DT_IRQ_BY_IDX(TESTER_NODE, 0, priority),
		    nrfx_spis_irq_handler, &tester_data.spis, 0);
#endif

	tester_data.spis.p_reg = (NRF_SPIS_Type *)DT_REG_ADDR(TESTER_NODE);
	rv = nrfx_spis_init(&tester_data.spis, &config, spis_event_handler, NULL);
	zassert_ok(rv, "Unexpected error:%d", rv);
}

static void twis_init(void)
{
	int rv;
	const nrfx_twis_config_t config = {
		.addr = {TWI_ADDR, 0},
		.skip_gpio_cfg = true,
		.skip_psel_cfg = true,
	};

#ifdef TEST_I2C
	IRQ_CONNECT(DT_IRQN(TESTER_NODE), DT_IRQ_BY_IDX(TESTER_NODE, 0, priority),
		    nrfx_twis_irq_handler, &tester_data.twis, 0);
	PINCTRL_DT_DEFINE(TESTER_NODE);

	rv = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(TESTER_NODE), PINCTRL_STATE_DEFAULT);
	zassert_ok(rv, "Unexpected error:%d", rv);
#endif

	tester_data.twis.p_reg = (NRF_TWIS_Type *)DT_REG_ADDR(TESTER_NODE);
	rv = nrfx_twis_init(&tester_data.twis, &config, i2c_slave_handler);
	zassert_ok(rv, "Unexpected error:%d", rv);

	nrfx_twis_enable(&tester_data.twis);
}

static void ppi_seq_common_cb(const struct device *dev, struct ppi_seq_i2c_spi_batch *batch,
			      bool last, void *user_data, uint8_t *rx_buf, size_t rx_length)
{
	uint32_t xfers_cnt = batch->batch_cnt;
	uint32_t rx_bytes = xfers_cnt * rx_length;

	dut_data.t_cb = k_cycle_get_32();
	dut_data.cb_cnt++;
	if (last) {
		k_sem_give((struct k_sem *)&dut_data.end_sem);
	}

	if (dut_data.rpt > 0) {
		dut_data.rpt--;
		if (dut_data.rpt == 0) {
			ppi_seq_i2c_spi_stop(dev, true);
		}
	}

	if (dut_data.err == false) {
		dut_data.err = check_buf(rx_buf, rx_bytes, (uint8_t)dut_data.rx_cnt, __LINE__);
		dut_data.rx_cnt += rx_bytes;
	}
}

static void ppi_seq_twim_cb(const struct device *dev, struct ppi_seq_i2c_spi_batch *batch,
			    bool last, void *user_data)
{
	uint8_t *rx_buf = batch->desc.twim.p_secondary_buf;

	ppi_seq_common_cb(dev, batch, last, user_data, rx_buf, XFER_RX_LEN);
}

static void ppi_seq_spim_cb(const struct device *dev, struct ppi_seq_i2c_spi_batch *batch,
			    bool last, void *user_data)
{
	uint8_t *rx_buf = batch->desc.spim.p_rx_buffer;

	ppi_seq_common_cb(dev, batch, last, user_data, rx_buf, XFER_LEN);
}

static uint32_t single_run(struct ppi_seq_i2c_spi_job *job, uint32_t period, uint32_t timeout)
{
	static const struct device *ppi_seq_dev = DEVICE_DT_GET(DUT_NODE);
	int rv;
	uint32_t t;
	uint32_t prev_rx_cnt = dut_data.rx_cnt;
	uint32_t prev_cb_cnt = dut_data.cb_cnt;
	uint32_t prev_twis_tx_cnt = tester_data.tx_cnt;
	uint32_t exp_rx_bytes = XFER_CNT * XFER_RX_LEN;

	k_sem_init(&dut_data.end_sem, 0, 1);
	t = k_cycle_get_32();
	rv = ppi_seq_i2c_spi_start(ppi_seq_dev, period, job);
	zassert_ok(rv, "Unexpected error:%d", rv);

	rv = k_sem_take(&dut_data.end_sem, K_USEC(timeout));
	zassert_ok(rv, "Unexpected error:%d", rv);

	zassert_equal(dut_data.rx_cnt - prev_rx_cnt, exp_rx_bytes,
		      "Unexpected amount of bytes received: %d, exp:%d",
		      dut_data.rx_cnt - prev_rx_cnt, exp_rx_bytes);
	zassert_equal(dut_data.cb_cnt - prev_cb_cnt, 1);
	zassert_equal(tester_data.tx_cnt - prev_twis_tx_cnt, exp_rx_bytes);

	return dut_data.t_cb - t;
}

static uint32_t op_time(uint32_t bytes)
{
	uint32_t frequency = DT_PROP_OR(DUT_NODE, clock_frequency, DT_PROP(DUT_NODE, frequency));

	return (uint32_t)(((uint64_t)(8 * bytes) * 1000000) / frequency);
}

static void test_single_run(void)
{
	static const struct device *ppi_seq_dev = DEVICE_DT_GET(DUT_NODE);
	nrfx_twim_xfer_desc_t twim_desc = {
		.type = NRFX_TWIM_XFER_TXRX,
		.address = TWI_ADDR,
		.primary_length = XFER_TX_LEN,
		.secondary_length = XFER_RX_LEN,
		.p_primary_buf = dut_data.tx_buf,
		.p_secondary_buf = dut_data.rx_buf,
	};
	nrfx_spim_xfer_desc_t spim_desc = {
		.tx_length = XFER_LEN,
		.rx_length = XFER_LEN,
		.p_tx_buffer = dut_data.tx_buf,
		.p_rx_buffer = dut_data.rx_buf,
	};
	struct ppi_seq_i2c_spi_job job = {
		.batch_cnt = XFER_CNT,
		.repeat = 1,
		.tx_postinc = true,
	};
	uint32_t rpt = XFER_CNT;
	uint32_t period = 2000;
	uint32_t exp_time;
	uint32_t delta;
	uint32_t tx_len;
	uint32_t rx_len;
	uint32_t t;
	int rv;

	if (IS_ENABLED(TEST_I2C)) {
		job.desc.twim = twim_desc;
		job.cb = ppi_seq_twim_cb;
		tx_len = XFER_TX_LEN;
		rx_len = XFER_RX_LEN;
	} else {
		job.desc.spim = spim_desc;
		job.cb = ppi_seq_spim_cb;
		tx_len = XFER_LEN;
		rx_len = XFER_LEN;
	}

	if (DT_NODE_HAS_PROP(DUT_NODE, timer) && !DT_NODE_HAS_PROP(DUT_NODE, timer_notifier) &&
	    !DT_NODE_HAS_PROP(DUT_NODE, extra_transfers)) {
		/* Unsupported configuration. */
		zassert_false(device_is_ready(ppi_seq_dev));
		return;
	}

	/* If TIMER is used for triggering then enable HFXO to ensure clock accuracy. */
#if DT_NODE_HAS_PROP(DUT_NODE, timer)
	struct onoff_client cli;

	sys_notify_init_spinwait(&cli.notify);
	rv = onoff_request(z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF), &cli);
	zassert_ok(rv);

	while (sys_notify_fetch_result(&cli.notify, &rv)) {
		/* pend until clock is up and running */
	}
	zassert_ok(rv);
#endif

	if (DT_NODE_HAS_PROP(DUT_NODE, timer_notifier)) {
		uint32_t bytes = IS_ENABLED(TEST_I2C) ? 2 * (XFER_RX_LEN + XFER_TX_LEN) : XFER_LEN;

		/* Interrupt is synchronized with the last event so it should be more accurate.
		 * Only inaccuracy is due to interrupt latency and handling.
		 */
		delta = 200;
		/* Transfer time need to be included. */
		exp_time = op_time(bytes);

	} else {
		delta = 100 + period / 2;
		exp_time = 0;
	}

	zassert_true(device_is_ready(ppi_seq_dev));

	if (DT_NODE_HAS_PROP(DUT_NODE, extra_transfers)) {
		uint32_t offset = COND_CODE_1(DT_NODE_HAS_PROP(DUT_NODE, extra_transfers),
					      (DT_PROP_BY_IDX(DUT_NODE, extra_transfers, 0)), (0));

		exp_time += (rpt / 2 - 1) * period + delta / 2;
		tester_data.exp_time[0] = period - offset;
		tester_data.exp_time[1] = offset;
		tester_data.timestamp_idx = 1;
		tester_data.timestamp_max = 2;
	} else {
		exp_time += (rpt - 1) * period + delta / 2;
		tester_data.exp_time[0] = period;
		tester_data.timestamp_idx = 0;
		tester_data.timestamp_max = 1;
	}

	if (IS_ENABLED(TEST_SPI)) {
		next_spis();
	}

	/* Start by performing a single manual transfer. */
	prepare_buf(dut_data.tx_buf, tx_len, dut_data.tx_cnt);
	dut_data.tx_cnt = tx_len;
	dut_data.rx_cnt += rx_len;
	rv = ppi_seq_i2c_spi_xfer(ppi_seq_dev, &job.desc);
	zassert_ok(rv);

	/* Reset timestamp in tester to avoid validating timing in the first transfer. */
	tester_data.timestamp = 0;
	prepare_buf(dut_data.tx_buf, tx_len * XFER_CNT, dut_data.tx_cnt);
	dut_data.tx_cnt = tx_len * XFER_CNT;

	t = single_run(&job, period, exp_time + 1000);

	zassert_within(t, exp_time, delta, "Time %d us not within:%d with delta:%d", t, exp_time,
		       delta);

#if DT_NODE_HAS_PROP(DUT_NODE, timer)
	rv = onoff_release(z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF));
#endif
}

ZTEST(ppi_seq_twim, test_single_run)
{
	test_single_run();
}

static void before(void *not_used)
{
	ARG_UNUSED(not_used);

	memset(&tester_data, 0, sizeof(tester_data));
	tester_data.rx_buf = tester_rx_buf;
	tester_data.tx_buf = tester_tx_buf;
	memset(&dut_data, 0, sizeof(dut_data));
	dut_data.rx_buf = dut_rx_buf;
	dut_data.tx_buf = dut_tx_buf;

	if (IS_ENABLED(TEST_I2C)) {
		twis_init();
	} else {
		spis_init();
	}
	test_timer_init();
}

ZTEST_SUITE(ppi_seq_twim, NULL, NULL, before, NULL, NULL);
