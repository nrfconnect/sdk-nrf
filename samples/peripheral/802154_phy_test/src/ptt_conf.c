/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: Production configuration routines */

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include "nrf_802154.h"

bool ptt_get_mode_mask_ext(uint32_t *mode_mask)
{
	assert(mode_mask != NULL);

#ifdef CONFIG_PTT_DUT_MODE
	*mode_mask = 0x00000001;
#elif CONFIG_PTT_CMD_MODE
	*mode_mask = 0x00000002;
#else
	*mode_mask = 0x00000003;
#endif
	return true;
}

bool ptt_get_channel_mask_ext(uint32_t *channel_mask)
{
	assert(channel_mask != NULL);

	*channel_mask = CONFIG_PTT_CHANNEL_MASK;
	return true;
}

bool ptt_get_power_ext(int8_t *power)
{
	assert(power != NULL);

	*power = CONFIG_PTT_POWER;
	return true;
}

bool ptt_get_antenna_ext(uint8_t *antenna)
{
	assert(antenna != NULL);

#if CONFIG_PTT_ANTENNA_DIVERSITY
	*antenna = CONFIG_PTT_ANTENNA_NUMBER;
	return true;
#else
	return false;
#endif
}

bool ptt_get_sw_version_ext(uint8_t *sw_version)
{
	assert(sw_version != NULL);

	*sw_version = CONFIG_PTT_SW_VERSION;
	return true;
}

bool ptt_get_hw_version_ext(uint8_t *hw_version)
{
	assert(hw_version != NULL);

	*hw_version = CONFIG_PTT_HW_VERSION;
	return true;
}

bool ptt_get_ant_mode_ext(uint8_t *ant_mode)
{
	assert(ant_mode != NULL);

#if CONFIG_PTT_ANTENNA_DIVERSITY
#if CONFIG_PTT_ANT_MODE_AUTO
	nrf_802154_antenna_diversity_rx_mode_set(NRF_802154_SL_ANT_DIV_MODE_AUTO);
	nrf_802154_antenna_diversity_tx_mode_set(NRF_802154_SL_ANT_DIV_MODE_AUTO);
#elif CONFIG_PTT_ANT_MODE_MANUAL
	nrf_802154_antenna_diversity_rx_mode_set(NRF_802154_SL_ANT_DIV_MODE_AUTO);
	nrf_802154_antenna_diversity_tx_mode_set(NRF_802154_SL_ANT_DIV_MODE_AUTO);

	nrf_802154_antenna_diversity_rx_mode_set(NRF_802154_SL_ANT_DIV_MODE_MANUAL);
	nrf_802154_antenna_diversity_tx_mode_set(NRF_802154_SL_ANT_DIV_MODE_MANUAL);
#else
#endif
	return true;
#else
	return false;
#endif
}

bool ptt_get_ant_num_ext(uint8_t *ant_num)
{
	assert(ant_num != NULL);

#if CONFIG_PTT_ANTENNA_DIVERSITY
	*ant_num = CONFIG_PTT_ANTENNA_NUMBER;
	return true;
#else
	return false;
#endif
}
