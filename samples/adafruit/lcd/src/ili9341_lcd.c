/*
 * Copyright (c) 2015 - 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <device.h>
#include <gpio.h>
#include <spi.h>
#include <string.h>
#include "Adafruit_ILI9341.h"
#include "ili9341_lcd.h"

#if defined(CONFIG_BOARD_NRF52840_PCA10056)
#define GPIO_PORT				DT_GPIO_P1_DEV_NAME
#else
#define GPIO_PORT				DT_GPIO_P0_DEV_NAME
#endif
#define SPI_PORT				DT_SPI_0_NAME

/* Adafruit LCD pin assignments */
#if defined(CONFIG_BOARD_NRF52840_PCA10056)
#define N_RESET     9
#define TFT_CS      12
#define TFT_DC      11
#define TFT_SCK     15
#define TFT_MOSI    13
#define TFT_MISO    14
#elif defined(CONFIG_BOARD_NRF9160_PCA10090)
#define N_RESET     24
#define TFT_CS      10
#define TFT_DC      9
#define TFT_SCK     13
#define TFT_MOSI    11
#define TFT_MISO    12
#else
#define N_RESET     21
#define TFT_CS      22
#define TFT_DC      20
#define TFT_SCK     25
#define TFT_MOSI    23
#define TFT_MISO    24
#endif

static struct device		*gpio_port;
static struct device		*spi_port;
static struct spi_config	spi_config;

static u8_t m_tx_data[ILI9341_LCD_WIDTH * 2];

/**@brief Function to wait microseconds.
 */
static void wait_us(u16_t t)
{
	k_busy_wait(t);
}

/**@brief Function to wait milliseconds.
 */
static void wait_ms(u16_t t)
{
	k_sleep(t);
}

/**@brief Function to pull down LCD \RESET pin.
 */
static int lcd_nreset_down(void)
{
	return gpio_pin_write(gpio_port, N_RESET, 0);
}

/**@brief Function to pull up LCD \RESET pin.
 */
static int lcd_nreset_up(void)
{
	return gpio_pin_write(gpio_port, N_RESET, 1);
}

/**@brief Function to pull down LCD CS pin.
 */
static int lcd_cs_down(void)
{
	return gpio_pin_write(gpio_port, TFT_CS, 0);
}

/**@brief Function to pull up LCD CS pin.
 */
static int lcd_cs_up(void)
{
	return gpio_pin_write(gpio_port, TFT_CS, 1);
}

/**@brief Function to pull down LCD D/C pin.
 */
static int lcd_dc_down(void)
{
	return gpio_pin_write(gpio_port, TFT_DC, 0);
}

/**@brief Function to pull up LCD D/C pin.
 */
static int lcd_dc_up(void)
{
	return gpio_pin_write(gpio_port, TFT_DC, 1);
}

/**@brief Function to send a LCD command byte by SPI.
 */
static void wr_cmd(u8_t cmd)
{
	lcd_dc_down();
	lcd_cs_down();
	wait_us(1);

	struct spi_buf_set tx_bufs;
	struct spi_buf tx_buff;
	u8_t buff = cmd;

	tx_buff.buf = &buff;
	tx_buff.len = 1;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;
	spi_write(spi_port, &spi_config, &tx_bufs);

	lcd_dc_up();
	wait_us(1);
}

/**@brief Function to send data to LCD by SPI.
 */
static void wr_dat(u8_t *data, u8_t len)
{
	struct spi_buf_set tx_bufs;
	struct spi_buf tx_buff;

	tx_buff.buf = data;
	tx_buff.len = len;
	tx_bufs.buffers = &tx_buff;
	tx_bufs.count = 1;
	spi_write(spi_port, &spi_config, &tx_bufs);
}

/**@brief Function to set LCD driver window.
 */
static void window(u16_t x, u16_t y, u16_t w, u16_t h)
{
	u8_t data[4];

	wr_cmd(0x2A);
	data[0] = x >> 8;
	data[1] = x;
	data[2] = (x+w-1) >> 8;
	data[3] = x+w-1;
	wr_dat(data, 4);
	lcd_cs_up();

	wr_cmd(0x2B);
	data[0] = y >> 8;
	data[1] = y;
	data[2] = (y+h-1) >> 8;
	data[3] = y+h-1;
	wr_dat(data, 4);
	lcd_cs_up();
}

/**@brief Function to initiate LCD GPIO pins.
 */
static void gpio_init(void)
{
	int err;

	gpio_port = device_get_binding(GPIO_PORT);
	if (gpio_port == NULL) {
		return;
	}

	err = gpio_pin_configure(gpio_port, N_RESET, GPIO_DIR_OUT);
	err += gpio_pin_configure(gpio_port, TFT_CS, GPIO_DIR_OUT);
	err += gpio_pin_configure(gpio_port, TFT_DC, GPIO_DIR_OUT);
	if (!err) {
		err += lcd_nreset_up();
		err += lcd_cs_up();
		err += lcd_dc_up();
	}

	if (err) {
		gpio_port = NULL;
	}
}

/**@brief Function to initiate LCD.
 */
void ili9341_lcd_init(void)
{
	u8_t spi_data[16];

	gpio_init();

	spi_port = device_get_binding(SPI_PORT);
	if (spi_port == NULL) {
		return;
	}

	spi_config.frequency = 8000000;
	spi_config.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |
			SPI_WORD_SET(8) | SPI_LINES_SINGLE;
	spi_config.slave = 0;		/* MOSI & CLK only; CS is not used. */
	spi_config.cs = NULL;

	wait_ms(5);
	lcd_nreset_down();
	wait_us(10);
	lcd_nreset_up();
	wait_ms(5);

	const u8_t *p_init = Adafruit_ILI9341_initcmd;

	while (*p_init != ILI9341_NOP) {
		wr_cmd(*p_init++);

		u8_t data_parm = *p_init++;
		u8_t num_data = data_parm & 0x0F;

		if (num_data > 0) {
			memcpy(spi_data, p_init, num_data);
			p_init += num_data;
			wr_dat(spi_data, num_data);
			lcd_cs_up();
		}
		if (data_parm & 0x40) {
			wait_ms(10);
		}
		if (data_parm & 0x80) {
			wait_ms(120);
		}
	}
}

/**@brief Function to turn on LCD.
 */
void ili9341_lcd_on(void)
{
	wr_cmd(0x29);               /* display on */
	lcd_cs_up();

	wait_ms(120);
}

/**@brief Function to turn off LCD.
 */
void ili9341_lcd_off(void)
{
	wr_cmd(0x28);               /* display off */
	lcd_cs_up();
}

struct ili9341_tx_buffer {
	u8_t			*p_tx_data;
	u32_t			len;
	struct spi_buf		tx_buff;
	struct spi_buf_set	tx_bufs;
};

/**@brief Function to put a graphics image on LCD.
 */
void ili9341_lcd_put_gfx(u16_t x, u16_t y, u16_t w, u16_t h, const u8_t *p_lcd_data)
{
	/* set display window */
	window(x, y, w, h);

	wr_cmd(0x2C);  /* send pixel */

	/* LittlevGL is little-endian and ILI9341 is big-endian. */
	/* Each 16-bit data takes a byte swap. */

#if defined(CONFIG_SPI_ASYNC) && CONFIG_SPI_ASYNC > 0
	/* Use SPI asynchronous call to utilize CPU during DMA transfer. */
	int err;
	struct k_poll_signal async_sig = K_POLL_SIGNAL_INITIALIZER(async_sig);
	struct k_poll_event async_evt =
				 K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
				 K_POLL_MODE_NOTIFY_ONLY,
				 &async_sig);

	u32_t len = (uint32_t)w * 2 * h;
	u32_t n;
	u32_t i, l;

	/* Divide SPI TX data buffer into 2 parts. */
	/* Start DMA transfer at a part and work on another part. */

	struct ili9341_tx_buffer tx_buffer[2], *p_tx_buffer;

	tx_buffer[0].len = ILI9341_LCD_WIDTH / 2 * 2;
	tx_buffer[1].len = sizeof(m_tx_data) - tx_buffer[0].len;
	tx_buffer[0].p_tx_data = m_tx_data;
	tx_buffer[1].p_tx_data = m_tx_data + tx_buffer[0].len;

	u32_t tx_buff_i = 0;

	for (n = 0; n < len; n += l) {
		/* get the data length to transfer */
		l = len - n;
		p_tx_buffer = tx_buffer + tx_buff_i;
		if (l > p_tx_buffer->len)
			l = p_tx_buffer->len;

		/* copy data with swap */
		for (i = 0; i < l; i += 2) {
			*(p_tx_buffer->p_tx_data + i + 0) = *(p_lcd_data + n + i + 1);
			*(p_tx_buffer->p_tx_data + i + 1) = *(p_lcd_data + n + i + 0);
		}

		p_tx_buffer->tx_buff.buf = p_tx_buffer->p_tx_data;
		p_tx_buffer->tx_buff.len = l;
		p_tx_buffer->tx_bufs.buffers = &p_tx_buffer->tx_buff;
		p_tx_buffer->tx_bufs.count = 1;

		if (n > 0) {
			/* hold until previous SPI transfer is done before starting another. */
			err = k_poll(&async_evt, 1, K_FOREVER);
			if (err) {
				printk("SPI write error #%d!\n", err);
				break;
			}
			k_poll_signal_reset(&async_sig);
		}
		spi_write_async(spi_port, &spi_config, &p_tx_buffer->tx_bufs, &async_sig);

		/* swap TX buffer structure */
		tx_buff_i ^= 0x1;
	}

	/* wait until SPI transfer is done */
	k_poll(&async_evt, 1, K_FOREVER);
#else
	/* No SPI asynchronous call. A little slower. */
	struct spi_buf		tx_buff;
	struct spi_buf_set	tx_bufs;

	u32_t len = (uint32_t)w * 2 * h;
	u32_t n;
	u32_t i, l;

	for (n = 0; n < len; n += l) {
		/* get the data length to transfer */
		l = len - n;
		if (l > sizeof(m_tx_data))
			l = sizeof(m_tx_data);

		/* copy data with swap */
		for (i = 0; i < l; i += 2) {
			m_tx_data[i + 0] = *(p_lcd_data + n + i + 1);
			m_tx_data[i + 1] = *(p_lcd_data + n + i + 0);
		}

		tx_buff.buf = m_tx_data;
		tx_buff.len = l;
		tx_bufs.buffers = &tx_buff;
		tx_bufs.count = 1;
		spi_write(spi_port, &spi_config, &tx_bufs);
	}
#endif

	lcd_cs_up();
}
