/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <hal/nrf_gpiote.h>
#include <nrfx_gpiote.h>
#include <helpers/nrfx_gppi.h>

#include <debug/ppi_trace.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ppi_trace, CONFIG_PPI_TRACE_LOG_LEVEL);

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

static atomic_t alloc_cnt;
static uint32_t handle_pool[CONFIG_PPI_TRACE_PIN_CNT];

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
	uint32_t idx = atomic_inc(&alloc_cnt);
	nrfx_gppi_handle_t *handle = (nrfx_gppi_handle_t *)&handle_pool[idx];
	ppi_trace_gpiote_pin_t ppi_trace_gpiote_pin = {};

	/* All slots taken. */
	if (alloc_cnt > CONFIG_PPI_TRACE_PIN_CNT) {
		return NULL;
	}

	if (!ppi_trace_gpiote_pin_init(&ppi_trace_gpiote_pin, pin)) {
		return NULL;
	}

	uint32_t tep = nrf_gpiote_task_address_get(ppi_trace_gpiote_pin.gpiote->p_reg,
				nrf_gpiote_out_task_get(ppi_trace_gpiote_pin.gpiote_channel));

	if (nrfx_gppi_conn_alloc(evt, tep, handle) < 0) {
		LOG_ERR("Failed to allocate GPPI channel.");
		return NULL;
	}
	return handle;
}

void *ppi_trace_pair_config(uint32_t pin, uint32_t start_evt, uint32_t stop_evt)
{
	uint32_t idx = atomic_add(&alloc_cnt, 2);
	nrfx_gppi_handle_t *handle1 = (nrfx_gppi_handle_t *)&handle_pool[idx];
	nrfx_gppi_handle_t *handle2 = (nrfx_gppi_handle_t *)&handle_pool[idx + 1];

	/* All slots taken. */
	if (alloc_cnt > CONFIG_PPI_TRACE_PIN_CNT) {
		return NULL;
	}

#if !defined(GPIOTE_FEATURE_SET_PRESENT) || \
    !defined(GPIOTE_FEATURE_CLR_PRESENT)
	__ASSERT(0, "Function not supported on this platform.");
	return NULL;
#else
	ppi_trace_gpiote_pin_t ppi_trace_gpiote_pin = {};

	if (!ppi_trace_gpiote_pin_init(&ppi_trace_gpiote_pin, pin)) {
		return NULL;
	}

	uint32_t tep;

	tep = nrf_gpiote_task_address_get(ppi_trace_gpiote_pin.gpiote->p_reg,
			nrf_gpiote_set_task_get(ppi_trace_gpiote_pin.gpiote_channel));
	if (nrfx_gppi_conn_alloc(start_evt, tep, handle1) < 0) {
		LOG_ERR("Failed to allocate GPPI channel.");
		return NULL;
	}

	tep = nrf_gpiote_task_address_get(ppi_trace_gpiote_pin.gpiote->p_reg,
			nrf_gpiote_clr_task_get(ppi_trace_gpiote_pin.gpiote_channel));
	if (nrfx_gppi_conn_alloc(stop_evt, tep, handle2) < 0) {
		LOG_ERR("Failed to allocate GPPI channel.");
		return NULL;
	}

	/* Address to aligned 32 bit variable will always have 0 on last two bits. Last bit is
	 * used to indicated that it is a pair.
	 */
	return (void *)((uintptr_t)handle1 | 0x1);
#endif
}

#if defined(DPPI_PRESENT)
int ppi_trace_dppi_ch_trace(uint32_t pin, uint32_t dppi_ch, NRF_DPPIC_Type *p_dppi)
{
	nrfx_gppi_handle_t handle;
	ppi_trace_gpiote_pin_t ppi_trace_gpiote_pin = {};

	if (!ppi_trace_gpiote_pin_init(&ppi_trace_gpiote_pin, pin)) {
		return -ENOMEM;
	}

	uint32_t tep = nrf_gpiote_task_address_get(ppi_trace_gpiote_pin.gpiote->p_reg,
				nrf_gpiote_out_task_get(ppi_trace_gpiote_pin.gpiote_channel));
	uint32_t dst_domain = nrfx_gppi_domain_id_get((uint32_t)ppi_trace_gpiote_pin.gpiote->p_reg);
	nrfx_gppi_resource_t resource = {
		.domain_id = nrfx_gppi_domain_id_get((uint32_t)p_dppi),
		.channel = dppi_ch
	};

	if (nrfx_gppi_ext_conn_alloc(resource.domain_id, dst_domain, &handle, &resource) < 0) {
		LOG_ERR("Failed to allocate GPPI channel.");
		return -ENOMEM;
	}
	nrfx_gppi_ep_attach(tep, handle);
	nrfx_gppi_conn_enable(handle);

	return 0;
}
#endif /* defined(DPPI_PRESENT) */

void ppi_trace_enable(void *handle)
{
	nrfx_gppi_handle_t *handles = (nrfx_gppi_handle_t *)((uintptr_t)handle & 0x1);

	nrfx_gppi_conn_enable(handles[0]);
	/* If LSB bit is set it indicates that handle is for pair of connections. */
	if ((uintptr_t)handle & 0x1) {
		nrfx_gppi_conn_enable(handles[1]);
	}
}

void ppi_trace_disable(void *handle)
{
	nrfx_gppi_handle_t *handles = (nrfx_gppi_handle_t *)((uintptr_t)handle & 0x1);

	nrfx_gppi_conn_disable(handles[0]);
	/* If LSB bit is set it indicates that handle is for pair of connections. */
	if ((uintptr_t)handle & 0x1) {
		nrfx_gppi_conn_disable(handles[1]);
	}
}
