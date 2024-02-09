/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <hal/nrf_gpiote.h>
#include <nrfx_gpiote.h>
#ifdef DPPI_PRESENT
#include <nrfx_dppi.h>
#else
#include <nrfx_ppi.h>
#endif
#include <debug/ppi_trace.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ppi_trace, CONFIG_PPI_TRACE_LOG_LEVEL);

/* Convert task address to associated subscribe register */
#define SUBSCRIBE_ADDR(task) (volatile uint32_t *)(task + 0x80)

/* Convert event address to associated publish register */
#define PUBLISH_ADDR(evt) (volatile uint32_t *)(evt + 0x80)

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
#define GPIOTE_INST_AND_COMMA(gpio_node) \
	[DT_PROP(gpio_node, port)] = \
		NRFX_GPIOTE_INSTANCE(DT_PROP(GPIOTE_NODE(gpio_node), instance)),

static const nrfx_gpiote_t *get_gpiote(nrfx_gpiote_pin_t pin)
{
	static const nrfx_gpiote_t gpiote[GPIO_COUNT] = {
		DT_FOREACH_STATUS_OKAY(nordic_nrf_gpio, GPIOTE_INST_AND_COMMA)
	};

	return &gpiote[pin >> 5];
}

/** @brief Allocate (D)PPI channel. */
static nrfx_err_t ppi_alloc(uint8_t *ch, uint32_t evt)
{
	nrfx_err_t err;
#ifdef DPPI_PRESENT
	if (*PUBLISH_ADDR(evt) != 0) {
		/* Use mask of one of subscribe registers in the system,
		 * assuming that all subscribe registers has the same mask for
		 * channel id.
		 */
		*ch = *PUBLISH_ADDR(evt) & DPPIC_SUBSCRIBE_CHG_EN_CHIDX_Msk;
		err = NRFX_SUCCESS;
	} else {
		err = nrfx_dppi_channel_alloc(ch);
	}
#else
	err = nrfx_ppi_channel_alloc((nrf_ppi_channel_t *)ch);
#endif
	return err;
}

/** @brief Set task and event on (D)PPI channel. */
static void ppi_assign(uint8_t ch, uint32_t evt, uint32_t task)
{
#ifdef DPPI_PRESENT
	/* Use mask of one of subscribe registers in the system, assuming that
	 * all subscribe registers has the same mask for enabling channel.
	 */
	*SUBSCRIBE_ADDR(task) = DPPIC_SUBSCRIBE_CHG_EN_EN_Msk | (uint32_t)ch;
	*PUBLISH_ADDR(evt) = DPPIC_SUBSCRIBE_CHG_EN_EN_Msk | (uint32_t)ch;
#else
	(void)nrfx_ppi_channel_assign(ch, evt, task);
#endif
}

/** @brief Enable (D)PPI channels. */
static void ppi_enable(uint32_t channel_mask)
{
#ifdef DPPI_PRESENT
	nrf_dppi_channels_enable(NRF_DPPIC, channel_mask);
#else
	nrf_ppi_channels_enable(NRF_PPI, channel_mask);
#endif
}

/** @brief Disable (D)PPI channels. */
static void ppi_disable(uint32_t channel_mask)
{
#ifdef DPPI_PRESENT
	nrf_dppi_channels_disable(NRF_DPPIC, channel_mask);
#else
	nrf_ppi_channels_disable(NRF_PPI, channel_mask);
#endif
}

/** @brief Allocate GPIOTE channel.
 *
 * @param pin Pin.
 *
 * @return Allocated channel or -1 if failed to allocate.
 */
static int gpiote_channel_alloc(uint32_t pin)
{
	uint8_t channel;
	const nrfx_gpiote_t *gpiote = get_gpiote(pin);

	if (nrfx_gpiote_channel_alloc(gpiote, &channel) != NRFX_SUCCESS) {
		return -1;
	}

	nrf_gpiote_task_configure(gpiote->p_reg, channel, pin,
				  NRF_GPIOTE_POLARITY_TOGGLE,
				  NRF_GPIOTE_INITIAL_VALUE_LOW);
	nrf_gpiote_task_enable(gpiote->p_reg, channel);

	return channel;
}

void *ppi_trace_config(uint32_t pin, uint32_t evt)
{
	int err;
	uint32_t task;
	int gpiote_ch;
	nrf_gpiote_task_t task_id;
	uint8_t ppi_ch;

	err = ppi_alloc(&ppi_ch, evt);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("Failed to allocate PPI channel.");
		return NULL;
	}

	gpiote_ch = gpiote_channel_alloc(pin);
	if (gpiote_ch < 0) {
		LOG_ERR("Failed to allocate GPIOTE channel.");
		return NULL;
	}

	task_id = offsetof(NRF_GPIOTE_Type, TASKS_OUT[gpiote_ch]);
	task = nrf_gpiote_task_address_get(get_gpiote(pin)->p_reg, task_id);
	ppi_assign(ppi_ch, evt, task);

	return HANDLE_ENCODE(ppi_ch);
}

void *ppi_trace_pair_config(uint32_t pin, uint32_t start_evt, uint32_t stop_evt)
{
	int err;
	uint32_t task;
	uint8_t start_ch;
	uint8_t stop_ch;
	int gpiote_ch;
	nrf_gpiote_task_t task_set_id;
	nrf_gpiote_task_t task_clr_id;

#if !defined(GPIOTE_FEATURE_SET_PRESENT) || \
    !defined(GPIOTE_FEATURE_CLR_PRESENT)
	__ASSERT(0, "Function not supported on this platform.");
	return NULL;
#else

	err = ppi_alloc(&start_ch, start_evt);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("Failed to allocate PPI channel.");
		return NULL;
	}

	err = ppi_alloc(&stop_ch, stop_evt);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("Failed to allocate PPI channel.");
		return NULL;
	}

	gpiote_ch = gpiote_channel_alloc(pin);
	if (gpiote_ch < 0) {
		LOG_ERR("Failed to allocate GPIOTE channel.");
		return NULL;
	}

	task_set_id = offsetof(NRF_GPIOTE_Type, TASKS_SET[gpiote_ch]);
	task_clr_id = offsetof(NRF_GPIOTE_Type, TASKS_CLR[gpiote_ch]);

	task = nrf_gpiote_task_address_get(get_gpiote(pin)->p_reg, task_set_id);
	ppi_assign(start_ch, start_evt, task);
	task = nrf_gpiote_task_address_get(get_gpiote(pin)->p_reg, task_clr_id);
	ppi_assign(stop_ch, stop_evt, task);

	return HANDLE_ENCODE(PACK_CHANNELS(start_ch, stop_ch));
#endif
}

int ppi_trace_dppi_ch_trace(uint32_t pin, uint32_t dppi_ch)
{
#ifdef DPPI_PRESENT
	uint32_t task;
	int gpiote_ch;
	nrf_gpiote_task_t task_id;

	gpiote_ch = gpiote_channel_alloc(pin);
	if (gpiote_ch < 0) {
		LOG_ERR("Failed to allocate GPIOTE channel.");
		return -ENOMEM;
	}

	task_id = offsetof(NRF_GPIOTE_Type, TASKS_OUT[gpiote_ch]);
	task = nrf_gpiote_task_address_get(NRF_GPIOTE, task_id);

	*SUBSCRIBE_ADDR(task) = DPPIC_SUBSCRIBE_CHG_EN_EN_Msk | dppi_ch;

	return 0;
#else
	(void)pin;
	(void)dppi_ch;

	return -ENOTSUP;
#endif
}

static uint32_t ppi_channel_mask_get(void *handle)
{
	return IS_PAIR(handle) ?
			BIT(GET_START_CH(handle)) | BIT(GET_STOP_CH(handle)) :
			BIT(GET_CH(handle));
}

void ppi_trace_enable(void *handle)
{
	ppi_enable(ppi_channel_mask_get(handle));
}

void ppi_trace_disable(void *handle)
{
	ppi_disable(ppi_channel_mask_get(handle));
}
