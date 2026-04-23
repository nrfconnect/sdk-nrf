/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/drivers/i2c.h>
#include <nrfx_twis.h>
#include <hal/nrf_twim.h>
#include <hal/nrf_gpio.h>

#include <zephyr/ztest.h>

/* #region agent log helpers */
#define DBG_NODE_TWIM_REG ((NRF_TWIM_Type *)DT_REG_ADDR(DT_BUS(DT_NODELABEL(sensor))))
#define DBG_NODE_TWIS_REG ((NRF_TWIS_Type *)DT_REG_ADDR(DT_ALIAS(i2c_slave)))

static void dbg_dump_twim(const char *tag)
{
	NRF_TWIM_Type *twim = DBG_NODE_TWIM_REG;

	TC_PRINT("[DBG TWIM @%s] ENABLE=0x%x INTEN=0x%x SHORTS=0x%x ERRORSRC=0x%x "
		 "PSEL.SCL=0x%x PSEL.SDA=0x%x ADDR=0x%x\n",
		 tag, twim->ENABLE, twim->INTEN, twim->SHORTS, twim->ERRORSRC,
		 twim->PSEL.SCL, twim->PSEL.SDA, twim->ADDRESS);
	TC_PRINT("[DBG TWIM @%s] EV: STOPPED=%u ERROR=%u SUSPENDED=%u "
		 "LASTRX=%u LASTTX=%u BB=%u\n",
		 tag, twim->EVENTS_STOPPED, twim->EVENTS_ERROR, twim->EVENTS_SUSPENDED,
		 twim->EVENTS_LASTRX, twim->EVENTS_LASTTX, twim->EVENTS_BB);
}

static void dbg_dump_twis(const char *tag)
{
	NRF_TWIS_Type *t = DBG_NODE_TWIS_REG;
	uint32_t pinstatus;

	t->DEBUGENABLE = 0x1UL;
	pinstatus = t->PINSTATUS;

	TC_PRINT("[DBG TWIS @%s] ENABLE=0x%x INTEN=0x%x SHORTS=0x%x ERRORSRC=0x%x "
		 "PSEL.SCL=0x%x PSEL.SDA=0x%x PINSTATUS=0x%x\n",
		 tag, t->ENABLE, t->INTEN, t->SHORTS, t->ERRORSRC,
		 t->PSEL.SCL, t->PSEL.SDA, pinstatus);
	TC_PRINT("[DBG TWIS @%s] EV: STARTED=%u STOPPED=%u ERROR=%u NACKTX=%u "
		 "WRITE=%u READ=%u CSSTARTED=%u CSSTOPPED=%u\n",
		 tag, t->EVENTS_STARTED, t->EVENTS_STOPPED, t->EVENTS_ERROR,
		 t->EVENTS_NACKTX, t->EVENTS_WRITE, t->EVENTS_READ,
		 t->EVENTS_CSSTARTED, t->EVENTS_CSSTOPPED);
}

static void dbg_dump_pin(const char *tag, uint32_t psel)
{
	NRF_GPIO_Type *port;
	uint32_t abs_pin;
	uint32_t pin_in_port;
	uint32_t cnf;

	if (psel & (1u << 31)) {
		TC_PRINT("[DBG PIN @%s] psel=0x%x DISCONNECTED\n", tag, psel);
		return;
	}
	abs_pin = psel & 0xff;
	pin_in_port = abs_pin;
	port = nrf_gpio_pin_port_decode(&pin_in_port);
	cnf = port->PIN_CNF[pin_in_port];
	TC_PRINT("[DBG PIN @%s] psel=0x%x port=%p pin=%u PIN_CNF=0x%x "
		 "DIR=%u INPUT=%u PULL=%u DRIVE=%u SENSE=%u OUT=%u IN=%u\n",
		 tag, psel, (void *)port, (unsigned)pin_in_port, cnf,
		 (cnf >> 0) & 0x1, (cnf >> 1) & 0x1, (cnf >> 2) & 0x3,
		 (cnf >> 8) & 0xf, (cnf >> 16) & 0x3,
		 (port->OUT >> pin_in_port) & 0x1,
		 (port->IN >> pin_in_port) & 0x1);
}
/* #endregion */

#define NODE_TWIM	    DT_NODELABEL(sensor)
#define NODE_TWIS	    DT_ALIAS(i2c_slave)
#define MEASUREMENT_REPEATS 10

#define TWIS_MEMORY_SECTION                                                                        \
	COND_CODE_1(DT_NODE_HAS_PROP(NODE_TWIS, memory_regions),                                   \
		    (__attribute__((__section__(                                                   \
			    LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(NODE_TWIS, memory_regions)))))), \
		    ())

#define TEST_BUFFER_SIZE 128

static nrfx_twis_t twis = {.p_reg = (NRF_TWIS_Type *)DT_REG_ADDR(NODE_TWIS)};
static uint8_t i2c_slave_buffer[TEST_BUFFER_SIZE] TWIS_MEMORY_SECTION;
static uint8_t i2c_master_buffer[TEST_BUFFER_SIZE];

struct i2c_api_twis_fixture {
	const struct device *dev;
	uint8_t addr;
	uint8_t *const master_buffer;
	uint8_t *const slave_buffer;
};

static struct i2c_api_twis_fixture fixture = {
	.dev = DEVICE_DT_GET(DT_BUS(NODE_TWIM)),
	.addr = DT_REG_ADDR(NODE_TWIM),
	.master_buffer = i2c_master_buffer,
	.slave_buffer = i2c_slave_buffer,
};

static void i2s_slave_handler(nrfx_twis_event_t const *p_event)
{
	switch (p_event->type) {
	case NRFX_TWIS_EVT_READ_REQ:
		nrfx_twis_tx_prepare(&twis, i2c_slave_buffer, TEST_BUFFER_SIZE);
		break;
	case NRFX_TWIS_EVT_READ_DONE:
		break;
	case NRFX_TWIS_EVT_WRITE_REQ:
		nrfx_twis_rx_prepare(&twis, i2c_slave_buffer, TEST_BUFFER_SIZE);
		break;
	case NRFX_TWIS_EVT_WRITE_DONE:
		break;
	default:
		break;
	}
}

static void *test_setup(void)
{
	const nrfx_twis_config_t config = {
		.addr = {fixture.addr, 0},
		.skip_gpio_cfg = true,
		.skip_psel_cfg = true,
	};
	int ret;

	zassert_true(device_is_ready(fixture.dev), "TWIM device is not ready");
	zassert_equal(0, nrfx_twis_init(&twis, &config, i2s_slave_handler),
		      "TWIS initialization failed");

	PINCTRL_DT_DEFINE(NODE_TWIS);
	ret = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(NODE_TWIS), PINCTRL_STATE_DEFAULT);
	zassert_ok(ret);

	IRQ_CONNECT(DT_IRQN(NODE_TWIS), DT_IRQ(NODE_TWIS, priority), nrfx_twis_irq_handler, &twis,
		    0);

	nrfx_twis_enable(&twis);

	return NULL;
}

static void prepare_test_data(uint8_t *const buffer)
{
	for (size_t counter = 0; counter < TEST_BUFFER_SIZE; counter++) {
		buffer[counter] = counter;
	}
}

static void cleanup_buffers(void *nullp)
{
	memset(fixture.slave_buffer, 0, TEST_BUFFER_SIZE);
	memset(fixture.master_buffer, 0, TEST_BUFFER_SIZE);
}

static void configure_twim(void)
{
	uint32_t i2c_config = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;

	zassert_ok(i2c_configure(fixture.dev, i2c_config));
}

/*
 * Verify driver ability to recover after long clock stretching (corner case)
 * During I2C transaction, TWIM SCL is 0 either before or after disabling TWIM.
 */
ZTEST(i2c_pan, test_clock_stretching_recovery)
{
	int ret;
	uint32_t scl_pin;

	configure_twim();
	prepare_test_data(fixture.master_buffer);
	memset(fixture.slave_buffer, 0, TEST_BUFFER_SIZE);

	TC_PRINT("STEP 1: TWIM - TWIS transmission with valid configuration\n");
	scl_pin = nrf_twis_scl_pin_get(twis.p_reg);
	TC_PRINT("TWIS SCL pin: 0x%x\n", scl_pin);
	dbg_dump_twim("step1-pre");
	dbg_dump_twis("step1-pre");
	dbg_dump_pin("step1-pre TWIM SCL", DBG_NODE_TWIM_REG->PSEL.SCL);
	dbg_dump_pin("step1-pre TWIS SCL", scl_pin);
	ret = i2c_read(fixture.dev, fixture.master_buffer, TEST_BUFFER_SIZE, fixture.addr);
	zassert_ok(ret, "i2c_read failed: %d\n", ret);
	zassert_mem_equal(fixture.master_buffer, fixture.slave_buffer, TEST_BUFFER_SIZE);

	TC_PRINT("STEP 2: TWIM - TWIS transmission with SCL pulled low "
		 "(ETIMEDOUT error is expected)\n");
	/* Pull TWIS SCL pin low */
	nrf_twis_scl_pin_set(twis.p_reg, 1 << 31);
	nrf_gpio_cfg_output(scl_pin);
	nrf_gpio_pin_clear(scl_pin);
	TC_PRINT("TWIS SCL pin (disconnected): 0x%x\n", nrf_twis_scl_pin_get(twis.p_reg));
	memset(fixture.slave_buffer, 0, TEST_BUFFER_SIZE);
	ret = i2c_read(fixture.dev, fixture.master_buffer, TEST_BUFFER_SIZE, fixture.addr);
	zassert_equal(ret, -ETIMEDOUT,
		      "i2c_read failed with different error than expeced (ETIMEDOUT) %d\n", ret);
	dbg_dump_twim("step2-post-timeout");
	dbg_dump_twis("step2-post-timeout");
	dbg_dump_pin("step2-post TWIM SCL", DBG_NODE_TWIM_REG->PSEL.SCL);
	dbg_dump_pin("step2-post TWIS SCL pin", scl_pin);

	TC_PRINT("STEP 3: TWIM - TWIS transmission after TWIS pin reconfiguration\n");
	/* Restore original TWIS pin configuration */
	PINCTRL_DT_DEFINE(NODE_TWIS);
	ret = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(NODE_TWIS), PINCTRL_STATE_DEFAULT);
	zassert_ok(ret);
	TC_PRINT("TWIS SCL pin (reconfigured): 0x%x\n", nrf_twis_scl_pin_get(twis.p_reg));
	dbg_dump_twim("step3-pre-read");
	dbg_dump_twis("step3-pre-read");
	dbg_dump_pin("step3-pre TWIM SCL", DBG_NODE_TWIM_REG->PSEL.SCL);
	dbg_dump_pin("step3-pre TWIS SCL pin", scl_pin);
	dbg_dump_pin("step3-pre TWIM SDA", DBG_NODE_TWIM_REG->PSEL.SDA);
	dbg_dump_pin("step3-pre TWIS SDA", DBG_NODE_TWIS_REG->PSEL.SDA);

	/* Attempt proper transfer, expected to fail on devices affected by nRf54L anomaly 105. */
	ret = i2c_read(fixture.dev, fixture.master_buffer, TEST_BUFFER_SIZE, fixture.addr);
	TC_PRINT("[DBG] step3 i2c_read ret=%d\n", ret);
	dbg_dump_twim("step3-post-read");
	dbg_dump_twis("step3-post-read");
	dbg_dump_pin("step3-post TWIM SCL", DBG_NODE_TWIM_REG->PSEL.SCL);
	dbg_dump_pin("step3-post TWIM SDA", DBG_NODE_TWIM_REG->PSEL.SDA);
	dbg_dump_pin("step3-post TWIS SCL", DBG_NODE_TWIS_REG->PSEL.SCL);
	dbg_dump_pin("step3-post TWIS SDA", DBG_NODE_TWIS_REG->PSEL.SDA);

	/* Try a 2nd attempt after explicit recover_bus, to test if extra recovery helps */
	TC_PRINT("[DBG] calling i2c_recover_bus then i2c_read again\n");
	(void)i2c_recover_bus(fixture.dev);
	ret = i2c_read(fixture.dev, fixture.master_buffer, TEST_BUFFER_SIZE, fixture.addr);
	TC_PRINT("[DBG] step3 retry i2c_read ret=%d\n", ret);
	dbg_dump_twim("step3-after-retry");
	dbg_dump_twis("step3-after-retry");

	/* HARD RESET attempt: power-cycle the TWIM peripheral by writing 0 to ENABLE,
	 * waiting, restoring frequency/address/PSEL/SHORTS to defaults written by the
	 * driver, then writing back the enable value. If THIS recovers the chip, it
	 * means i2c_nrfx_twim_recover_bus() is incomplete for nRF92. If it still
	 * fails, the chip is hardware-stuck (PAN, analogous to nRF54L errata 105).
	 */
	TC_PRINT("[DBG] HARD RESET TWIM ENABLE 0->6\n");
	{
		NRF_TWIM_Type *twim = DBG_NODE_TWIM_REG;
		uint32_t saved_freq = twim->FREQUENCY;
		uint32_t saved_addr = twim->ADDRESS;
		uint32_t saved_psel_scl = twim->PSEL.SCL;
		uint32_t saved_psel_sda = twim->PSEL.SDA;

		twim->ENABLE = 0;
		k_busy_wait(1000);
		twim->EVENTS_STOPPED = 0;
		twim->EVENTS_ERROR = 0;
		twim->EVENTS_SUSPENDED = 0;
		twim->EVENTS_LASTRX = 0;
		twim->EVENTS_LASTTX = 0;
		twim->EVENTS_BB = 0;
		twim->ERRORSRC = twim->ERRORSRC;
		twim->FREQUENCY = saved_freq;
		twim->ADDRESS = saved_addr;
		twim->PSEL.SCL = saved_psel_scl;
		twim->PSEL.SDA = saved_psel_sda;
		twim->ENABLE = 6;
		k_busy_wait(100);
	}
	dbg_dump_twim("after-hard-reset-pre-read");
	ret = i2c_read(fixture.dev, fixture.master_buffer, TEST_BUFFER_SIZE, fixture.addr);
	TC_PRINT("[DBG] step3 hard-reset retry ret=%d\n", ret);
	dbg_dump_twim("after-hard-reset-post-read");
	dbg_dump_twis("after-hard-reset-post-read");
	if (NRF_ERRATA_DYNAMIC_CHECK(54L, 105)) {
		zassert_equal(ret, -ETIMEDOUT,
			      "i2c_read failed with different error than expeced (ETIMEDOUT) %d\n",
			      ret);
	} else {
		zassert_ok(ret, "i2c_read failed (after SCL release): %d\n", ret);
		zassert_mem_equal(fixture.master_buffer, fixture.slave_buffer, TEST_BUFFER_SIZE);
	}
}

ZTEST_SUITE(i2c_pan, NULL, test_setup, NULL, cleanup_buffers, NULL);
