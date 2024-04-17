/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "platform/nrf_802154_platform_timestamper.h"

#include <haly/nrfy_dppi.h>
#include <haly/nrfy_grtc.h>
#include <hal/nrf_dppi.h>
#include <hal/nrf_ppib.h>
#if defined(NRF54H_SERIES)
#include <hal/nrf_ipct.h>
#endif

#include <assert.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>

static int32_t m_timestamp_cc_channel;

#if defined(NRF54H_SERIES)
/* To trigger GRTC.TASKS_CAPTURE[#cc] with RADIO.EVENT_{?}, the following connection chain must be
 * created:
 *    - starting from LOCAL (RADIO core) domain:
 *        {a} RADIO.EVENT_{?}  --> DPPIC_020
 *        {b} DPPIC_020        --> IPCT_radio
 *    - entering the GLOBAL "Main power" domain (G1):
 *        {c} IPCT_radio       --> IPCT_130
 *        {d} IPCT_130         --> DPPIC_130
 *        {e} DPPIC_130        --> PPIB_130
 *        {f} PPIB_130         --> PPIB_133
 *    - ending in the GLOBAL "Active power" domain (G2):
 *        {g} PPIB_133         --> DPPIC_132
 *        {h} DPPIC_132        --> GRTC.CC
 */

/* Peripherals used for timestamping - located in local domain (_L_) */
/*   - DPPIC_L : DPPIC020 (RADIOCORE.DPPIC020) */
#define DPPIC_L_INST                 NRF_DPPIC020

/*   - IPCT_L : NRF_IPCT (RADIOCORE.IPCT) */
#define IPCT_L_TS_CHANNEL            0
#define IPCT_L_TASK_SEND             NRFX_CONCAT_2(NRF_IPCT_TASK_SEND_, IPCT_L_TS_CHANNEL)

/* Peripherals used for timestamping - located in global "Main power domain" (_G1_) */
/*   - IPCT_G1 : IPCT130_S */
#define IPCT_G1_INST                 NRF_IPCT130_S
#define IPCT_G1_TS_CHANNEL           0
#if defined(NRF54H20_ENGA_XXAA)
#define IPCT_G1_SHORTS               IPCT_SHORTS_RECEIVE0_ACK0_Msk
#else
#define IPCT_G1_SHORTS               IPCT_SHORTS_RECEIVE0_FLUSH0_Msk
#endif
#define IPCT_G1_EVENT_RECEIVE        NRFX_CONCAT_2(NRF_IPCT_EVENT_RECEIVE_, IPCT_G1_TS_CHANNEL)

/*   - DPPIC_G1 : DPPIC130_S */
/* The channel must be in the [0..7] range to satisfy the requirements for the dependent macros */
#define DPPIC_G1_INST                NRF_DPPIC130_S
#define DPPIC_G1_TS_CHANNEL          3

/*   - PPIB_G1 : PPIB130_S */
#define PPIB_G1_TS_CHANNEL           (DPPIC_G1_TS_CHANNEL + 8) /* hw-fixed dependency */

/* Peripherals used for timestamping - located in global "Active power domain" (_G2_) */
/*   - PPIB_G2 : PPIB133_S */
#define PPIB_G2_TS_CHANNEL           (PPIB_G1_TS_CHANNEL - 8) /* hw-fixed dependency */

/*   - DPPIC_G2 : DPPIC132_S */
#define DPPIC_G2_INST                NRF_DPPIC132_S
#define DPPIC_G2_TS_CHANNEL          (PPIB_G2_TS_CHANNEL + 0) /* hw-fixed dependency */

#if (PPIB_G1_TS_CHANNEL < 8) || (PPIB_G1_TS_CHANNEL > 15)
/* Only this range of channels can be connected to PPIB133 */
#error PPIB_G1_TS_CHANNEL is required to be in the [8..15] range
#endif

void nrf_802154_platform_timestamper_cross_domain_connections_setup(void)
{
	/* {c} IPCT_radio --> IPCT_130
	 * It is assumed that this connection has already been made by SECURE-core as a result of
	 * configuring the UICR->IPCMAP[] registers.
	 *
	 * Only enable auto confirmations on destination - IPCT_130.
	 */
	nrf_ipct_shorts_enable(IPCT_G1_INST, IPCT_G1_SHORTS);

	/* {d} IPCT_130 --> DPPIC_130 */
	nrf_ipct_publish_set(IPCT_G1_INST, IPCT_G1_EVENT_RECEIVE, DPPIC_G1_TS_CHANNEL);

	/* {e} DPPIC_130 --> PPIB_130
	 * It is assumed that this connection has already been made by SECURE-core as a result of
	 * configuring the UICR->DPPI.GLOBAL[].CH.LINK.SOURCE registers.
	 *
	 * Only enable relevant DPPIC_G1_INST channel.
	 */
	nrfy_dppi_channels_enable(DPPIC_G1_INST, 1UL << DPPIC_G1_TS_CHANNEL);

	/* {f} PPIB_130 --> PPIB_133
	 * One of HW-fixed connections (channels [8..15] --> [0..7]), so nothing to do.
	 */

	/* {g} PPIB_133 --> DPPIC_132
	 * It is assumed that this connection has already been made by SECURE-core as a result of
	 * configuring of the UICR->DPPI.GLOBAL[].CH.LINK.SINK registers.
	 */

	/* {h} DPPIC_132 --> GRTC.CC */
	nrf_grtc_task_t capture_task =
		nrfy_grtc_sys_counter_capture_task_get(m_timestamp_cc_channel);
	NRF_DPPI_ENDPOINT_SETUP(
		nrfy_grtc_task_address_get(NRF_GRTC, capture_task), DPPIC_G2_TS_CHANNEL);

	/* Enable relevant DPPIC_G2_INST channel. */
	nrfy_dppi_channels_enable(DPPIC_G2_INST, 1UL << DPPIC_G2_TS_CHANNEL);
}

void nrf_802154_platform_timestamper_local_domain_connections_setup(uint32_t dppi_ch)
{
	/* {a} RADIO.EVENT_{?} --> DPPIC_020[dppi_ch]
	 * It is the responsibility of the user of this platform to make the {a} connection
	 * and pass the DPPI channel number as a parameter here.
	 */

	/* {b} DPPIC_020[dppi_ch] to IPCT_radio. */
	nrf_ipct_subscribe_set(NRF_IPCT, IPCT_L_TASK_SEND, dppi_ch);
}

#elif defined(NRF54L_SERIES)

/* To trigger GRTC.TASKS_CAPTURE[#cc] with RADIO.EVENT_{?}, the following connection chain must be
 * created:
 *    - starting from RADIO domain (_R_):
 *        {a} RADIO.EVENT_{?}  --> DPPIC_10
 *        {b} DPPIC_10         --> PPIB_11
 *    - crossing domain boundaries
 *        {c} PPIB_11          --> PPIB_21
 *    - ending in the PERI domain (_P_):
 *        {d} PPIB_21          --> DPPIC_20
 *        {e} DPPIC_20         --> GRTC.CC
 */

/* Peripherals used for timestamping - located in radio power domain (_R_) */
/*   - DPPIC_L : DPPIC10 */
#define DPPIC_R_INST                NRF_DPPIC10
#define PPIB_R_INST                 NRF_PPIB11

/* Peripherals used for timestamping - located in peripheral power domain (_P_) */

/*   - DPPIC_P : DPPIC20 */
#define DPPIC_P_INST                NRF_DPPIC20

/*  - PPIB_P : PPIB21 */
#define PPIB_P_INST                 NRF_PPIB21

void nrf_802154_platform_timestamper_cross_domain_connections_setup(void)
{
	/* Intentionally empty. */
}

void nrf_802154_platform_timestamper_local_domain_connections_setup(uint32_t dppi_ch)
{
	z_nrf_grtc_timer_capture_prepare(m_timestamp_cc_channel);

	/* {a} RADIO.EVENT_{?} --> DPPIC_10
	 * It is the responsibility of the user of this platform to make the {a} connection
	 * and pass the DPPI channel number as a parameter here.
	 */

	/* {b} DPPIC_10 --> PPIB_11 */
	nrf_ppib_subscribe_set(PPIB_R_INST, nrf_ppib_send_task_get(dppi_ch), dppi_ch);

	/* {c} PPIB_11 --> PPIB_21
	 * One of HW-fixed connections, so nothing to do.
	 */

	/* {d} PPIB_21 --> DPPIC_20 */
	nrf_ppib_publish_set(PPIB_P_INST, nrf_ppib_receive_event_get(dppi_ch), dppi_ch);

	/* {e} DPPIC_20[dppi_ch] --> GRTC.CC[cc_channel] */
	nrf_grtc_task_t capture_task =
		nrfy_grtc_sys_counter_capture_task_get(m_timestamp_cc_channel);
	NRF_DPPI_ENDPOINT_SETUP(nrfy_grtc_task_address_get(NRF_GRTC, capture_task), dppi_ch);

	nrfy_dppi_channels_enable(DPPIC_P_INST, 1UL << dppi_ch);
}

#endif

void nrf_802154_platform_timestamper_init(void)
{
	m_timestamp_cc_channel = z_nrf_grtc_timer_chan_alloc();
	assert(m_timestamp_cc_channel >= 0);
}

void nrf_802154_platform_timestamper_cross_domain_connections_clear(void)
{
	nrf_grtc_task_t capture_task =
		nrfy_grtc_sys_counter_capture_task_get(m_timestamp_cc_channel);

	NRF_DPPI_ENDPOINT_CLEAR(nrfy_grtc_task_address_get(NRF_GRTC, capture_task));
}

void nrf_802154_platform_timestamper_local_domain_connections_clear(uint32_t dppi_ch)
{
	/* Intentionally empty. */
}

bool nrf_802154_platform_timestamper_captured_timestamp_read(uint64_t *p_captured)
{
	/* @todo: check if this can be replaced with:
	 *
	 * z_nrf_grtc_timer_capture_read(m_timestamp_cc_channel, p_captured);
	 */
	if (nrf_grtc_sys_counter_cc_enable_check(NRF_GRTC, m_timestamp_cc_channel)) {
		return false;
	}

	*p_captured = nrfy_grtc_sys_counter_cc_get(NRF_GRTC, m_timestamp_cc_channel);
	return true;
}
