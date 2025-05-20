/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <hal/nrf_gpiote.h>
#include <nrfx_gpiote.h>
#ifdef DPPI_PRESENT
#include <nrfx_dppi.h>
#endif
#include <helpers/nrfx_gppi.h>

#include <debug/ppi_trace.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ppi_trace, CONFIG_PPI_TRACE_LOG_LEVEL);

/* Handle which is used by the user to enable and disable the trace pin is
 * encapsulating ppi channel(s). Bit 31 is set to indicate that handle is
 * valid. Bit 30 is set to indicate that handle contains pair of PPI channels.
 *
 * Handle structure:
 * ----------------------------------------------------------------
 * |handle_flag|pair_flag|  not used | stop_ch | start_ch/main_ch |
 * |-----------|---------|-----------|---------|------------------|
 * |    31     |   30    |   29-16   | 15 - 8  |       7-0        |
 * ----------------------------------------------------------------
 */
#define HANDLE_FLAG BIT(31)
#define PAIR_FLAG BIT(30)

#define HANDLE_ENCODE(value) (void *)((uint32_t)value | HANDLE_FLAG)
#define PACK_CHANNELS(start_ch, stop_ch) (PAIR_FLAG | start_ch | (stop_ch << 8))

#define IS_VALID_HANDLE(handle) ((uint32_t)handle & HANDLE_FLAG)
#define IS_PAIR(handle) ((uint32_t)handle & PAIR_FLAG)

#define GET_CH(handle) ((uint32_t)handle & 0xFF)
#define GET_START_CH(handle) GET_CH(handle)
#define GET_STOP_CH(handle) (((uint32_t)handle >> 8) & 0xFF)

#define GPIOTE_NODE(gpio_node) DT_PHANDLE(gpio_node, gpiote_instance)
#define INVALID_NRFX_GPIOTE_INSTANCE \
	{ .p_reg = NULL }
#define GPIOTE_INST_AND_COMMA(gpio_node) \
	[DT_PROP(gpio_node, port)] = \
		COND_CODE_1(DT_NODE_HAS_PROP(gpio_node, gpiote_instance), \
			(NRFX_GPIOTE_INSTANCE(DT_PROP(GPIOTE_NODE(gpio_node), instance))), \
			(INVALID_NRFX_GPIOTE_INSTANCE)),

typedef struct {
	const nrfx_gpiote_t *gpiote;
	uint8_t              gpiote_channel;
} ppi_trace_gpiote_pin_t;

static const nrfx_gpiote_t *get_gpiote(nrfx_gpiote_pin_t pin)
{
	static const nrfx_gpiote_t gpiote[GPIO_COUNT] = {
		DT_FOREACH_STATUS_OKAY(nordic_nrf_gpio, GPIOTE_INST_AND_COMMA)
	};

	const nrfx_gpiote_t *result = &gpiote[pin >> 5];

	if (result->p_reg == NULL) {
		/* On given pin's port there is no GPIOTE. */
		result = NULL;
	}

	return result;
}

static bool ppi_trace_gpiote_pin_init(
		ppi_trace_gpiote_pin_t *ppi_trace_gpiote_pin, nrfx_gpiote_pin_t pin)
{
	ppi_trace_gpiote_pin->gpiote = get_gpiote(pin);
	if (ppi_trace_gpiote_pin->gpiote == NULL) {
		LOG_ERR("Given GPIO pin has no associated GPIOTE.");
		return false;
	}

	if (nrfx_gpiote_channel_alloc(ppi_trace_gpiote_pin->gpiote,
		&ppi_trace_gpiote_pin->gpiote_channel) != NRFX_SUCCESS) {
		LOG_ERR("Failed to allocate GPIOTE channel.");
		return false;
	}

	nrf_gpiote_task_configure(ppi_trace_gpiote_pin->gpiote->p_reg,
				  ppi_trace_gpiote_pin->gpiote_channel,
				  pin,
				  NRF_GPIOTE_POLARITY_TOGGLE,
				  NRF_GPIOTE_INITIAL_VALUE_LOW);
	nrf_gpiote_task_enable(ppi_trace_gpiote_pin->gpiote->p_reg,
			       ppi_trace_gpiote_pin->gpiote_channel);

	return true;
}

void *ppi_trace_config(uint32_t pin, uint32_t evt)
{
	uint8_t gppi_ch = UINT8_MAX;

	if (NRFX_SUCCESS != nrfx_gppi_channel_alloc(&gppi_ch)) {
		LOG_ERR("Failed to allocate GPPI channel.");
		return NULL;
	}

	ppi_trace_gpiote_pin_t ppi_trace_gpiote_pin = {};

	if (!ppi_trace_gpiote_pin_init(&ppi_trace_gpiote_pin, pin)) {
		nrfx_gppi_channel_free(gppi_ch);
		return NULL;
	}

	uint32_t tep = nrf_gpiote_task_address_get(ppi_trace_gpiote_pin.gpiote->p_reg,
				nrf_gpiote_out_task_get(ppi_trace_gpiote_pin.gpiote_channel));
	nrfx_gppi_channel_endpoints_setup(gppi_ch, evt, tep);

	return HANDLE_ENCODE(gppi_ch);
}

void *ppi_trace_pair_config(uint32_t pin, uint32_t start_evt, uint32_t stop_evt)
{
#if !defined(GPIOTE_FEATURE_SET_PRESENT) || \
    !defined(GPIOTE_FEATURE_CLR_PRESENT)
	__ASSERT(0, "Function not supported on this platform.");
	return NULL;
#else
	uint8_t gppi_ch_start_evt = UINT8_MAX;

	if (NRFX_SUCCESS != nrfx_gppi_channel_alloc(&gppi_ch_start_evt)) {
		LOG_ERR("Failed to allocate GPPI channel.");
		return NULL;
	}

	uint8_t gppi_ch_stop_evt = UINT8_MAX;

	if (NRFX_SUCCESS != nrfx_gppi_channel_alloc(&gppi_ch_stop_evt)) {
		LOG_ERR("Failed to allocate GPPI channel.");
		nrfx_gppi_channel_free(gppi_ch_start_evt);
		return NULL;
	}

	ppi_trace_gpiote_pin_t ppi_trace_gpiote_pin = {};

	if (!ppi_trace_gpiote_pin_init(&ppi_trace_gpiote_pin, pin)) {
		nrfx_gppi_channel_free(gppi_ch_stop_evt);
		nrfx_gppi_channel_free(gppi_ch_start_evt);
		return NULL;
	}

	uint32_t tep;

	tep = nrf_gpiote_task_address_get(ppi_trace_gpiote_pin.gpiote->p_reg,
			nrf_gpiote_set_task_get(ppi_trace_gpiote_pin.gpiote_channel));
	nrfx_gppi_channel_endpoints_setup(gppi_ch_start_evt, start_evt, tep);

	tep = nrf_gpiote_task_address_get(ppi_trace_gpiote_pin.gpiote->p_reg,
			nrf_gpiote_clr_task_get(ppi_trace_gpiote_pin.gpiote_channel));
	nrfx_gppi_channel_endpoints_setup(gppi_ch_stop_evt, stop_evt, tep);

	return HANDLE_ENCODE(PACK_CHANNELS(gppi_ch_start_evt, gppi_ch_stop_evt));
#endif
}

#if defined(DPPI_PRESENT)

static const nrfx_dppi_t *get_dppic_for_gpiote(const nrfx_gpiote_t *gpiote)
{
	/* The Device Tree does not provide information about the DPPIC controller for which
	 * the given GPIOTE instance can subscribe to. That's why we need to provide the
	 * matching DPPIC instance ourselves.
	 */

#if (!defined(DPPIC_COUNT) || (DPPIC_COUNT == 1))
	static const nrfx_dppi_t dppic = NRFX_DPPI_INSTANCE(0);

	return &dppic;
#elif defined(CONFIG_SOC_SERIES_NRF54LX)
	if (gpiote->p_reg == NRF_GPIOTE20) {
		static const nrfx_dppi_t dppic20 = NRFX_DPPI_INSTANCE(20);

		return &dppic20;
	} else if (gpiote->p_reg == NRF_GPIOTE30) {
		static const nrfx_dppi_t dppic30 = NRFX_DPPI_INSTANCE(30);

		return &dppic30;
	} else {
		return NULL;
	}
#else
#error Unsupported SoC series
	return NULL;
#endif
}

int ppi_trace_dppi_ch_trace(uint32_t pin, uint32_t dppi_ch, const nrfx_dppi_t *dppic)
{
	ppi_trace_gpiote_pin_t ppi_trace_gpiote_pin = {};

	if (!ppi_trace_gpiote_pin_init(&ppi_trace_gpiote_pin, pin)) {
		return -ENOMEM;
	}

	const nrfx_dppi_t *dppic_for_gpiote = get_dppic_for_gpiote(ppi_trace_gpiote_pin.gpiote);

	if (dppic_for_gpiote == NULL) {
		LOG_ERR("For given GPIO pin, the GPIOTE has no associated DPPIC.");
		return -ENOMEM;
	}

	if (dppic_for_gpiote->p_reg == dppic->p_reg) {
		/* The GPIOTE can directly subscribe to DPPI channels of `dppic` */
		nrf_gpiote_subscribe_set(ppi_trace_gpiote_pin.gpiote->p_reg,
			nrf_gpiote_out_task_get(ppi_trace_gpiote_pin.gpiote_channel),
			dppi_ch);
	} else {
		/* Let the GPIOTE channel subscribe to a local dppi_channel_for_gpiote of
		 * ppi_trace_gpiote_pin.dppic first.
		 */
		uint8_t dppi_channel_for_gpiote = UINT8_MAX;

		if (nrfx_dppi_channel_alloc(dppic_for_gpiote,
			&dppi_channel_for_gpiote) != NRFX_SUCCESS) {
			return -ENOMEM;
		}

		nrf_gpiote_subscribe_set(ppi_trace_gpiote_pin.gpiote->p_reg,
			nrf_gpiote_out_task_get(ppi_trace_gpiote_pin.gpiote_channel),
			dppi_channel_for_gpiote);

		(void)nrfx_dppi_channel_enable(dppic_for_gpiote, dppi_channel_for_gpiote);

		/* Then, let the dppi_ch channel of dppic controller passed by parameters
		 * trigger the local dppi_channel_for_gpiote.
		 */
		uint8_t gppi_ch = UINT8_MAX;

		if (NRFX_SUCCESS != nrfx_gppi_channel_alloc(&gppi_ch)) {
			LOG_ERR("Failed to allocate GPPI channel.");
			return -ENOMEM;
		}

		if (NRFX_SUCCESS != nrfx_gppi_edge_connection_setup(gppi_ch,
			dppic, dppi_ch, dppic_for_gpiote, dppi_channel_for_gpiote)) {
			LOG_ERR("Failed to setup a GPPI edge connection.");
			return -ENOMEM;
		}

		nrfx_gppi_channels_enable(1U << gppi_ch);
	}
	return 0;
}
#endif /* defined(DPPI_PRESENT) */

static uint32_t ppi_channel_mask_get(void *handle)
{
	return IS_PAIR(handle) ?
			BIT(GET_START_CH(handle)) | BIT(GET_STOP_CH(handle)) :
			BIT(GET_CH(handle));
}

void ppi_trace_enable(void *handle)
{
	nrfx_gppi_channels_enable(ppi_channel_mask_get(handle));
}

void ppi_trace_disable(void *handle)
{
	nrfx_gppi_channels_disable(ppi_channel_mask_get(handle));
}
