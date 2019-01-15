/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <gps.h>
#include <sensor.h>
#include <console.h>
#include <dk_buttons_and_leds.h>
#include <lte_lc.h>
#include <misc/reboot.h>
#include <bsd.h>

#include "nrf_socket.h"

/* Interval in milliseconds between each time status LEDs are updated. */
#define LEDS_UPDATE_INTERVAL	        500

/* Interval in microseconds between each time LEDs are updated when indicating
 * that an error has occurred.
 */
#define LEDS_ERROR_UPDATE_INTERVAL      250000

#define APP_CUSTOM_APN              "VZWADMIN"
#define APP_TCP_REMOTE_SERVER_IP    5,189,130,26
#define APP_TCP_REMOTE_SERVER_PORT  1884
#define APP_SEND_COUNT              28

#define BUTTON_1			BIT(0)
#define BUTTON_2			BIT(1)
#define SWITCH_1			BIT(2)
#define SWITCH_2			BIT(3)

#define LED_ON(x)			(x)
#define LED_BLINK(x)			((x) << 8)
#define LED_GET_ON(x)			((x) & 0xFF)
#define LED_GET_BLINK(x)		(((x) >> 8) & 0xFF)

#if !defined(CONFIG_LTE_LINK_CONTROL)
#error "Missing CONFIG_LTE_LINK_CONTROL"
#endif /* !defined(CONFIG_LTE_LINK_CONTROL) */

enum {
	LEDS_INITIALIZING     = LED_ON(0),
	LEDS_CONNECTING       = LED_BLINK(DK_LED3_MSK),
	LEDS_CONNECTED        = LED_ON(DK_LED3_MSK),
	LEDS_PDN_CONNECTION   = LED_BLINK(DK_LED3_MSK | DK_LED4_MSK),
	LEDS_TCP_CONNECTION   = LED_ON(DK_LED3_MSK) | LED_BLINK(DK_LED4_MSK),
	LEDS_TCP_TRANSFER     = LED_BLINK(DK_LED4_MSK),
	LEDS_ERROR            = LED_ON(DK_ALL_LEDS_MSK)
} display_state;

/* Structures for work */
static struct k_delayed_work leds_update_work;

/* File descriptor for APN management. */
static int apn_fd;

enum error_type {
	ERROR_BSD_RECOVERABLE,
	ERROR_BSD_IRRECOVERABLE,
	ERROR_LTE_LC
};

/* Forward declaration of functions */
static void work_init(void);


const char * const mp_send_data[APP_SEND_COUNT] =
{
    "Puff, the magic dragon lived by the sea",
    "And frolicked in the autumn mist in a land called Honahlee",
    "Little Jackie paper loved that rascal puff",
    "And brought him strings and sealing wax and other fancy stuff oh",
    "Puff, the magic dragon lived by the sea",
    "And frolicked in the autumn mist in a land called Honahlee",
    "Puff, the magic dragon lived by the sea",
    "And frolicked in the autumn mist in a land called Honahlee",
    "Together they would travel on a boat with billowed sail",
    "Jackie kept a lookout perched on puff's gigantic tail",
    "Noble kings and princes would bow whene'er they came",
    "Pirate ships would lower their flag when puff roared out his name oh",
    "Puff, the magic dragon lived by the sea",
    "And frolicked in the autumn mist in a land called Honahlee",
    "Puff, the magic dragon lived by the sea",
    "And frolicked in the autumn mist in a land called Honahlee",
    "A dragon lives forever but not so little boys",
    "Painted wings and giant rings make way for other toys",
    "One grey night it happened, Jackie Paper came no more",
    "And puff that mighty dragon, he ceased his fearless roar",
    "His head was bent in sorrow, green scales fell like rain",
    "Puff no longer went to play along the cherry lane",
    "Without his life-long friend, puff could not be brave",
    "So Puff that mighty dragon sadly slipped into his cave oh",
    "Puff, the magic dragon lived by the sea",
    "And frolicked in the autumn mist in a land called Honahlee",
    "Puff, the magic dragon lived by the sea",
    "And frolicked in the autumn mist in a land called Honahle"
};
/**@brief nRF Cloud error handler. */
void error_handler(enum error_type err_type, int err_code)
{
	u8_t led_pattern;

	switch (err_type) {
	case ERROR_BSD_RECOVERABLE:
		/* Blinking all LEDs ON/OFF in pairs (1 and 3, 2 and 4)
		 * if there is a recoverable error.
		 */
		led_pattern = DK_LED1_MSK | DK_LED3_MSK;
		printk("Error of type ERROR_BSD_RECOVERABLE: %d\n", err_code);
	break;
	case ERROR_BSD_IRRECOVERABLE:
		/* Blinking all LEDs ON/OFF if there is an
		 * irrecoverable error.
		 */
		led_pattern = DK_ALL_LEDS_MSK;
		printk("Error of type ERROR_BSD_IRRECOVERABLE: %d\n",
								err_code);
	break;
	default:
		/* Blinking all LEDs ON/OFF in pairs (1 and 2, 3 and 4)
		 * undefined error.
		 */
		led_pattern = DK_LED1_MSK | DK_LED2_MSK;
		printk("Unknown error type: %d err_code: %d\n",
			err_type, err_code);
	break;
	}

	while (true) {
		dk_set_leds(led_pattern & 0x0f);
		k_busy_wait(LEDS_ERROR_UPDATE_INTERVAL);
		dk_set_leds(~led_pattern & 0x0f);
		k_busy_wait(LEDS_ERROR_UPDATE_INTERVAL);
	}
}


/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
	error_handler(ERROR_BSD_RECOVERABLE, (int)err);
}

/**@brief Irrecoverable BSD library error. */
void bsd_irrecoverable_error_handler(uint32_t err)
{
	error_handler(ERROR_BSD_IRRECOVERABLE, (int)err);
}


/**@brief Update LEDs state. */
static void leds_update(struct k_work *work)
{
	static bool led_on;
	static u8_t current_led_on_mask;
	u8_t led_on_mask = current_led_on_mask;

	ARG_UNUSED(work);

	/* Reset LED3 and LED4. */
	led_on_mask &= ~(DK_LED3_MSK | DK_LED4_MSK);

	/* Set LED3 and LED4 to match current state. */
	led_on_mask |= LED_GET_ON(display_state);

	led_on = !led_on;
	if (led_on) {
		led_on_mask |= LED_GET_BLINK(display_state);
	} else {
		led_on_mask &= ~LED_GET_BLINK(display_state);
	}

	if (led_on_mask != current_led_on_mask) {
		dk_set_leds(led_on_mask);
		current_led_on_mask = led_on_mask;
	}

	k_delayed_work_submit(&leds_update_work, LEDS_UPDATE_INTERVAL);
}

/**@brief Initializes and submits delayed work. */
static void work_init(void)
{
	k_delayed_work_init(&leds_update_work, leds_update);
	k_delayed_work_submit(&leds_update_work, LEDS_UPDATE_INTERVAL);
}

/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */
static void modem_configure(void)
{
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already turned on
		 * and connected.
		 */
	} else {
		int err;

		printk("LTE LC config ...\n");
		display_state = LEDS_CONNECTING;
		err = lte_lc_init_and_connect();
		__ASSERT(err == 0, "LTE link could not be established.");
		display_state = LEDS_CONNECTED;
	}
}


void app_apn_connect(void)
{
    apn_fd = nrf_socket(NRF_AF_LTE, NRF_SOCK_MGMT, NRF_PROTO_PDN);
    __ASSERT(apn_fd >= 0, "PDN Socket creation failed.");
	printk("PDN management socket created.\n");
	printk("Connecting to APN %s.\n", APP_CUSTOM_APN);

    // Connect to the APN.
    int err = nrf_connect(apn_fd, APP_CUSTOM_APN, strlen(APP_CUSTOM_APN));
    __ASSERT(err == 0, "APN Connection Failed.");
    display_state = LEDS_PDN_CONNECTION;
}



static uint32_t inet_addr(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    uint32_t ret_val = 0;

    uint8_t * byte_array = (uint8_t *) &ret_val;

    byte_array[0] = a;
    byte_array[1] = b;
    byte_array[2] = c;
    byte_array[3] = d;

    return ret_val;
}

/**@brief Disconnects the PDN connection towards the APN.
 */
void app_tcp_client_on_pdn_start(void)
{
    int fd = nrf_socket(NRF_AF_INET, NRF_SOCK_STREAM, 0);
    __ASSERT(fd >= 0, "Failed to create TCP socket.");

    struct nrf_ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    memcpy(ifr.ifr_name, APP_CUSTOM_APN, strlen(APP_CUSTOM_APN));

    int err = nrf_setsockopt(fd, NRF_SOL_SOCKET, NRF_SO_BINDTODEVICE, (void *)&ifr, strlen(APP_CUSTOM_APN));
    __ASSERT(err == 0, "Failed to set APN for TCP socket.");

    struct nrf_sockaddr_in dest_addr;
    dest_addr.sin_len = sizeof(struct nrf_sockaddr_in);
    dest_addr.sin_family = NRF_AF_INET;
    dest_addr.sin_port = nrf_htons(APP_TCP_REMOTE_SERVER_PORT);
    dest_addr.sin_addr.s_addr = inet_addr(APP_TCP_REMOTE_SERVER_IP);

    err = nrf_connect(fd, (const struct nrf_sockaddr *)&dest_addr, sizeof(dest_addr));
    __ASSERT(err == 0, "TCP Socket connection failed.");
    display_state = LEDS_TCP_CONNECTION;

    struct nrf_pollfd poll_fd;

    int write_offset = 0;
    int read_bytes   = 0;

    do
    {
        poll_fd.handle    = fd;
        poll_fd.requested = NRF_POLLIN | NRF_POLLOUT;

        err = nrf_poll(&poll_fd, 1, 0);
        if (err > 0)
        {
            if ((poll_fd.returned & NRF_POLLIN) == NRF_POLLIN)
            {
                // recv ready.
                char read_buffer[128];
                int  received = -1;

                received = nrf_recv(fd, read_buffer, 128, NRF_MSG_DONTWAIT);
                if (received > 0)
                {
                    read_bytes += received;
                }
            }
            if ((poll_fd.returned & NRF_POLLOUT) == NRF_POLLOUT)
            {
                // send ready.
                const int data_size = strlen(mp_send_data[write_offset]);
                int written;

                written = nrf_send(fd, mp_send_data[write_offset], data_size, 0);
                if (data_size == written)
                {
                    write_offset++;
                }
                display_state = LEDS_TCP_TRANSFER;
            }
        }
    } while (write_offset < APP_SEND_COUNT);

    err = nrf_close (fd);
    __ASSERT(err == 0, "Failed to close TCP socket.");

}


/**@brief Disconnects the PDN connection towards the APN.
 */
void app_apn_disconnect(void)
{
    int err = nrf_close(apn_fd);
    __ASSERT(err == 0, "APN disconnection failed.");
    display_state = LEDS_CONNECTED;
}


/**@brief Initializes LEDs, using the DK LEDs library.
 */
static void leds_init(void)
{
	int err;

	err = dk_leds_init();
	if (err) {
		printk("Could not initialize leds, err code: %d\n", err);
	}

	err = dk_set_leds_state(0x00, DK_ALL_LEDS_MSK);
	if (err) {
		printk("Could not set leds state, err code: %d\n", err);
	}
}


void main(void)
{
	printk("Application started\n");

	leds_init();
	work_init();
	modem_configure();
	app_apn_connect();
    app_tcp_client_on_pdn_start();
    app_apn_disconnect();
	while (true) {

		if (IS_ENABLED(CONFIG_LOG)) {
			/* if logging is enabled, sleep */
			k_sleep(K_MSEC(10));
		} else {
			/* other, put CPU to idle to save power */
			k_cpu_idle();
		}
	}
}
