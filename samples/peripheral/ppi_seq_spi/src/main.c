/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <dmm.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_grtc.h>
#if DT_NODE_EXISTS(DT_NODELABEL(sample_rtc))
#include <hal/nrf_rtc.h>
#endif
#include <ppi_seq/ppi_seq_i2c_spi.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/cpu_load.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

#ifdef CONFIG_SOC_NRF54H20_CPUPPR
#include <hal/nrf_vpr_csr.h>
#endif

#define SPIM_NODE DT_NODELABEL(sample_spi)
#define RTC_NODE DT_NODELABEL(sample_rtc)
#define TRANSFER_LENGTH 8
#define BATCH_LENGTH	16

static uint8_t tx_dma_buffer[TRANSFER_LENGTH] DMM_MEMORY_SECTION(SPIM_NODE);
static uint8_t rx_dma_buffer[2][BATCH_LENGTH * TRANSFER_LENGTH] DMM_MEMORY_SECTION(SPIM_NODE);
#ifdef CONFIG_HAS_NORDIC_DMM
static uint8_t rx_buffer[BATCH_LENGTH * TRANSFER_LENGTH];
#endif

struct sample_data {
	struct k_sem sem;
	uint32_t cb_cnt;
	uint32_t data_cnt;
};

static void ppi_seq_cb(const struct device *dev, struct ppi_seq_i2c_spi_batch *batch, bool last,
		       void *user_data)
{
	struct sample_data *data = user_data;

#ifdef CONFIG_HAS_NORDIC_DMM
	uint32_t *src = (uint32_t *)batch->desc.spim.p_rx_buffer;
	uint32_t *dst = (uint32_t *)rx_buffer;

	/* Perform copying from DMA memory to the user buffer. Data is not used, it's done only
	 * to estimate CPU load and power consumption.
	 */
	for (int i = 0; i < (TRANSFER_LENGTH * batch->batch_cnt) / sizeof(uint32_t); i++) {
		dst[i] = src[i];
	}
#endif

	data->data_cnt += (batch->desc.spim.rx_length * batch->batch_cnt);
	data->cb_cnt++;

	if (last) {
		k_sem_give(&data->sem);
	}
}

static int seq_run(const struct device *dev, uint32_t timeout_ms, uint32_t period, bool log_load)
{
	struct sample_data data;
	uint32_t repeat = (timeout_ms * USEC_PER_MSEC) / (BATCH_LENGTH * period);
	struct ppi_seq_i2c_spi_job job = {
		.desc = {
			.spim = {
				.p_tx_buffer = tx_dma_buffer,
				.tx_length = TRANSFER_LENGTH,
				.p_rx_buffer = rx_dma_buffer[0],
				.rx_length = TRANSFER_LENGTH,
			}
		},
		.rx_second_buf = rx_dma_buffer[1],
		.repeat = repeat,
		.batch_cnt = BATCH_LENGTH,
		.tx_postinc = false,
		.cb = ppi_seq_cb,
		.user_data = &data
	};
	int rv;
	int load = 0;

	memset(&data, 0, sizeof(data));
	k_sem_init(&data.sem, 0, 1);

	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		load = cpu_load_get(true);
	}

	rv = ppi_seq_i2c_spi_start(dev, period, &job);
	if (rv < 0) {
		LOG_ERR("PPI sequencer start failed %d", rv);
		return rv;
	}

	rv = k_sem_take(&data.sem, K_MSEC(timeout_ms + 10));
	if (rv < 0) {
		LOG_ERR("PPI sequencer timeout: %d", rv);
		return rv;
	}

	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		load = cpu_load_get(true);
	}

	printk("PPI sequence completed. Period: %d, data received:%d, callback:%d, load:%d.%d%%\n",
	       period, data.data_cnt, data.cb_cnt, load / 10, load % 10);

	return 0;
}

static int manual_xfer(const struct device *dev)
{
	union ppi_seq_i2c_spi_xfer_desc desc;

	desc.spim.p_tx_buffer = tx_dma_buffer;
	desc.spim.tx_length = 2;
	desc.spim.p_rx_buffer = rx_dma_buffer[0];
	desc.spim.rx_length = 2;

	tx_dma_buffer[0] = 0xA4;
	tx_dma_buffer[1] = 0xB5;
	tx_dma_buffer[1] = 0xC6;
	return ppi_seq_i2c_spi_xfer(dev, &desc);
}

/* Function sets up a PPI connection that is used by the sequencer running on cpuppr. It is
 * done to reduce code size on cpuppr by not requiring GPPI driver to be used there.
 */
static int setup_ppr_ppi_seq(void)
{
#if DT_NODE_EXISTS(DT_NODELABEL(sample_rtc))
	NRF_RTC_Type *rtc_reg = (NRF_RTC_Type *)DT_REG_ADDR(RTC_NODE);
	uint32_t tep = nrf_spim_task_address_get((NRF_SPIM_Type *)DT_REG_ADDR(SPIM_NODE),
				NRF_SPIM_TASK_START);
	uint32_t eep = nrf_rtc_event_address_get(rtc_reg, nrf_rtc_compare_event_get(1));
	nrfx_gppi_handle_t handle;
	int rv;

	rv = nrfx_gppi_conn_alloc(eep, tep, &handle);
	if (rv < 0) {
		return rv;
	}

	nrfx_gppi_conn_enable(handle);
#endif

	return 0;
}

int main(void)
{
	static const struct device *ppi_seq_spi_dev =
		COND_CODE_1(CONFIG_NORDIC_VPR_LAUNCHER, (NULL), (DEVICE_DT_GET(SPIM_NODE)));
	int rv;
	bool log_cpu_load = true;
	uint32_t periods[] = {
			      1000,
			      2500,
			      5000,
			      10000,
			      25000,
			      50000
#ifdef USE_RTC
			      ,
			      100000
#endif
	};
	uint32_t idx = 0;
	uint32_t timeout;

	if (IS_ENABLED(CONFIG_NORDIC_VPR_LAUNCHER)) {
		/* Setup DPPI connections for the sequencer on cpuapp if sequencer is running
		 * on cpuppr core. With this approach PPR does not require to have GPPI driver.
		 */
		printk("PPI Sequencer for SPI transfers on PPR %s\n", CONFIG_BOARD_TARGET);
		return setup_ppr_ppi_seq();
	}

	if (!device_is_ready(ppi_seq_spi_dev)) {
		printk("PPI Sequencer with SPI is not ready\n");
		return -EINVAL;
	}

	printk("PPI Sequencer for SPI transfers %s\n", CONFIG_BOARD_TARGET);


	while (1) {
		/* Execute standard blocking SPI transfer using the SPIM instance that is used
		 * by the sequencer.
		 */
		rv = manual_xfer(ppi_seq_spi_dev);
		if (rv < 0) {
			printk("Manual transfer failed:%d\n", rv);
			return rv;
		}

		/* Pick a timeout and execute batch operations. */
		timeout = periods[idx] <= 25000 ? 1000 : 3000;
		rv = seq_run(ppi_seq_spi_dev, timeout, periods[idx], log_cpu_load);
		if (rv < 0) {
			printk("Sequence failed:%d\n", rv);
			return rv;
		}

		idx++;
		if (idx == ARRAY_SIZE(periods)) {
			idx = 0;
		}
	}

	return 0;
}

#ifdef CONFIG_SOC_NRF54H20_CPUPPR
static int vpr_init(void)
{
	csr_write(VPRCSR_NORDIC_VPRNORDICSLEEPCTRL,
		  VPRCSR_NORDIC_VPRNORDICSLEEPCTRL_SLEEPSTATE_DEEPSLEEP);
	return 0;
}
SYS_INIT(vpr_init, PRE_KERNEL_1, 0);
#endif
