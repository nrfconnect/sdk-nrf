/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <hal/nrf_gpio.h>
#include <soc.h>
#include <dmm.h>
#include <nrfx_spim.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rem_temp_monitor);
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/random/random.h>
#include "ipc_comm.h"
#include "temp_monitor.h"

#ifdef CONFIG_RISCV_CORE_NORDIC_VPR
#define REMOTE_SUPPORT 1
#endif

/* Module is using LPS22HH (temperature and pressure sensor). It allows to monitor
 * temperature and notify cpuapp if temperature falls outside of the defined range.
 * If the sensor is not present then temperature results are simulated.
 * nrfx_spim driver in a blocking mode is used because transfers are short (up to 3 bytes)
 * which lasts few microseconds so it is ok to just busywait for the end of the transfers.
 */

#define LPS22HH_WHO_AM_I_VAL 0xB3
#define LPS22HH_WHO_AM_I     0x0F
#define LPS22HH_CTRL_REG1    0x10
#define LPS22HH_TEMP_OUT     0x2B
#define LPS22HH_ODR_REG_VAL(val)                                                                   \
	(val < 10    ? 1                                                                           \
	 : val < 25  ? 2                                                                           \
	 : val < 50  ? 3                                                                           \
	 : val < 75  ? 4                                                                           \
	 : val < 100 ? 5                                                                           \
	 : val < 200 ? 6                                                                           \
		     : 7)

#define SPI_NODE DT_NODELABEL(app_spi)
#define SPI_MODE                                                                                   \
	(SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_MSB | SPI_MODE_CPHA | SPI_MODE_CPOL)
#define SPIM_OP (SPI_OP_MODE_MASTER | SPI_MODE)

#define SPIM_CONFIG(node)                                                                          \
	{                                                                                          \
		.ss_pin = NRF_GPIO_PIN_MAP(DT_PROP(DT_GPIO_CTLR(node, cs_gpios), port),            \
					   DT_GPIO_PIN(node, cs_gpios)),                           \
		.bit_order = NRF_SPIM_BIT_ORDER_MSB_FIRST,                                         \
		.rx_delay = DT_PROP(node, rx_delay),                                               \
		.frequency = 8000000,                                                              \
		.mode = NRF_SPIM_MODE_3,                                                           \
		.use_hw_ss = true,                                                                 \
		.ss_duration = 16,                                                                 \
		.ss_active_high = false,                                                           \
		.orc = DT_PROP(node, overrun_character),                                           \
		.skip_psel_cfg = true,                                                             \
		.skip_gpio_cfg = true,                                                             \
	}

struct temp_monitor_data {
	nrfx_spim_t spim;
#ifdef REMOTE_SUPPORT
	struct ipc_comm_data ipc;
#else
	temp_monitor_event_handler_t handler;
#endif
	struct temp_monitor_start start;
	struct k_timer timer;
	uint32_t cnt;
	int32_t aggr_temp;
	bool above;
	bool below;
	bool sim;
};

struct temp_monitor_config {
	NRF_SPIM_Type *reg;
#ifdef REMOTE_SUPPORT
	struct ipc_comm ipc;
#endif
	const nrfx_spim_config_t spim_config;
	const struct pinctrl_dev_config *pcfg;
	uint8_t *tx_buf;
	uint8_t *rx_buf;
};

struct temp_monitor {
	const struct temp_monitor_config *config;
	struct temp_monitor_data *data;
};

int spi_reg_write(const struct temp_monitor *mon, uint8_t addr, uint8_t val)
{
	nrfx_spim_xfer_desc_t desc = {
		.p_tx_buffer = mon->config->tx_buf,
		.tx_length = 2,
		.rx_length = 0,
	};

	mon->config->tx_buf[0] = addr;
	mon->config->tx_buf[1] = val;

	return nrfx_spim_xfer(&mon->data->spim, &desc, 0);
}

int spi_reg_read8(const struct temp_monitor *mon, uint8_t addr)
{
	nrfx_spim_xfer_desc_t desc = {
		.p_tx_buffer = mon->config->tx_buf,
		.tx_length = 2,
		.p_rx_buffer = mon->config->rx_buf,
		.rx_length = 2,
	};
	int rv;

	mon->config->tx_buf[0] = BIT(7) | addr;
	mon->config->tx_buf[1] = 0;

	rv = nrfx_spim_xfer(&mon->data->spim, &desc, 0);
	if (rv < 0) {
		return rv;
	} else {
		return (int)mon->config->rx_buf[1];
	}
}

int spi_reg_read16(const struct temp_monitor *mon, uint8_t addr)
{
	nrfx_spim_xfer_desc_t desc = {
		.p_tx_buffer = mon->config->tx_buf,
		.tx_length = 3,
		.p_rx_buffer = mon->config->rx_buf,
		.rx_length = 3,
	};
	int rv;

	mon->config->tx_buf[0] = BIT(7) | addr;
	mon->config->tx_buf[1] = 0;
	mon->config->tx_buf[2] = 0;

	rv = nrfx_spim_xfer(&mon->data->spim, &desc, 0);
	if (rv < 0) {
		return rv;
	}

	return (int)(mon->config->rx_buf[1] | (uint32_t)(mon->config->rx_buf[2] << 8));
}

static int send_report(struct temp_monitor *mon, uint8_t type, int16_t value)
{
#ifdef REMOTE_SUPPORT
	struct temp_monitor_msg msg = {.type = type, .report = {.value = value}};

	return ipc_comm_send(&mon->config->ipc, &msg, sizeof(msg));
#else
	if (mon->data->handler) {
		mon->data->handler(type, value);
	}
	return 0;
#endif
}

static void process_temp(struct temp_monitor *mon, int16_t temp)
{
	if (mon->data->sim) {
		uint16_t r = sys_rand16_get();
		int16_t t = 2000 + (uint8_t)r;

		if ((r % mon->data->start.odr) == 0) {
			/* Temperature below the limit.*/
			t -= 1000;
		}
		temp = t;
	}

	mon->data->cnt++;
	mon->data->aggr_temp += temp;
	if (temp > mon->data->start.limith) {
		if (!mon->data->above) {
			mon->data->above = true;
			(void)send_report(mon, TEMP_MONITOR_MSG_ABOVE, temp);
		}
	} else if (temp < mon->data->start.limitl) {
		if (!mon->data->below) {
			mon->data->below = true;
			(void)send_report(mon, TEMP_MONITOR_MSG_BELOW, temp);
		}
	} else {
		mon->data->below = false;
		mon->data->above = false;
	}

	if (mon->data->cnt == mon->data->start.report_avg_period) {
		int16_t avg = (int16_t)(mon->data->aggr_temp / mon->data->cnt);

		/* Report average temperature. */
		mon->data->cnt = 0;
		mon->data->aggr_temp = 0;
		if (send_report(mon, TEMP_MONITOR_MSG_AVG, avg) < 0) {
			__ASSERT_NO_MSG(0);
		}
	}
}

static void temp_mon_timeout_handle(struct k_timer *timer)
{
	struct temp_monitor *mon = k_timer_user_data_get(timer);
	int rv;

	rv = spi_reg_read16(mon, LPS22HH_TEMP_OUT);
	if (rv < 0) {
		return;
	}

	process_temp(mon, (int16_t)rv);
}

int start_handle(const struct temp_monitor *mon, const struct temp_monitor_start *params)
{
	uint32_t us = 1000000 / params->odr;
	uint8_t ctrl_val = LPS22HH_ODR_REG_VAL(params->odr) << 4;
	int rv;

	mon->data->start = *params;
	rv = spi_reg_write(mon, LPS22HH_CTRL_REG1, ctrl_val);
	if (rv < 0) {
		LOG_ERR("ODR setting failed %d", rv);
	}

	k_timer_start(&mon->data->timer, K_USEC(us), K_USEC(us));

	return 0;
}

static int stop_handle(const struct temp_monitor *mon)
{
	int rv;

	rv = spi_reg_write(mon, LPS22HH_CTRL_REG1, 0);
	if (rv < 0) {
		LOG_ERR("ODR setting failed %d", rv);
	}

	k_timer_stop(&mon->data->timer);
	return 0;
}

static int spi_init(const struct temp_monitor *mon)
{
	const nrfx_spim_config_t *spim_cfg = &mon->config->spim_config;
	uint32_t ss_pin = spim_cfg->ss_pin;
	uint32_t ss_dur = spim_cfg->ss_duration;
	nrf_spim_csn_pol_t ss_pol =
		spim_cfg->ss_active_high ? NRF_SPIM_CSN_POL_HIGH : NRF_SPIM_CSN_POL_LOW;
	int rv;

	mon->data->spim.p_reg = mon->config->reg;

	(void)pinctrl_apply_state(mon->config->pcfg, PINCTRL_STATE_DEFAULT);

	/* Manually configure HW slave select. */
	if (spim_cfg->ss_active_high) {
		nrf_gpio_pin_clear(ss_pin);
	} else {
		nrf_gpio_pin_set(ss_pin);
	}
	nrf_gpio_cfg_output(ss_pin);
	nrf_spim_csn_configure(mon->config->reg, ss_pin, ss_pol, ss_dur);

	rv = nrfx_spim_init(&mon->data->spim, spim_cfg, NULL, NULL);
	if (rv < 0) {
		return rv;
	}

	rv = spi_reg_read8(mon, LPS22HH_WHO_AM_I);
	if (rv < 0) {
		return rv;
	}

	return ((uint8_t)rv == LPS22HH_WHO_AM_I_VAL) ? 0 : -ENODEV;
}

#ifdef REMOTE_SUPPORT
static void ipc_comm_cb(void *instance, const void *buf, size_t len)
{
	struct temp_monitor *mon = instance;
	const struct temp_monitor_msg *msg = buf;
	int rv = -EINVAL;

	switch (msg->type) {
	case TEMP_MONITOR_MSG_START:
		rv = start_handle(mon, &msg->start);
		break;
	case TEMP_MONITOR_MSG_STOP:
		rv = stop_handle(mon);
		break;
	default:
		break;
	}

	if (rv < 0) {
		if (send_report(mon, TEMP_MONITOR_MSG_STATUS, (int16_t)rv) < 0) {
			__ASSERT_NO_MSG(0);
		}
	}
}
#endif

PINCTRL_DT_DEFINE(SPI_NODE);
static struct temp_monitor_data data;
static const struct temp_monitor_config config;
static const struct temp_monitor monitor;
static uint8_t tx_buf[3] DMM_MEMORY_SECTION(SPI_NODE);
static uint8_t rx_buf[3] DMM_MEMORY_SECTION(SPI_NODE);
static const struct temp_monitor_config config = {
#ifdef REMOTE_SUPPORT
	.ipc = IPC_COMM_INIT(TEMP_MONITOR_EP_NAME, ipc_comm_cb, &monitor,
			     DEVICE_DT_GET(DT_ALIAS(remote_temp_monitor_ipc)), &data.ipc,
			     &config.ipc),
#endif
	.pcfg = PINCTRL_DT_DEV_CONFIG_GET(SPI_NODE),
	.spim_config = SPIM_CONFIG(SPI_NODE),
	.reg = (NRF_SPIM_Type *)DT_REG_ADDR(SPI_NODE),
	.tx_buf = tx_buf,
	.rx_buf = rx_buf,
};

static const struct temp_monitor monitor = {.data = &data, .config = &config};

#ifndef REMOTE_SUPPORT
int temp_monitor_start(const struct temp_monitor_start *params)
{
	return start_handle(&monitor, params);
}

int temp_monitor_stop(void)
{
	return stop_handle(&monitor);
}

void temp_monitor_set_handler(temp_monitor_event_handler_t handler)
{
	monitor.data->handler = handler;
}
#endif

static int rem_temp_init(void)
{
	const struct temp_monitor *mon = &monitor;
	int rv;

	k_timer_init(&mon->data->timer, temp_mon_timeout_handle, NULL);
	k_timer_user_data_set(&mon->data->timer, (void *)mon);

	rv = spi_init(mon);
	if (rv == -ENODEV) {
		mon->data->sim = true;
	} else if (rv < 0) {
		return rv;
	}

#ifdef REMOTE_SUPPORT
	return ipc_comm_init(&monitor.config->ipc);
#else
	return 0;
#endif
}

SYS_INIT(rem_temp_init, APPLICATION, 0);
