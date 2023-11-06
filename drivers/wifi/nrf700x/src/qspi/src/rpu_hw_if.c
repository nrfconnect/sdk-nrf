/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing common functions for RPU hardware interaction
 * using QSPI and SPI that can be invoked by shell or the driver.
 */

#include <string.h>
#include <sys/time.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/gpio/nordic-nrf-gpio.h>
#include <zephyr/logging/log.h>

#include "rpu_hw_if.h"
#include "qspi_if.h"
#include "spi_if.h"

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF700X_BUS_LOG_LEVEL);

#define NRF7002_NODE DT_NODELABEL(nrf700x)

static const struct gpio_dt_spec host_irq_spec =
GPIO_DT_SPEC_GET(NRF7002_NODE, host_irq_gpios);

static const struct gpio_dt_spec iovdd_ctrl_spec =
GPIO_DT_SPEC_GET(NRF7002_NODE, iovdd_ctrl_gpios);

static const struct gpio_dt_spec bucken_spec =
GPIO_DT_SPEC_GET(NRF7002_NODE, bucken_gpios);

#if defined(CONFIG_BOARD_NRF7002DK_NRF7001_NRF5340_CPUAPP) || \
	defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP)
#define NRF_RADIO_COEX_NODE DT_NODELABEL(nrf_radio_coex)
static const struct gpio_dt_spec btrf_switch_spec =
GPIO_DT_SPEC_GET(NRF_RADIO_COEX_NODE, btrf_switch_gpios);
#endif /* CONFIG_BOARD_NRF700XDK_NRF5340_CPUAPP */

char blk_name[][15] = { "SysBus",   "ExtSysBus",	   "PBus",	   "PKTRAM",
			       "GRAM",	   "LMAC_ROM",	   "LMAC_RET_RAM", "LMAC_SRC_RAM",
			       "UMAC_ROM", "UMAC_RET_RAM", "UMAC_SRC_RAM" };

uint32_t rpu_7002_memmap[][3] = {
	{ 0x000000, 0x008FFF, 1 },
	{ 0x009000, 0x03FFFF, 2 },
	{ 0x040000, 0x07FFFF, 1 },
	{ 0x0C0000, 0x0F0FFF, 0 },
	{ 0x080000, 0x092000, 1 },
	{ 0x100000, 0x134000, 1 },
	{ 0x140000, 0x14C000, 1 },
	{ 0x180000, 0x190000, 1 },
	{ 0x200000, 0x261800, 1 },
	{ 0x280000, 0x2A4000, 1 },
	{ 0x300000, 0x338000, 1 }
};

static const struct qspi_dev *qdev;
static struct qspi_config *cfg;

static int validate_addr_blk(uint32_t start_addr,
							 uint32_t end_addr,
							 uint32_t block_no,
							 bool *hl_flag,
							 int *selected_blk)
{
	uint32_t *block_map = rpu_7002_memmap[block_no];

	if (((start_addr >= block_map[0]) && (start_addr <= block_map[1])) &&
	    ((end_addr >= block_map[0]) && (end_addr <= block_map[1]))) {
		if (block_no == PKTRAM) {
			*hl_flag = 0;
		}
		*selected_blk = block_no;
		return 0;
	}

	return -1;
}

static int rpu_validate_addr(uint32_t start_addr, uint32_t len, bool *hl_flag)
{
	int ret = 0, i;
	uint32_t end_addr;
	int selected_blk;

	end_addr = start_addr + len - 1;

	*hl_flag = 1;

	for (i = 0; i < NUM_MEM_BLOCKS; i++) {
		ret = validate_addr_blk(start_addr, end_addr, i, hl_flag, &selected_blk);
		if (!ret) {
			break;
		}
	}

	if (ret) {
		LOG_ERR("Address validation failed - pls check memmory map and re-try");
		return -1;
	}

	if ((selected_blk == LMAC_ROM) || (selected_blk == UMAC_ROM)) {
		LOG_ERR("Error: Cannot write to ROM blocks");
		return -1;
	}

	cfg->qspi_slave_latency = (*hl_flag) ? rpu_7002_memmap[selected_blk][2] : 0;

	return 0;
}

int rpu_irq_config(struct gpio_callback *irq_callback_data, void (*irq_handler)())
{
	int ret;

	if (!device_is_ready(host_irq_spec.port)) {
		LOG_ERR("Host IRQ GPIO %s is not ready", host_irq_spec.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&host_irq_spec, GPIO_INPUT);
	if (ret) {
		LOG_ERR("Failed to configure host_irq pin %d", host_irq_spec.pin);
		goto out;
	}

	ret = gpio_pin_interrupt_configure_dt(&host_irq_spec,
			GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		LOG_ERR("Failed to configure interrupt on host_irq pin %d",
				host_irq_spec.pin);
		goto out;
	}

	gpio_init_callback(irq_callback_data,
			irq_handler,
			BIT(host_irq_spec.pin));

	ret = gpio_add_callback(host_irq_spec.port, irq_callback_data);
	if (ret) {
		LOG_ERR("Failed to add callback on host_irq pin %d",
				host_irq_spec.pin);
		goto out;
	}

	LOG_DBG("Finished Interrupt config\n");

out:
	return ret;
}

int rpu_irq_remove(struct gpio_callback *irq_callback_data)
{
	int ret;

	ret = gpio_pin_configure_dt(&host_irq_spec, GPIO_DISCONNECTED);
	if (ret) {
		LOG_ERR("Failed to remove host_irq pin %d", host_irq_spec.pin);
		goto out;
	}

	ret = gpio_remove_callback(host_irq_spec.port, irq_callback_data);
	if (ret) {
		LOG_ERR("Failed to remove callback on host_irq pin %d",
				host_irq_spec.pin);
		goto out;
	}

out:
	return ret;
}


static int ble_gpio_config(void)
{
#if defined(CONFIG_BOARD_NRF7002DK_NRF7001_NRF5340_CPUAPP) || \
	defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP)
	int ret;

	if (!device_is_ready(btrf_switch_spec.port)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&btrf_switch_spec, GPIO_OUTPUT);
	if (ret) {
		LOG_ERR("BLE GPIO configuration failed %d", ret);
		return ret;
	}

	return ret;
#else
	return 0;
#endif /* CONFIG_BOARD_NRF700XDK_NRF5340 */
}

static int ble_gpio_remove(void)
{
#if defined(CONFIG_BOARD_NRF7002DK_NRF7001_NRF5340_CPUAPP) || \
	defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP)
	int ret;

	ret = gpio_pin_configure_dt(&btrf_switch_spec, GPIO_DISCONNECTED);
	if (ret) {
		LOG_ERR("Bluetooth LE GPIO remove failed %d", ret);
		return ret;
	}

	return ret;
#else
	return 0;
#endif
}

static int rpu_gpio_config(void)
{
	int ret;

	if (!device_is_ready(iovdd_ctrl_spec.port)) {
		LOG_ERR("IOVDD GPIO %s is not ready", iovdd_ctrl_spec.port->name);
		return -ENODEV;
	}

	if (!device_is_ready(bucken_spec.port)) {
		LOG_ERR("BUCKEN GPIO %s is not ready", bucken_spec.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&bucken_spec, (GPIO_OUTPUT | NRF_GPIO_DRIVE_H0H1));
	if (ret) {
		LOG_ERR("BUCKEN GPIO configuration failed...");
		return ret;
	}

	ret = gpio_pin_configure_dt(&iovdd_ctrl_spec, GPIO_OUTPUT);
	if (ret) {
		LOG_ERR("IOVDD GPIO configuration failed...");
		gpio_pin_configure_dt(&bucken_spec, GPIO_DISCONNECTED);
		return ret;
	}

	LOG_DBG("GPIO configuration done...\n");

	return 0;
}

static int rpu_gpio_remove(void)
{
	int ret;

	ret = gpio_pin_configure_dt(&bucken_spec, GPIO_DISCONNECTED);
	if (ret) {
		LOG_ERR("BUCKEN GPIO remove failed...");
		return ret;
	}

	ret = gpio_pin_configure_dt(&iovdd_ctrl_spec, GPIO_DISCONNECTED);
	if (ret) {
		LOG_ERR("IOVDD GPIO remove failed...");
		return ret;
	}

	LOG_DBG("GPIO remove done...\n");
	return ret;
}

static int rpu_pwron(void)
{
	int ret;

	ret = gpio_pin_set_dt(&bucken_spec, 1);
	if (ret) {
		LOG_ERR("BUCKEN GPIO set failed...");
		return ret;
	}
	/* Settling time is 50us (H0) or 100us (L0) */
	k_msleep(1);

	ret = gpio_pin_set_dt(&iovdd_ctrl_spec, 1);
	if (ret) {
		LOG_ERR("IOVDD GPIO set failed...");
		gpio_pin_set_dt(&bucken_spec, 0);
		return ret;
	}
	/* Settling time for iovdd nRF7002 DK/EK - switch (TCK106AG): ~600us */
	k_msleep(1);
#ifdef CONFIG_SHIELD_NRF7002EB
	/* For nRF7002 Expansion board, we need a total wait time after bucken assertion
	 * to be 6ms (1ms + 1ms + 4ms).
	 */
	k_msleep(4);
#endif /* SHIELD_NRF7002EB */

	LOG_DBG("Bucken = %d, IOVDD = %d", gpio_pin_get_dt(&bucken_spec),
			gpio_pin_get_dt(&iovdd_ctrl_spec));

	return ret;
}

static int rpu_pwroff(void)
{
	int ret;

	ret = gpio_pin_set_dt(&bucken_spec, 0); /* BUCKEN = 0 */
	if (ret) {
		LOG_ERR("BUCKEN GPIO set failed...");
		return ret;
	}

	ret = gpio_pin_set_dt(&iovdd_ctrl_spec, 0); /* IOVDD CNTRL = 0 */
	if (ret) {
		LOG_ERR("IOVDD GPIO set failed...");
		return ret;
	}

	return ret;
}

#if defined(CONFIG_BOARD_NRF7002DK_NRF7001_NRF5340_CPUAPP) || \
	defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP)
int ble_ant_switch(unsigned int ant_switch)
{
	int ret;

	ret = gpio_pin_set_dt(&btrf_switch_spec, ant_switch & 0x1);
	if (ret) {
		LOG_ERR("BLE GPIO set failed %d", ret);
		return ret;
	}

	return ret;
}
#endif /* CONFIG_BOARD_NRF7002DK_NRF5340 */

int rpu_read(unsigned int addr, void *data, int len)
{
	bool hl_flag;

	if (rpu_validate_addr(addr, len, &hl_flag)) {
		return -1;
	}

	if (hl_flag)
		return qdev->hl_read(addr, data, len);
	else
		return qdev->read(addr, data, len);
}

int rpu_write(unsigned int addr, const void *data, int len)
{
	bool hl_flag;

	if (rpu_validate_addr(addr, len, &hl_flag)) {
		return -1;
	}

	return qdev->write(addr, data, len);
}

int rpu_sleep(void)
{
#if CONFIG_NRF700X_ON_QSPI
	return qspi_cmd_sleep_rpu(&qspi_perip);
#else
	return spim_cmd_sleep_rpu_fn();
#endif
}

int rpu_wakeup(void)
{
	int ret;

	ret = rpu_wrsr2(1);
	if (ret) {
		LOG_ERR("Error: WRSR2 failed");
		return ret;
	}

	/* These return actual values not return values */
	(void)rpu_rdsr2();

	(void)rpu_rdsr1();

	return 0;
}

int rpu_sleep_status(void)
{
	return rpu_rdsr1();
}

void rpu_get_sleep_stats(uint32_t addr, uint32_t *buff, uint32_t wrd_len)
{
	int ret;

	ret = rpu_wakeup();
	if (ret) {
		LOG_ERR("Error: RPU wakeup failed");
		return;
	}

	ret = rpu_read(addr, buff, wrd_len * 4);
	if (ret) {
		LOG_ERR("Error: RPU read failed");
		return;
	}

	ret = rpu_sleep();
	if (ret) {
		LOG_ERR("Error: RPU sleep failed");
		return;
	}
}

int rpu_wrsr2(uint8_t data)
{
	int ret;

#if CONFIG_NRF700X_ON_QSPI
	ret = qspi_cmd_wakeup_rpu(&qspi_perip, data);
#else
	ret = spim_cmd_rpu_wakeup_fn(data);
#endif

	LOG_DBG("Written 0x%x to WRSR2", data);
	return ret;
}

int rpu_rdsr2(void)
{
#if CONFIG_NRF700X_ON_QSPI
	return qspi_validate_rpu_wake_writecmd(&qspi_perip);
#else
	return spi_validate_rpu_wake_writecmd();
#endif
}

int rpu_rdsr1(void)
{
#if CONFIG_NRF700X_ON_QSPI
	return qspi_wait_while_rpu_awake(&qspi_perip);
#else
	return spim_wait_while_rpu_awake();
#endif
}


int rpu_clks_on(void)
{
	uint32_t rpu_clks = 0x100;
	/* Enable RPU Clocks */
	qdev->write(0x048C20, &rpu_clks, 4);
	LOG_DBG("RPU Clocks ON...");
	return 0;
}

#define CALL_RPU_FUNC(func, ...) \
	do { \
		ret = func(__VA_ARGS__); \
		if (ret) { \
			LOG_DBG("Error: %s failed with %d", #func, ret); \
			goto out; \
		} \
	} while (0)

int rpu_enable(void)
{
	int ret;

	qdev = qspi_dev();
	cfg = qspi_get_config();

	CALL_RPU_FUNC(rpu_gpio_config);

	ret = ble_gpio_config();
	if (ret) {
		goto rpu_gpio_remove;
	}

	ret = rpu_pwron();
	if (ret) {
		goto ble_gpio_remove;
	}

	ret = rpu_wakeup();
	if (ret) {
		goto rpu_pwroff;
	}

	ret = rpu_clks_on();
	if (ret) {
		goto rpu_pwroff;
	}

	return 0;

rpu_pwroff:
	rpu_pwroff();
ble_gpio_remove:
	ble_gpio_remove();
rpu_gpio_remove:
	rpu_gpio_remove();
out:
	return ret;
}

int rpu_disable(void)
{
	int ret;

	CALL_RPU_FUNC(rpu_pwroff);
	CALL_RPU_FUNC(rpu_gpio_remove);
	CALL_RPU_FUNC(ble_gpio_remove);

	qdev = NULL;
	cfg = NULL;

out:
	return ret;
}
