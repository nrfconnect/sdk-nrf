/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <errno.h>

#include <hal/nrf_radio.h>
#include <hal/nrf_timer.h>
#include <helpers/nrfx_gppi.h>
#include <nrf_erratas.h>

#include <zephyr/sys/printk.h>

#include <mpsl_fem_config_common.h>
#include <mpsl_fem_protocol_api.h>

#include "fem_al/fem_al.h"
#include "fem_interface.h"
#include "radio_def.h"

static const struct fem_interface_api *fem_api;

static mpsl_fem_event_t fem_activate_event = {
	.type = MPSL_FEM_EVENT_TYPE_TIMER,
};

static mpsl_fem_event_t fem_deactivate_evt = {
	.type = MPSL_FEM_EVENT_TYPE_GENERIC,
};

int fem_power_up(void)
{
	/* Handle front-end module specific power up operation. */
	if (fem_api->power_up) {
		return fem_api->power_up();
	}

	return 0;
}

int fem_power_down(void)
{
	int32_t err;

	err = mpsl_fem_disable();
	if (err) {
		printk("Failed to disable front-end module\n");
		return -EPERM;
	}

	/* Fallback to front-end module specific power down operation. */
	if (fem_api->power_down) {
		return fem_api->power_down();
	}

	return 0;
}

int fem_antenna_select(enum fem_antenna ant)
{
	return fem_api->antenna_select(ant);
}

int fem_interface_api_set(const struct fem_interface_api *api)
{
	if (api == NULL) {
		return -EINVAL;
	}

	fem_api = api;

	return 0;
}

uint32_t fem_radio_tx_ramp_up_delay_get(bool fast, nrf_radio_mode_t mode)
{
	uint32_t ramp_up;

	switch (mode) {
	case NRF_RADIO_MODE_BLE_1MBIT:
		ramp_up = fast ? RADIO_RAMP_UP_TX_1M_FAST_US :
				 RADIO_RAMP_UP_TX_1M_US;
		break;

	case NRF_RADIO_MODE_BLE_2MBIT:
		ramp_up = fast ? RADIO_RAMP_UP_TX_2M_FAST_US :
				 RADIO_RAMP_UP_TX_2M_US;
		break;
#if CONFIG_HAS_HW_NRF_RADIO_BLE_CODED
	case NRF_RADIO_MODE_BLE_LR125KBIT:
		ramp_up = fast ? RADIO_RAMP_UP_TX_S8_FAST_US :
				 RADIO_RAMP_UP_TX_S8_US;
		break;

	case NRF_RADIO_MODE_BLE_LR500KBIT:
		ramp_up = fast ? RADIO_RAMP_UP_TX_S2_FAST_US :
				 RADIO_RAMP_UP_TX_S2_US;
		break;
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */

#if CONFIG_HAS_HW_NRF_RADIO_IEEE802154
	case NRF_RADIO_MODE_IEEE802154_250KBIT:
		ramp_up = fast ? RADIO_RAMP_UP_IEEE_FAST_US :
				 RADIO_RAMP_UP_IEEE_US;
		break;
#endif /* CONFIG_HAS_HW_NRF_RADIO_IEEE802154 */

	default:
		ramp_up = fast ? RADIO_RAMP_UP_DEFAULT_FAST_US :
				 RADIO_RAMP_UP_DEFAULT_US;
		break;
	}

	return ramp_up;
}

uint32_t fem_radio_rx_ramp_up_delay_get(bool fast, nrf_radio_mode_t mode)
{
	uint32_t ramp_up;

	switch (mode) {
	case NRF_RADIO_MODE_BLE_1MBIT:
		ramp_up = fast ? RADIO_RAMP_UP_RX_1M_FAST_US :
				 RADIO_RAMP_UP_RX_1M_US;
		break;

	case NRF_RADIO_MODE_BLE_2MBIT:
		ramp_up = fast ? RADIO_RAMP_UP_RX_2M_FAST_US :
				 RADIO_RAMP_UP_RX_2M_US;
		break;

#if CONFIG_HAS_HW_NRF_RADIO_BLE_CODED
	case NRF_RADIO_MODE_BLE_LR125KBIT:
		ramp_up = fast ? RADIO_RAMP_UP_RX_S8_FAST_US :
				 RADIO_RAMP_UP_RX_S8_US;
		break;

	case NRF_RADIO_MODE_BLE_LR500KBIT:
		ramp_up = fast ? RADIO_RAMP_UP_RX_S2_FAST_US :
				 RADIO_RAMP_UP_RX_S2_US;
		break;
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */

#if CONFIG_HAS_HW_NRF_RADIO_IEEE802154
	case NRF_RADIO_MODE_IEEE802154_250KBIT:
		ramp_up = fast ? RADIO_RAMP_UP_IEEE_FAST_US :
				 RADIO_RAMP_UP_IEEE_US;
		break;
#endif /* CONFIG_HAS_HW_NRF_RADIO_IEEE802154 */

	default:
		ramp_up = fast ? RADIO_RAMP_UP_DEFAULT_FAST_US :
				 RADIO_RAMP_UP_DEFAULT_US;
		break;
	}

	return ramp_up;
}

uint32_t fem_default_ramp_up_time_get(bool rx, nrf_radio_mode_t mode)
{
	bool fast_ramp_up = nrf_radio_modecnf0_ru_get(NRF_RADIO);

	return rx ? fem_radio_rx_ramp_up_delay_get(fast_ramp_up, mode) :
		    fem_radio_tx_ramp_up_delay_get(fast_ramp_up, mode);
}

int fem_tx_configure(uint32_t ramp_up_time)
{
	int32_t err;

	fem_activate_event.event.timer.counter_period.end = ramp_up_time;

	err = mpsl_fem_pa_configuration_set(&fem_activate_event, &fem_deactivate_evt);
	if (err) {
		printk("PA configuration set failed (err %d)\n", err);
		return -EFAULT;
	}

	return 0;
}

int fem_rx_configure(uint32_t ramp_up_time)
{
	int32_t err;

	fem_activate_event.event.timer.counter_period.end = ramp_up_time;

	err = mpsl_fem_lna_configuration_set(&fem_activate_event, &fem_deactivate_evt);
	if (err) {
		printk("LNA configuration set failed (err %d)\n", err);
		return -EFAULT;
	}

	return 0;
}

int fem_tx_power_control_set(fem_tx_power_control tx_power_control)
{
	int32_t err;
	mpsl_fem_pa_power_control_t fem_pa_power_control = 0;

	/* Fallback to FEM specific function. It can be used for checking the valid output power
	 * range.
	 */
	if (fem_api->tx_power_control_validate) {
		err = fem_api->tx_power_control_validate(tx_power_control);
		if (err) {
			return err;
		}
	}

	fem_pa_power_control = tx_power_control;

	err = mpsl_fem_pa_power_control_set(fem_pa_power_control);
	if (err) {
		printk("Failed to set front-end module Tx power control (err %d)\n", err);
		return -EINVAL;
	}

	return 0;
}

int fem_txrx_configuration_clear(void)
{
	int32_t err;

	err = mpsl_fem_pa_configuration_clear();
	if (err) {
		printk("Failed to clear PA configuration\n");
		return -EPERM;
	}
	err = mpsl_fem_lna_configuration_clear();
	if (err) {
		printk("Failed to clear LNA configuration\n");
		return -EPERM;
	}

	return 0;
}

void fem_txrx_stop(void)
{
	mpsl_fem_deactivate_now(MPSL_FEM_ALL);
}

int8_t fem_tx_output_power_prepare(int8_t power, int8_t *radio_tx_power, uint16_t freq_mhz)
{
	int8_t output_power;
	int32_t err;
	mpsl_tx_power_split_t power_split = { 0 };

	output_power = mpsl_fem_tx_power_split(power, &power_split, freq_mhz, false);

	*radio_tx_power = power_split.radio_tx_power;

	err = mpsl_fem_pa_power_control_set(power_split.fem_pa_power_control);
	if (err) {
		/* Should not happen */
		printk("Failed to set front-end module Tx power control (err %d)\n", err);
		__ASSERT_NO_MSG(false);
	}

	return output_power;
}

int8_t fem_tx_output_power_check(int8_t power, uint16_t freq_mhz, bool tx_power_ceiling)
{
	mpsl_tx_power_split_t power_split = { 0 };

	return mpsl_fem_tx_power_split(power, &power_split, freq_mhz, tx_power_ceiling);
}

int8_t fem_default_tx_output_power_get(void)
{
	if (fem_api->default_tx_output_power_get) {
		return fem_api->default_tx_output_power_get();
	}

	return 0;
}

int fem_init(NRF_TIMER_Type *timer_instance, uint8_t compare_channel_mask)
{
	if (!timer_instance || (compare_channel_mask == 0)) {
		return -EINVAL;
	}

	fem_activate_event.event.timer.p_timer_instance = timer_instance;
	fem_activate_event.event.timer.compare_channel_mask = compare_channel_mask;

#if defined(DPPI_PRESENT)
	nrfx_err_t err;
	uint8_t fem_generic_dppi;

	err = nrfx_gppi_channel_alloc(&fem_generic_dppi);
	if (err != NRFX_SUCCESS) {
		printk("gppi_channel_alloc failed with: %d\n", err);
		return -ENODEV;
	}

	fem_deactivate_evt.event.generic.event = fem_generic_dppi;

	nrfx_gppi_event_endpoint_setup(fem_generic_dppi,
			nrf_radio_event_address_get(NRF_RADIO, NRF_RADIO_EVENT_DISABLED));
	nrfx_gppi_channels_enable(BIT(fem_generic_dppi));
#elif defined(PPI_PRESENT)
	fem_deactivate_evt.event.generic.event =
				nrf_radio_event_address_get(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);
#else
	__ASSERT_NO_MSG(false);
	return -ENOTSUP;
#endif

	return 0;
}

#if (NRF52_CONFIGURATION_254_PRESENT || NRF52_CONFIGURATION_255_PRESENT || \
	NRF52_CONFIGURATION_256_PRESENT || NRF52_CONFIGURATION_257_PRESENT)
static void apply_fem_errata_25X(nrf_radio_mode_t mode)
{
	static uint8_t old_mode = NRF_RADIO_MODE_NRF_1MBIT;

	if (*(volatile uint32_t *) 0x10000330UL != 0xFFFFFFFFUL) {
		*(volatile uint32_t *) 0x4000174CUL = *(volatile uint32_t *) 0x10000330UL;
	}

	if (mode == NRF_RADIO_MODE_IEEE802154_250KBIT) {
		if (*(volatile uint32_t *) 0x10000334UL != 0xFFFFFFFFUL) {
			*(volatile uint32_t *) 0x40001584UL = *(volatile uint32_t *) 0x10000334UL;
		}
		if (*(volatile uint32_t *) 0x10000338UL != 0xFFFFFFFFUL) {
			*(volatile uint32_t *) 0x40001588UL = *(volatile uint32_t *) 0x10000338UL;
		}
	}

	if ((mode != NRF_RADIO_MODE_IEEE802154_250KBIT) && (mode != old_mode)) {
		if (*(volatile uint32_t *) 0x10000334UL != 0xFFFFFFFFUL) {
			*(volatile uint32_t *) 0x40001584UL =
			((*(volatile uint32_t *) 0x40001584UL) & 0xBFFFFFFUL) | 0x00010000UL;
		}
		if (*(volatile uint32_t *) 0x10000338UL != 0xFFFFFFFFUL) {
			*(volatile uint32_t *) 0x40001588UL =
			((*(volatile uint32_t *) 0x40001588UL) & 0xBFFFFFFUL);
		}
	}

	old_mode = mode;
}
#else
static void apply_fem_errata_25X(nrf_radio_mode_t mode)
{
	ARG_UNUSED(mode);
}
#endif /* (NRF52_CONFIGURATION_254_PRESENT || NRF52_CONFIGURATION_255_PRESENT || \
	*  NRF52_CONFIGURATION_256_PRESENT || NRF52_CONFIGURATION_257_PRESENT)
	*/

void fem_errata_25X(nrf_radio_mode_t mode)
{
	if (nrf52_configuration_254() || nrf52_configuration_255() ||
	    nrf52_configuration_256() || nrf52_configuration_257()) {
		apply_fem_errata_25X(mode);
	}
}
