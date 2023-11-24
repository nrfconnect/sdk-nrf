/*
 * Copyright (c) 2019-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define DT_DRV_COMPAT pixart_pmw3360

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <sensor/pmw3360.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pmw3360, CONFIG_PMW3360_LOG_LEVEL);

/* Timings defined by spec */
#define T_NCS_SCLK	1			/* 120 ns */
#define T_SRX		(20 - T_NCS_SCLK)	/* 20 us */
#define T_SCLK_NCS_WR	(35 - T_NCS_SCLK)	/* 35 us */
#define T_SWX		(180 - T_SCLK_NCS_WR)	/* 180 us */
#define T_SRAD		160			/* 160 us */
#define T_SRAD_MOTBR	35			/* 35 us */
#define T_BEXIT		1			/* 500 ns */

/* Timing defined on SROM download burst mode figure */
#define T_BRSEP		15			/* 15 us */


/* Sensor registers */
#define PMW3360_REG_PRODUCT_ID			0x00
#define PMW3360_REG_REVISION_ID			0x01
#define PMW3360_REG_MOTION			0x02
#define PMW3360_REG_DELTA_X_L			0x03
#define PMW3360_REG_DELTA_X_H			0x04
#define PMW3360_REG_DELTA_Y_L			0x05
#define PMW3360_REG_DELTA_Y_H			0x06
#define PMW3360_REG_SQUAL			0x07
#define PMW3360_REG_RAW_DATA_SUM		0x08
#define PMW3360_REG_MAXIMUM_RAW_DATA		0x09
#define PMW3360_REG_MINIMUM_RAW_DATA		0x0A
#define PMW3360_REG_SHUTTER_LOWER		0x0B
#define PMW3360_REG_SHUTTER_UPPER		0x0C
#define PMW3360_REG_CONTROL			0x0D
#define PMW3360_REG_CONFIG1			0x0F
#define PMW3360_REG_CONFIG2			0x10
#define PMW3360_REG_ANGLE_TUNE			0x11
#define PMW3360_REG_FRAME_CAPTURE		0x12
#define PMW3360_REG_SROM_ENABLE			0x13
#define PMW3360_REG_RUN_DOWNSHIFT		0x14
#define PMW3360_REG_REST1_RATE_LOWER		0x15
#define PMW3360_REG_REST1_RATE_UPPER		0x16
#define PMW3360_REG_REST1_DOWNSHIFT		0x17
#define PMW3360_REG_REST2_RATE_LOWER		0x18
#define PMW3360_REG_REST2_RATE_UPPER		0x19
#define PMW3360_REG_REST2_DOWNSHIFT		0x1A
#define PMW3360_REG_REST3_RATE_LOWER		0x1B
#define PMW3360_REG_REST3_RATE_UPPER		0x1C
#define PMW3360_REG_OBSERVATION			0x24
#define PMW3360_REG_DATA_OUT_LOWER		0x25
#define PMW3360_REG_DATA_OUT_UPPER		0x26
#define PMW3360_REG_RAW_DATA_DUMP		0x29
#define PMW3360_REG_SROM_ID			0x2A
#define PMW3360_REG_MIN_SQ_RUN			0x2B
#define PMW3360_REG_RAW_DATA_THRESHOLD		0x2C
#define PMW3360_REG_CONFIG5			0x2F
#define PMW3360_REG_POWER_UP_RESET		0x3A
#define PMW3360_REG_SHUTDOWN			0x3B
#define PMW3360_REG_INVERSE_PRODUCT_ID		0x3F
#define PMW3360_REG_LIFTCUTOFF_TUNE3		0x41
#define PMW3360_REG_ANGLE_SNAP			0x42
#define PMW3360_REG_LIFTCUTOFF_TUNE1		0x4A
#define PMW3360_REG_MOTION_BURST		0x50
#define PMW3360_REG_LIFTCUTOFF_TUNE_TIMEOUT	0x58
#define PMW3360_REG_LIFTCUTOFF_TUNE_MIN_LENGTH	0x5A
#define PMW3360_REG_SROM_LOAD_BURST		0x62
#define PMW3360_REG_LIFT_CONFIG			0x63
#define PMW3360_REG_RAW_DATA_BURST		0x64
#define PMW3360_REG_LIFTCUTOFF_TUNE2		0x65

/* Sensor identification values */
#define PMW3360_PRODUCT_ID			0x42
#define PMW3360_FIRMWARE_ID			0x04

/* Max register count readable in a single motion burst */
#define PMW3360_MAX_BURST_SIZE			12

/* Register count used for reading a single motion burst */
#define PMW3360_BURST_SIZE			6

/* Position of X in motion burst data */
#define PMW3360_DX_POS				2
#define PMW3360_DY_POS				4

/* Rest_En position in Config2 register. */
#define PMW3360_REST_EN_POS			5

#define PMW3360_MAX_CPI				12000
#define PMW3360_MIN_CPI				100


#define SPI_WRITE_BIT				BIT(7)

/* Helper macros used to convert sensor values. */
#define PMW3360_SVALUE_TO_CPI(svalue) ((uint32_t)(svalue).val1)
#define PMW3360_SVALUE_TO_TIME(svalue) ((uint32_t)(svalue).val1)
#define PMW3360_SVALUE_TO_BOOL(svalue) ((svalue).val1 != 0)


extern const size_t pmw3360_firmware_length;
extern const uint8_t pmw3360_firmware_data[];


enum async_init_step {
	ASYNC_INIT_STEP_POWER_UP,
	ASYNC_INIT_STEP_FW_LOAD_START,
	ASYNC_INIT_STEP_FW_LOAD_CONTINUE,
	ASYNC_INIT_STEP_FW_LOAD_VERIFY,
	ASYNC_INIT_STEP_CONFIGURE,

	ASYNC_INIT_STEP_COUNT
};

struct pmw3360_data {
	const struct device          *dev;
	struct gpio_callback         irq_gpio_cb;
	struct k_spinlock            lock;
	int16_t                      x;
	int16_t                      y;
	sensor_trigger_handler_t     data_ready_handler;
	struct k_work                trigger_handler_work;
	struct k_work_delayable      init_work;
	enum async_init_step         async_init_step;
	int                          err;
	bool                         ready;
	bool                         last_read_burst;
};

struct pmw3360_config {
	struct gpio_dt_spec irq_gpio;
	struct spi_dt_spec bus;
	struct gpio_dt_spec cs_gpio;
};

static const int32_t async_init_delay[ASYNC_INIT_STEP_COUNT] = {
	[ASYNC_INIT_STEP_POWER_UP]         = 1,
	[ASYNC_INIT_STEP_FW_LOAD_START]    = 50,
	[ASYNC_INIT_STEP_FW_LOAD_CONTINUE] = 10,
	[ASYNC_INIT_STEP_FW_LOAD_VERIFY]   = 1,
	[ASYNC_INIT_STEP_CONFIGURE]        = 0,
};


static int pmw3360_async_init_power_up(const struct device *dev);
static int pmw3360_async_init_configure(const struct device *dev);
static int pmw3360_async_init_fw_load_verify(const struct device *dev);
static int pmw3360_async_init_fw_load_continue(const struct device *dev);
static int pmw3360_async_init_fw_load_start(const struct device *dev);

static int (* const async_init_fn[ASYNC_INIT_STEP_COUNT])(const struct device *dev) = {
	[ASYNC_INIT_STEP_POWER_UP] = pmw3360_async_init_power_up,
	[ASYNC_INIT_STEP_FW_LOAD_START] = pmw3360_async_init_fw_load_start,
	[ASYNC_INIT_STEP_FW_LOAD_CONTINUE] = pmw3360_async_init_fw_load_continue,
	[ASYNC_INIT_STEP_FW_LOAD_VERIFY] = pmw3360_async_init_fw_load_verify,
	[ASYNC_INIT_STEP_CONFIGURE] = pmw3360_async_init_configure,
};

static int spi_cs_ctrl(const struct device *dev, bool enable)
{
	const struct pmw3360_config *config = dev->config;
	int err;

	if (!enable) {
		k_busy_wait(T_NCS_SCLK);
	}

	err = gpio_pin_set_dt(&config->cs_gpio, (int)enable);
	if (err) {
		LOG_ERR("SPI CS ctrl failed");
	}

	if (enable) {
		k_busy_wait(T_NCS_SCLK);
	}

	return err;
}

static int reg_read(const struct device *dev, uint8_t reg, uint8_t *buf)
{
	int err;
	struct pmw3360_data *data = dev->data;
	const struct pmw3360_config *config = dev->config;

	__ASSERT_NO_MSG((reg & SPI_WRITE_BIT) == 0);

	err = spi_cs_ctrl(dev, true);
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

	err = spi_write_dt(&config->bus, &tx);
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

	err = spi_read_dt(&config->bus, &rx);
	if (err) {
		LOG_ERR("Reg read failed on SPI read");
		return err;
	}

	err = spi_cs_ctrl(dev, false);
	if (err) {
		return err;
	}

	k_busy_wait(T_SRX);

	data->last_read_burst = false;

	return 0;
}

static int reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	int err;
	struct pmw3360_data *data = dev->data;
	const struct pmw3360_config *config = dev->config;

	__ASSERT_NO_MSG((reg & SPI_WRITE_BIT) == 0);

	err = spi_cs_ctrl(dev, true);
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

	err = spi_write_dt(&config->bus, &tx);
	if (err) {
		LOG_ERR("Reg write failed on SPI write");
		return err;
	}

	k_busy_wait(T_SCLK_NCS_WR);

	err = spi_cs_ctrl(dev, false);
	if (err) {
		return err;
	}

	k_busy_wait(T_SWX);

	data->last_read_burst = false;

	return 0;
}

static int motion_burst_read(const struct device *dev, uint8_t *buf,
			     size_t burst_size)
{
	int err;
	struct pmw3360_data *data = dev->data;
	const struct pmw3360_config *config = dev->config;

	__ASSERT_NO_MSG(burst_size <= PMW3360_MAX_BURST_SIZE);

	/* Write any value to motion burst register only if there have been
	 * other SPI transmissions with sensor since last burst read.
	 */
	if (!data->last_read_burst) {
		err = reg_write(dev, PMW3360_REG_MOTION_BURST, 0x00);
		if (err) {
			return err;
		}
	}

	err = spi_cs_ctrl(dev, true);
	if (err) {
		return err;
	}

	/* Send motion burst address */
	uint8_t reg_buf[] = {
		PMW3360_REG_MOTION_BURST
	};
	const struct spi_buf tx_buf = {
		.buf = reg_buf,
		.len = ARRAY_SIZE(reg_buf)
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	err = spi_write_dt(&config->bus, &tx);
	if (err) {
		LOG_ERR("Motion burst failed on SPI write");
		return err;
	}

	k_busy_wait(T_SRAD_MOTBR);

	const struct spi_buf rx_buf = {
		.buf = buf,
		.len = burst_size,
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	err = spi_read_dt(&config->bus, &rx);
	if (err) {
		LOG_ERR("Motion burst failed on SPI read");
		return err;
	}

	/* Terminate burst */
	err = spi_cs_ctrl(dev, false);
	if (err) {
		return err;
	}

	k_busy_wait(T_BEXIT);

	data->last_read_burst = true;

	return 0;
}

static int burst_write(const struct device *dev, uint8_t reg, const uint8_t *buf,
		       size_t size)
{
	int err;
	struct pmw3360_data *data = dev->data;
	const struct pmw3360_config *config = dev->config;

	/* Write address of burst register */
	uint8_t write_buf = reg | SPI_WRITE_BIT;
	struct spi_buf tx_buf = {
		.buf = &write_buf,
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	err = spi_cs_ctrl(dev, true);
	if (err) {
		return err;
	}

	err = spi_write_dt(&config->bus, &tx);
	if (err) {
		LOG_ERR("Burst write failed on SPI write");
		return err;
	}

	/* Write data */
	for (size_t i = 0; i < size; i++) {
		write_buf = buf[i];

		err = spi_write_dt(&config->bus, &tx);
		if (err) {
			LOG_ERR("Burst write failed on SPI write (data)");
			return err;
		}

		k_busy_wait(T_BRSEP);
	}

	/* Terminate burst mode. */
	err = spi_cs_ctrl(dev, false);
	if (err) {
		return err;
	}

	k_busy_wait(T_BEXIT);

	data->last_read_burst = false;

	return 0;
}

static int update_cpi(const struct device *dev, uint32_t cpi)
{
	/* Set resolution with CPI step of 100 cpi
	 * 0x00: 100 cpi (minimum cpi)
	 * 0x01: 200 cpi
	 * :
	 * 0x31: 5000 cpi (default cpi)
	 * :
	 * 0x77: 12000 cpi (maximum cpi)
	 */

	if ((cpi > PMW3360_MAX_CPI) || (cpi < PMW3360_MIN_CPI)) {
		LOG_ERR("CPI value %u out of range", cpi);
		return -EINVAL;
	}

	/* Convert CPI to register value */
	uint8_t value = (cpi / 100) - 1;

	LOG_DBG("Setting CPI to %u (reg value 0x%x)", cpi, value);

	int err = reg_write(dev, PMW3360_REG_CONFIG1, value);
	if (err) {
		LOG_ERR("Failed to change CPI");
	}

	return err;
}

static int update_downshift_time(const struct device *dev, uint8_t reg_addr,
				 uint32_t time)
{
	/* Set downshift time:
	 * - Run downshift time (from Run to Rest1 mode)
	 * - Rest 1 downshift time (from Rest1 to Rest2 mode)
	 * - Rest 2 downshift time (from Rest2 to Rest3 mode)
	 */
	uint32_t maxtime;
	uint32_t mintime;

	switch (reg_addr) {
	case PMW3360_REG_RUN_DOWNSHIFT:
		/*
		 * Run downshift time = PMW3360_REG_RUN_DOWNSHIFT * 10 ms
		 */
		maxtime = 2550;
		mintime = 10;
		break;

	case PMW3360_REG_REST1_DOWNSHIFT:
		/*
		 * Rest1 downshift time = PMW3360_REG_RUN_DOWNSHIFT
		 *                        * 320 * Rest1 rate (default 1 ms)
		 */
		maxtime = 81600;
		mintime = 320;
		break;

	case PMW3360_REG_REST2_DOWNSHIFT:
		/*
		 * Rest2 downshift time = PMW3360_REG_REST2_DOWNSHIFT
		 *                        * 32 * Rest2 rate (default 100 ms)
		 */
		maxtime = 816000;
		mintime = 3200;
		break;

	default:
		LOG_ERR("Not supported");
		return -ENOTSUP;
	}

	if ((time > maxtime) || (time < mintime)) {
		LOG_WRN("Downshift time %u out of range", time);
		return -EINVAL;
	}

	__ASSERT_NO_MSG((mintime > 0) && (maxtime/mintime <= UINT8_MAX));

	/* Convert time to register value */
	uint8_t value = time / mintime;

	LOG_DBG("Set downshift time to %u ms (reg value 0x%x)", time, value);

	int err = reg_write(dev, reg_addr, value);
	if (err) {
		LOG_ERR("Failed to change downshift time");
	}

	return err;
}

static int update_sample_time(const struct device *dev,
			      uint8_t reg_addr_lower,
			      uint8_t reg_addr_upper,
			      uint32_t sample_time)
{
	/* Set sample time for the Rest1-Rest3 modes.
	 * Values above 0x09B0 will trigger internal watchdog reset.
	 */
	uint32_t maxtime = 0x9B0;
	uint32_t mintime = 1;

	if ((sample_time > maxtime) || (sample_time < mintime)) {
		LOG_WRN("Sample time %u out of range", sample_time);
		return -EINVAL;
	}

	LOG_DBG("Set sample time to %u ms", sample_time);

	/* The sample time is (reg_value + 1) ms. */
	sample_time--;
	uint8_t buf[2];

	sys_put_le16((uint16_t)sample_time, buf);

	int err = reg_write(dev, reg_addr_lower, buf[0]);

	if (!err) {
		err = reg_write(dev, reg_addr_upper, buf[1]);
	} else {
		LOG_ERR("Failed to change sample time");
	}

	return err;
}

static int toggle_rest_modes(const struct device *dev, uint8_t reg_addr,
			     bool enable)
{
	uint8_t value;
	int err = reg_read(dev, reg_addr, &value);

	if (err) {
		LOG_ERR("Failed to read Config2 register");
		return err;
	}

	WRITE_BIT(value, PMW3360_REST_EN_POS, enable);

	LOG_DBG("%sable rest modes", (enable) ? ("En") : ("Dis"));
	err = reg_write(dev, reg_addr, value);

	if (err) {
		LOG_ERR("Failed to set rest mode");
	}

	return err;
}

static int pmw3360_async_init_fw_load_start(const struct device *dev)
{
	int err = 0;

	/* Read from registers 0x02-0x06 regardless of the motion pin state. */
	for (uint8_t reg = 0x02; (reg <= 0x06) && !err; reg++) {
		uint8_t buf[1];
		err = reg_read(dev, reg, buf);
	}

	if (err) {
		LOG_ERR("Cannot read from data registers");
		return err;
	}

	/* Write 0 to Rest_En bit of Config2 register to disable Rest mode. */
	err = reg_write(dev, PMW3360_REG_CONFIG2, 0x00);
	if (err) {
		LOG_ERR("Cannot disable REST mode");
		return err;
	}

	/* Write 0x1D in SROM_enable register to initialize the operation */
	err = reg_write(dev, PMW3360_REG_SROM_ENABLE, 0x1D);
	if (err) {
		LOG_ERR("Cannot initialize SROM");
		return err;
	}

	return err;
}

static int pmw3360_async_init_fw_load_continue(const struct device *dev)
{
	int err;

	LOG_INF("Uploading optical sensor firmware...");

	/* Write 0x18 to SROM_enable to start SROM download */
	err = reg_write(dev, PMW3360_REG_SROM_ENABLE, 0x18);
	if (err) {
		LOG_ERR("Cannot start SROM download");
		return err;
	}

	/* Write SROM file into SROM_Load_Burst register.
	 * Data must start with SROM_Load_Burst address.
	 */
	err = burst_write(dev, PMW3360_REG_SROM_LOAD_BURST,
			  pmw3360_firmware_data, pmw3360_firmware_length);
	if (err) {
		LOG_ERR("Cannot write firmware to sensor");
	}

	return err;
}

static int pmw3360_async_init_fw_load_verify(const struct device *dev)
{
	int err;

	/* Read the SROM_ID register to verify the firmware ID before any
	 * other register reads or writes
	 */

	uint8_t fw_id;
	err = reg_read(dev, PMW3360_REG_SROM_ID, &fw_id);
	if (err) {
		LOG_ERR("Cannot obtain firmware id");
		return err;
	}

	LOG_DBG("Optical chip firmware ID: 0x%x", fw_id);
	if (fw_id != PMW3360_FIRMWARE_ID) {
		LOG_ERR("Chip is not running from SROM!");
		return -EIO;
	}

	uint8_t product_id;
	err = reg_read(dev, PMW3360_REG_PRODUCT_ID, &product_id);
	if (err) {
		LOG_ERR("Cannot obtain product id");
		return err;
	}

	if (product_id != PMW3360_PRODUCT_ID) {
		LOG_ERR("Invalid product id!");
		return -EIO;
	}

	/* Write 0x20 to Config2 register for wireless mouse design.
	 * This enables entering rest modes.
	 */
	err = reg_write(dev, PMW3360_REG_CONFIG2, 0x20);
	if (err) {
		LOG_ERR("Cannot enable REST modes");
	}

	return err;
}

static void irq_handler(const struct device *gpiob, struct gpio_callback *cb,
			uint32_t pins)
{
	int err;
	struct pmw3360_data *data = CONTAINER_OF(cb, struct pmw3360_data,
						 irq_gpio_cb);
	const struct device *dev = data->dev;
	const struct pmw3360_config *config = dev->config;

	err = gpio_pin_interrupt_configure_dt(&config->irq_gpio,
					      GPIO_INT_DISABLE);
	if (unlikely(err)) {
		LOG_ERR("Cannot disable IRQ");
		k_panic();
	}

	k_work_submit(&data->trigger_handler_work);
}

static void trigger_handler(struct k_work *work)
{
	sensor_trigger_handler_t handler;
	int err = 0;
	struct pmw3360_data *data = CONTAINER_OF(work, struct pmw3360_data,
						 trigger_handler_work);
	const struct device *dev = data->dev;
	const struct pmw3360_config *config = dev->config;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	handler = data->data_ready_handler;
	k_spin_unlock(&data->lock, key);

	if (!handler) {
		return;
	}

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	handler(dev, &trig);

	key = k_spin_lock(&data->lock);
	if (data->data_ready_handler) {
		err = gpio_pin_interrupt_configure_dt(&config->irq_gpio,
						      GPIO_INT_LEVEL_ACTIVE);
	}
	k_spin_unlock(&data->lock, key);

	if (unlikely(err)) {
		LOG_ERR("Cannot re-enable IRQ");
		k_panic();
	}
}

static int pmw3360_async_init_power_up(const struct device *dev)
{
	/* Reset sensor */

	return reg_write(dev, PMW3360_REG_POWER_UP_RESET, 0x5A);
}

static int pmw3360_async_init_configure(const struct device *dev)
{
	int err;

	err = update_cpi(dev, CONFIG_PMW3360_CPI);

	if (!err) {
		err = update_downshift_time(dev,
					    PMW3360_REG_RUN_DOWNSHIFT,
					    CONFIG_PMW3360_RUN_DOWNSHIFT_TIME_MS);
	}

	if (!err) {
		err = update_downshift_time(dev,
					    PMW3360_REG_REST1_DOWNSHIFT,
					    CONFIG_PMW3360_REST1_DOWNSHIFT_TIME_MS);
	}

	if (!err) {
		err = update_downshift_time(dev,
					    PMW3360_REG_REST2_DOWNSHIFT,
					    CONFIG_PMW3360_REST2_DOWNSHIFT_TIME_MS);
	}

	return err;
}

static void pmw3360_async_init(struct k_work *work)
{
	struct pmw3360_data *data = CONTAINER_OF(work, struct pmw3360_data,
						 init_work.work);
	const struct device *dev = data->dev;

	LOG_DBG("PMW3360 async init step %d", data->async_init_step);

	data->err = async_init_fn[data->async_init_step](dev);
	if (data->err) {
		LOG_ERR("PMW3360 initialization failed");
	} else {
		data->async_init_step++;

		if (data->async_init_step == ASYNC_INIT_STEP_COUNT) {
			data->ready = true;
			LOG_INF("PMW3360 initialized");
		} else {
			k_work_schedule(&data->init_work,
					K_MSEC(async_init_delay[
						data->async_init_step]));
		}
	}
}

static int pmw3360_init_irq(const struct device *dev)
{
	int err;
	struct pmw3360_data *data = dev->data;
	const struct pmw3360_config *config = dev->config;

	if (!device_is_ready(config->irq_gpio.port)) {
		LOG_ERR("IRQ GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->irq_gpio, GPIO_INPUT);
	if (err) {
		LOG_ERR("Cannot configure IRQ GPIO");
		return err;
	}

	gpio_init_callback(&data->irq_gpio_cb, irq_handler,
			   BIT(config->irq_gpio.pin));

	err = gpio_add_callback(config->irq_gpio.port, &data->irq_gpio_cb);
	if (err) {
		LOG_ERR("Cannot add IRQ GPIO callback");
	}

	return err;
}

static int pmw3360_init(const struct device *dev)
{
	struct pmw3360_data *data = dev->data;
	const struct pmw3360_config *config = dev->config;
	int err;

	data->dev = dev;
	k_work_init(&data->trigger_handler_work, trigger_handler);

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(config->cs_gpio.port)) {
		LOG_ERR("SPI CS device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->cs_gpio, GPIO_OUTPUT_INACTIVE);
	if (err) {
		LOG_ERR("Cannot configure SPI CS GPIO");
		return err;
	}

	err = pmw3360_init_irq(dev);
	if (err) {
		return err;
	}

	k_work_init_delayable(&data->init_work, pmw3360_async_init);

	k_work_schedule(&data->init_work,
			K_MSEC(async_init_delay[data->async_init_step]));

	return err;
}

static int pmw3360_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct pmw3360_data *data = dev->data;
	uint8_t buf[PMW3360_BURST_SIZE];

	if (unlikely(chan != SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	if (unlikely(!data->ready)) {
		LOG_DBG("Device is not initialized yet");
		return -EBUSY;
	}

	int err = motion_burst_read(dev, buf, sizeof(buf));

	if (!err) {
		int16_t x = sys_get_le16(&buf[PMW3360_DX_POS]);
		int16_t y = sys_get_le16(&buf[PMW3360_DY_POS]);

		if (IS_ENABLED(CONFIG_PMW3360_ORIENTATION_0)) {
			data->x = -x;
			data->y = y;
		} else if (IS_ENABLED(CONFIG_PMW3360_ORIENTATION_90)) {
			data->x = y;
			data->y = x;
		} else if (IS_ENABLED(CONFIG_PMW3360_ORIENTATION_180)) {
			data->x = x;
			data->y = -y;
		} else if (IS_ENABLED(CONFIG_PMW3360_ORIENTATION_270)) {
			data->x = -y;
			data->y = -x;
		}
	}

	return err;
}

static int pmw3360_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct pmw3360_data *data = dev->data;

	if (unlikely(!data->ready)) {
		LOG_DBG("Device is not initialized yet");
		return -EBUSY;
	}

	switch (chan) {
	case SENSOR_CHAN_POS_DX:
		val->val1 = data->x;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_POS_DY:
		val->val1 = data->y;
		val->val2 = 0;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int pmw3360_trigger_set(const struct device *dev,
			       const struct sensor_trigger *trig,
			       sensor_trigger_handler_t handler)
{
	struct pmw3360_data *data = dev->data;
	const struct pmw3360_config *config = dev->config;
	int err;

	if (unlikely(trig->type != SENSOR_TRIG_DATA_READY)) {
		return -ENOTSUP;
	}

	if (unlikely(trig->chan != SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	if (unlikely(!data->ready)) {
		LOG_DBG("Device is not initialized yet");
		return -EBUSY;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (handler) {
		err = gpio_pin_interrupt_configure_dt(&config->irq_gpio,
						      GPIO_INT_LEVEL_ACTIVE);
	} else {
		err = gpio_pin_interrupt_configure_dt(&config->irq_gpio,
						      GPIO_INT_DISABLE);
	}

	if (!err) {
		data->data_ready_handler = handler;
	}

	k_spin_unlock(&data->lock, key);

	return err;
}

static int pmw3360_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	struct pmw3360_data *data = dev->data;
	int err;

	if (unlikely(chan != SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	if (unlikely(!data->ready)) {
		LOG_DBG("Device is not initialized yet");
		return -EBUSY;
	}

	switch ((uint32_t)attr) {
	case PMW3360_ATTR_CPI:
		err = update_cpi(dev, PMW3360_SVALUE_TO_CPI(*val));
		break;

	case PMW3360_ATTR_REST_ENABLE:
		err = toggle_rest_modes(dev,
					PMW3360_REG_CONFIG2,
					PMW3360_SVALUE_TO_BOOL(*val));
		break;

	case PMW3360_ATTR_RUN_DOWNSHIFT_TIME:
		err = update_downshift_time(dev,
					    PMW3360_REG_RUN_DOWNSHIFT,
					    PMW3360_SVALUE_TO_TIME(*val));
		break;

	case PMW3360_ATTR_REST1_DOWNSHIFT_TIME:
		err = update_downshift_time(dev,
					    PMW3360_REG_REST1_DOWNSHIFT,
					    PMW3360_SVALUE_TO_TIME(*val));
		break;

	case PMW3360_ATTR_REST2_DOWNSHIFT_TIME:
		err = update_downshift_time(dev,
					    PMW3360_REG_REST2_DOWNSHIFT,
					    PMW3360_SVALUE_TO_TIME(*val));
		break;

	case PMW3360_ATTR_REST1_SAMPLE_TIME:
		err = update_sample_time(dev,
					 PMW3360_REG_REST1_RATE_LOWER,
					 PMW3360_REG_REST1_RATE_UPPER,
					 PMW3360_SVALUE_TO_TIME(*val));
		break;

	case PMW3360_ATTR_REST2_SAMPLE_TIME:
		err = update_sample_time(dev,
					 PMW3360_REG_REST2_RATE_LOWER,
					 PMW3360_REG_REST2_RATE_UPPER,
					 PMW3360_SVALUE_TO_TIME(*val));
		break;

	case PMW3360_ATTR_REST3_SAMPLE_TIME:
		err = update_sample_time(dev,
					 PMW3360_REG_REST3_RATE_LOWER,
					 PMW3360_REG_REST3_RATE_UPPER,
					 PMW3360_SVALUE_TO_TIME(*val));
		break;

	default:
		LOG_ERR("Unknown attribute");
		return -ENOTSUP;
	}

	return err;
}

static const struct sensor_driver_api pmw3360_driver_api = {
	.sample_fetch = pmw3360_sample_fetch,
	.channel_get  = pmw3360_channel_get,
	.trigger_set  = pmw3360_trigger_set,
	.attr_set     = pmw3360_attr_set,
};

#define PMW3360_DEFINE(n)						       \
	static struct pmw3360_data data##n;				       \
									       \
	static const struct pmw3360_config config##n = {		       \
		.irq_gpio = GPIO_DT_SPEC_INST_GET(n, irq_gpios),	       \
		.bus = {						       \
			.bus = DEVICE_DT_GET(DT_INST_BUS(n)),		       \
			.config = {					       \
				.frequency = DT_INST_PROP(n,		       \
							  spi_max_frequency),  \
				.operation = SPI_WORD_SET(8) |		       \
					     SPI_TRANSFER_MSB |		       \
					     SPI_MODE_CPOL | SPI_MODE_CPHA,    \
				.slave = DT_INST_REG_ADDR(n),		       \
			},						       \
		},							       \
		.cs_gpio = SPI_CS_GPIOS_DT_SPEC_GET(DT_DRV_INST(n)),	       \
	};								       \
									       \
	DEVICE_DT_INST_DEFINE(n, pmw3360_init, NULL, &data##n, &config##n,     \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,	       \
			      &pmw3360_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PMW3360_DEFINE)
