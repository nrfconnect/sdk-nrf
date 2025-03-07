/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DRIVERS_MSPI_NRFE_MSPI_H
#define DRIVERS_MSPI_NRFE_MSPI_H

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/mspi.h>
#include <hal/nrf_timer.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SOC_NRF54L15
#define NRFE_MSPI_PINS_MAX	 11
#else
#error "Unsupported SoC for SDP MSPI"
#endif

/** @brief eMSPI opcodes. */
typedef enum {
	NRFE_MSPI_EP_BOUNDED = 0,
	NRFE_MSPI_CONFIG_TIMER_PTR, /* nrfe_mspi_flpr_timer_msg_t */
	NRFE_MSPI_CONFIG_PINS,      /* nrfe_mspi_pinctrl_soc_pin_msg_t */
	NRFE_MSPI_CONFIG_DEV,       /* nrfe_mspi_dev_config_msg_t */
	NRFE_MSPI_CONFIG_XFER,      /* nrfe_mspi_xfer_config_msg_t */
	NRFE_MSPI_TX,	            /* nrfe_mspi_xfer_packet_msg_t + data buffer at the end */
	NRFE_MSPI_TXRX,
	NRFE_MSPI_SDP_APP_HARD_FAULT,
	NRFE_MSPI_WRONG_OPCODE,
	NRFE_MSPI_OPCODES_COUNT = NRFE_MSPI_WRONG_OPCODE,
	/* This is to make sizeof(nrfe_mspi_opcode_t)==32bit, for alignment purpouse. */
	NREE_MSPI_OPCODES_MAX = 0xFFFFFFFFU,
} nrfe_mspi_opcode_t;

typedef struct {
	enum mspi_io_mode io_mode;
	enum mspi_cpp_mode cpp;
	uint8_t ce_index;
	enum mspi_ce_polarity ce_polarity;
	uint16_t cnt0_value;
} nrfe_mspi_dev_config_t;

typedef struct {
	uint8_t device_index;
	uint8_t command_length;
	uint8_t address_length;
	bool hold_ce;
	uint16_t tx_dummy;
	uint16_t rx_dummy;
} nrfe_mspi_xfer_config_t;

typedef struct {
	nrfe_mspi_opcode_t opcode; /* NRFE_MSPI_CONFIG_TIMER_PTR */
	NRF_TIMER_Type *timer_ptr;
} nrfe_mspi_flpr_timer_msg_t;

typedef struct {
	nrfe_mspi_opcode_t opcode; /* NRFE_MSPI_CONFIG_PINS */
	uint8_t pins_count;
	pinctrl_soc_pin_t pin[NRFE_MSPI_PINS_MAX];
} nrfe_mspi_pinctrl_soc_pin_msg_t;

typedef struct {
	nrfe_mspi_opcode_t opcode; /* NRFE_MSPI_CONFIG_DEV */
	uint8_t device_index;
	nrfe_mspi_dev_config_t dev_config;
} nrfe_mspi_dev_config_msg_t;

typedef struct {
	nrfe_mspi_opcode_t opcode; /* NRFE_MSPI_CONFIG_XFER */
	nrfe_mspi_xfer_config_t xfer_config;
} nrfe_mspi_xfer_config_msg_t;

typedef struct {
	nrfe_mspi_opcode_t opcode; /* NRFE_MSPI_TX or NRFE_MSPI_TXRX */
	uint32_t command;
	uint32_t address;
	uint32_t num_bytes; /* Size of data */
#if (defined(CONFIG_MSPI_NRFE_IPC_NO_COPY) || defined(CONFIG_SDP_MSPI_IPC_NO_COPY))
	uint8_t *data;
#else
	uint8_t data[]; /* Variable length data field at the end of packet. */
#endif
} nrfe_mspi_xfer_packet_msg_t;

typedef struct {
	nrfe_mspi_opcode_t opcode;
	uint8_t data;
} nrfe_mspi_flpr_response_msg_t;

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_MSPI_NRFE_MSPI_H */
