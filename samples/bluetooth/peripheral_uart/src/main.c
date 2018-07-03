/** @file
 *  @brief Nordic UART Bridge Service (NUS) sample
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr.h>
#include <uart.h>

#include <board.h>
#include <gpio.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>

#include <../gatt/nus.h>

#include <stdio.h>

#define STACKSIZE               1024
#define PRIORITY                7

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN	        (sizeof(DEVICE_NAME) - 1)

/* Change this if you have an LED connected to a custom port */
#ifndef LED0_GPIO_CONTROLLER
#define LED0_GPIO_CONTROLLER    LED0_GPIO_PORT
#endif

#define LED_PORT                LED0_GPIO_CONTROLLER

#define RUN_STATUS_LED          LED0_GPIO_PIN
#define RUN_LED_BLINK_INTERVAL  1000
#define ERR_LED_BLINK_INTERVAL  200

#define CON_STATUS_LED          LED1_GPIO_PIN

#define LED_ON                  0
#define LED_OFF                 1

#define UART_BUF_SIZE           16

#define STATE_INIT_LEDS         0
#define STATE_INIT_UART         1
#define STATE_ENABLE_BT         2
#define STATE_SET_BT_CALLBACKS  3
#define STATE_ERROR             4
#define STATE_END               5

static struct   bt_conn * current_conn;
static volatile u16_t   led_blink_interval = RUN_LED_BLINK_INTERVAL;

static struct   device  * led_port;
static struct   device  * uart;

struct uart_data_t {
        void  *fifo_reserved;
        u8_t  data[UART_BUF_SIZE];
        u16_t len;
};
K_FIFO_DEFINE(fifo_uart_tx_data);
K_FIFO_DEFINE(fifo_uart_rx_data);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, NUS_UUID_SERVICE),
};

static void connected(struct bt_conn * conn, u8_t err)
{                                                                                                                                      
        if (err) {                                                                                                                     
                printk("Connection failed (err %u)\n", err);
                led_blink_interval = ERR_LED_BLINK_INTERVAL;
        } else {
                printk("Connected\n");

                current_conn = bt_conn_ref(conn);
                gpio_pin_write(led_port, CON_STATUS_LED, LED_ON);
        }                                               
}          

static void disconnected(struct bt_conn * conn, u8_t reason)
{
        printk("Disconnected (reason %u)\n", reason);

        if (current_conn) {
                bt_conn_unref(current_conn);
                current_conn = NULL;
                gpio_pin_write(led_port, CON_STATUS_LED, LED_OFF);
        }
}          

static struct bt_conn_cb conn_callbacks = {                                                                                            
        .connected = connected,
        .disconnected = disconnected,
};

static void uart_cb(struct device * uart)
{
        uart_irq_update(uart);

        if(uart_irq_rx_ready(uart)) {
                struct uart_data_t * rx = k_malloc(sizeof(struct uart_data_t));

                if (!rx) {
                        printk("Not able to allocate UART receive buffer\n");
                        return;
                }

                rx->len = uart_fifo_read(uart, rx->data, UART_BUF_SIZE);
                k_fifo_put(&fifo_uart_rx_data, rx);
        }

        if(uart_irq_tx_ready(uart)) {
                struct uart_data_t * buf = k_fifo_get(&fifo_uart_tx_data, K_NO_WAIT);
                int written             = 0;

                /* Nothing in the FIFO, nothing to send */
                if (!buf) {
                        uart_irq_tx_disable(uart);
                        return;
                }

                while(buf->len != written) {
                        written += uart_fifo_fill(uart, &buf->data[written], buf->len - written);
                }

                uart_irq_tx_disable(uart);
                k_free(buf);
        }
}

static void bt_receive_cb(u8_t * data, u16_t len)
{
        struct uart_data_t * tx = k_malloc(sizeof(*tx));

        if (!tx) {
                printk("Not able to allocate UART send data buffer\n");
                return;
        }

        printk("Got data over bluetooth with length (%d)\n", len);

        tx->len  = len;
        memcpy(tx->data, data, len);
        k_fifo_put(&fifo_uart_tx_data, tx);

        /* Start the UART tranfer by enabling the TX ready interrupt */
        uart_irq_tx_enable(uart);
}

static int init_leds()
{
        led_port = device_get_binding(LED_PORT);

        if (!led_port) {
                printk("Could not bind to LED port\n");
                return -ENXIO;
        }

        gpio_pin_configure(led_port, RUN_STATUS_LED,
                           GPIO_DIR_OUT);
        gpio_pin_write(led_port, RUN_STATUS_LED, LED_OFF);

        gpio_pin_configure(led_port, CON_STATUS_LED,
                           GPIO_DIR_OUT);
        gpio_pin_write(led_port, CON_STATUS_LED, LED_OFF);

	return 0;
}

static int init_usart()
{
        uart = device_get_binding("UART_0");
        if (!uart) {
                printk("Could not bind to USART module\n");
                return -ENXIO;
        }

        uart_irq_callback_set(uart, uart_cb);
        uart_irq_rx_enable(uart);

	return 0;
}

void led_blink_thread(void)
{
        int blink_status = 0;
        int err          = 0;

        printf("Starting Nordic UART service example\n");

        for(int i=STATE_INIT_LEDS;i!=STATE_END;) {

                switch(i) {
                        case STATE_INIT_LEDS:
                                if (init_leds()) {
                                        i = STATE_ERROR;
                                        break;
                                }
                                i = STATE_INIT_UART;
                                break;

                        case STATE_INIT_UART:
                                if (init_usart()) {
                                        i = STATE_ERROR;
                                        break;
                                }
                                i = STATE_ENABLE_BT;
                                break;

                        case STATE_ENABLE_BT:
                                err = bt_enable(NULL);
                                if (err) {
                                        printk("BT initialization failed (err %d)\n", err);
                                        i = STATE_ERROR;
                                        break;
                                }

                                printk("BT initialization OK\n");

                                err = nus_init(bt_receive_cb, NULL);
	                        if (err) {
                                        printk("Failed to initialize UART service (err: %d)\n", err);
                                        i = STATE_ERROR;
                                        break;
                                }

                                printk("UART service initialized sucessfullly\n");

                                err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			                              sd, ARRAY_SIZE(sd));
	                        if (err) {
		                        printk("Advertising failed to start (err %d)\n", err);
                                        i = STATE_ERROR;
                                        break;
	                        }

	                        printk("Advertising successfully started\n");
                                i = STATE_SET_BT_CALLBACKS;
                                break;

                        case STATE_SET_BT_CALLBACKS:
                                bt_conn_cb_register(&conn_callbacks);
                                i = STATE_END;
                                break;

                        case STATE_ERROR:
                                led_blink_interval = ERR_LED_BLINK_INTERVAL;
                                i = STATE_END;
                                break;

                        case STATE_END:
                                break;

                        default:
                                break;

		}
        }

        for(;;) {
                gpio_pin_write(led_port, RUN_STATUS_LED, (++blink_status) % 2);
                k_sleep(led_blink_interval);
        }
}

void ble_write_thread()
{
        for(;;) {
                /* Wait indefinitely for data to send over bluetooth */
                struct uart_data_t *buf = k_fifo_get(&fifo_uart_rx_data, K_FOREVER);
                if(nus_data_send(buf->data, buf->len)) {
                        printk("Failed to send data over BLE connection\n");
                }
                k_free(buf);
        }
}

K_THREAD_DEFINE(led_blink_thread_id, STACKSIZE, led_blink_thread, NULL, NULL, NULL,
                PRIORITY, 0, K_NO_WAIT);

K_THREAD_DEFINE(ble_write_thread_id, STACKSIZE, ble_write_thread, NULL, NULL, NULL,
                PRIORITY, 0, K_NO_WAIT);
