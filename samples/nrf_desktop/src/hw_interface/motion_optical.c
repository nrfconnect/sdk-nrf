/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <atomic.h>

#include <soc.h>
#include <device.h>
#include <spi.h>
#include <gpio.h>

#include "event_manager.h"
#include "motion_event.h"
#include "power_event.h"
#include "hid_event.h"
#include "config_event.h"

#define MODULE motion
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_MOTION_LOG_LEVEL);


/* Use timings defined by spec */
#define CORRECT_TIMINGS 1
#if CORRECT_TIMINGS
#define T_NCS_SCLK	1			/* 120 ns */
#define T_SRX		(20 - T_NCS_SCLK)	/* 20 us */
#define T_SCLK_NCS_WR	(35 - T_NCS_SCLK)	/* 35 us */
#define T_SWX		(180 - T_SCLK_NCS_WR)	/* 180 us */
#define T_SRAD		160			/* 160 us */
#define T_SRAD_MOTBR	35			/* 35 us */
#define T_BEXIT		1			/* 500 ns */
#else
#define T_NCS_SCLK	0
#define T_SRX		19
#define T_SCLK_NCS_WR	10
#define T_SWX		50
#define T_SRAD		160
#define T_SRAD_MOTBR	20
#define T_BEXIT		0
#endif


/* Sensor registers */
#define OPTICAL_REG_PRODUCT_ID			0x00
#define OPTICAL_REG_REVISION_ID			0x01
#define OPTICAL_REG_MOTION			0x02
#define OPTICAL_REG_DELTA_X_L			0x03
#define OPTICAL_REG_DELTA_X_H			0x04
#define OPTICAL_REG_DELTA_Y_L			0x05
#define OPTICAL_REG_DELTA_Y_H			0x06
#define OPTICAL_REG_SQUAL			0x07
#define OPTICAL_REG_RAW_DATA_SUM		0x08
#define OPTICAL_REG_MAXIMUM_RAW_DATA		0x09
#define OPTICAL_REG_MINIMUM_RAW_DATA		0x0A
#define OPTICAL_REG_SHUTTER_LOWER		0x0B
#define OPTICAL_REG_SHUTTER_UPPER		0x0C
#define OPTICAL_REG_CONTROL			0x0D
#define OPTICAL_REG_CONFIG1			0x0F
#define OPTICAL_REG_CONFIG2			0x10
#define OPTICAL_REG_ANGLE_TUNE			0x11
#define OPTICAL_REG_FRAME_CAPTURE		0x12
#define OPTICAL_REG_SROM_ENABLE			0x13
#define OPTICAL_REG_RUN_DOWNSHIFT		0x14
#define OPTICAL_REG_REST1_RATE_LOWER		0x15
#define OPTICAL_REG_REST1_RATE_UPPER		0x16
#define OPTICAL_REG_REST1_DOWNSHIFT		0x17
#define OPTICAL_REG_REST2_RATE_LOWER		0x18
#define OPTICAL_REG_REST2_RATE_UPPER		0x19
#define OPTICAL_REG_REST2_DOWNSHIFT		0x1A
#define OPTICAL_REG_REST3_RATE_LOWER		0x1B
#define OPTICAL_REG_REST3_RATE_UPPER		0x1C
#define OPTICAL_REG_OBSERVATION			0x24
#define OPTICAL_REG_DATA_OUT_LOWER		0x25
#define OPTICAL_REG_DATA_OUT_UPPER		0x26
#define OPTICAL_REG_RAW_DATA_DUMP		0x29
#define OPTICAL_REG_SROM_ID			0x2A
#define OPTICAL_REG_MIN_SQ_RUN			0x2B
#define OPTICAL_REG_RAW_DATA_THRESHOLD		0x2C
#define OPTICAL_REG_CONFIG5			0x2F
#define OPTICAL_REG_POWER_UP_RESET		0x3A
#define OPTICAL_REG_SHUTDOWN			0x3B
#define OPTICAL_REG_INVERSE_PRODUCT_ID		0x3F
#define OPTICAL_REG_LIFTCUTOFF_TUNE3		0x41
#define OPTICAL_REG_ANGLE_SNAP			0x42
#define OPTICAL_REG_LIFTCUTOFF_TUNE1		0x4A
#define OPTICAL_REG_MOTION_BURST		0x50
#define OPTICAL_REG_LIFTCUTOFF_TUNE_TIMEOUT	0x58
#define OPTICAL_REG_LIFTCUTOFF_TUNE_MIN_LENGTH	0x5A
#define OPTICAL_REG_SROM_LOAD_BURST		0x62
#define OPTICAL_REG_LIFT_CONFIG			0x63
#define OPTICAL_REG_RAW_DATA_BURST		0x64
#define OPTICAL_REG_LIFTCUTOFF_TUNE2		0x65

/* Sensor pin configuration */
#define OPTICAL_PIN_PWR_CTRL			14
#define OPTICAL_PIN_MOTION			18
#define OPTICAL_PIN_CS				13

/* Sensor initialization values */
#define OPTICAL_PRODUCT_ID			0x42
#define OPTICAL_FIRMWARE_ID			0x04

#define SPI_WRITE_MASK				0x80

/* Max register count readable in a single motion burst */
#define OPTICAL_MAX_BURST_SIZE			12

/* Register count used for reading a single motion burst */
#define OPTICAL_BURST_SIZE			6

#define OPTICAL_MAX_CPI				12000
#define OPTICAL_MIN_CPI				100

/* Sampling thread poll timeout */
#define OPTICAL_POLL_TIMEOUT_MS			500

#define OPTICAL_THREAD_STACK_SIZE		512
#define OPTICAL_THREAD_PRIORITY			K_PRIO_PREEMPT(0)

#define NODATA_LIMIT				10

extern const u16_t firmware_length;
extern const u8_t firmware_data[];


enum {
	STATE_IDLE,
	STATE_FETCHING,
	STATE_SUSPENDING,
	STATE_SUSPENDED,
};


static const struct spi_config spi_cfg = {
	.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB |
		     SPI_MODE_CPOL | SPI_MODE_CPHA,
	.frequency = CONFIG_DESKTOP_SPI_FREQ_HZ,
	.slave = 0,
};


static K_SEM_DEFINE(sem, 1, 1);
static K_THREAD_STACK_DEFINE(thread_stack, OPTICAL_THREAD_STACK_SIZE);
static struct k_thread thread;

static struct gpio_callback gpio_cb;

static struct device *gpio_dev;
static struct device *spi_dev;

static atomic_t state;
static atomic_t connected;
static bool last_read_burst;

static atomic_t sensor_cpi;

static int spi_cs_ctrl(bool enable)
{
	if (!enable) {
		k_busy_wait(T_NCS_SCLK);
	}

	u32_t val = (enable) ? (0) : (1);
	int err = gpio_pin_write(gpio_dev, OPTICAL_PIN_CS, val);
	if (err) {
		LOG_ERR("SPI CS ctrl failed");
	}

	if (enable) {
		k_busy_wait(T_NCS_SCLK);
	}

	return err;
}

static int reg_read(u8_t reg, u8_t *buf)
{
	int err;

	__ASSERT_NO_MSG((reg & SPI_WRITE_MASK) == 0);
	last_read_burst = false;

	err = spi_cs_ctrl(true);
	if (err) {
		goto error;
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

	err = spi_transceive(spi_dev, &spi_cfg, &tx, NULL);
	if (err) {
		goto error;
	}

	k_busy_wait(T_SRAD);

	/* Read register value. */
	struct spi_buf rx_buf = {
		.buf = buf,
		.len = 1,
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	err = spi_transceive(spi_dev, &spi_cfg, NULL, &rx);
	if (err) {
		goto error;
	}

	err = spi_cs_ctrl(false);
	if (err) {
		goto error;
	}
	k_busy_wait(T_SRX);

	return 0;

error:
	LOG_ERR("SPI reg read failed");

	return err;
}

static int reg_write(u8_t reg, u8_t val)
{
	int err;

	__ASSERT_NO_MSG((reg & SPI_WRITE_MASK) == 0);
	last_read_burst = false;

	err = spi_cs_ctrl(true);
	if (err) {
		goto error;
	}

	u8_t buf[] = {
		SPI_WRITE_MASK | reg,
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

	err = spi_transceive(spi_dev, &spi_cfg, &tx, NULL);
	if (err) {
		goto error;
	}

	k_busy_wait(T_SCLK_NCS_WR);
	err = spi_cs_ctrl(false);
	if (err) {
		goto error;
	}
	k_busy_wait(T_SWX);

	return 0;

error:
	LOG_ERR("SPI reg write failed");

	return err;
}

static int motion_burst_read(u8_t *data, size_t burst_size)
{
	int err;

	__ASSERT_NO_MSG(burst_size <= OPTICAL_MAX_BURST_SIZE);

	/* Write any value to motion burst register only if there have been
	 * other SPI transmissions with sensor since last burst read.
	 */
	if (!last_read_burst) {
		err = reg_write(OPTICAL_REG_MOTION_BURST, 0x00);
		if (err) {
			goto error;
		}
		last_read_burst = true;
	}

	err = spi_cs_ctrl(true);
	if (err) {
		goto error;
	}

	/* Send motion burst address */
	u8_t reg_buf[] = {
		OPTICAL_REG_MOTION_BURST
	};
	const struct spi_buf tx_buf = {
		.buf = reg_buf,
		.len = ARRAY_SIZE(reg_buf)
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	err = spi_transceive(spi_dev, &spi_cfg, &tx, NULL);
	if (err) {
		goto error;
	}

	k_busy_wait(T_SRAD_MOTBR);

	/* Read up to 12 bytes in burst */
	const struct spi_buf rx_buf = {
		.buf = data,
		.len = burst_size,
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	err = spi_transceive(spi_dev, &spi_cfg, NULL, &rx);
	if (err) {
		goto error;
	}

	/* Terminate burst */
	err = spi_cs_ctrl(false);
	if (err) {
		goto error;
	}
	k_busy_wait(T_BEXIT);

	return 0;

error:
	LOG_ERR("SPI burst write failed");

	return err;
}

static int burst_write(u8_t reg, const u8_t *buf, size_t size)
{
	int err;

	last_read_burst = false;
	err = spi_cs_ctrl(true);
	if (err) {
		goto error;
	}

	/* Write address of burst register */
	u8_t write_buf = reg | SPI_WRITE_MASK;
	struct spi_buf tx_buf = {
		.buf = &write_buf,
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	err = spi_transceive(spi_dev, &spi_cfg, &tx, NULL);
	if (err) {
		goto error;
	}

	/* Write data */
	for (size_t i = 0; i < size; i++) {
		write_buf = buf[i];

		err = spi_transceive(spi_dev, &spi_cfg, &tx, NULL);
		if (err) {
			goto error;
		}

		k_busy_wait(15);
	}

	/* Terminate burst mode. */
	err = spi_cs_ctrl(false);
	if (err) {
		goto error;
	}

	k_busy_wait(T_BEXIT);

	return 0;

error:
	LOG_ERR("SPI reg write failed");

	return err;
}

static void update_cpi(u16_t cpi)
{
	/* Set resolution with CPI step of 100 cpi
	 * 0x00: 100 cpi (minimum cpi)
	 * 0x01: 200 cpi
	 * :
	 * 0x31: 5000 cpi (default cpi)
	 * :
	 * 0x77: 12000 cpi (maximum cpi)
	 */

	if ((cpi > OPTICAL_MAX_CPI) || (cpi < OPTICAL_MIN_CPI)) {
		LOG_WRN("CPI value out of range");
		return;
	}

	/* Convert CPI to register value */
	u8_t value = (cpi / 100) - 1;

	LOG_INF("Setting CPI to %u (reg value 0x%x)", cpi, value);

	int err = reg_write(OPTICAL_REG_CONFIG1, value);
	if (err) {
		LOG_ERR("Failed to change CPI");
		module_set_state(MODULE_STATE_ERROR);
	}
}

static int firmware_load(void)
{
	int err;

	LOG_INF("Uploading optical sensor firmware...");

	/* Write 0 to Rest_En bit of Config2 register to disable Rest mode */
	err = reg_write(OPTICAL_REG_CONFIG2, 0x00);
	if (err) {
		goto error;
	}

	/* Write 0x1D in SROM_enable register to initialize the operation */
	err = reg_write(OPTICAL_REG_SROM_ENABLE, 0x1D);
	if (err) {
		goto error;
	}

	k_sleep(10);

	/* Write 0x18 to SROM_enable to start SROM download */
	err = reg_write(OPTICAL_REG_SROM_ENABLE, 0x18);
	if (err) {
		goto error;
	}

	/* Write SROM file into SROM_Load_Burst register.
	 * Data must start with SROM_Load_Burst address.
	 */
	err = burst_write(OPTICAL_REG_SROM_LOAD_BURST, firmware_data,
			firmware_length);
	if (err) {
		LOG_ERR("Loading firmware to optical sensor failed!");
		goto error;
	}

	/* Read the SROM_ID register to verify the firmware ID before any
	 * other register reads or writes
	 */
	k_busy_wait(200);
	u8_t id;
	err = reg_read(OPTICAL_REG_SROM_ID, &id);
	if (err) {
		goto error;
	}

	LOG_DBG("Optical chip firmware ID: 0x%x", id);
	if (id != OPTICAL_FIRMWARE_ID) {
		LOG_ERR("Chip is not running from SROM!");
		err = -EIO;
		goto error;
	}

	/* Write 0x00 to Config2 register for wired mouse or 0x20 for
	 * wireless mouse design.
	 */
	err = reg_write(OPTICAL_REG_CONFIG2, 0x20);
	if (err) {
		goto error;
	}

	return 0;

error:
	LOG_ERR("SPI firmware load failed");

	return err;
}

static void irq_handler(struct device *gpiob, struct gpio_callback *cb,
			u32_t pins)
{
	/* Enter active polling mode until no motion */
	gpio_pin_disable_callback(gpio_dev, OPTICAL_PIN_MOTION);

	if (atomic_cas(&state, STATE_IDLE, STATE_FETCHING)) {
		/* Wake up thread */
		k_sem_give(&sem);
	} else if (atomic_get(&state) == STATE_SUSPENDED) {
		/* Wake up system - this will wake up thread */
		struct wake_up_event *event = new_wake_up_event();

		if (event) {
			EVENT_SUBMIT(event);
		}
	} else {
		/* Ignore when fetching or suspending */
	}
}

static int motion_read(void)
{
	u8_t data[OPTICAL_BURST_SIZE];

	int err = motion_burst_read(data, sizeof(data));
	if (err) {
		return err;
	}

	static unsigned int nodata;
	if (!data[2] && !data[3] && !data[4] && !data[5]) {
		if (nodata < NODATA_LIMIT) {
			nodata++;
		} else {
			nodata = 0;

			return -ENODATA;
		}
	} else {
		nodata = 0;
	}

	struct motion_event *event = new_motion_event();

	if (event) {
		event->dx =  ((s16_t)(data[5] << 8) | data[4]);
		event->dy = -((s16_t)(data[3] << 8) | data[2]);

		EVENT_SUBMIT(event);
	} else {
		LOG_ERR("No memory");
		err = -ENOMEM;
	}

	return err;
}

static int init(u16_t cpi)
{
	int err = -ENXIO;

	/* Obtain bindings */
	gpio_dev = device_get_binding(DT_GPIO_P0_DEV_NAME);
	if (!gpio_dev) {
		LOG_ERR("Cannot get GPIO device");
		goto error;
	}

	spi_dev = device_get_binding(DT_SPI_1_NAME);
	if (!spi_dev) {
		LOG_ERR("Cannot get SPI device");
		goto error;
	}

	/* Power on the optical sensor */
	err = gpio_pin_configure(gpio_dev, OPTICAL_PIN_PWR_CTRL, GPIO_DIR_OUT);
	if (err) {
		goto error;
	}

	err = gpio_pin_write(gpio_dev, OPTICAL_PIN_PWR_CTRL, 1);
	if (err) {
		goto error;
	}

	/* Wait for VCC to be stable */
	k_sleep(1);

	/* Setup SPI CS */
	err = gpio_pin_configure(gpio_dev, OPTICAL_PIN_CS, GPIO_DIR_OUT);
	if (err) {
		goto error;
	}

	spi_cs_ctrl(false);

	/* Reset sensor */
	err = reg_write(OPTICAL_REG_POWER_UP_RESET, 0x5A);
	if (err) {
		goto error;
	}
	k_sleep(50);

	/* Read from registers 0x02-0x06 regardless of the motion pin state. */
	for (u8_t reg = 0x02; reg <= 0x06; reg++) {
		u8_t buf[1];
		err = reg_read(reg, buf);
		if (err) {
			goto error;
		}
	}

	/* Perform SROM download. */
	err = firmware_load();
	if (err) {
		goto error;
	}

	/* Set initial CPI resolution. */
	update_cpi(cpi);
	if (err) {
		goto error;
	}

	/* Verify product id */
	u8_t product_id;
	err = reg_read(OPTICAL_REG_PRODUCT_ID, &product_id);
	if (err || (product_id != OPTICAL_PRODUCT_ID)) {
		LOG_ERR("Wrong product ID");
		goto error;
	}

	/* Enable interrupt from motion pin falling edge. */
	err = gpio_pin_configure(gpio_dev, OPTICAL_PIN_MOTION,
				 GPIO_DIR_IN | GPIO_INT | GPIO_PUD_PULL_UP |
				 GPIO_INT_LEVEL | GPIO_INT_ACTIVE_LOW);
	if (err) {
		LOG_ERR("Failed to confiugure GPIO");
		goto error;
	}

	gpio_init_callback(&gpio_cb, irq_handler, BIT(OPTICAL_PIN_MOTION));
	err = gpio_add_callback(gpio_dev, &gpio_cb);
	if (err) {
		LOG_ERR("Cannot add GPIO callback");
		goto error;
	}

	err = gpio_pin_enable_callback(gpio_dev, OPTICAL_PIN_MOTION);
	if (err) {
		LOG_ERR("Cannot enable GPIO interrupt");
		goto error;
	}

	/* Inform all that module is ready */
	module_set_state(MODULE_STATE_READY);

	return 0;

error:
	module_set_state(MODULE_STATE_ERROR);

	return err;
}

/* Optical hardware interface state machine. */
static void optical_thread_fn(void)
{
	s32_t timeout = K_FOREVER;
	u16_t cpi = CONFIG_DESKTOP_OPTICAL_DEFAULT_CPI;

	atomic_set(&state, STATE_IDLE);

	int err = init(cpi);

	while (!err) {
		err = k_sem_take(&sem, timeout);

		switch (atomic_get(&state)) {
		case STATE_IDLE:
			timeout = K_FOREVER;
			break;

		case STATE_FETCHING:
			if (!atomic_get(&connected)) {
				/* HID notification was disabled. */
				/* Ignore possible timeout. */
				err = -ENODATA;
			}
			if (err == 0) {
				err = motion_read();
			}
			if ((err == -ENODATA) || (err == -EAGAIN)) {
				/* No motion or timeout. */
				LOG_DBG("Stop polling, wait for interrupt");

				/* Read data to clear interrupt. */
				u8_t data[OPTICAL_BURST_SIZE];
				err = motion_burst_read(data, sizeof(data));

				/* Switch state. */
				atomic_cas(&state, STATE_FETCHING,
						STATE_IDLE);
				gpio_pin_enable_callback(gpio_dev,
						OPTICAL_PIN_MOTION);
				timeout = K_FOREVER;
			} else {
				timeout = K_MSEC(OPTICAL_POLL_TIMEOUT_MS);
			}
			break;

		case STATE_SUSPENDING:
			__ASSERT_NO_MSG(!err || (err == -EAGAIN));
			err = 0; /* Ignore possible timeout. */
			atomic_set(&state, STATE_SUSPENDED);
			gpio_pin_enable_callback(gpio_dev, OPTICAL_PIN_MOTION);
			module_set_state(MODULE_STATE_STANDBY);
			timeout = K_FOREVER;
			break;

		case STATE_SUSPENDED:
			/* Sensor will downshift to "rest modes" when inactive.
			 * Interrupt is left enabled to wake system up.
			 */
			break;

		default:
			__ASSERT_NO_MSG(false);
			break;
		}

		if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
			u16_t new_cpi = atomic_get(&sensor_cpi);
			if (new_cpi != cpi) {
				cpi = new_cpi;
				update_cpi(cpi);
			}
		}

	}
	/* This thread is supposed to run forever. */
	__ASSERT_NO_MSG(false);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_hid_report_sent_event(eh)) {
		const struct hid_report_sent_event *event =
			cast_hid_report_sent_event(eh);

		if ((event->report_type == TARGET_REPORT_MOUSE) &&
		    (atomic_get(&state) == STATE_FETCHING)) {
			k_sem_give(&sem);
		}

		return false;
	}

	if (is_hid_report_subscription_event(eh)) {
		const struct hid_report_subscription_event *event =
			cast_hid_report_subscription_event(eh);

		if (event->report_type == TARGET_REPORT_MOUSE) {
			static u8_t peer_count;

			if (event->enabled) {
				__ASSERT_NO_MSG(peer_count < UCHAR_MAX);
				peer_count++;
			} else {
				__ASSERT_NO_MSG(peer_count > 0);
				peer_count--;
			}

			bool new_state = (peer_count != 0);
			bool old_state = atomic_set(&connected, new_state);

			if (old_state != new_state) {
				k_sem_give(&sem);
			}
		}

		return false;
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			/* Start state machine thread */
			k_thread_create(&thread, thread_stack,
					OPTICAL_THREAD_STACK_SIZE,
					(k_thread_entry_t)optical_thread_fn,
					NULL, NULL, NULL,
					OPTICAL_THREAD_PRIORITY, 0, K_NO_WAIT);

			return false;
		}

		return false;
	}

	if (is_wake_up_event(eh)) {
		if (atomic_cas(&state, STATE_SUSPENDED, STATE_FETCHING)) {
			module_set_state(MODULE_STATE_READY);
			k_sem_give(&sem);
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		bool suspended = false;
		switch (atomic_get(&state)) {
		case STATE_SUSPENDED:
			suspended = true;
			break;

		case STATE_SUSPENDING:
			/* No action */
			break;

		default:
			atomic_set(&state, STATE_SUSPENDING);
			k_sem_give(&sem);
			break;
		}

		return !suspended;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		if (is_config_event(eh)) {
			const struct config_event *event = cast_config_event(eh);

			if (event->id == CONFIG_EVENT_ID_MOUSE_CPI) {
				u16_t new_cpi = event->data[0] + (event->data[1] << 8);
				atomic_set(&sensor_cpi, new_cpi);
			}

			return false;
		}
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
EVENT_SUBSCRIBE(MODULE, hid_report_sent_event);
EVENT_SUBSCRIBE(MODULE, hid_report_subscription_event);
EVENT_SUBSCRIBE(MODULE, config_event);
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
