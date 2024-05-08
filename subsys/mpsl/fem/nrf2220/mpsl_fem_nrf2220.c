/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if defined(CONFIG_MPSL_FEM_NRF2220)

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include <soc_nrf_common.h>

#include <mpsl_fem_config_nrf2220.h>
#include <nrfx_gpiote.h>
#include <nrfx_twim.h>
#include <mpsl_fem_utils.h>

#if IS_ENABLED(CONFIG_HAS_HW_NRF_PPI)
#include <nrfx_ppi.h>
#elif IS_ENABLED(CONFIG_HAS_HW_NRF_DPPIC)
#include <nrfx_dppi.h>
#endif

#include <soc_secure.h>

#if IS_ENABLED(CONFIG_PINCTRL)
#include <pinctrl_soc.h>
#endif

#if !defined(CONFIG_PINCTRL)
#error "The nRF2220 driver must be used with CONFIG_PINCTRL! Set CONFIG_PINCTRL=y"
#endif

#if defined(CONFIG_EARLY_CONSOLE)
#error "The nRF2220 driver cannot be used with CONFIG_EARLY_CONSOLE! \
	Set CONFIG_BOOT_BANNER=n, CONFIG_EARLY_CONSOLE=n"
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), twi_if)
#define MPSL_FEM_TWI_IF     DT_PHANDLE(DT_NODELABEL(nrf_radio_fem), twi_if)
#define MPSL_FEM_TWI_BUS    DT_BUS(MPSL_FEM_TWI_IF)
#define MPSL_FEM_TWI_REG    ((NRF_TWIM_Type *) DT_REG_ADDR(MPSL_FEM_TWI_BUS))
#define MPSL_FEM_TWI_ADDR   ((uint8_t) DT_REG_ADDR(MPSL_FEM_TWI_IF))
#define MPSL_FEM_TWI_FREQ   DT_PROP(MPSL_FEM_TWI_BUS, clock_frequency)
static nrfx_twim_t m_twim = NRFX_TWIM_INSTANCE(0);
#endif

#define FEM_OUTPUT_POWER_DBM     DT_PROP(DT_NODELABEL(nrf_radio_fem), output_power_dbm)

#define PIN_NUM(node_id, prop, idx)  NRF_GET_PIN(DT_PROP_BY_IDX(node_id, prop, idx)),
#define PIN_FUNC(node_id, prop, idx) NRF_GET_FUN(DT_PROP_BY_IDX(node_id, prop, idx)),

#define NRF2220_OUTPUT_POWER_DBM_MIN 7
#define NRF2220_OUTPUT_POWER_DBM_MAX 14

BUILD_ASSERT(
	(FEM_OUTPUT_POWER_DBM >= NRF2220_OUTPUT_POWER_DBM_MIN) &&
	(FEM_OUTPUT_POWER_DBM <= NRF2220_OUTPUT_POWER_DBM_MAX),
	"Value of output-power-dbm property out of range.");

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), twi_if)
#if DT_NODE_HAS_PROP(MPSL_FEM_TWI_IF, init_regs)
static void fem_nrf2220_twi_init_regs_configure(mpsl_fem_nrf2220_interface_config_t *cfg)
{
	const int32_t fem_twi_init_regs[] = DT_PROP(MPSL_FEM_TWI_IF, init_regs);

	for (uint32_t i = 0; i < cfg->twi_regs_init_map.nb_regs; i++) {
		cfg->twi_regs_init_map.p_regs[i].addr = (uint8_t)fem_twi_init_regs[2 * i];
		cfg->twi_regs_init_map.p_regs[i].val = (uint8_t)fem_twi_init_regs[2 * i + 1];
	}
}
#endif /* DT_NODE_HAS_PROP(MPSL_FEM_TWI_IF, init_regs) */

static void fem_nrf2220_twi_configure(mpsl_fem_nrf2220_interface_config_t *cfg)
{
	cfg->twi_config = (mpsl_fem_twi_config_t) {
		.p_twim = MPSL_FEM_TWI_REG,
		.freq_hz = MPSL_FEM_TWI_FREQ,
		.address = MPSL_FEM_TWI_ADDR,
	};

	static const uint8_t fem_twi_pin_nums[] = {
		DT_FOREACH_CHILD_VARGS(
			DT_PINCTRL_BY_NAME(MPSL_FEM_TWI_BUS, default, 0),
			DT_FOREACH_PROP_ELEM, psels, PIN_NUM
		)
	};

	static const uint8_t fem_twi_pin_funcs[] = {
		DT_FOREACH_CHILD_VARGS(
			DT_PINCTRL_BY_NAME(MPSL_FEM_TWI_BUS, default, 0),
			DT_FOREACH_PROP_ELEM, psels, PIN_FUNC
		)
	};

	for (size_t i = 0U; i < ARRAY_SIZE(fem_twi_pin_nums); i++) {
		switch (fem_twi_pin_funcs[i]) {
		case NRF_FUN_TWIM_SCL:
			mpsl_fem_extended_pin_to_mpsl_fem_pin(fem_twi_pin_nums[i],
							      &cfg->twi_config.scl);
			break;
		case NRF_FUN_TWIM_SDA:
			mpsl_fem_extended_pin_to_mpsl_fem_pin(fem_twi_pin_nums[i],
							      &cfg->twi_config.sda);
			break;
		default:
			break;
		}
	}
}

#else /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), twi_if) */

static void fem_nrf2220_twi_configure_disabled(mpsl_fem_nrf2220_interface_config_t *cfg)
{
	cfg->twi_config.p_twim = NULL;
}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), twi_if) */

/* Required for testing only. */
static uint8_t cs_gpiote_channel;
static uint8_t md_gpiote_channel;

uint8_t nrf2220_cs_gpiote_channel_get(void)
{
	return cs_gpiote_channel;
}

uint8_t nrf2220_md_gpiote_channel_get(void)
{
	return md_gpiote_channel;
}

static int fem_nrf2220_configure(void)
{
	int err;

	const nrfx_gpiote_t cs_gpiote = NRFX_GPIOTE_INSTANCE(
		NRF_DT_GPIOTE_INST(DT_NODELABEL(nrf_radio_fem), cs_gpios));
	if (nrfx_gpiote_channel_alloc(&cs_gpiote, &cs_gpiote_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}

	const nrfx_gpiote_t md_gpiote = NRFX_GPIOTE_INSTANCE(
		NRF_DT_GPIOTE_INST(DT_NODELABEL(nrf_radio_fem), md_gpios));
	if (nrfx_gpiote_channel_alloc(&md_gpiote, &md_gpiote_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}

	mpsl_fem_nrf2220_interface_config_t cfg = {
		.fem_config = {
			.output_power_dbm = FEM_OUTPUT_POWER_DBM,
			.bypass_gain_db = DT_PROP(DT_NODELABEL(nrf_radio_fem), bypass_gain_db),
		},
		.cs_pin_config = {
			.gpio_pin      = {
				.p_port   = MPSL_FEM_GPIO_PORT_REG(cs_gpios),
				.port_no  = MPSL_FEM_GPIO_PORT_NO(cs_gpios),
				.port_pin = MPSL_FEM_GPIO_PIN_NO(cs_gpios),
			},
			.enable        = true,
			.active_high   = MPSL_FEM_GPIO_POLARITY_GET(cs_gpios),
			.gpiote_ch_id  = cs_gpiote_channel
		},
		.md_pin_config = {
			.gpio_pin      = {
				.p_port   = MPSL_FEM_GPIO_PORT_REG(md_gpios),
				.port_no  = MPSL_FEM_GPIO_PORT_NO(md_gpios),
				.port_pin = MPSL_FEM_GPIO_PIN_NO(md_gpios),
			},
			.enable        = true,
			.active_high   = MPSL_FEM_GPIO_POLARITY_GET(md_gpios),
			.gpiote_ch_id  = md_gpiote_channel
		}
	};

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), twi_if)
	fem_nrf2220_twi_configure(&cfg);
#else
	fem_nrf2220_twi_configure_disabled(&cfg);
#endif

#if DT_NODE_HAS_PROP(MPSL_FEM_TWI_IF, init_regs)
	const int32_t fem_twi_init_regs[] = DT_PROP(MPSL_FEM_TWI_IF, init_regs);
	const uint8_t nb_regs = ARRAY_SIZE(fem_twi_init_regs) / 2;

	BUILD_ASSERT(ARRAY_SIZE(fem_twi_init_regs) % 2 == 0,
		"The number of specified nRF2220 TWI initialization registers to write differs from"
		" the number of values specified for those registers.");

	mpsl_fem_twi_reg_val_t init_regs[nb_regs];

	cfg.twi_regs_init_map.p_regs = init_regs;
	cfg.twi_regs_init_map.nb_regs = nb_regs;

	fem_nrf2220_twi_init_regs_configure(&cfg);
#endif /* DT_NODE_HAS_PROP(MPSL_FEM_TWI_IF, init_regs) */

	IF_ENABLED(CONFIG_HAS_HW_NRF_PPI,
		   (err = mpsl_fem_utils_ppi_channel_alloc(cfg.ppi_channels,
						ARRAY_SIZE(cfg.ppi_channels));));
	IF_ENABLED(CONFIG_HAS_HW_NRF_DPPIC,
		   (err = mpsl_fem_utils_ppi_channel_alloc(cfg.dppi_channels,
						ARRAY_SIZE(cfg.dppi_channels));));
	if (err) {
		return err;
	}

	err = mpsl_fem_nrf2220_interface_config_set(&cfg);

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), twi_if)
	nrfx_twim_uninit(&m_twim);
#endif

	return err;
}

static int mpsl_fem_init(void)
{
	mpsl_fem_device_config_254_apply_set(IS_ENABLED(CONFIG_MPSL_FEM_DEVICE_CONFIG_254));

	return fem_nrf2220_configure();
}

SYS_INIT(mpsl_fem_init, POST_KERNEL, 49);

#endif /* defined(CONFIG_MPSL_FEM_NRF2220) */
