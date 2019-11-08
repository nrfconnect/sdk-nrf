/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <hal/nrf_gpiote.h>
#ifdef DPPI_PRESENT
#include <nrfx_dppi.h>
#else
#include <nrfx_ppi.h>
#endif
#include <debug/ppi_trace.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(ppi_trace, CONFIG_PPI_TRACE_LOG_LEVEL);

/* Convert task address to associated subscribe register */
#define SUBSCRIBE_ADDR(task) (volatile u32_t *)(task + 0x80)

/* Convert event address to associated publish register */
#define PUBLISH_ADDR(evt) (volatile u32_t *)(evt + 0x80)

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

#define HANDLE_ENCODE(value) (void *)((u32_t)value | HANDLE_FLAG)
#define PACK_CHANNELS(start_ch, stop_ch) (PAIR_FLAG | start_ch | (stop_ch << 8))

#define IS_VALID_HANDLE(handle) ((u32_t)handle & HANDLE_FLAG)
#define IS_PAIR(handle) ((u32_t)handle & PAIR_FLAG)

#define GET_CH(handle) ((u32_t)handle & 0xFF)
#define GET_START_CH(handle) GET_CH(handle)
#define GET_STOP_CH(handle) (((u32_t)handle >> 8) & 0xFF)

/** @brief Allocate (D)PPI channel. */
static nrfx_err_t ppi_alloc(u8_t *ch, u32_t evt)
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
static void ppi_assign(u8_t ch, u32_t evt, u32_t task)
{
#ifdef DPPI_PRESENT
	/* Use mask of one of subscribe registers in the system, assuming that
	 * all subscribe registers has the same mask for enabling channel.
	 */
	*SUBSCRIBE_ADDR(task) = DPPIC_SUBSCRIBE_CHG_EN_EN_Msk | (u32_t)ch;
	*PUBLISH_ADDR(evt) = DPPIC_SUBSCRIBE_CHG_EN_EN_Msk | (u32_t)ch;
#else
	(void)nrfx_ppi_channel_assign(ch, evt, task);
#endif
}

/** @brief Enable (D)PPI channels. */
static void ppi_enable(u32_t channel_mask)
{
#ifdef DPPI_PRESENT
	nrf_dppi_channels_enable(NRF_DPPIC, channel_mask);
#else
	nrf_ppi_channels_enable(channel_mask);
#endif
}

/** @brief Disable (D)PPI channels. */
static void ppi_disable(u32_t channel_mask)
{
#ifdef DPPI_PRESENT
	nrf_dppi_channels_disable(NRF_DPPIC, channel_mask);
#else
	nrf_ppi_channels_disable(channel_mask);
#endif
}

/** @brief Allocate GPIOTE channel.
 *
 * @param pin Pin.
 *
 * @return Allocated channel or -1 if failed to allocate.
 */
static int gpiote_channel_alloc(u32_t pin)
{
	for (u8_t channel = 0; channel < GPIOTE_CH_NUM; ++channel) {
		if (!nrf_gpiote_te_is_enabled(channel)) {
			nrf_gpiote_task_configure(channel, pin,
						  NRF_GPIOTE_POLARITY_TOGGLE,
						  NRF_GPIOTE_INITIAL_VALUE_LOW);
			nrf_gpiote_task_enable(channel);
			return channel;
		}
	}

	return -1;
}

void *ppi_trace_config(u32_t pin, u32_t evt)
{
	int err;
	u32_t task;
	int gpiote_ch;
	nrf_gpiote_tasks_t task_id;
	u8_t ppi_ch;

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
	task = nrf_gpiote_task_addr_get(task_id);
	ppi_assign(ppi_ch, evt, task);

	return HANDLE_ENCODE(ppi_ch);
}

void *ppi_trace_pair_config(u32_t pin, u32_t start_evt, u32_t stop_evt)
{
	int err;
	u32_t task;
	u8_t start_ch;
	u8_t stop_ch;
	int gpiote_ch;
	nrf_gpiote_tasks_t task_set_id;
	nrf_gpiote_tasks_t task_clr_id;

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


	task = nrf_gpiote_task_addr_get(task_set_id);
	ppi_assign(start_ch, start_evt, task);
	task = nrf_gpiote_task_addr_get(task_clr_id);
	ppi_assign(stop_ch, stop_evt, task);

	return HANDLE_ENCODE(PACK_CHANNELS(start_ch, stop_ch));
#endif
}

static u32_t ppi_channel_mask_get(void *handle)
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
