/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Minimal stand-in for the nrfx UARTE driver API used by the trace backend. */

#ifndef NRFX_UARTE_H__
#define NRFX_UARTE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/sys/util.h>

typedef struct {
	uint32_t unused;
} NRF_UARTE_Type;

#define UARTE_TXD_MAXCNT_MAXCNT_Pos 0UL
#define UARTE_TXD_MAXCNT_MAXCNT_Msk (0xFFFFUL << UARTE_TXD_MAXCNT_MAXCNT_Pos)

#define NRF_UARTE_PSEL_DISCONNECTED 0xFFFFFFFFUL

typedef uint32_t nrf_uarte_baudrate_t;

typedef enum {
	NRF_UARTE_HWFC_DISABLED,
	NRF_UARTE_HWFC_ENABLED,
} nrf_uarte_hwfc_t;

typedef struct {
	nrf_uarte_hwfc_t hwfc;
} nrf_uarte_config_t;

typedef struct {
	NRF_UARTE_Type *p_reg;
} nrfx_uarte_t;

#define NRFX_UARTE_INSTANCE(reg_addr) {.p_reg = (NRF_UARTE_Type *)(reg_addr)}

typedef struct {
	uint32_t txd_pin;
	uint32_t rxd_pin;
	uint32_t rts_pin;
	uint32_t cts_pin;
	void *p_context;
	nrf_uarte_baudrate_t baudrate;
	nrf_uarte_config_t config;
	uint8_t interrupt_priority;
	bool skip_gpio_cfg;
	bool skip_psel_cfg;
} nrfx_uarte_config_t;

#define NRFX_UARTE_DEFAULT_CONFIG(_pin_tx, _pin_rx)                                                \
	{                                                                                          \
		.txd_pin = (_pin_tx),                                                              \
		.rxd_pin = (_pin_rx),                                                              \
		.rts_pin = NRF_UARTE_PSEL_DISCONNECTED,                                            \
		.cts_pin = NRF_UARTE_PSEL_DISCONNECTED,                                            \
		.baudrate = 0x01D60000UL,                                                          \
		.config = {.hwfc = NRF_UARTE_HWFC_DISABLED},                                       \
		.interrupt_priority = 7,                                                           \
	}

typedef enum {
	NRFX_UARTE_EVT_TX_DONE,
	NRFX_UARTE_EVT_RX_DONE,
	NRFX_UARTE_EVT_ERROR,
} nrfx_uarte_event_type_t;

typedef struct {
	const uint8_t *p_buffer;
	size_t length;
	uint32_t flags;
} nrfx_uarte_tx_evt_t;

typedef struct {
	nrfx_uarte_event_type_t type;
	union {
		nrfx_uarte_tx_evt_t tx;
	} data;
} nrfx_uarte_event_t;

typedef void (*nrfx_uarte_event_handler_t)(nrfx_uarte_event_t const *p_event, void *p_context);

#define NRFX_UARTE_TX_LINK	   BIT(2)
#define NRFX_UARTE_TX_DONE_ABORTED BIT(0)

int nrfx_uarte_init(nrfx_uarte_t *p_instance, nrfx_uarte_config_t const *p_config,
		    nrfx_uarte_event_handler_t event_handler);

void nrfx_uarte_uninit(nrfx_uarte_t *p_instance);

int nrfx_uarte_tx(nrfx_uarte_t *p_instance, uint8_t const *p_data, size_t length, uint32_t flags);

int nrfx_uarte_tx_abort(nrfx_uarte_t *p_instance, bool sync);

bool nrfx_uarte_tx_in_progress(nrfx_uarte_t const *p_instance);

void nrf_uarte_enable(NRF_UARTE_Type *p_reg);

void nrf_uarte_baudrate_set(NRF_UARTE_Type *p_reg, nrf_uarte_baudrate_t baudrate);

/* Takes a const void pointer to match the ISR signature of IRQ_CONNECT on native_sim. */
void nrfx_uarte_irq_handler(const void *p_instance);

bool nrf_dma_accessible_check(void const *p_reg, void const *p_object);

#endif /* NRFX_UARTE_H__ */
