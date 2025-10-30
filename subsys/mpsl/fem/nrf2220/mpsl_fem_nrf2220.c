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
#include <gpiote_nrfx.h>
#include <mpsl_fem_utils.h>
#include <mpsl_fem_twi_drv.h>

#include <soc_secure.h>

#if IS_ENABLED(CONFIG_PINCTRL)
#include <pinctrl_soc.h>
#endif

#if IS_ENABLED(CONFIG_MPSL_FEM_NRF2220_TEMPERATURE_COMPENSATION)
#include <protocol/mpsl_fem_nrf2220_protocol_api.h>
#include <zephyr/drivers/i2c/i2c_nrfx_twim.h>
#endif

#if !defined(CONFIG_PINCTRL)
#error "The nRF2220 driver must be used with CONFIG_PINCTRL! Set CONFIG_PINCTRL=y"
#endif

#define RADIO_FEM_NODE DT_NODELABEL(nrf_radio_fem)

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), twi_if)
#define MPSL_FEM_TWI_IF      DT_PHANDLE(DT_NODELABEL(nrf_radio_fem), twi_if)
#define MPSL_FEM_TWI_ADDRESS DT_REG_ADDR(MPSL_FEM_TWI_IF)
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
	static mpsl_fem_twi_drv_t fem_twi_drv = MPSL_FEM_TWI_DRV_INITIALIZER(MPSL_FEM_TWI_IF);

	mpsl_fem_twi_drv_fem_twi_if_prepare(&fem_twi_drv, &cfg->twi_if, MPSL_FEM_TWI_ADDRESS);
	if (IS_ENABLED(CONFIG_MPSL_FEM_NRF2220_TEMPERATURE_COMPENSATION_WITH_MPSL_SCHEDULER)) {
		/* The nRF2220 temperature compensation with the MPSL scheduler requires
		 * asynchronous interface.
		 * See mpsl_fem_nrf2220_temperature_changed_update_request function.
		 */
		mpsl_fem_twi_drv_fem_twi_if_prepare_add_async(&cfg->twi_if);
	}
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

	nrfx_gpiote_t *cs_gpiote =
		&GPIOTE_NRFX_INST_BY_NODE(NRF_DT_GPIOTE_NODE(RADIO_FEM_NODE, cs_gpios));

	if (nrfx_gpiote_channel_alloc(cs_gpiote, &cs_gpiote_channel) != 0) {
		return -ENOMEM;
	}

	nrfx_gpiote_t *md_gpiote =
		&GPIOTE_NRFX_INST_BY_NODE(NRF_DT_GPIOTE_NODE(RADIO_FEM_NODE, md_gpios));

	if (nrfx_gpiote_channel_alloc(md_gpiote, &md_gpiote_channel) != 0) {
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
			.gpiote_ch_id  = cs_gpiote_channel,
#if defined(NRF54L_SERIES)
			.p_gpiote = cs_gpiote->p_reg,
#endif
		},
		.md_pin_config = {
			.gpio_pin      = {
				.p_port   = MPSL_FEM_GPIO_PORT_REG(md_gpios),
				.port_no  = MPSL_FEM_GPIO_PORT_NO(md_gpios),
				.port_pin = MPSL_FEM_GPIO_PIN_NO(md_gpios),
			},
			.enable        = true,
			.active_high   = MPSL_FEM_GPIO_POLARITY_GET(md_gpios),
			.gpiote_ch_id  = md_gpiote_channel,
#if defined(NRF54L_SERIES)
			.p_gpiote = md_gpiote->p_reg,
#endif
		}
	};

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), twi_if)
	fem_nrf2220_twi_configure(&cfg);
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

	return err;
}

static int mpsl_fem_init(void)
{
	mpsl_fem_device_config_254_apply_set(IS_ENABLED(CONFIG_MPSL_FEM_DEVICE_CONFIG_254));

	return fem_nrf2220_configure();
}

#if defined(CONFIG_I2C) && \
	DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), twi_if)
BUILD_ASSERT(CONFIG_MPSL_FEM_INIT_PRIORITY > CONFIG_I2C_INIT_PRIORITY,
	"The initialization of nRF2220 Front-End Module must happen after initialization of I2C");
#endif

SYS_INIT(mpsl_fem_init, POST_KERNEL, CONFIG_MPSL_FEM_INIT_PRIORITY);

#if defined(CONFIG_MPSL_FEM_NRF2220_TEMPERATURE_COMPENSATION)

#define NRF2220_TEMPERATURE_DEFAULT 25

static K_SEM_DEFINE(fem_temperature_new_value_sem, 0, 1);

#if defined(CONFIG_MPSL_FEM_NRF2220_TEMPERATURE_COMPENSATION_WITH_MPSL_SCHEDULER)
static int32_t fem_temperature_update_result;
static K_SEM_DEFINE(fem_temperature_updated_cb_sem, 0, 1);

static void fem_temperature_update_cb(int32_t res)
{
	fem_temperature_update_result = res;
	k_sem_give(&fem_temperature_updated_cb_sem);
}

static int32_t fem_temperature_changed_update_now(void)
{
	const struct device *i2c_bus_dev = DEVICE_DT_GET(DT_BUS(MPSL_FEM_TWI_IF));

	/* Let's have initially "taken" semaphore that will inform the operation
	 * on the nrf2220 is finished.
	 */
	k_sem_take(&fem_temperature_updated_cb_sem, K_NO_WAIT);

	(void)i2c_nrfx_twim_exclusive_access_acquire(i2c_bus_dev, K_FOREVER);

	/* Temporary raise i2c_bus_dev IRQ priority, as required by the
	 * mpsl_fem_nrf2220_temperature_changed_update_request function.
	 * The IRQs from i2c may not be delayed while performing operations
	 * in a timeslot granted by the MPSL scheduler.
	 */
	z_arm_irq_priority_set(DT_IRQN(DT_BUS(MPSL_FEM_TWI_IF)), 0, IRQ_ZERO_LATENCY);

	mpsl_fem_nrf2220_temperature_changed_update_request(fem_temperature_update_cb);

	/* Let's wait until the operation is finished */
	k_sem_take(&fem_temperature_updated_cb_sem, K_FOREVER);

	/* Restore original i2c_bus_dev IRQ priority. */
	z_arm_irq_priority_set(DT_IRQN(DT_BUS(MPSL_FEM_TWI_IF)),
		DT_IRQ(DT_BUS(MPSL_FEM_TWI_IF), priority), 0);

	i2c_nrfx_twim_exclusive_access_release(i2c_bus_dev);

	return fem_temperature_update_result;
}
#endif /* CONFIG_MPSL_FEM_NRF2220_TEMPERATURE_COMPENSATION_WITH_MPSL_SCHEDULER */

#if defined(CONFIG_MPSL_FEM_NRF2220_TEMPERATURE_SOURCE_SOC)

#include <zephyr/drivers/sensor.h>
#define FEM_TEMPERATURE_NEW_VALUE_SEM_TIMEOUT \
	(K_MSEC(CONFIG_MPSL_FEM_NRF2220_TEMPERATURE_POLL_PERIOD))

static int8_t fem_temperature_sensor_value_get(void)
{
	const struct device *dev = DEVICE_DT_GET_ONE(nordic_nrf_temp);
	struct sensor_value v = { .val1 = NRF2220_TEMPERATURE_DEFAULT };
	int ret = 0;

	if (!device_is_ready(dev)) {
		__ASSERT(false, "Temperature sensor not ready");
		ret = -EIO;
	}

	if (ret == 0) {
		ret = sensor_sample_fetch(dev);
		__ASSERT(ret == 0, "Can't fetch temperature sensor sample");
	}

	if (ret == 0) {
		ret = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &v);
		__ASSERT(ret == 0, "Can't get temperature of the die");
		(void)ret;
	}

	return v.val1;
}
#endif /* CONFIG_MPSL_FEM_NRF2220_TEMPERATURE_SOURCE_SOC */

#if defined(CONFIG_MPSL_FEM_NRF2220_TEMPERATURE_SOURCE_CUSTOM)

#define FEM_TEMPERATURE_NEW_VALUE_SEM_TIMEOUT  K_FOREVER

static int8_t fem_temperature_value = NRF2220_TEMPERATURE_DEFAULT;

void fem_temperature_change(int8_t temperature)
{
	fem_temperature_value = temperature;
	k_sem_give(&fem_temperature_new_value_sem);
}

static int8_t fem_temperature_sensor_value_get(void)
{
	return fem_temperature_value;
}
#endif /* CONFIG_MPSL_FEM_NRF2220_TEMPERATURE_SOURCE_CUSTOM */

static void fem_temperature_compensation_thread(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	while (true) {
		(void)k_sem_take(&fem_temperature_new_value_sem,
			FEM_TEMPERATURE_NEW_VALUE_SEM_TIMEOUT);

		int8_t new_temperature = fem_temperature_sensor_value_get();

		if (mpsl_fem_nrf2220_temperature_changed(new_temperature)) {
#if defined(CONFIG_MPSL_FEM_NRF2220_TEMPERATURE_COMPENSATION_WITH_MPSL_SCHEDULER)
			int32_t res = fem_temperature_changed_update_now();

			__ASSERT(res == 0, "FEM update on temperature change failed");
			(void)res;
#endif /* CONFIG_MPSL_FEM_NRF2220_TEMPERATURE_COMPENSATION_WITH_MPSL_SCHEDULER */
		}
	}
}

#define FEM_TEMPERATURE_COMPENSATION_THREAD_STACK_SIZE 512

static K_THREAD_STACK_DEFINE(fem_temperature_compensation_thread_stack,
			FEM_TEMPERATURE_COMPENSATION_THREAD_STACK_SIZE);
static struct k_thread fem_temperature_compensation_thread_data;

static int fem_nrf2220_temperature_compensation_init(void)
{
	(void)k_thread_create(&fem_temperature_compensation_thread_data,
		fem_temperature_compensation_thread_stack,
		K_THREAD_STACK_SIZEOF(fem_temperature_compensation_thread_stack),
		fem_temperature_compensation_thread,
		NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

	return 0;
}

SYS_INIT(fem_nrf2220_temperature_compensation_init, APPLICATION,
	CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_MPSL_FEM_NRF2220_TEMPERATURE_COMPENSATION */

#endif /* defined(CONFIG_MPSL_FEM_NRF2220) */
