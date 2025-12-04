/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DRIVERS_MSPI_HPF_MSPI_H
#define DRIVERS_MSPI_HPF_MSPI_H

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/mspi.h>
#include <hal/nrf_timer.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_SOC_NRF54L15) || defined(CONFIG_SOC_NRF54LM20A)
#define HPF_MSPI_PINS_MAX	 11
#else
#error "Unsupported SoC for HPF MSPI"
#endif

/** @brief eMSPI opcodes. */
typedef enum {
	HPF_MSPI_EP_BOUNDED = 0,
	HPF_MSPI_CONFIG_TIMER_PTR, /* hpf_mspi_flpr_timer_msg_t */
	HPF_MSPI_CONFIG_PINS,      /* hpf_mspi_pinctrl_soc_pin_msg_t */
	HPF_MSPI_CONFIG_DEV,       /* hpf_mspi_dev_config_msg_t */
	HPF_MSPI_CONFIG_XFER,      /* hpf_mspi_xfer_config_msg_t */
	HPF_MSPI_TX,	            /* hpf_mspi_xfer_packet_msg_t + data buffer at the end */
	HPF_MSPI_TXRX,
	HPF_MSPI_HPF_APP_HARD_FAULT,
	HPF_MSPI_WRONG_OPCODE,
	HPF_MSPI_OPCODES_COUNT = HPF_MSPI_WRONG_OPCODE,
	/* This is to make sizeof(hpf_mspi_opcode_t)==32bit, for alignment purpose. */
	HPF_MSPI_OPCODES_MAX = 0xFFFFFFFFU,
} hpf_mspi_opcode_t;

typedef struct {
	enum mspi_io_mode io_mode;
	enum mspi_cpp_mode cpp;
	uint8_t ce_index;
	enum mspi_ce_polarity ce_polarity;
	uint16_t cnt0_value;
} hpf_mspi_dev_config_t;

typedef struct {
	uint8_t device_index;
	uint8_t command_length;
	uint8_t address_length;
	bool hold_ce;
	uint16_t tx_dummy;
	uint16_t rx_dummy;
} hpf_mspi_xfer_config_t;

typedef struct {
	hpf_mspi_opcode_t opcode; /* HPF_MSPI_CONFIG_TIMER_PTR */
	NRF_TIMER_Type *timer_ptr;
} hpf_mspi_flpr_timer_msg_t;

typedef struct {
	hpf_mspi_opcode_t opcode; /* HPF_MSPI_CONFIG_PINS */
	uint8_t pins_count;
	pinctrl_soc_pin_t pin[HPF_MSPI_PINS_MAX];
} hpf_mspi_pinctrl_soc_pin_msg_t;

typedef struct {
	hpf_mspi_opcode_t opcode; /* HPF_MSPI_CONFIG_DEV */
	uint8_t device_index;
	hpf_mspi_dev_config_t dev_config;
} hpf_mspi_dev_config_msg_t;

typedef struct {
	hpf_mspi_opcode_t opcode; /* HPF_MSPI_CONFIG_XFER */
	hpf_mspi_xfer_config_t xfer_config;
} hpf_mspi_xfer_config_msg_t;

typedef struct {
	hpf_mspi_opcode_t opcode; /* HPF_MSPI_TX or HPF_MSPI_TXRX */
	uint32_t command;
	uint32_t address;
	uint32_t num_bytes; /* Size of data */
#if (defined(CONFIG_MSPI_HPF_IPC_NO_COPY) || defined(CONFIG_HPF_MSPI_IPC_NO_COPY))
	uint8_t *data;
#else
	uint8_t data[]; /* Variable length data field at the end of packet. */
#endif
} hpf_mspi_xfer_packet_msg_t;

typedef struct {
	hpf_mspi_opcode_t opcode;
	uint8_t data;
} hpf_mspi_flpr_response_msg_t;

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_MSPI_HPF_MSPI_H */
