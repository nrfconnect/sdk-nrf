/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <uart.h>
#include <string.h>

#include <net/coap_api.h>
#include <net/socket.h>
#include <lte_lc.h>

#define APP_COAP_SEND_INTERVAL_MS K_MSEC(5000)
#define APP_COAP_TICK_INTERVAL_MS K_MSEC(1000)

struct pollfd fds;
static struct sockaddr_storage server;
static u16_t next_token;
static int transport_handle;

#if defined(CONFIG_BSD_LIBRARY)

/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
	printk("bsdlib recoverable error: %u\n", err);
}

/**@brief Irrecoverable BSD library error. */
void bsd_irrecoverable_error_handler(uint32_t err)
{
	printk("bsdlib irrecoverable error: %u\n", err);

	__ASSERT_NO_MSG(false);
}

#endif /* defined(CONFIG_BSD_LIBRARY) */

/**@brief Handles an error notified by CoAP. */
static void coap_client_error_handler(u32_t error_code, coap_message_t *message)
{
	ARG_UNUSED(message);

	printk("CoAP error handler: error_code: %u\n", error_code);
}

/**@brief Resolves the configured hostname. */
static int server_resolve(void)
{
	int err;
	struct addrinfo *result;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_DGRAM
	};

	err = getaddrinfo(CONFIG_COAP_SERVER_HOSTNAME, NULL, &hints, &result);
	if (err != 0) {
		printk("ERROR: getaddrinfo failed %d\n", err);
		return -EIO;
	}

	if (result == NULL) {
		printk("ERROR: Address not found\n");
		return -ENOENT;
	}

	/* IPv4 Address. */
	struct sockaddr_in *server4 = ((struct sockaddr_in *)&server);

	server4->sin_addr.s_addr =
		((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
	server4->sin_family = AF_INET;
	server4->sin_port = htons(CONFIG_COAP_SERVER_PORT);

	printk("IPv4 Address found 0x%08x\n", server4->sin_addr.s_addr);

	/* Free the address. */
	freeaddrinfo(result);

	return 0;
}

/**@brief Initialize the CoAP client */
static int client_init(void)
{
	int err;

	const struct sockaddr_in client_addr = {
		.sin_port = 0,
		.sin_family = AF_INET,
		.sin_addr.s_addr = 0
	};

	coap_local_t local_port_list[] = {
		{
			.addr = (struct sockaddr *)&client_addr,
			.protocol = IPPROTO_UDP,
			.setting = NULL
		}
	};

	coap_transport_init_t transport_param = {
		.port_table = local_port_list
	};

	err = coap_init(sys_rand32_get(), &transport_param, k_malloc, k_free);
	if (err != 0) {
		printf("Failed to initialize CoAP %d\n", err);
		return -err;
	}

	/* NOTE: transport_handle is the socket descriptor. */
	transport_handle = local_port_list[0].transport;

	fds.fd = transport_handle;
	fds.events = POLLIN;

	err = coap_error_handler_register(coap_client_error_handler);
	if (err != 0) {
		printf("Failed to register CoAP error handler\n");
		return -err;
	}

	next_token = sys_rand32_get();

	return 0;
}

/**@brief Handles responses from the remote CoAP server. */
static void client_get_response_handle(u32_t status, void *arg,
				     coap_message_t *response)
{
	char buf[16];

	printk("CoAP response: status: 0x%x", status);

	if (status == 0) {
		snprintf(buf, MAX(response->payload_len, sizeof(buf)), "%s",
			 response->payload);
		printk(", token 0x%02x%02x, payload: %s", response->token[0],
		       response->token[1], buf);
	}

	printk("\n");
}

/**@biref Send CoAP GET request. */
static int client_get_send(void)
{
	int err;
	u32_t handle;
	coap_message_t *request;
	coap_message_conf_t message_conf;

	memset(&message_conf, 0x00, sizeof(message_conf));
	message_conf.type = COAP_TYPE_CON;
	message_conf.code = COAP_CODE_GET;
	message_conf.transport = transport_handle;
	message_conf.id = 0; /* Auto-generate message ID. */
	message_conf.token[0] = (next_token >> 8) & 0xFF;
	message_conf.token[1] = next_token & 0xFF;
	message_conf.token_len = 2;
	message_conf.response_callback = client_get_response_handle;

	next_token++;

	err = coap_message_new(&request, &message_conf);
	if (err != 0) {
		printk("Failed to allocate CoAP request message %d!\n", err);
		return -err;
	}

	err = coap_message_remote_addr_set(request, (struct sockaddr *)&server);
	if (err != 0) {
		goto exit;
	}

	err = coap_message_opt_str_add(request, COAP_OPT_URI_PATH,
				       (u8_t *)CONFIG_COAP_RESOURCE,
				       strlen(CONFIG_COAP_RESOURCE));
	if (err != 0) {
		goto exit;
	}

	err = coap_message_send(&handle, request);
	if (err != 0) {
		goto exit;
	}

	printk("CoAP request sent: token 0x%02x%02x\n",
	       request->token[0], request->token[1]);

exit:
	(void)coap_message_delete(request);

	return -err;
}

/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */
static void modem_configure(void)
{
#if defined(CONFIG_LTE_LINK_CONTROL)
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already turned on
		 * and connected.
		 */
	} else {
		int err;

		printk("LTE Link Connecting ...\n");
		err = lte_lc_init_and_connect();
		__ASSERT(err == 0, "LTE link could not be established.");
		printk("LTE Link Connected!\n");
	}
#endif
}

static int wait(int timeout)
{
	int ret = poll(&fds, 1, timeout);

	if (ret < 0) {
		printk("poll error: %d\n", errno);
	}

	return ret;
}

void main(void)
{
	s64_t next_msg_time = APP_COAP_SEND_INTERVAL_MS;
	s64_t next_tick_time = APP_COAP_TICK_INTERVAL_MS;

	printk("The nRF CoAP client sample started\n");

	modem_configure();

	if (server_resolve() != 0) {
		printk("Failed to resolve server name\n");
		return;
	}

	if (client_init() != 0) {
		printk("Failed to initialize CoAP client\n");
		return;
	}

	while (1) {
		if (k_uptime_get() >= next_tick_time) {
			coap_time_tick();
			next_tick_time += APP_COAP_TICK_INTERVAL_MS;
		}

		if (k_uptime_get() >= next_msg_time) {
			client_get_send();
			next_msg_time += APP_COAP_SEND_INTERVAL_MS;
		}

		s64_t remaining = next_tick_time - k_uptime_get();

		if (remaining < 0) {
			remaining = 0;
		}

		if (wait(remaining) > 0) {
			coap_input();
		}

		/* A workaround for incomplete bsd_os_timedwait implementation. */
		k_sleep(K_MSEC(10));
	}
}
