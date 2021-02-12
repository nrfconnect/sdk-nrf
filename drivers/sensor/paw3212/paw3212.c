/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <device.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <sensor/paw3212.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(paw3212, CONFIG_PAW3212_LOG_LEVEL);

#define DT_DRV_COMPAT pixart_paw3212

#define PAW3212_SPI_DEV_NAME DT_BUS_LABEL(DT_DRV_INST(0))

#define PAW3212_IRQ_GPIO_DEV_NAME DT_INST_GPIO_LABEL(0, irq_gpios)
#define PAW3212_IRQ_GPIO_PIN      DT_INST_GPIO_PIN(0, irq_gpios)

#define PAW3212_CS_GPIO_DEV_NAME DT_INST_SPI_DEV_CS_GPIOS_LABEL(0)
#define PAW3212_CS_GPIO_PIN      DT_INST_SPI_DEV_CS_GPIOS_PIN(0)


/* Timings defined by spec */
#define T_NCS_SCLK	1			/* 120 ns */
#define T_SRX		(20 - T_NCS_SCLK)	/* 20 us */
#define T_SCLK_NCS_WR	(35 - T_NCS_SCLK)	/* 35 us */
#define T_SWX		(180 - T_SCLK_NCS_WR)	/* 180 us */
#define T_SRAD		160			/* 160 us */


/* Sensor registers */
#define PAW3212_REG_PRODUCT_ID		0x00
#define PAW3212_REG_REVISION_ID		0x01
#define PAW3212_REG_MOTION		0x02
#define PAW3212_REG_DELTA_X_LOW		0x03
#define PAW3212_REG_DELTA_Y_LOW		0x04
#define PAW3212_REG_OPERATION_MODE	0x05
#define PAW3212_REG_CONFIGURATION	0x06
#define PAW3212_REG_WRITE_PROTECT	0x09
#define PAW3212_REG_DELTA_XY_HIGH	0x12
#define PAW3212_REG_MOUSE_OPTION	0x19
#define PAW3212_REG_SLEEP1		0x0A
#define PAW3212_REG_SLEEP2		0x0B
#define PAW3212_REG_SLEEP3		0x0C
#define PAW3212_REG_CPI_X		0x0D
#define PAW3212_REG_CPI_Y		0x0E

/* CPI */
#define PAW3212_CPI_STEP		38u
#define PAW3212_CPI_MIN			(0x00 * PAW3212_CPI_STEP)
#define PAW3212_CPI_MAX			(0x3F * PAW3212_CPI_STEP)

/* Sleep */
#define PAW3212_SLP_ENH_POS		4u
#define PAW3212_SLP2_ENH_POS		3u
#define PAW3212_SLP3_ENH_POS		5u

#define PAW3212_ETM_POS			0u
#define PAW3212_ETM_SIZE		4u
#define PAW3212_FREQ_POS		(PAW3212_ETM_POS + PAW3212_ETM_SIZE)
#define PAW3212_FREQ_SIZE		4u

#define PAW3212_ETM_MIN			0u
#define PAW3212_ETM_MAX			BIT_MASK(PAW3212_ETM_SIZE)
#define PAW3212_ETM_MASK		(PAW3212_ETM_MAX << PAW3212_ETM_POS)

#define PAW3212_FREQ_MIN		0u
#define PAW3212_FREQ_MAX		BIT_MASK(PAW3212_FREQ_SIZE)
#define PAW3212_FREQ_MASK		(PAW3212_FREQ_MAX << PAW3212_FREQ_POS)

/* Motion status bits */
#define PAW3212_MOTION_STATUS_MOTION	BIT(7)
#define PAW3212_MOTION_STATUS_DYOVF	BIT(4)
#define PAW3212_MOTION_STATUS_DXOVF	BIT(3)

/* Configuration bits */
#define PAW3212_CONFIG_CHIP_RESET	BIT(7)

/* Mouse option bits */
#define PAW3212_MOUSE_OPT_XY_SWAP	BIT(5)
#define PAW3212_MOUSE_OPT_Y_INV		BIT(4)
#define PAW3212_MOUSE_OPT_X_INV		BIT(3)
#define PAW3212_MOUSE_OPT_12BITMODE	BIT(2)

/* Convert deltas to x and y */
#define PAW3212_DELTA_X(xy_high, x_low)	expand_s12((((xy_high) & 0xF0) << 4) | (x_low))
#define PAW3212_DELTA_Y(xy_high, y_low)	expand_s12((((xy_high) & 0x0F) << 8) | (y_low))

/* Sensor identification values */
#define PAW3212_PRODUCT_ID		0x30

/* Write protect magic */
#define PAW3212_WPMAGIC			0x5A

#define SPI_WRITE_BIT			BIT(7)


enum async_init_step {
	ASYNC_INIT_STEP_POWER_UP,
	ASYNC_INIT_STEP_VERIFY_ID,
	ASYNC_INIT_STEP_CONFIGURE,

	ASYNC_INIT_STEP_COUNT
};

struct paw3212_data {
	const struct device          *cs_gpio_dev;
	const struct device          *irq_gpio_dev;
	const struct device          *spi_dev;
	struct gpio_callback         irq_gpio_cb;
	struct k_spinlock            lock;
	int16_t                        x;
	int16_t                        y;
	sensor_trigger_handler_t     data_ready_handler;
	struct k_work                trigger_handler_work;
	struct k_delayed_work        init_work;
	enum async_init_step         async_init_step;
	int                          err;
	bool                         ready;
};

static const struct spi_config spi_cfg = {
	.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB |
		     SPI_MODE_CPOL | SPI_MODE_CPHA,
	.frequency = DT_PROP(DT_DRV_INST(0), spi_max_frequency),
	.slave = DT_REG_ADDR(DT_DRV_INST(0)),
};

static const int32_t async_init_delay[ASYNC_INIT_STEP_COUNT] = {
	[ASYNC_INIT_STEP_POWER_UP]  = 1,
	[ASYNC_INIT_STEP_VERIFY_ID] = 0,
	[ASYNC_INIT_STEP_CONFIGURE] = 0,
};


static int paw3212_async_init_power_up(struct paw3212_data *dev_data);
static int paw3212_async_init_verify_id(struct paw3212_data *dev_data);
static int paw3212_async_init_configure(struct paw3212_data *dev_data);


static int (* const async_init_fn[ASYNC_INIT_STEP_COUNT])(struct paw3212_data *dev_data) = {
	[ASYNC_INIT_STEP_POWER_UP]  = paw3212_async_init_power_up,
	[ASYNC_INIT_STEP_VERIFY_ID] = paw3212_async_init_verify_id,
	[ASYNC_INIT_STEP_CONFIGURE] = paw3212_async_init_configure,
};


static struct paw3212_data paw3212_data;

static int16_t expand_s12(int16_t x)
{
	/* Left shifting of negative values is undefined behavior, so we cannot
	 * depend on automatic integer promotion (it will convert int16_t to int).
	 * To expand sign we cast s16 to unsigned int and left shift it then
	 * cast it to signed integer and do the right shift. Since type is
	 * signed compiler will perform arithmetic shift. This last operation
	 * is implementation defined in C but defined in the compiler's
	 * documentation and common for modern compilers and CPUs.
	 */
	return ((signed int)((unsigned int)x << 20)) >> 20;
}

static int spi_cs_ctrl(struct paw3212_data *dev_data, bool enable)
{
	int val = (enable) ? (0) : (1);
	int err;

	if (!enable) {
		k_busy_wait(T_NCS_SCLK);
	}

	err = gpio_pin_set_raw(dev_data->cs_gpio_dev, PAW3212_CS_GPIO_PIN, val);

	if (err) {
		LOG_ERR("SPI CS ctrl failed");
	}

	if (enable) {
		k_busy_wait(T_NCS_SCLK);
	}

	return err;
}

static int reg_read(struct paw3212_data *dev_data, uint8_t reg, uint8_t *buf)
{
	int err;

	__ASSERT_NO_MSG((reg & SPI_WRITE_BIT) == 0);

	err = spi_cs_ctrl(dev_data, true);
	if (err) {
		return err;
	}

	/* Write register address. */
	const struct spi_buf tx_buf = {
		.buf = &reg,
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	err = spi_write(dev_data->spi_dev, &spi_cfg, &tx);
	if (err) {
		LOG_ERR("Reg read failed on SPI write");
		return err;
	}

	k_busy_wait(T_SRAD);

	/* Read register value. */
	struct spi_buf rx_buf = {
		.buf = buf,
		.len = 1,
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1,
	};

	err = spi_read(dev_data->spi_dev, &spi_cfg, &rx);
	if (err) {
		LOG_ERR("Reg read failed on SPI read");
		return err;
	}

	err = spi_cs_ctrl(dev_data, false);
	if (err) {
		return err;
	}

	k_busy_wait(T_SRX);

	return 0;
}

static int reg_write(struct paw3212_data *dev_data, uint8_t reg, uint8_t val)
{
	int err;

	__ASSERT_NO_MSG((reg & SPI_WRITE_BIT) == 0);

	err = spi_cs_ctrl(dev_data, true);
	if (err) {
		return err;
	}

	uint8_t buf[] = {
		SPI_WRITE_BIT | reg,
		val
	};
	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = ARRAY_SIZE(buf)
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	err = spi_write(dev_data->spi_dev, &spi_cfg, &tx);
	if (err) {
		LOG_ERR("Reg write failed on SPI write");
		return err;
	}

	k_busy_wait(T_SCLK_NCS_WR);

	err = spi_cs_ctrl(dev_data, false);
	if (err) {
		return err;
	}

	k_busy_wait(T_SWX);

	return 0;
}

static int update_cpi(struct paw3212_data *dev_data, uint32_t cpi)
{
	int err;

	if ((cpi > PAW3212_CPI_MAX) || (cpi < PAW3212_CPI_MIN)) {
		LOG_ERR("CPI %" PRIu32 " out of range", cpi);
		return -EINVAL;
	}

	uint8_t regval = cpi / PAW3212_CPI_STEP;

	LOG_INF("Set CPI: %u (requested: %u, reg:0x%" PRIx8 ")",
		regval * PAW3212_CPI_STEP, cpi, regval);

	err = reg_write(dev_data, PAW3212_REG_WRITE_PROTECT, PAW3212_WPMAGIC);
	if (err) {
		LOG_ERR("Cannot disable write protect");
		return err;
	}

	err = reg_write(dev_data, PAW3212_REG_CPI_X, regval);
	if (err) {
		LOG_ERR("Failed to change x CPI");
		return err;
	}

	err = reg_write(dev_data, PAW3212_REG_CPI_Y, regval);
	if (err) {
		LOG_ERR("Failed to change y CPI");
		return err;
	}

	err = reg_write(dev_data, PAW3212_REG_WRITE_PROTECT, 0);
	if (err) {
		LOG_ERR("Cannot enable write protect");
	}

	return err;
}

static int update_sleep_timeout(struct paw3212_data *dev_data, uint8_t reg_addr,
				uint32_t timeout_ms)
{
	uint32_t timeout_step_ms;

	switch (reg_addr) {
	case PAW3212_REG_SLEEP1:
		timeout_step_ms = 32;
		break;

	case PAW3212_REG_SLEEP2:
	case PAW3212_REG_SLEEP3:
		timeout_step_ms = 20480;
		break;

	default:
		LOG_ERR("Not supported");
		return -ENOTSUP;
	}

	uint32_t etm = timeout_ms / timeout_step_ms - 1;

	if ((etm < PAW3212_ETM_MIN) || (etm > PAW3212_ETM_MAX)) {
		LOG_WRN("Sleep timeout %" PRIu32 " out of range", timeout_ms);
		return -EINVAL;
	}

	LOG_INF("Set sleep%d timeout: %u (requested: %u, reg:0x%" PRIx8 ")",
		reg_addr - PAW3212_REG_SLEEP1 + 1,
		(etm + 1) * timeout_step_ms,
		timeout_ms,
		etm);

	int err;

	err = reg_write(dev_data, PAW3212_REG_WRITE_PROTECT, PAW3212_WPMAGIC);
	if (err) {
		LOG_ERR("Cannot disable write protect");
		return err;
	}

	uint8_t regval;

	err = reg_read(dev_data, reg_addr, &regval);
	if (err) {
		LOG_ERR("Failed to read sleep register");
		return err;
	}

	__ASSERT_NO_MSG((etm & PAW3212_ETM_MASK) == etm);

	regval &= ~PAW3212_ETM_MASK;
	regval |= (etm << PAW3212_ETM_POS);

	err = reg_write(dev_data, reg_addr, regval);
	if (err) {
		LOG_ERR("Failed to change sleep time");
		return err;
	}

	err = reg_write(dev_data, PAW3212_REG_WRITE_PROTECT, 0);
	if (err) {
		LOG_ERR("Cannot enable write protect");
	}

	return err;
}

static int update_sample_time(struct paw3212_data *dev_data, uint8_t reg_addr,
			      uint32_t sample_time_ms)
{
	uint32_t sample_time_step;
	uint32_t sample_time_min;
	uint32_t sample_time_max;

	switch (reg_addr) {
	case PAW3212_REG_SLEEP1:
		sample_time_step = 4;
		sample_time_min = 4;
		sample_time_max = 64;
		break;

	case PAW3212_REG_SLEEP2:
	case PAW3212_REG_SLEEP3:
		sample_time_step = 64;
		sample_time_min = 64;
		sample_time_max = 1024;
		break;

	default:
		LOG_ERR("Not supported");
		return -ENOTSUP;
	}

	if ((sample_time_ms > sample_time_max) ||
	    (sample_time_ms < sample_time_min))	{
		LOG_WRN("Sample time %" PRIu32 " out of range", sample_time_ms);
		return -EINVAL;
	}

	uint8_t reg_freq = (sample_time_ms - sample_time_min) / sample_time_step;

	LOG_INF("Set sleep%d sample time: %u (requested: %u, reg:0x%" PRIx8 ")",
		reg_addr - PAW3212_REG_SLEEP1 + 1,
		(reg_freq * sample_time_step) + sample_time_min,
		sample_time_ms,
		reg_freq);

	__ASSERT_NO_MSG(reg_freq <= PAW3212_FREQ_MAX);

	int err;

	err = reg_write(dev_data, PAW3212_REG_WRITE_PROTECT, PAW3212_WPMAGIC);
	if (err) {
		LOG_ERR("Cannot disable write protect");
		return err;
	}

	uint8_t regval;

	err = reg_read(dev_data, reg_addr, &regval);
	if (err) {
		LOG_ERR("Failed to read sleep register");
		return err;
	}

	regval &= ~PAW3212_FREQ_MASK;
	regval |= (reg_freq << PAW3212_FREQ_POS);

	err = reg_write(dev_data, reg_addr, regval);
	if (err) {
		LOG_ERR("Failed to change sample time");
		return err;
	}

	err = reg_write(dev_data, PAW3212_REG_WRITE_PROTECT, 0);
	if (err) {
		LOG_ERR("Cannot enable write protect");
	}

	return err;
}

static int toggle_sleep_modes(struct paw3212_data *dev_data, uint8_t reg_addr1, uint8_t reg_addr2,
			      bool enable)
{
	int err = reg_write(dev_data, PAW3212_REG_WRITE_PROTECT,
			    PAW3212_WPMAGIC);
	if (err) {
		LOG_ERR("Cannot disable write protect");
		return err;
	}

	uint8_t regval;

	LOG_INF("%sable sleep", (enable) ? ("En") : ("Dis"));

	/* Sleep 1 and Sleep 2 */
	err = reg_read(dev_data, reg_addr1, &regval);
	if (err) {
		LOG_ERR("Failed to read operation mode register");
		return err;
	}

	uint8_t sleep_enable_mask = BIT(PAW3212_SLP_ENH_POS) |
				 BIT(PAW3212_SLP2_ENH_POS);

	if (enable) {
		regval |= sleep_enable_mask;
	} else {
		regval &= ~sleep_enable_mask;
	}

	err = reg_write(dev_data, reg_addr1, regval);
	if (err) {
		LOG_ERR("Failed to %sable sleep", (enable) ? ("en") : ("dis"));
		return err;
	}

	/* Sleep 3 */
	err = reg_read(dev_data, reg_addr2, &regval);
	if (err) {
		LOG_ERR("Failed to read configuration register");
		return err;
	}

	sleep_enable_mask = BIT(PAW3212_SLP3_ENH_POS);

	if (enable) {
		regval |= sleep_enable_mask;
	} else {
		regval &= ~sleep_enable_mask;
	}

	err = reg_write(dev_data, reg_addr2, regval);
	if (err) {
		LOG_ERR("Failed to %sable sleep", (enable) ? ("en") : ("dis"));
		return err;
	}

	err = reg_write(dev_data, PAW3212_REG_WRITE_PROTECT, 0);
	if (err) {
		LOG_ERR("Cannot enable write protect");
	}

	return err;
}

static void irq_handler(const struct device *gpiob, struct gpio_callback *cb,
			uint32_t pins)
{
	int err;

	err = gpio_pin_interrupt_configure(paw3212_data.irq_gpio_dev,
					   PAW3212_IRQ_GPIO_PIN,
					   GPIO_INT_DISABLE);
	if (unlikely(err)) {
		LOG_ERR("Cannot disable IRQ");
		k_panic();
	}

	k_work_submit(&paw3212_data.trigger_handler_work);
}

static void trigger_handler(struct k_work *work)
{
	sensor_trigger_handler_t handler;
	int err = 0;

	k_spinlock_key_t key = k_spin_lock(&paw3212_data.lock);
	handler = paw3212_data.data_ready_handler;
	k_spin_unlock(&paw3212_data.lock, key);

	if (!handler) {
		return;
	}

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	handler(DEVICE_DT_INST_GET(0), &trig);

	key = k_spin_lock(&paw3212_data.lock);
	if (paw3212_data.data_ready_handler) {
		err = gpio_pin_interrupt_configure(paw3212_data.irq_gpio_dev,
						   PAW3212_IRQ_GPIO_PIN,
						   GPIO_INT_LEVEL_LOW);
	}
	k_spin_unlock(&paw3212_data.lock, key);

	if (unlikely(err)) {
		LOG_ERR("Cannot re-enable IRQ");
		k_panic();
	}
}

static int paw3212_async_init_power_up(struct paw3212_data *dev_data)
{
	return reg_write(dev_data, PAW3212_REG_CONFIGURATION,
			 PAW3212_CONFIG_CHIP_RESET);
}

static int paw3212_async_init_verify_id(struct paw3212_data *dev_data)
{
	int err;

	uint8_t product_id;
	err = reg_read(dev_data, PAW3212_REG_PRODUCT_ID, &product_id);
	if (err) {
		LOG_ERR("Cannot obtain product ID");
		return err;
	}

	LOG_INF("Product ID: 0x%" PRIx8, product_id);
	if (product_id != PAW3212_PRODUCT_ID) {
		LOG_ERR("Invalid product ID (0x%" PRIx8")!", product_id);
		return -EIO;
	}

	return err;
}

static int paw3212_async_init_configure(struct paw3212_data *dev_data)
{
	int err;

	err = reg_write(dev_data, PAW3212_REG_WRITE_PROTECT, PAW3212_WPMAGIC);
	if (!err) {
		uint8_t mouse_option = 0;

		if (IS_ENABLED(CONFIG_PAW3212_ORIENTATION_90)) {
			mouse_option |= PAW3212_MOUSE_OPT_XY_SWAP |
					PAW3212_MOUSE_OPT_Y_INV;
		} else if (IS_ENABLED(CONFIG_PAW3212_ORIENTATION_180)) {
			mouse_option |= PAW3212_MOUSE_OPT_Y_INV |
					PAW3212_MOUSE_OPT_X_INV;
		} else if (IS_ENABLED(CONFIG_PAW3212_ORIENTATION_270)) {
			mouse_option |= PAW3212_MOUSE_OPT_XY_SWAP |
					PAW3212_MOUSE_OPT_X_INV;
		}

		if (IS_ENABLED(CONFIG_PAW3212_12_BIT_MODE)) {
			mouse_option |= PAW3212_MOUSE_OPT_12BITMODE;
		}

		err = reg_write(dev_data, PAW3212_REG_MOUSE_OPTION,
				mouse_option);
	}
	if (!err) {
		err = reg_write(dev_data, PAW3212_REG_WRITE_PROTECT, 0);
	}

	return err;
}

static void paw3212_async_init(struct k_work *work)
{
	struct paw3212_data *dev_data = &paw3212_data;

	ARG_UNUSED(work);

	LOG_DBG("PAW3212 async init step %d", dev_data->async_init_step);

	dev_data->err = async_init_fn[dev_data->async_init_step](dev_data);
	if (dev_data->err) {
		LOG_ERR("PAW3212 initialization failed");
	} else {
		dev_data->async_init_step++;

		if (dev_data->async_init_step == ASYNC_INIT_STEP_COUNT) {
			dev_data->ready = true;
			LOG_INF("PAW3212 initialized");
		} else {
			k_delayed_work_submit(&dev_data->init_work,
					      K_MSEC(async_init_delay[
						dev_data->async_init_step]));
		}
	}
}

static int paw3212_init_cs(struct paw3212_data *dev_data)
{
	int err;

	dev_data->cs_gpio_dev =
		device_get_binding(PAW3212_CS_GPIO_DEV_NAME);
	if (!dev_data->cs_gpio_dev) {
		LOG_ERR("Cannot get CS GPIO device");
		return -ENXIO;
	}

	err = gpio_pin_configure(dev_data->cs_gpio_dev,
				 PAW3212_CS_GPIO_PIN,
				 GPIO_OUTPUT);
	if (!err) {
		err = spi_cs_ctrl(dev_data, false);
	} else {
		LOG_ERR("Cannot configure CS PIN");
	}

	return err;
}

static int paw3212_init_irq(struct paw3212_data *dev_data)
{
	int err;

	dev_data->irq_gpio_dev =
		device_get_binding(PAW3212_IRQ_GPIO_DEV_NAME);
	if (!dev_data->irq_gpio_dev) {
		LOG_ERR("Cannot get IRQ GPIO device");
		return -ENXIO;
	}

	err = gpio_pin_configure(dev_data->irq_gpio_dev,
				 PAW3212_IRQ_GPIO_PIN,
				 GPIO_INPUT | GPIO_PULL_UP);
	if (err) {
		LOG_ERR("Cannot configure IRQ GPIO");
		return err;
	}

	gpio_init_callback(&dev_data->irq_gpio_cb, irq_handler,
			   BIT(PAW3212_IRQ_GPIO_PIN));

	err = gpio_add_callback(dev_data->irq_gpio_dev, &dev_data->irq_gpio_cb);

	if (err) {
		LOG_ERR("Cannot add IRQ GPIO callback");
	}

	return err;
}

static int paw3212_init_spi(struct paw3212_data *dev_data)
{
	dev_data->spi_dev = device_get_binding(PAW3212_SPI_DEV_NAME);
	if (!dev_data->spi_dev) {
		LOG_ERR("Cannot get SPI device");
		return -ENXIO;
	}

	return 0;
}

static int paw3212_init(const struct device *dev)
{
	struct paw3212_data *dev_data = &paw3212_data;
	int err;

	ARG_UNUSED(dev);

	/* Assert that negative numbers are processed as expected */
	__ASSERT_NO_MSG(-1 == expand_s12(0xFFF));

	k_work_init(&dev_data->trigger_handler_work, trigger_handler);

	err = paw3212_init_cs(dev_data);
	if (err) {
		return err;
	}

	err = paw3212_init_irq(dev_data);
	if (err) {
		return err;
	}

	err = paw3212_init_spi(dev_data);
	if (err) {
		return err;
	}

	k_delayed_work_init(&dev_data->init_work, paw3212_async_init);

	k_delayed_work_submit(&dev_data->init_work,
			      K_MSEC(async_init_delay[
				dev_data->async_init_step]));

	return err;
}

static int paw3212_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct paw3212_data *dev_data = &paw3212_data;
	uint8_t motion_status;
	int err;

	ARG_UNUSED(dev);

	if (unlikely(chan != SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	if (unlikely(!dev_data->ready)) {
		LOG_INF("Device is not initialized yet");
		return -EBUSY;
	}

	err = reg_read(dev_data, PAW3212_REG_MOTION, &motion_status);
	if (err) {
		LOG_ERR("Cannot read motion");
		return err;
	}

	if ((motion_status & PAW3212_MOTION_STATUS_MOTION) != 0) {
		uint8_t x_low;
		uint8_t y_low;

		if ((motion_status & PAW3212_MOTION_STATUS_DXOVF) != 0) {
			LOG_WRN("X delta overflowed");
		}
		if ((motion_status & PAW3212_MOTION_STATUS_DYOVF) != 0) {
			LOG_WRN("Y delta overflowed");
		}

		err = reg_read(dev_data, PAW3212_REG_DELTA_X_LOW, &x_low);
		if (err) {
			LOG_ERR("Cannot read X delta");
			return err;
		}

		err = reg_read(dev_data, PAW3212_REG_DELTA_Y_LOW, &y_low);
		if (err) {
			LOG_ERR("Cannot read Y delta");
			return err;
		}

		if (IS_ENABLED(CONFIG_PAW3212_12_BIT_MODE)) {
			uint8_t xy_high;

			err = reg_read(dev_data, PAW3212_REG_DELTA_XY_HIGH,
				       &xy_high);
			if (err) {
				LOG_ERR("Cannot read XY delta high");
				return err;
			}

			dev_data->x = PAW3212_DELTA_X(xy_high, x_low);
			dev_data->y = PAW3212_DELTA_Y(xy_high, y_low);
		} else {
			dev_data->x = (int8_t)x_low;
			dev_data->y = (int8_t)y_low;
		}
	} else {
		dev_data->x = 0;
		dev_data->y = 0;
	}

	return err;
}

static int paw3212_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct paw3212_data *dev_data = &paw3212_data;

	ARG_UNUSED(dev);

	if (unlikely(!dev_data->ready)) {
		LOG_INF("Device is not initialized yet");
		return -EBUSY;
	}

	switch (chan) {
	case SENSOR_CHAN_POS_DX:
		val->val1 = dev_data->x;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_POS_DY:
		val->val1 = dev_data->y;
		val->val2 = 0;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int paw3212_trigger_set(const struct device *dev,
			       const struct sensor_trigger *trig,
			       sensor_trigger_handler_t handler)
{
	struct paw3212_data *dev_data = &paw3212_data;
	int err;

	ARG_UNUSED(dev);

	if (unlikely(trig->type != SENSOR_TRIG_DATA_READY)) {
		return -ENOTSUP;
	}

	if (unlikely(trig->chan != SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	if (unlikely(!dev_data->ready)) {
		LOG_INF("Device is not initialized yet");
		return -EBUSY;
	}

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	if (handler) {
		err = gpio_pin_interrupt_configure(dev_data->irq_gpio_dev,
						   PAW3212_IRQ_GPIO_PIN,
						   GPIO_INT_LEVEL_LOW);
	} else {
		err = gpio_pin_interrupt_configure(dev_data->irq_gpio_dev,
						   PAW3212_IRQ_GPIO_PIN,
						   GPIO_INT_DISABLE);
	}

	if (!err) {
		dev_data->data_ready_handler = handler;
	}

	k_spin_unlock(&dev_data->lock, key);

	return err;
}

static int paw3212_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	struct paw3212_data *dev_data = &paw3212_data;
	int err;

	ARG_UNUSED(dev);

	if (unlikely(chan != SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	if (unlikely(!dev_data->ready)) {
		LOG_INF("Device is not initialized yet");
		return -EBUSY;
	}

	switch ((uint32_t)attr) {
	case PAW3212_ATTR_CPI:
		err = update_cpi(dev_data, PAW3212_SVALUE_TO_CPI(*val));
		break;

	case PAW3212_ATTR_SLEEP_ENABLE:
		err = toggle_sleep_modes(dev_data,
					 PAW3212_REG_OPERATION_MODE,
					 PAW3212_REG_CONFIGURATION,
					 PAW3212_SVALUE_TO_BOOL(*val));
		break;

	case PAW3212_ATTR_SLEEP1_TIMEOUT:
		err = update_sleep_timeout(dev_data,
					   PAW3212_REG_SLEEP1,
					   PAW3212_SVALUE_TO_TIME(*val));
		break;

	case PAW3212_ATTR_SLEEP2_TIMEOUT:
		err = update_sleep_timeout(dev_data,
					   PAW3212_REG_SLEEP2,
					   PAW3212_SVALUE_TO_TIME(*val));
		break;

	case PAW3212_ATTR_SLEEP3_TIMEOUT:
		err = update_sleep_timeout(dev_data,
					   PAW3212_REG_SLEEP3,
					   PAW3212_SVALUE_TO_TIME(*val));
		break;

	case PAW3212_ATTR_SLEEP1_SAMPLE_TIME:
		err = update_sample_time(dev_data,
					 PAW3212_REG_SLEEP1,
					 PAW3212_SVALUE_TO_TIME(*val));
		break;

	case PAW3212_ATTR_SLEEP2_SAMPLE_TIME:
		err = update_sample_time(dev_data,
					 PAW3212_REG_SLEEP2,
					 PAW3212_SVALUE_TO_TIME(*val));
		break;

	case PAW3212_ATTR_SLEEP3_SAMPLE_TIME:
		err = update_sample_time(dev_data,
					 PAW3212_REG_SLEEP3,
					 PAW3212_SVALUE_TO_TIME(*val));
		break;

	default:
		LOG_ERR("Unknown attribute");
		return -ENOTSUP;
	}

	return err;
}

static const struct sensor_driver_api paw3212_driver_api = {
	.sample_fetch = paw3212_sample_fetch,
	.channel_get  = paw3212_channel_get,
	.trigger_set  = paw3212_trigger_set,
	.attr_set     = paw3212_attr_set,
};

DEVICE_DT_INST_DEFINE(0, paw3212_init, device_pm_control_nop,
		      NULL, NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		      &paw3212_driver_api);
