/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <drivers/uart.h>
#include "spm_internal.h"

struct spm_uart_config {
	const struct device *uart;
	NRF_UARTE_Type *regs;
};

/** @brief Change state of the peripheral.
 *
 * @param id Peripheral ID.
 * @param secure If true peripheral will be set as secure. Otherwise it is
 * reconfigured to non-secure.
 *
 * @retval true if peripheral was previously configured as secure.
 * @retval false if peripheral was previously configured as non-secure.
 */
static bool spm_periph_reconf(uint8_t id, bool secure)
{
	bool prev_sec_state = (NRF_SPU->PERIPHID[id].PERM & PERIPH_SEC);

	NRF_SPU->PERIPHID[id].PERM = secure ?
			(PERIPH_DMASEC | PERIPH_SEC) : PERIPH_NONSEC;

	return prev_sec_state;
}

/** @brief Poll out with switching to secure state and restoring previous state.
 *
 * This approach can be used to share single UARTE instance with secure and
 * non-secure. In order to allow that LOCK bit must not be set in UARTE PERM
 * configuration.
 *
 * Note! Only for debugging purposes.
 */
static void poll_out(const struct device *dev, unsigned char c)
{
	const struct spm_uart_config *cfg = (const struct spm_uart_config *)dev->config;
	uint8_t id = NRFX_PERIPHERAL_ID_GET(cfg->regs);
	bool sec;

	sec  = spm_periph_reconf(id, true);
	uart_poll_out(cfg->uart, c);
	spm_periph_reconf(id, sec);
}

static const struct uart_driver_api spm_uart_api = {
	.poll_out = poll_out,
};

#define DT_DRV_COMPAT nordic_nrf_spm_uart

static const struct spm_uart_config spm_uart_config = {
	.uart = DEVICE_DT_GET(DT_BUS(DT_NODELABEL(spm_uart))),
	.regs = (NRF_UARTE_Type *)DT_REG_ADDR(DT_BUS(DT_NODELABEL(spm_uart)))
};

static int spm_uart_init(const struct device *dev)
{
	return 0;
}

DEVICE_DT_DEFINE(DT_NODELABEL(spm_uart), spm_uart_init,
		 NULL, NULL, &spm_uart_config,
		 PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		 &spm_uart_api);
