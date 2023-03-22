/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <errno.h>

#include <hal/nrf_radio.h>
#include <hal/nrf_timer.h>
#include <helpers/nrfx_gppi.h>

#include <zephyr/sys/printk.h>

#include <mpsl_fem_config_common.h>
#include <mpsl_fem_protocol_api.h>

#include "fem.h"
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

int fem_tx_gain_set(uint32_t gain)
{
	int32_t err;
	mpsl_fem_gain_t fem_gain = { 0 };

	/* Fallback to FEM specific function. It can be used for checking the valid output power
	 * range.
	 */
	if (fem_api->tx_gain_validate) {
		err = fem_api->tx_gain_validate(gain);
		if (err) {
			return err;
		}
	}

	if (!fem_api->tx_default_gain_get) {
		/* Should not happen. */
		__ASSERT(false, "Missing FEM specific function for getting default Tx gain");
		return -EFAULT;
	}

	/* We need to pass valid gain db value for given front-end module to have possibility to set
	 * gain value directly in arbitrary value defined in your front-end module
	 * product specification. In this case the default Tx gain value will be used.
	 */
	fem_gain.gain_db = fem_api->tx_default_gain_get();
	fem_gain.private_setting = gain;

	err = mpsl_fem_pa_gain_set(&fem_gain);
	if (err) {
		printk("Failed to set front-end module gain (err %d)\n", err);
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

	err = mpsl_fem_pa_gain_set(&power_split.fem);
	if (err) {
		/* Should not happen */
		printk("Failed to set front-end module gain (err %d)\n", err);
		__ASSERT_NO_MSG(false);
	}

	return output_power;
}

int8_t fem_tx_output_power_check(int8_t power, uint16_t freq_mhz, bool tx_power_ceiling)
{
	mpsl_tx_power_split_t power_split = { 0 };

	return mpsl_fem_tx_power_split(power, &power_split, freq_mhz, tx_power_ceiling);
}

uint32_t fem_default_tx_gain_get(void)
{
	if (fem_api->tx_default_gain_get) {
		return fem_api->tx_default_gain_get();
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

#if defined(NRF52840_XXAA) || defined(NRF52833_XXAA)
void fem_errata_254(nrf_radio_mode_t mode)
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
void fem_errata_254(nrf_radio_mode_t mode)
{

}
#endif /* defined(NRF52840_XXAA) || defined(NRF52833_XXAA) */
