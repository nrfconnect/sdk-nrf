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

#define DEVICE_NAME		CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN	        (sizeof(DEVICE_NAME) - 1)

/* Change this if you have an LED connected to a custom port */
#ifndef LED0_GPIO_CONTROLLER
#define LED0_GPIO_CONTROLLER    LED0_GPIO_PORT
#endif

#define LED_PORT                LED0_GPIO_CONTROLLER

#define RUN_STATUS_LED          LED0_GPIO_PIN
#define CON_STATUS_LED          LED1_GPIO_PIN

#define LED_BLINK_INTERVAL      1000

#define LED_ON                  0
#define LED_OFF                 1

#define UART_MAX_RX_SIZE        16

static struct bt_conn *default_conn;
static struct device  *led_port;
static struct device  *uart;

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
        } else {                                                                                                                       
                default_conn = bt_conn_ref(conn);                                                                                      
                printk("Connected\n");                                                                                                 
        }                                               

        default_conn = bt_conn_ref(conn);
        gpio_pin_write(led_port, CON_STATUS_LED, LED_ON);
}          

static void disconnected(struct bt_conn *conn, u8_t reason)                                                                            
{                                                                                                                                      
        printk("Disconnected (reason %u)\n", reason);                                                                                  
                                                                                                                                       
        if (default_conn) {                                                                                                            
                bt_conn_unref(default_conn);                                                                                           
                default_conn = NULL;                                                                                                   
        }

        gpio_pin_write(led_port, CON_STATUS_LED, LED_OFF);
}          

static struct bt_conn_cb conn_callbacks = {                                                                                            
        .connected = connected,
        .disconnected = disconnected,
};

void bt_receive_cb(u8_t data)
{
        printk("Got data over bluetooth (%c)\n", data);

        //uart_fifo_fill(uart,data,1);
}

void bt_usart_sendt_cb(u8_t data)
{
        uint8_t      buf[UART_MAX_RX_SIZE+1];
        int          len;


        printk("Data was sendt over bluetooth (%c)\n", data);

        if(uart_irq_rx_ready(uart)) {
                len = uart_fifo_read(uart, buf, UART_MAX_RX_SIZE);
                buf[len] = '\0';
                nus_data_send(buf);
        } else {
                printk("No more data to send\n");
        }
}

void bt_ready(int err)
{
	if(err) {
		printk("BT initialization failed (err %d)\n", err);
		return;
	}

	printk("BT initialization OK\n");

	nus_init(bt_receive_cb, bt_usart_sendt_cb);

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
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

void uart_cb(struct device *uart)
{
        int len=0;
        u8_t buf[UART_MAX_RX_SIZE+1];

        if(uart_irq_rx_ready(uart)) {
                len = uart_fifo_read(uart, buf, UART_MAX_RX_SIZE);
                buf[len] = '\0';
                nus_data_send(buf);
        }
}

void init_usart()
{
        uart = device_get_binding(UART_1);
        if(!uart) {
                printk("Could not bind to USART module\n");
                return;
        }

        uart_irq_callback_set(uart,uart_cb);
        uart_irq_rx_enable(uart);
}

int main()
{
        int blink_status = 0;

        printf("Starting Nordic UART service example\n");

        init_leds();
        init_usart();

	bt_enable(bt_ready);
        bt_conn_cb_register(&conn_callbacks);

        for(;;) {
                gpio_pin_write(led_port, RUN_STATUS_LED, (++blink_status) % 2);
                k_sleep(LED_BLINK_INTERVAL);
        }

	return 0;
}

