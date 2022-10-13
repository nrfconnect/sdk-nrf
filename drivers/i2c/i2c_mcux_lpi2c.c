/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2019, NXP
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_lpi2c

#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <fsl_lpi2c.h>

#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif /* CONFIG_PINCTRL */

#ifdef CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY
#include "i2c_bitbang.h"
#include <zephyr/drivers/gpio.h>
#endif /* CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mcux_lpi2c);


#include "i2c-priv.h"
/* Wait for the duration of 12 bits to detect a NAK after a bus
 * address scan.  (10 appears sufficient, 20% safety factor.)
 */
#define SCAN_DELAY_US(baudrate) (12 * USEC_PER_SEC / baudrate)

struct mcux_lpi2c_config {
	LPI2C_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	uint32_t bitrate;
	uint32_t bus_idle_timeout_ns;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pincfg;
#endif /* CONFIG_PINCTRL */
#ifdef CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY
	struct gpio_dt_spec scl;
	struct gpio_dt_spec sda;
#endif /* CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY */
};

struct mcux_lpi2c_data {
	lpi2c_master_handle_t handle;
	struct k_sem lock;
	struct k_sem device_sync_sem;
	status_t callback_status;
};

static int mcux_lpi2c_configure(const struct device *dev,
				uint32_t dev_config_raw)
{
	const struct mcux_lpi2c_config *config = dev->config;
	struct mcux_lpi2c_data *data = dev->data;
	LPI2C_Type *base = config->base;
	uint32_t clock_freq;
	uint32_t baudrate;
	int ret;

	if (!(I2C_MODE_CONTROLLER & dev_config_raw)) {
		return -EINVAL;
	}

	if (I2C_ADDR_10_BITS & dev_config_raw) {
		return -EINVAL;
	}

	switch (I2C_SPEED_GET(dev_config_raw)) {
	case I2C_SPEED_STANDARD:
		baudrate = KHZ(100);
		break;
	case I2C_SPEED_FAST:
		baudrate = KHZ(400);
		break;
	case I2C_SPEED_FAST_PLUS:
		baudrate = MHZ(1);
		break;
	default:
		return -EINVAL;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	ret = k_sem_take(&data->lock, K_FOREVER);
	if (ret) {
		return ret;
	}

	LPI2C_MasterSetBaudRate(base, clock_freq, baudrate);
	k_sem_give(&data->lock);

	return 0;
}

static void mcux_lpi2c_master_transfer_callback(LPI2C_Type *base,
						lpi2c_master_handle_t *handle,
						status_t status, void *userData)
{
	struct mcux_lpi2c_data *data = userData;

	ARG_UNUSED(handle);
	ARG_UNUSED(base);

	data->callback_status = status;
	k_sem_give(&data->device_sync_sem);
}

static uint32_t mcux_lpi2c_convert_flags(int msg_flags)
{
	uint32_t flags = 0U;

	if (!(msg_flags & I2C_MSG_STOP)) {
		flags |= kLPI2C_TransferNoStopFlag;
	}

	if (msg_flags & I2C_MSG_RESTART) {
		flags |= kLPI2C_TransferRepeatedStartFlag;
	}

	return flags;
}

static int mcux_lpi2c_transfer(const struct device *dev, struct i2c_msg *msgs,
			       uint8_t num_msgs, uint16_t addr)
{
	const struct mcux_lpi2c_config *config = dev->config;
	struct mcux_lpi2c_data *data = dev->data;
	LPI2C_Type *base = config->base;
	lpi2c_master_transfer_t transfer;
	status_t status;
	int ret = 0;

	ret = k_sem_take(&data->lock, K_FOREVER);
	if (ret) {
		return ret;
	}

	/* Iterate over all the messages */
	for (int i = 0; i < num_msgs; i++) {
		if (I2C_MSG_ADDR_10_BITS & msgs->flags) {
			ret = -ENOTSUP;
			break;
		}

		/* Initialize the transfer descriptor */
		transfer.flags = mcux_lpi2c_convert_flags(msgs->flags);

		/* Prevent the controller to send a start condition between
		 * messages, except if explicitly requested.
		 */
		if (i != 0 && !(msgs->flags & I2C_MSG_RESTART)) {
			transfer.flags |= kLPI2C_TransferNoStartFlag;
		}

		transfer.slaveAddress = addr;
		transfer.direction = (msgs->flags & I2C_MSG_READ)
			? kLPI2C_Read : kLPI2C_Write;
		transfer.subaddress = 0;
		transfer.subaddressSize = 0;
		transfer.data = msgs->buf;
		transfer.dataSize = msgs->len;

		/* Start the transfer */
		status = LPI2C_MasterTransferNonBlocking(base,
				&data->handle, &transfer);

		/* Return an error if the transfer didn't start successfully
		 * e.g., if the bus was busy
		 */
		if (status != kStatus_Success) {
			LPI2C_MasterTransferAbort(base, &data->handle);
			ret = -EIO;
			break;
		}

		/* Wait for the transfer to complete */
		k_sem_take(&data->device_sync_sem, K_FOREVER);

		/* Return an error if the transfer didn't complete
		 * successfully. e.g., nak, timeout, lost arbitration
		 */
		if (data->callback_status != kStatus_Success) {
			LPI2C_MasterTransferAbort(base, &data->handle);
			ret = -EIO;
			break;
		}
		if (msgs->len == 0) {
			k_busy_wait(SCAN_DELAY_US(config->bitrate));
			if (0 != (base->MSR & LPI2C_MSR_NDF_MASK)) {
				LPI2C_MasterTransferAbort(base, &data->handle);
				ret = -EIO;
				break;
			}
		}
		/* Move to the next message */
		msgs++;
	}

	k_sem_give(&data->lock);

	return ret;
}

#if CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY
static void mcux_lpi2c_bitbang_set_scl(void *io_context, int state)
{
	const struct mcux_lpi2c_config *config = io_context;

	gpio_pin_set_dt(&config->scl, state);
}

static void mcux_lpi2c_bitbang_set_sda(void *io_context, int state)
{
	const struct mcux_lpi2c_config *config = io_context;

	gpio_pin_set_dt(&config->sda, state);
}

static int mcux_lpi2c_bitbang_get_sda(void *io_context)
{
	const struct mcux_lpi2c_config *config = io_context;

	return gpio_pin_get_dt(&config->sda) == 0 ? 0 : 1;
}

static int mcux_lpi2c_recover_bus(const struct device *dev)
{
	const struct mcux_lpi2c_config *config = dev->config;
	struct mcux_lpi2c_data *data = dev->data;
	struct i2c_bitbang bitbang_ctx;
	struct i2c_bitbang_io bitbang_io = {
		.set_scl = mcux_lpi2c_bitbang_set_scl,
		.set_sda = mcux_lpi2c_bitbang_set_sda,
		.get_sda = mcux_lpi2c_bitbang_get_sda,
	};
	uint32_t bitrate_cfg;
	int error = 0;

	if (!device_is_ready(config->scl.port)) {
		LOG_ERR("SCL GPIO device not ready");
		return -EIO;
	}

	if (!device_is_ready(config->sda.port)) {
		LOG_ERR("SDA GPIO device not ready");
		return -EIO;
	}

	k_sem_take(&data->lock, K_FOREVER);

	error = gpio_pin_configure_dt(&config->scl, GPIO_OUTPUT_HIGH);
	if (error != 0) {
		LOG_ERR("failed to configure SCL GPIO (err %d)", error);
		goto restore;
	}

	error = gpio_pin_configure_dt(&config->sda, GPIO_OUTPUT_HIGH);
	if (error != 0) {
		LOG_ERR("failed to configure SDA GPIO (err %d)", error);
		goto restore;
	}

	i2c_bitbang_init(&bitbang_ctx, &bitbang_io, (void *)config);

	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate) | I2C_MODE_CONTROLLER;
	error = i2c_bitbang_configure(&bitbang_ctx, bitrate_cfg);
	if (error != 0) {
		LOG_ERR("failed to configure I2C bitbang (err %d)", error);
		goto restore;
	}

	error = i2c_bitbang_recover_bus(&bitbang_ctx);
	if (error != 0) {
		LOG_ERR("failed to recover bus (err %d)", error);
		goto restore;
	}

restore:
	(void)pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);

	k_sem_give(&data->lock);

	return error;
}
#endif /* CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY */

static void mcux_lpi2c_isr(const struct device *dev)
{
	const struct mcux_lpi2c_config *config = dev->config;
	struct mcux_lpi2c_data *data = dev->data;
	LPI2C_Type *base = config->base;

	LPI2C_MasterTransferHandleIRQ(base, &data->handle);
}

static int mcux_lpi2c_init(const struct device *dev)
{
	const struct mcux_lpi2c_config *config = dev->config;
	struct mcux_lpi2c_data *data = dev->data;
	LPI2C_Type *base = config->base;
	uint32_t clock_freq, bitrate_cfg;
	lpi2c_master_config_t master_config;
	int error;

	k_sem_init(&data->lock, 1, 1);
	k_sem_init(&data->device_sync_sem, 0, K_SEM_MAX_LIMIT);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	LPI2C_MasterGetDefaultConfig(&master_config);
	master_config.busIdleTimeout_ns = config->bus_idle_timeout_ns;
	LPI2C_MasterInit(base, &master_config, clock_freq);
	LPI2C_MasterTransferCreateHandle(base, &data->handle,
					 mcux_lpi2c_master_transfer_callback,
					 data);

	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);

	error = mcux_lpi2c_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (error) {
		return error;
	}

#ifdef CONFIG_PINCTRL
	error = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (error) {
		return error;
	}
#endif /* CONFIG_PINCTRL */

	config->irq_config_func(dev);

	return 0;
}

static const struct i2c_driver_api mcux_lpi2c_driver_api = {
	.configure = mcux_lpi2c_configure,
	.transfer = mcux_lpi2c_transfer,
#if CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY
	.recover_bus = mcux_lpi2c_recover_bus,
#endif /* CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY */
};

#ifdef CONFIG_PINCTRL
#define I2C_MCUX_LPI2C_PINCTRL_DEFINE(n) PINCTRL_DT_INST_DEFINE(n);
#define I2C_MCUX_LPI2C_PINCTRL_INIT(n) .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),
#else
#define I2C_MCUX_LPI2C_PINCTRL_DEFINE(n)
#define I2C_MCUX_LPI2C_PINCTRL_INIT(n)
#endif /* CONFIG_PINCTRL */

#if CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY
#define I2C_MCUX_LPI2C_SCL_INIT(n) .scl = GPIO_DT_SPEC_INST_GET_OR(n, scl_gpios, {0}),
#define I2C_MCUX_LPI2C_SDA_INIT(n) .sda = GPIO_DT_SPEC_INST_GET_OR(n, sda_gpios, {0}),
#else
#define I2C_MCUX_LPI2C_SCL_INIT(n)
#define I2C_MCUX_LPI2C_SDA_INIT(n)
#endif /* CONFIG_I2C_MCUX_LPI2C_BUS_RECOVERY */

#define I2C_MCUX_LPI2C_INIT(n)						\
	I2C_MCUX_LPI2C_PINCTRL_DEFINE(n)				\
									\
	static void mcux_lpi2c_config_func_##n(const struct device *dev); \
									\
	static const struct mcux_lpi2c_config mcux_lpi2c_config_##n = {	\
		.base = (LPI2C_Type *)DT_INST_REG_ADDR(n),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_subsys =						\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),\
		.irq_config_func = mcux_lpi2c_config_func_##n,		\
		.bitrate = DT_INST_PROP(n, clock_frequency),		\
		I2C_MCUX_LPI2C_PINCTRL_INIT(n)				\
		I2C_MCUX_LPI2C_SCL_INIT(n)				\
		I2C_MCUX_LPI2C_SDA_INIT(n)				\
		.bus_idle_timeout_ns =					\
			UTIL_AND(DT_INST_NODE_HAS_PROP(n, bus_idle_timeout),\
				 DT_INST_PROP(n, bus_idle_timeout)),	\
	};								\
									\
	static struct mcux_lpi2c_data mcux_lpi2c_data_##n;		\
									\
	I2C_DEVICE_DT_INST_DEFINE(n, mcux_lpi2c_init, NULL,		\
			    &mcux_lpi2c_data_##n,			\
			    &mcux_lpi2c_config_##n, POST_KERNEL,	\
			    CONFIG_I2C_INIT_PRIORITY,			\
			    &mcux_lpi2c_driver_api);			\
									\
	static void mcux_lpi2c_config_func_##n(const struct device *dev) \
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    mcux_lpi2c_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
									\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(I2C_MCUX_LPI2C_INIT)
