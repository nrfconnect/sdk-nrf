/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing QSPI device interface specific declarations for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#ifndef __QSPI_IF_H__
#define __QSPI_IF_H__

#include <zephyr/drivers/gpio.h>
#ifdef CONFIG_NRFX_QSPI
#include <nrfx_qspi.h>
#endif

#define RPU_WAKEUP_NOW BIT(0) /* WAKEUP RPU - RW */
#define RPU_AWAKE_BIT BIT(1) /* RPU AWAKE FROM SLEEP - RO */
#define RPU_READY_BIT BIT(2) /* RPU IS READY - RO*/

struct qspi_config {
#ifdef CONFIG_NRFX_QSPI
	nrf_qspi_addrmode_t addrmode;
	nrf_qspi_readoc_t readoc;
	nrf_qspi_writeoc_t writeoc;
	nrf_qspi_frequency_t sckfreq;
#endif
	unsigned int freq;
	unsigned int spimfreq;
	unsigned char RDC4IO;
	bool easydma;
	bool single_op;
	bool quad_spi;
	bool encryption;
	bool CMD_CNONCE;
	bool enc_enabled;
	struct k_sem lock;
	unsigned int addrmask;
	unsigned char qspi_slave_latency;
#ifdef CONFIG_NRFX_QSPI
	nrf_qspi_encryption_t p_cfg;
#endif /*CONFIG_NRFX_QSPI*/
	int test_hlread;
	char *test_name;
	int test_start;
	int test_end;
	int test_iterations;
	int test_timediff_read;
	int test_timediff_write;
	int test_status;
	int test_iteration;
};
struct qspi_dev {
	int (*deinit)(void);
	void *config;
	int (*init)(struct qspi_config *config);
	int (*write)(unsigned int addr, const void *data, int len);
	int (*read)(unsigned int addr, void *data, int len);
	int (*hl_read)(unsigned int addr, void *data, int len);
	void (*hard_reset)(void);
};

int qspi_cmd_wakeup_rpu(const struct device *dev, uint8_t data);

int qspi_init(struct qspi_config *config);

int qspi_write(unsigned int addr, const void *data, int len);

int qspi_read(unsigned int addr, void *data, int len);

int qspi_hl_read(unsigned int addr, void *data, int len);

int qspi_deinit(void);

void gpio_free_irq(int pin, struct gpio_callback *button_cb_data);

int gpio_request_irq(int pin, struct gpio_callback *button_cb_data, void (*irq_handler)());

struct qspi_config *qspi_defconfig(void);

struct qspi_dev *qspi_dev(void);

int qspi_cmd_sleep_rpu(const struct device *dev);

void hard_reset(void);
void get_sleep_stats(uint32_t addr, uint32_t *buff, uint32_t wrd_len);

extern struct device qspi_perip;

int qspi_validate_rpu_wake_writecmd(const struct device *dev);
int qspi_cmd_wakeup_rpu(const struct device *dev, uint8_t data);
int qspi_wait_while_firmware_awake(const struct device *dev);
int qspi_wait_while_rpu_awake(const struct device *dev);

int qspi_RDSR1(const struct device *dev, uint8_t *rdsr1);
int qspi_RDSR2(const struct device *dev, uint8_t *rdsr2);
int qspi_WRSR2(const struct device *dev, const uint8_t wrsr2);

#ifdef RPU_SLEEP_SUPPORT
int func_rpu_sleep(void);
int func_rpu_wake(void);
int func_rpu_sleep_status(void);
#endif /* RPU_SLEEP_SUPPORT */
#endif /* __QSPI_IF_H__ */
