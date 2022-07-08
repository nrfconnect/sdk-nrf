/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing SPI device interface specific declarations for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

/* SPIM driver config */

int spim_init(struct qspi_config *config);

int spim_write(unsigned int addr, const void *data, int len);

int spim_read(unsigned int addr, void *data, int len);

int spim_hl_read(unsigned int addr, void *data, int len);

int spim_cmd_rpu_wakeup_fn(uint32_t data);

int spim_wait_while_rpu_awake_fn(void);

int spim_validate_rpu_awake_fn(void);

int spim_cmd_sleep_rpu_fn(void);

int spim_RDSR1(void);
