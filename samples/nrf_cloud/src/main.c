/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <console.h>
#include <stdio.h>
#include <string.h>
#include <nrf_cloud.h>
#include "nrf_socket.h"

#define GPGGA_STR                                                              \
	"$GPGGA,063158.959,6325.830,N,01023.703,E,1,12,1.0,0.0,M,0.0,M,,*6F"

#define AT_CMD 100

static bool pattern_recording;
static int buttons_to_capture;
static int buttons_captured;

 /* Captured button input. */
static u8_t *buttons;

struct k_delayed_work sensor_data_work;

static void cloud_connect(void);

static void sensor_data_send(struct k_work *work)
{
	int err;

	struct nrf_cloud_sensor_data data = {
		.type = NRF_CLOUD_SENSOR_GPS,
		.data = {
			.ptr = GPGGA_STR,
			.len = sizeof(GPGGA_STR),
		},
		.tag = 4334,
	};

	err = nrf_cloud_sensor_data_send(&data);
	__ASSERT_NO_MSG(err == 0);

	printk("Sensor data sent\n");
	k_delayed_work_submit(&sensor_data_work, K_SECONDS(4));
}

static void on_user_association_req(const struct nrf_cloud_evt *p_evt)
{
	if (!pattern_recording) {
		pattern_recording = true;
		buttons_captured = 0;
		buttons_to_capture = p_evt->param.ua_req.sequence.len;
		buttons = k_malloc(buttons_to_capture);
	}
}

static void cloud_event_handler(const struct nrf_cloud_evt *p_evt)
{
	int err;

	switch (p_evt->type) {
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
	{
		printk("NRF_CLOUD_EVT_TRANSPORT_CONNECTED\n");
		break;
	}
	case NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST:
	{
		printk("NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST\n");
		on_user_association_req(p_evt);
		break;
	}
	case NRF_CLOUD_EVT_USER_ASSOCIATED:
	{
		printk("NRF_CLOUD_EVT_USER_ASSOCIATED\n");
		break;
	}
	case NRF_CLOUD_EVT_READY:
	{
		printk("NRF_CLOUD_EVT_READY\n");
		struct nrf_cloud_sa_param param = {
			.sensor_type = NRF_CLOUD_SENSOR_FLIP,
		};
		err = nrf_cloud_sensor_attach(&param);
		__ASSERT_NO_MSG(err == 0);

		param.sensor_type = NRF_CLOUD_SENSOR_GPS;
		err = nrf_cloud_sensor_attach(&param);
		__ASSERT_NO_MSG(err == 0);
		break;
	}
	case NRF_CLOUD_EVT_SENSOR_ATTACHED:
	{
		printk("NRF_CLOUD_EVT_SENSOR_ATTACHED\n");
		struct nrf_cloud_sensor_data data = {
			.type = NRF_CLOUD_SENSOR_FLIP,
			.data = {
				.ptr = "NORMAL",
				.len = sizeof("NORMAL"),
			},
			.tag = 4224,
		};
		err = nrf_cloud_sensor_data_send(&data);
		__ASSERT_NO_MSG(err == 0);

		k_delayed_work_submit(&sensor_data_work, K_SECONDS(4));

		break;
	}
	case NRF_CLOUD_EVT_SENSOR_DATA_ACK:
	{
		printk("NRF_CLOUD_EVT_SENSOR_DATA_ACK\n");
		break;
	}
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
	{
		k_delayed_work_cancel(&sensor_data_work);
		printk("NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED\n");
		cloud_connect();
		break;
	}
	case NRF_CLOUD_EVT_ERROR:
	{
		k_delayed_work_cancel(&sensor_data_work);
		printk("NRF_CLOUD_EVT_ERROR\n");
		break;
	}
	default:
	{
		printk("Received unknown %d\n", p_evt->type);
		break;
	}
	}
}

static void cloud_init(void)
{
	const struct nrf_cloud_init_param param = {
		.event_handler = cloud_event_handler
	};

	int ret_code = nrf_cloud_init(&param);

	__ASSERT_NO_MSG(ret_code == 0);
}

static void cloud_connect(void)
{
	const enum nrf_cloud_ua supported_uas[] = {
		NRF_CLOUD_UA_BUTTON
	};

	const enum nrf_cloud_sensor supported_sensors[] = {
		NRF_CLOUD_SENSOR_GPS,
		NRF_CLOUD_SENSOR_FLIP
	};

	const struct nrf_cloud_ua_list ua_list = {
		.size = ARRAY_SIZE(supported_uas),
		.ptr = supported_uas
	};

	const struct nrf_cloud_sensor_list sensor_list = {
		.size = ARRAY_SIZE(supported_sensors),
		.ptr = supported_sensors
	};

	const struct nrf_cloud_connect_param param = {
		.ua = &ua_list,
		.sensor = &sensor_list,
	};

	int ret_code = nrf_cloud_connect(&param);

	printk("nrf_cloud_connect() = %d\n", ret_code);
}

/**@brief Send the user association information. */
static void cloud_user_associate(void)
{
	const struct nrf_cloud_ua_param ua = {
		.type = NRF_CLOUD_UA_BUTTON,
		.sequence = {
			.len = buttons_to_capture,
			.ptr = buttons
		}
	};

	int err = nrf_cloud_user_associate(&ua);

	__ASSERT_NO_MSG(err == 0);

	k_free(buttons);
	buttons = NULL;
}

static void app_process(void)
{
	if (!pattern_recording || (buttons_captured >= buttons_to_capture)) {
		return;
	}

	u8_t c[2];

	c[0] = console_getchar();
	c[1] = console_getchar();
	if (c[0] == 'b' && c[1] == '1') {
		printk("b1\n");
		buttons[buttons_captured++] = NRF_CLOUD_UA_BUTTON_INPUT_3;
	} else if (c[0] == 'b' && c[1] == '2') {
		printk("b2\n");
		buttons[buttons_captured++] = NRF_CLOUD_UA_BUTTON_INPUT_4;
	} else if (c[0] == 's' && c[1] == '1') {
		printk("s1\n");
		buttons[buttons_captured++] = NRF_CLOUD_UA_BUTTON_INPUT_1;
	} else if (c[0] == 's' && c[1] == '2') {
		printk("s2\n");
		buttons[buttons_captured++] = NRF_CLOUD_UA_BUTTON_INPUT_2;
	}

	if (buttons_captured == buttons_to_capture) {
		cloud_user_associate();
		pattern_recording = false;
	}
}

static void work_init(void)
{
	k_delayed_work_init(&sensor_data_work, sensor_data_send);
}

static void modem_configure(void)
{
	int at_socket_fd = -1;
	int bytes_written = 0;
	int bytes_read = 0;

	char read_buffer[AT_CMD];

	printk("Modem configure ...\n");

	at_socket_fd = nrf_socket(NRF_AF_LTE, 0, NRF_PROTO_AT);
	__ASSERT_NO_MSG(at_socket_fd >= 0);

	bytes_written = nrf_write(at_socket_fd, "AT+CEREG=2", 10);
	__ASSERT_NO_MSG(bytes_written == 10);

	bytes_read = nrf_read(at_socket_fd, read_buffer, AT_CMD);
	__ASSERT_NO_MSG(bytes_read >= 2);
	__ASSERT_NO_MSG(strncmp("OK", read_buffer, 2) == 0);

	bytes_written = nrf_write(at_socket_fd, "AT+CFUN=1", 9);
	__ASSERT_NO_MSG(bytes_written == 9);

	bytes_read =
		nrf_read(at_socket_fd, read_buffer, AT_CMD);
	__ASSERT_NO_MSG(bytes_read >= 2);
	__ASSERT_NO_MSG(strncmp("OK", read_buffer, 2) == 0);

	while (true) {
		memset(read_buffer, 0x00, sizeof(read_buffer));

		bytes_read = nrf_read(at_socket_fd, read_buffer,
				      AT_CMD);

		if ((memcmp("+CEREG: 1", read_buffer, 9) == 0)  ||
		    (memcmp("+CEREG:1", read_buffer, 8) == 0)   ||
		    (memcmp("+CEREG: 5", read_buffer, 9) == 0)  ||
		    (memcmp("+CEREG:5", read_buffer, 8) == 0)) {
			break;
		}

		printk("%s", read_buffer);
	}

	printk("Modem configured\n");
}


void main(void)
{
	console_init();
	work_init();
	cloud_init();
	modem_configure();
	cloud_connect();

	while (true) {
		nrf_cloud_process();
		app_process();
	}
}
