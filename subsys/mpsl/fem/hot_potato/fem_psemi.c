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

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include <soc_nrf_common.h>
#include <nrfx_gpiote.h>
#include <mpsl_fem_utils.h>

#include "power_model.h"
#include "./psemi/include/mpsl_fem_psemi_interface.h"

static int fem_psemi_configure(void)
{
	int err;

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ctx_gpios)
	uint8_t ctx_gpiote_channel = MPSL_FEM_GPIOTE_INVALID_CHANNEL;
	const nrfx_gpiote_t ctx_gpiote =
		NRFX_GPIOTE_INSTANCE(NRF_DT_GPIOTE_INST(DT_NODELABEL(nrf_radio_fem), ctx_gpios));

	if (nrfx_gpiote_channel_alloc(&ctx_gpiote, &ctx_gpiote_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), crx_gpios)
	uint8_t crx_gpiote_channel = MPSL_FEM_GPIOTE_INVALID_CHANNEL;
	const nrfx_gpiote_t crx_gpiote =
		NRFX_GPIOTE_INSTANCE(NRF_DT_GPIOTE_INST(DT_NODELABEL(nrf_radio_fem), crx_gpios));

	if (nrfx_gpiote_channel_alloc(&crx_gpiote, &crx_gpiote_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}
#endif

	mpsl_fem_psemi_interface_config_t cfg = {
		.fem_config =
			{
				.pa_time_gap_us =
					DT_PROP(DT_NODELABEL(nrf_radio_fem), ctx_settle_time_us),
				.lna_time_gap_us =
					DT_PROP(DT_NODELABEL(nrf_radio_fem), crx_settle_time_us),
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), rx_gain_db)
				.lna_gain_db = DT_PROP(DT_NODELABEL(nrf_radio_fem), rx_gain_db),
#endif
			},
		.pa_pin_config =
			{
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), ctx_gpios)
				.gpio_pin =
					{
						.p_port = MPSL_FEM_GPIO_PORT_REG(ctx_gpios),
						.port_no = MPSL_FEM_GPIO_PORT_NO(ctx_gpios),
						.port_pin = MPSL_FEM_GPIO_PIN_NO(ctx_gpios),
					},
				.enable = true,
				.active_high = MPSL_FEM_GPIO_POLARITY_GET(ctx_gpios),
				.gpiote_ch_id = ctx_gpiote_channel,
#if defined(NRF54L_SERIES)
				.p_gpiote = ctx_gpiote.p_reg,
#endif
#else
				MPSL_FEM_DISABLED_GPIOTE_PIN_CONFIG_INIT
#endif
			},
		.lna_pin_config =
			{
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), crx_gpios)
				.gpio_pin =
					{
						.p_port = MPSL_FEM_GPIO_PORT_REG(crx_gpios),
						.port_no = MPSL_FEM_GPIO_PORT_NO(crx_gpios),
						.port_pin = MPSL_FEM_GPIO_PIN_NO(crx_gpios),
					},
				.enable = true,
				.active_high = MPSL_FEM_GPIO_POLARITY_GET(crx_gpios),
				.gpiote_ch_id = crx_gpiote_channel,
#if defined(NRF54L_SERIES)
				.p_gpiote = crx_gpiote.p_reg,
#endif
#else
				MPSL_FEM_DISABLED_GPIOTE_PIN_CONFIG_INIT
#endif
			},
		.att_pins[0] =
			{
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), att0_gpios)
				.gpio_pin =
					{
						.p_port = MPSL_FEM_GPIO_PORT_REG(att0_gpios),
						.port_no = MPSL_FEM_GPIO_PORT_NO(att0_gpios),
						.port_pin = MPSL_FEM_GPIO_PIN_NO(att0_gpios),
					},
				.enable = true,
				.active_high = MPSL_FEM_GPIO_POLARITY_GET(att0_gpios),
#else
				MPSL_FEM_DISABLED_GPIO_CONFIG_INIT
#endif
			},
		.att_pins[1] =
			{
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), att1_gpios)
				.gpio_pin =
					{
						.p_port = MPSL_FEM_GPIO_PORT_REG(att1_gpios),
						.port_no = MPSL_FEM_GPIO_PORT_NO(att1_gpios),
						.port_pin = MPSL_FEM_GPIO_PIN_NO(att1_gpios),
					},
				.enable = true,
				.active_high = MPSL_FEM_GPIO_POLARITY_GET(att1_gpios),
#else
				MPSL_FEM_DISABLED_GPIO_CONFIG_INIT
#endif
			},
		.att_pins[2] =
			{
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), att2_gpios)
				.gpio_pin =
					{
						.p_port = MPSL_FEM_GPIO_PORT_REG(att2_gpios),
						.port_no = MPSL_FEM_GPIO_PORT_NO(att2_gpios),
						.port_pin = MPSL_FEM_GPIO_PIN_NO(att2_gpios),
					},
				.enable = true,
				.active_high = MPSL_FEM_GPIO_POLARITY_GET(att2_gpios),
#else
				MPSL_FEM_DISABLED_GPIO_CONFIG_INIT
#endif
			},
		.att_pins[3] =
			{
#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), att3_gpios)
				.gpio_pin =
					{
						.p_port = MPSL_FEM_GPIO_PORT_REG(att3_gpios),
						.port_no = MPSL_FEM_GPIO_PORT_NO(att3_gpios),
						.port_pin = MPSL_FEM_GPIO_PIN_NO(att3_gpios),
					},
				.enable = true,
				.active_high = MPSL_FEM_GPIO_POLARITY_GET(att3_gpios),
#else
				MPSL_FEM_DISABLED_GPIO_CONFIG_INIT
#endif
			},
		.bypass_ble_enabled = IS_ENABLED(CONFIG_MPSL_FEM_HOT_POTATO_BYPASS_BLE),
		.pa_ldo_switch_enabled = IS_ENABLED(CONFIG_MPSL_FEM_HOT_POTATO_PA_LDO),
	};

	IF_ENABLED(CONFIG_HAS_HW_NRF_DPPIC,
		   (err = mpsl_fem_utils_ppi_channel_alloc(cfg.dppi_channels,
							   ARRAY_SIZE(cfg.dppi_channels));));
	if (err) {
		return err;
	}

	IF_ENABLED(CONFIG_HAS_HW_NRF_DPPIC,
		   (err = mpsl_fem_utils_egu_instance_alloc(&cfg.egu_instance_no);));
	if (err) {
		return err;
	}

	IF_ENABLED(CONFIG_HAS_HW_NRF_DPPIC,
		   (err = mpsl_fem_utils_egu_channel_alloc(cfg.egu_channels,
							   ARRAY_SIZE(cfg.egu_channels),
							   cfg.egu_instance_no);))
	if (err) {
		return err;
	}

	return mpsl_fem_psemi_interface_config_set(&cfg);
}

static int mpsl_fem_init(void)
{
	mpsl_fem_device_config_254_apply_set(IS_ENABLED(CONFIG_MPSL_FEM_DEVICE_CONFIG_254));

	if (IS_ENABLED(CONFIG_POWER_MAP_MODEL)) {
		power_model_init();
	}

	return fem_psemi_configure();
}

SYS_INIT(mpsl_fem_init, POST_KERNEL, CONFIG_MPSL_FEM_INIT_PRIORITY);
