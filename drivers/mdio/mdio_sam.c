/*
 * Copyright (c) 2021 IP-Logix Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_mdio

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdio_sam, CONFIG_MDIO_LOG_LEVEL);

/* GMAC */
#ifdef CONFIG_SOC_FAMILY_SAM0
#define GMAC_MAN        MAN.reg
#define GMAC_NSR        NSR.reg
#define GMAC_NCR        NCR.reg
#endif

struct mdio_sam_dev_data {
	struct k_sem sem;
};

struct mdio_sam_dev_config {
	Gmac * const regs;
	const struct pinctrl_dev_config *pcfg;
	int protocol;
};

static int mdio_transfer(const struct device *dev, uint8_t prtad, uint8_t devad,
			 uint8_t rw, uint16_t data_in, uint16_t *data_out)
{
	const struct mdio_sam_dev_config *const cfg = dev->config;
	struct mdio_sam_dev_data *const data = dev->data;
	int timeout = 50;

	k_sem_take(&data->sem, K_FOREVER);

	/* Write mdio transaction */
	if (cfg->protocol == CLAUSE_45) {
		cfg->regs->GMAC_MAN = (GMAC_MAN_OP(rw ? 0x2 : 0x3))
				    |  GMAC_MAN_WTN(0x02)
				    |  GMAC_MAN_PHYA(prtad)
				    |  GMAC_MAN_REGA(devad)
				    |  GMAC_MAN_DATA(data_in);

	} else if (cfg->protocol == CLAUSE_22) {
		cfg->regs->GMAC_MAN =  GMAC_MAN_CLTTO
				    | (GMAC_MAN_OP(rw ? 0x2 : 0x1))
				    |  GMAC_MAN_WTN(0x02)
				    |  GMAC_MAN_PHYA(prtad)
				    |  GMAC_MAN_REGA(devad)
				    |  GMAC_MAN_DATA(data_in);

	} else {
		LOG_ERR("Unsupported protocol");
	}

	/* Wait until done */
	while (!(cfg->regs->GMAC_NSR & GMAC_NSR_IDLE)) {
		if (timeout-- == 0U) {
			LOG_ERR("transfer timedout %s", dev->name);
			k_sem_give(&data->sem);

			return -ETIMEDOUT;
		}

		k_sleep(K_MSEC(5));
	}

	if (data_out) {
		*data_out = cfg->regs->GMAC_MAN & GMAC_MAN_DATA_Msk;
	}

	k_sem_give(&data->sem);

	return 0;
}

static int mdio_sam_read(const struct device *dev, uint8_t prtad, uint8_t devad,
			 uint16_t *data)
{
	return mdio_transfer(dev, prtad, devad, 1, 0, data);
}

static int mdio_sam_write(const struct device *dev, uint8_t prtad,
			  uint8_t devad, uint16_t data)
{
	return mdio_transfer(dev, prtad, devad, 0, data, NULL);
}

static void mdio_sam_bus_enable(const struct device *dev)
{
	const struct mdio_sam_dev_config *const cfg = dev->config;

	cfg->regs->GMAC_NCR |= GMAC_NCR_MPE;
}

static void mdio_sam_bus_disable(const struct device *dev)
{
	const struct mdio_sam_dev_config *const cfg = dev->config;

	cfg->regs->GMAC_NCR &= ~GMAC_NCR_MPE;
}

static int mdio_sam_initialize(const struct device *dev)
{
	const struct mdio_sam_dev_config *const cfg = dev->config;
	struct mdio_sam_dev_data *const data = dev->data;
	int retval;

	k_sem_init(&data->sem, 1, 1);

	retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

	return retval;
}

static const struct mdio_driver_api mdio_sam_driver_api = {
	.read = mdio_sam_read,
	.write = mdio_sam_write,
	.bus_enable = mdio_sam_bus_enable,
	.bus_disable = mdio_sam_bus_disable,
};

#define MDIO_SAM_CONFIG(n)						\
static const struct mdio_sam_dev_config mdio_sam_dev_config_##n = {	\
	.regs = (Gmac *)DT_REG_ADDR(DT_INST_PARENT(n)),			\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
	.protocol = DT_INST_ENUM_IDX(n, protocol),			\
};

#define MDIO_SAM_DEVICE(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
	MDIO_SAM_CONFIG(n);						\
	static struct mdio_sam_dev_data mdio_sam_dev_data##n;		\
	DEVICE_DT_INST_DEFINE(n,					\
			      &mdio_sam_initialize,			\
			      NULL,					\
			      &mdio_sam_dev_data##n,			\
			      &mdio_sam_dev_config_##n, POST_KERNEL,	\
			      CONFIG_MDIO_INIT_PRIORITY,		\
			      &mdio_sam_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_SAM_DEVICE)
