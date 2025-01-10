/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DRIVERS_MSPI_NRFE_MSPI_H
#define DRIVERS_MSPI_NRFE_MSPI_H

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/mspi.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SOC_NRF54L15

#define NRFE_MSPI_CE_PINS_MAX   5
#define NRFE_MSPI_DATA_PINS_MAX 8
#define NRFE_MSPI_VIO_COUNT     11

#define NRFE_MSPI_PORT_NUMBER	 2 /* Physical port number */
#define NRFE_MSPI_SCK_PIN_NUMBER 1 /* Physical pins number on port 2 */
#define NRFE_MSPI_DQ0_PIN_NUMBER 2
#define NRFE_MSPI_DQ1_PIN_NUMBER 4
#define NRFE_MSPI_DQ2_PIN_NUMBER 3
#define NRFE_MSPI_DQ3_PIN_NUMBER 0
#define NRFE_MSPI_CS0_PIN_NUMBER 5
#define NRFE_MSPI_PINS_MAX	 6

#else
#error "Unsupported SoC for SDP MSPI"
#endif

/** @brief eMSPI opcodes. */
typedef enum {
	NRFE_MSPI_EP_BOUNDED = 0,
	NRFE_MSPI_CONFIG_PINS, /*nrfe_mspi_pinctrl_soc_pin_t*/
	NRFE_MSPI_CONFIG_XFER, /*nrfe_mspi_xfer_config_t*/
	NRFE_MSPI_TX, /*nrfe_mspi_xfer_packet_t + data buffer at the end*/
	NRFE_MSPI_RX, 
	NRFE_MSPI_WRONG_OPCODE,
	NRFE_MSPI_ALL_OPCODES = NRFE_MSPI_WRONG_OPCODE,
} nrfe_mspi_opcode_t;

typedef enum {
	NRFE_MSPI_POL_UNDEFINED = 0,
	NRFE_MSPI_POL_ACTIVE_LOW,
	NRFE_MSPI_POL_ACTIVE_HIGH,
} nrfe_mspi_polarity_t;

typedef struct {
	nrfe_mspi_opcode_t opcode; /* nrfe_mspi_opcode */
	pinctrl_soc_pin_t pin[NRFE_MSPI_PINS_MAX];
} nrfe_mspi_pinctrl_soc_pin_t;

typedef struct {
	nrfe_mspi_opcode_t opcode; /* nrfe_mspi_opcode */
        uint8_t command_length;
        uint8_t address_length;
        uint8_t ce_num; //vio number
	enum mspi_duplex duplex;
        enum mspi_io_mode io_mode;
        enum mspi_cpp_mode cpp;
        enum mspi_endian endian;
        bool hold_ce;
	nrfe_mspi_polarity_t ce_polarities[NRFE_MSPI_CE_PINS_MAX];
        uint16_t tx_dummy;
        uint16_t rx_dummy;
        uint32_t freq;
} nrfe_mspi_xfer_config_t;

typedef struct {
	nrfe_mspi_opcode_t opcode; /* nrfe_mspi_opcode */
        uint32_t command;
        uint32_t address;
        uint32_t num_bytes;
} nrfe_mspi_xfer_packet_t;

typedef struct __packed {
	uint8_t opcode; /* nrfe_mspi_opcode */
	uint8_t data;
} nrfe_mspi_flpr_response_t;

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_MSPI_NRFE_MSPI_H */
