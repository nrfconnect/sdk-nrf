/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DRIVERS_MSPI_NRFE_MSPI_H
#define DRIVERS_MSPI_NRFE_MSPI_H

#include <zephyr/drivers/pinctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SOC_NRF54L15
#define NRFE_MSPI_PORT_NUMBER    2 /* Physical port number */
#define NRFE_MSPI_SCK_PIN_NUMBER 1 /* Physical pins number on port 2 */
#define NRFE_MSPI_DQ0_PIN_NUMBER 2
#define NRFE_MSPI_DQ1_PIN_NUMBER 4
#define NRFE_MSPI_DQ2_PIN_NUMBER 3
#define NRFE_MSPI_DQ3_PIN_NUMBER 0
#define NRFE_MSPI_CS0_PIN_NUMBER 5
#define NRFE_MSPI_PINS_MAX       6

#define NRFE_MSPI_SCK_PIN_NUMBER_VIO 0 /* FLPR VIO SCLK pin number */
#define NRFE_MSPI_DQ0_PIN_NUMBER_VIO 1
#define NRFE_MSPI_DQ1_PIN_NUMBER_VIO 2
#define NRFE_MSPI_DQ2_PIN_NUMBER_VIO 3
#define NRFE_MSPI_DQ3_PIN_NUMBER_VIO 4
#define NRFE_MSPI_CS0_PIN_NUMBER_VIO 5

#define VIO(_pin_) _pin_##_VIO
#else
#error "Unsupported SoC for SDP MSPI"
#endif

#define NRFE_MSPI_MAX_CE_PINS_COUNT 5 /* Ex. CE0 CE1 CE2 CE3 CE4 */

/** @brief eMSPI opcodes. */
enum nrfe_mspi_opcode {
	NRFE_MSPI_EP_BOUNDED = 0,
	NRFE_MSPI_CONFIG_PINS,
	NRFE_MSPI_CONFIG_CTRL,		/* struct mspi_cfg */
	NRFE_MSPI_CONFIG_DEV,		/* struct mspi_dev_cfg */
	NRFE_MSPI_CONFIG_XFER,		/* struct mspi_xfer */
	NRFE_MSPI_TX,
	NRFE_MSPI_TXRX,
	NRFE_MSPI_WRONG_OPCODE,
	NRFE_MSPI_ALL_OPCODES = NRFE_MSPI_WRONG_OPCODE,
};

typedef struct __packed {
	uint8_t opcode; /* nrfe_mspi_opcode */
	pinctrl_soc_pin_t pin[NRFE_MSPI_PINS_MAX];
} nrfe_mspi_pinctrl_soc_pin_t;

typedef struct __packed {
	uint8_t opcode; /* nrfe_mspi_opcode */
	uint8_t data;
} nrfe_mspi_flpr_response_t;

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_MSPI_NRFE_MSPI_H */
