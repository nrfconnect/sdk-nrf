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


#define DEVICE_NAME		CONFIG_BT_DEVICE_NAME
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

static struct   bt_conn *default_conn;
static volatile u16_t   led_blink_interval = RUN_LED_BLINK_INTERVAL;

static struct   device  *led_port;
static struct   device  *uart;

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

static void connected(struct bt_conn *conn, u8_t err)                                                                                  
{                                                                                                                                      
        if (err) {                                                                                                                     
                printk("Connection failed (err %u)\n", err);
                led_blink_interval = ERR_LED_BLINK_INTERVAL;
        } else {
                printk("Connected\n");

                default_conn = bt_conn_ref(conn);                                                                                      
                gpio_pin_write(led_port, CON_STATUS_LED, LED_ON);
        }                                               
}          

static void disconnected(struct bt_conn *conn, u8_t reason)                                                                            
{                                                                                                                                      
        printk("Disconnected (reason %u)\n", reason);                                                                                  
                                                                                                                                       
        if (default_conn) {                                                                                                            
                bt_conn_unref(default_conn);                                                                                           
                default_conn = NULL;                                                                                                   
                gpio_pin_write(led_port, CON_STATUS_LED, LED_OFF);
        }
}          

static struct bt_conn_cb conn_callbacks = {                                                                                            
        .connected = connected,
        .disconnected = disconnected,
};

void uart_cb(struct device *uart)
{
        uart_irq_update(uart);

        if(uart_irq_rx_ready(uart)) {
                struct uart_data_t *rx = k_malloc(sizeof(struct uart_data_t));
                rx->len = uart_fifo_read(uart, rx->data, UART_BUF_SIZE);
                k_fifo_put(&fifo_uart_rx_data, rx);
        }

        if(uart_irq_tx_ready(uart)) {
                struct uart_data_t *buf = k_fifo_get(&fifo_uart_tx_data, K_NO_WAIT);
                int written             = 0;

                /* Nothing in the FIFO, nothing to send */
                if(!buf) {
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

void bt_receive_cb(u8_t *data, u16_t len)
{
        struct uart_data_t *tx = k_malloc(sizeof(struct uart_data_t));

        printk("Got data over bluetooth with length (%d)\n", len);

        tx->len  = len;
        memcpy(tx->data, data, len);
        k_fifo_put(&fifo_uart_tx_data, tx);

        /* Start the UART tranfer by enabling the TX ready interrupt */
        uart_irq_tx_enable(uart);
}


void bt_ready(int err)
{
	if(err) {
		printk("BT initialization failed (err %d)\n", err);
                led_blink_interval = ERR_LED_BLINK_INTERVAL;
		return;
	}

	printk("BT initialization OK\n");

	nus_init(bt_receive_cb, NULL);

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
                led_blink_interval = ERR_LED_BLINK_INTERVAL;
		return;
	}

	printk("Advertising successfully started\n");
}

void init_leds() 
{
        led_port = device_get_binding(LED_PORT);
        if (!led_port) {
                printk("Could not bind to LED port\n");
                return;
        }

        gpio_pin_configure(led_port, RUN_STATUS_LED,
                           GPIO_DIR_OUT);
        gpio_pin_write(led_port, RUN_STATUS_LED, LED_OFF);

        gpio_pin_configure(led_port, CON_STATUS_LED,
                           GPIO_DIR_OUT);
        gpio_pin_write(led_port, CON_STATUS_LED, LED_OFF);
}

void init_usart()
{
        uart = device_get_binding("UART_0");
        if(!uart) {
                printk("Could not bind to USART module\n");
                led_blink_interval = ERR_LED_BLINK_INTERVAL;
                return;
        }

        uart_irq_callback_set(uart,uart_cb);
        uart_irq_rx_enable(uart);
}

void init_thread(void)
{
        int blink_status = 0;

        printf("Starting Nordic UART service example\n");

        init_leds();
        init_usart();

	bt_enable(bt_ready);
        bt_conn_cb_register(&conn_callbacks);

        for(;;) {
                gpio_pin_write(led_port, RUN_STATUS_LED, (++blink_status) % 2);
                k_sleep(led_blink_interval);
        }
}

void ble_write()
{
        for(;;) {
                /* Wait indefinitely for data to send over bluetooth */
                struct uart_data_t *buf = k_fifo_get(&fifo_uart_rx_data, K_FOREVER);
                nus_data_send(buf->data, buf->len);
                k_free(buf);
        }
}

K_THREAD_DEFINE(init_thread_id, STACKSIZE, init_thread, NULL, NULL, NULL,
                PRIORITY, 0, K_NO_WAIT);

K_THREAD_DEFINE(ble_write_id, STACKSIZE, ble_write, NULL, NULL, NULL,
                PRIORITY, 0, K_NO_WAIT);
