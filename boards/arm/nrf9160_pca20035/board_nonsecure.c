/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */


#include <init.h>
#include <hal/nrf_clock.h>
#include <nrfx.h>
#include <gpio.h>
#include <drivers/adp536x.h>

#if defined(CONFIG_BSD_LIBRARY) && defined(CONFIG_NET_SOCKETS_OFFLOAD)
#include <net/socket.h>
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(board_nonsecure, CONFIG_BOARD_LOG_LEVEL);

#define ADP536X_I2C_DEV_NAME	DT_NORDIC_NRF_I2C_I2C_2_LABEL
#define LC_MAX_READ_LENGTH	128

#define AT_CMD_MAGPIO		"AT%XMAGPIO=1,1,1,450,451,746,803,698,748," \
				"824,894,880,960,1710,2200,791,849,1574,1577"
#define AT_CMD_MAGPIO_LEN	sizeof (AT_CMD_MAGPIO) - 1

#ifdef CONFIG_BOARD_NRF9160_PCA20035_V0_2_2NS
#define POWER_CTRL_1V8_PIN	3
#define POWER_CTRL_3V3_PIN	28

static struct device *gpio_dev;
#endif

static int pca20035_magpio_configure(void)
{
#if defined(CONFIG_BSD_LIBRARY) && \
defined(CONFIG_NET_SOCKETS_OFFLOAD)
	int at_socket_fd;
	int buffer;
	u8_t read_buffer[LC_MAX_READ_LENGTH];

	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (at_socket_fd == -1) {
		LOG_ERR("AT socket could not be opened");
		return -EFAULT;
	}

	LOG_DBG("AT CMD: %s", AT_CMD_MAGPIO);
	buffer = send(at_socket_fd, AT_CMD_MAGPIO, AT_CMD_MAGPIO_LEN, 0);
	if (buffer != AT_CMD_MAGPIO_LEN) {
		LOG_ERR("MAGPIO command failed");
		close(at_socket_fd);
		return -EIO;
	}

	buffer = recv(at_socket_fd, read_buffer, LC_MAX_READ_LENGTH, 0);
	LOG_DBG("AT RESP: %s", read_buffer);
	if ((buffer < 2) ||
	    (memcmp("OK", read_buffer, 2 != 0))) {
		LOG_ERR("MAGPIO command failed");
		close(at_socket_fd);
		return -EIO;
	}

	LOG_DBG("MAGPIO successfully configured");

	close(at_socket_fd);
#endif
	return 0;
}

#ifdef CONFIG_BOARD_NRF9160_PCA20035_V0_2_2NS
int pca20035_power_1v8_set(bool enable)
{
	if (!gpio_dev) {
		return -ENODEV;
	}

	return gpio_pin_write(gpio_dev, POWER_CTRL_1V8_PIN, enable);
}

int pca20035_power_3v3_set(bool enable)
{
	if (!gpio_dev) {
		return -ENODEV;
	}

	return gpio_pin_write(gpio_dev, POWER_CTRL_3V3_PIN, enable);
}

static int pca20035_power_ctrl_pins_init(void)
{
	int err;

	gpio_dev = device_get_binding(DT_GPIO_P0_DEV_NAME);
	if (!gpio_dev) {
		return -ENODEV;
	}

	err = gpio_pin_configure(gpio_dev, POWER_CTRL_1V8_PIN, GPIO_DIR_OUT);
	if (err) {
		return err;
	}

	err = gpio_pin_configure(gpio_dev, POWER_CTRL_3V3_PIN, GPIO_DIR_OUT);
	if (err) {
		return err;
	}

	return 0;
}
#endif

int leds_init(void)
{
	int err;
	struct device *gpio_dev;

	gpio_dev = device_get_binding(LED0_GPIO_CONTROLLER);
	if (gpio_dev == NULL) {
		LOG_ERR("Could not get binding to LED GPIO controller");
		return -ENODEV;
	}

	err = gpio_pin_configure(gpio_dev, LED0_GPIO_PIN,
			   GPIO_DIR_OUT);
	if (err) {
		LOG_ERR("gpio_pin_configure() failed with error: %d", err);
		return err;
	}
	gpio_pin_write(gpio_dev, LED0_GPIO_PIN, 0);

	err = gpio_pin_configure(gpio_dev, LED1_GPIO_PIN,
				 GPIO_DIR_OUT);
	if (err) {
		LOG_ERR("gpio_pin_configure() failed with error: %d", err);
		return err;
	}
	gpio_pin_write(gpio_dev, LED1_GPIO_PIN, 0);

	err = gpio_pin_configure(gpio_dev, LED2_GPIO_PIN,
				 GPIO_DIR_OUT);
	if (err) {
		LOG_ERR("gpio_pin_configure() failed with error: %d", err);
		return err;
	}
	gpio_pin_write(gpio_dev, LED2_GPIO_PIN, 0);

	err = gpio_pin_configure(gpio_dev, DT_GPIO_LEDS_SENSE_LED0_GPIO_PIN,
				 GPIO_DIR_OUT);
	if (err) {
		LOG_ERR("gpio_pin_configure() failed with error: %d", err);
		return err;
	}
	gpio_pin_write(gpio_dev, DT_GPIO_LEDS_SENSE_LED0_GPIO_PIN, 0);

	err = gpio_pin_configure(gpio_dev, DT_GPIO_LEDS_SENSE_LED1_GPIO_PIN,
				 GPIO_DIR_OUT);
	if (err) {
		LOG_ERR("gpio_pin_configure() failed with error: %d", err);
		return err;
	}
	gpio_pin_write(gpio_dev, DT_GPIO_LEDS_SENSE_LED1_GPIO_PIN, 0);

	err = gpio_pin_configure(gpio_dev, DT_GPIO_LEDS_SENSE_LED2_GPIO_PIN,
				 GPIO_DIR_OUT);
	if (err) {
		LOG_ERR("gpio_pin_configure() failed with error: %d", err);
		return err;
	}
	gpio_pin_write(gpio_dev, DT_GPIO_LEDS_SENSE_LED2_GPIO_PIN, 0);

	LOG_DBG("LEDs initialized\n");

	return 0;
}

static int power_mgmt_init(void)
{
	int err;

	err = leds_init();
	if (err) {
		LOG_ERR("Could not configure LEDs, error %d", err);
		return err;
	}

	err = adp536x_init(ADP536X_I2C_DEV_NAME);
	if (err) {
		return err;
	}

	err = adp536x_buck_1v8_set();
	if (err) {
		return err;
	}

	err = adp536x_buckbst_3v3_set();
	if (err) {
		return err;
	}

	err = adp536x_buckbst_enable(true);
	if (err) {
		return err;
	}

	/* The value 0x07 sets VBUS current limit to 500 mA. */
	err = adp536x_vbus_current_set(0x07);
	if (err) {
		return err;
	}

	/* The value 0x1F corresponds to 320 mA charging current. */
	err = adp536x_charger_current_set(0x1F);
	if (err) {
		return err;
	}

	/* The value 0x07 corresponds to a 400 mA peak charge current. */
	err = adp536x_oc_chg_current_set(0x07);
	if (err) {
		return err;
	}

	err = adp536x_charging_enable(true);
	if (err) {
		return err;
	}

	return 0;
}

static int pca20035_board_init(struct device *dev)
{
	int err;

#ifdef CONFIG_BOARD_NRF9160_PCA20035_V0_2_2NS
	err = pca20035_power_ctrl_pins_init();
	if (err) {
		LOG_ERR("pca20035_power_ctrl_pins_init: failed! %d", err);
		return err;
	}
#endif

	err = power_mgmt_init();
	if (err) {
		LOG_ERR("power_mgmt_init: failed! %d", err);
		return err;
	}

#ifdef CONFIG_BOARD_NRF9160_PCA20035_V0_2_2NS
	err = pca20035_power_1v8_set(true);
	if (err) {
		LOG_ERR("pca20035_power_1v8_set: failed! %d", err);
		return err;
	}

	err = pca20035_power_3v3_set(true);
	if (err) {
		LOG_ERR("pca20035_power_3v3_set: failed! %d", err);
		return err;
	}
#endif

	err = pca20035_magpio_configure();
	if (err) {
		LOG_ERR("pca20035_magpio_configure: failed! %d", err);
		return err;
	}

	err = adp536x_oc_chg_hiccup_set(true);
	if (err) {
		return err;
	}


	return 0;
}

SYS_INIT(pca20035_board_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
