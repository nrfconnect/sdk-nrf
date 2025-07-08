/* Copyright (c) 2024, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <hal/nrf_gpio.h>
#include <nrf_errno.h>
#include <mpsl_fem_power_model.h>
#include <protocol/mpsl_fem_protocol_api.h>

#include "fem_psemi_common.h"
#include "fem_psemi_config.h"
#include "mpsl_fem_psemi_pins.h"
#include "power_model.h"

/**< Front End Module controller configuration. */
fem_psemi_interface_config_t m_fem_interface_config;

/**< Is the bypass mode currently active. */
static fem_psemi_state_t m_fem_state = FEM_PSEMI_STATE_AUTO;

void fem_psemi_caps_get(mpsl_fem_caps_t *p_caps)
{
	*p_caps = (mpsl_fem_caps_t){.flags = MPSL_FEM_CAPS_FLAG_PA_SETUP_REQUIRED |
					     MPSL_FEM_CAPS_FLAG_LNA_SETUP_REQUIRED};
}

int8_t fem_psemi_tx_power_split(const mpsl_tx_power_t power,
				mpsl_tx_power_split_t *const p_tx_power_split, mpsl_phy_t phy,
				uint16_t freq_mhz, bool tx_power_ceiling)
{
	mpsl_fem_power_model_output_t output;

#if defined(CONFIG_POWER_MAP_MODEL)
	power_model_output_fetch(power, freq_mhz, &output, tx_power_ceiling);
#else
	output.soc_pwr = power;
	output.fem_pa_power_control = 0;
	output.achieved_pwr = power;
#endif

	p_tx_power_split->radio_tx_power = output.soc_pwr;
	p_tx_power_split->fem_pa_power_control = output.fem_pa_power_control;

	return output.achieved_pwr;
}

static uint8_t fem_gain_to_att_pin_mask(uint8_t fem_gain)
{
	fem_gain = (fem_gain >= FEM_MIN_GAIN) ? fem_gain : FEM_MIN_GAIN;
	fem_gain = (fem_gain <= FEM_MAX_GAIN) ? fem_gain : FEM_MAX_GAIN;

	/* Mask can be calculated according to the below table, where binary representation of
	 * attenuation value can be directly used to determine ATT[0:3] pins state.
	 *
	 * FEM gain = constant GAIN - attenuation -> ATT mask
	 *    5 dBm =        20 dBm -      15 dBm ->  0b1111
	 *    6 dBm =        20 dBm -      14 dBm ->  0b1110
	 *    7 dBm =        20 dBm -      13 dBm ->  0b1101
	 *    8 dBm =        20 dBm -      12 dBm ->  0b1100
	 *    9 dBm =        20 dBm -      11 dBm ->  0b1011
	 *   10 dBm =        20 dBm -      10 dBm ->  0b1010
	 *   11 dBm =        20 dBm -       9 dBm ->  0b1001
	 *   12 dBm =        20 dBm -       8 dBm ->  0b1000
	 *   13 dBm =        20 dBm -       7 dBm ->  0b0111
	 *   14 dBm =        20 dBm -       6 dBm ->  0b0110
	 *   15 dBm =        20 dBm -       5 dBm ->  0b0101
	 *   16 dBm =        20 dBm -       4 dBm ->  0b0100
	 *   17 dBm =        20 dBm -       3 dBm ->  0b0011
	 *   18 dBm =        20 dBm -       2 dBm ->  0b0010
	 *   19 dBm =        20 dBm -       1 dBm ->  0b0001
	 *   20 dBm =        20 dBm -       0 dBm ->  0b0000
	 */

	return (uint8_t)(FEM_MAX_GAIN - fem_gain);
}

int32_t fem_psemi_pa_power_control_set(mpsl_fem_pa_power_control_t pa_power_control)
{
	uint8_t att_mask = fem_gain_to_att_pin_mask(pa_power_control);

	for (uint8_t pin_id = 0U; pin_id < FEM_ATT_PINS_NUMBER; pin_id++) {
		mpsl_fem_gpio_pin_config_t *pin_cfg = &m_fem_interface_config.att_pins[pin_id];
		bool level = att_mask & (1U << pin_id);

		if (pin_cfg->enable) {
			mpsl_fem_gpio_pin_config_pin_output_write(pin_cfg,
								  !(level ^ pin_cfg->active_high));
		}
	}

	return 0;
}

/** Set default PA gain. */
void fem_psemi_pa_gain_default(fem_psemi_interface_config_t const *const p_config)
{
	for (uint8_t i = 0; i < FEM_ATT_PINS_NUMBER; i++) {
		const mpsl_fem_gpio_pin_config_t *pin_cfg = &p_config->att_pins[i];

		if (pin_cfg->enable) {
			mpsl_fem_gpio_pin_config_pin_output_write(pin_cfg, !pin_cfg->active_high);
		}
	}
}

/** Configure GPIO module. */
void fem_psemi_gain_gpio_configure(fem_psemi_interface_config_t const *const p_config)
{
	for (uint8_t i = 0; i < FEM_ATT_PINS_NUMBER; i++) {
		mpsl_fem_gpio_pin_config_configure(&p_config->att_pins[i]);
	}
	fem_psemi_pa_gain_default(p_config);
}

void gpiote_output_write(const mpsl_fem_gpiote_pin_config_t *p_pin_cfg, bool level)
{
	NRF_GPIOTE_Type *p_gpiote = mpsl_fem_gpiote_pin_config_gpiote_instance_get(p_pin_cfg);
	uint32_t task_event_idx = p_pin_cfg->gpiote_ch_id;
	uint32_t gpio_pin =
		NRF_GPIO_PIN_MAP(p_pin_cfg->gpio_pin.port_no, p_pin_cfg->gpio_pin.port_pin);

	nrf_gpiote_task_disable(p_gpiote, task_event_idx);

	nrf_gpio_pin_write(gpio_pin, level);
	nrf_gpio_cfg_output(gpio_pin);
}

void gpiote_configure(const mpsl_fem_gpiote_pin_config_t *p_pin_cfg)
{
	NRF_GPIOTE_Type *p_gpiote = mpsl_fem_gpiote_pin_config_gpiote_instance_get(p_pin_cfg);
	uint32_t task_event_idx = p_pin_cfg->gpiote_ch_id;
	uint32_t gpio_pin =
		NRF_GPIO_PIN_MAP(p_pin_cfg->gpio_pin.port_no, p_pin_cfg->gpio_pin.port_pin);

	nrf_gpiote_task_configure(p_gpiote, task_event_idx, gpio_pin,
				  (nrf_gpiote_polarity_t)GPIOTE_CONFIG_POLARITY_None,
				  (nrf_gpiote_outinit_t) !(p_pin_cfg->active_high));

	nrf_gpiote_task_enable(p_gpiote, task_event_idx);
}

static void pa_gain_enable(void)
{
	for (uint8_t i = 0; i < FEM_ATT_PINS_NUMBER; i++) {
		m_fem_interface_config.att_pins[i].enable = true;
	}

	fem_psemi_pa_gain_default(&m_fem_interface_config);
}

static void pa_gain_disable(void)
{
	fem_psemi_pa_gain_default(&m_fem_interface_config);

	for (uint8_t i = 0; i < FEM_ATT_PINS_NUMBER; i++) {
		m_fem_interface_config.att_pins[i].enable = false;
	}
}

int32_t fem_psemi_state_set(fem_psemi_state_t state)
{
	mpsl_fem_gpiote_pin_config_t *p_pa_pin = &m_fem_interface_config.pa_pin_config;
	mpsl_fem_gpiote_pin_config_t *p_lna_pin = &m_fem_interface_config.lna_pin_config;

	if (state == m_fem_state) {
		return 0;
	}

	m_fem_state = state;
	pa_gain_disable();

	p_pa_pin->enable = state == FEM_PSEMI_STATE_AUTO;
	p_lna_pin->enable = state == FEM_PSEMI_STATE_AUTO;

	switch (state) {
	case FEM_PSEMI_STATE_DISABLED:
		gpiote_output_write(p_pa_pin, !p_pa_pin->active_high);
		gpiote_output_write(p_lna_pin, !p_lna_pin->active_high);
		break;
	case FEM_PSEMI_STATE_LNA_ACTIVE:
		gpiote_output_write(p_pa_pin, !p_pa_pin->active_high);
		gpiote_output_write(p_lna_pin, p_lna_pin->active_high);
		break;
	case FEM_PSEMI_STATE_PA_ACTIVE:
		gpiote_output_write(p_pa_pin, p_pa_pin->active_high);
		gpiote_output_write(p_lna_pin, !p_lna_pin->active_high);
		pa_gain_enable();
		break;
	case FEM_PSEMI_STATE_BYPASS:
		gpiote_output_write(p_pa_pin, p_pa_pin->active_high);
		gpiote_output_write(p_lna_pin, p_lna_pin->active_high);
		break;
	case FEM_PSEMI_STATE_AUTO:
		gpiote_configure(p_pa_pin);
		gpiote_configure(p_lna_pin);
		pa_gain_enable();
		break;
	default:
		return -NRF_EINVAL;
	}

	return 0;
}

fem_psemi_state_t fem_psemi_state_get(void)
{
	return m_fem_state;
}
