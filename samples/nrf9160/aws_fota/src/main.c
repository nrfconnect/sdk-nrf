/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <uart.h>
#include <stdio.h>
#include <at_cmd.h>
#include <bsd.h>
#include <lte_lc.h>
#include <net/mqtt.h>
#include <net/socket.h>
#include <net/bsdlib.h>
#include <net/aws_fota.h>

#include <dfu/mcuboot.h>
#include <misc/reboot.h>

#if defined(CONFIG_BSD_LIBRARY)
#include "nrf_inbuilt_key.h"
#endif

#if !defined(CONFIG_USE_PROVISIONED_CERTIFICATES)
#include "certificates.h"
#endif

#if !defined(CONFIG_CLOUD_CLIENT_ID)
#define IMEI_LEN 15
#define CLIENT_ID_LEN (IMEI_LEN + 4)
#else
#define CLIENT_ID_LEN (sizeof(CONFIG_CLOUD_CLIENT_ID) - 1)
#endif
static u8_t client_id_buf[CLIENT_ID_LEN+1];

/* Buffers for MQTT client. */
static u8_t rx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static u8_t tx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static u8_t payload_buf[CONFIG_MQTT_PAYLOAD_BUFFER_SIZE];

/* MQTT Broker details. */
static struct sockaddr_storage broker_storage;

/* File descriptor */
static struct pollfd fds;

/* Set to true when application should teardown and reboot */
static bool do_reboot;

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

/**@brief Function to print strings without null-termination. */
static void data_print(u8_t *prefix, u8_t *data, size_t len)
{
	char buf[len + 1];

	memcpy(buf, data, len);
	buf[len] = 0;
	printk("%s%s\n", prefix, buf);
}

/**@brief Function to read the published payload.
 */
static int publish_get_payload(struct mqtt_client *c,
			       u8_t *write_buf,
			       size_t length)
{
	u8_t *buf = write_buf;
	u8_t *end = buf + length;

	if (length > sizeof(payload_buf)) {
		return -EMSGSIZE;
	}
	while (buf < end) {
		int ret = mqtt_read_publish_payload_blocking(c, buf, end - buf);

		if (ret < 0) {
			return ret;
		} else if (ret == 0) {
			return -EIO;
		}
		buf += ret;
	}
	return 0;
}

/**@brief MQTT client event handler */
void mqtt_evt_handler(struct mqtt_client * const c,
		      const struct mqtt_evt *evt)
{
	int err;

	err = aws_fota_mqtt_evt_handler(c, evt);
	if (err > 0) {
		/* Event handled by FOTA library so we can skip it */
		return;
	} else if (err < 0) {
		printk("aws_fota_mqtt_evt_handler: Failed! %d\n", err);
		printk("Disconnecting MQTT client...\n");
		err = mqtt_disconnect(c);
		if (err) {
			printk("Could not disconnect: %d\n", err);
		}
	}

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			printk("MQTT connect failed %d\n", evt->result);
			break;
		}

		printk("[%s:%d] MQTT client connected!\n", __func__, __LINE__);
		if (err) {
			printk("Unable to initialize AWS jobs upon "
			       "connection\n");
			err = mqtt_disconnect(c);
			if (err) {
				printk("Could not disconnect: %d\n", err);
			}
		}
		break;

	case MQTT_EVT_DISCONNECT:
		printk("[%s:%d] MQTT client disconnected %d\n", __func__,
		       __LINE__, evt->result);
		break;

	case MQTT_EVT_PUBLISH: {
		const struct mqtt_publish_param *p = &evt->param.publish;

		printk("[%s:%d] MQTT PUBLISH result=%d len=%d\n", __func__,
		       __LINE__, evt->result, p->message.payload.len);
		err = publish_get_payload(c,
					  payload_buf,
					  p->message.payload.len);
		if (err) {
			printk("mqtt_read_publish_payload: Failed! %d\n", err);
			printk("Disconnecting MQTT client...\n");

			err = mqtt_disconnect(c);
			if (err) {
				printk("Could not disconnect: %d\n", err);
			}
		}

		if (p->message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
			const struct mqtt_puback_param ack = {
				.message_id = p->message_id
			};

			/* Send acknowledgment. */
			err = mqtt_publish_qos1_ack(c, &ack);
			if (err) {
				printk("unable to ack\n");
			}
		}
		data_print("Received: ", payload_buf, p->message.payload.len);
		break;
	}

	case MQTT_EVT_PUBACK:
		if (evt->result != 0) {
			printk("MQTT PUBACK error %d\n", evt->result);
			break;
		}

		printk("[%s:%d] PUBACK packet id: %u\n", __func__, __LINE__,
		       evt->param.puback.message_id);
		break;

	case MQTT_EVT_SUBACK:
		if (evt->result != 0) {
			printk("MQTT SUBACK error %d\n", evt->result);
			break;
		}

		printk("[%s:%d] SUBACK packet id: %u\n", __func__, __LINE__,
		       evt->param.suback.message_id);
		break;

	default:
		printk("[%s:%d] default: %d\n", __func__, __LINE__,
		       evt->type);
		break;
	}
}


/**@brief Resolves the configured hostname and
 * initializes the MQTT broker structure
 */
static void broker_init(const char *hostname)
{
	int err;
	struct addrinfo *result;
	struct addrinfo *addr;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM
	};

	err = getaddrinfo(hostname, NULL, &hints, &result);
	if (err) {
		printk("ERROR: getaddrinfo failed %d\n", err);

		return;
	}

	addr = result;
	err = -ENOENT;

	while (addr != NULL) {
		/* IPv4 Address. */
		if (addr->ai_addrlen == sizeof(struct sockaddr_in)) {
			struct sockaddr_in *broker =
				((struct sockaddr_in *)&broker_storage);

			broker->sin_addr.s_addr =
				((struct sockaddr_in *)addr->ai_addr)
				->sin_addr.s_addr;
			broker->sin_family = AF_INET;
			broker->sin_port = htons(CONFIG_MQTT_BROKER_PORT);

			printk("IPv4 Address 0x%08x\n", broker->sin_addr.s_addr);
			break;
		} else if (addr->ai_addrlen == sizeof(struct sockaddr_in6)) {
			/* IPv6 Address. */
			struct sockaddr_in6 *broker =
				((struct sockaddr_in6 *)&broker_storage);

			memcpy(broker->sin6_addr.s6_addr,
				((struct sockaddr_in6 *)addr->ai_addr)
				->sin6_addr.s6_addr,
				sizeof(struct in6_addr));
			broker->sin6_family = AF_INET6;
			broker->sin6_port = htons(CONFIG_MQTT_BROKER_PORT);

			printk("IPv6 Address");
			break;
		} else {
			printk("error: ai_addrlen = %u should be %u or %u\n",
				(unsigned int)addr->ai_addrlen,
				(unsigned int)sizeof(struct sockaddr_in),
				(unsigned int)sizeof(struct sockaddr_in6));
		}

		addr = addr->ai_next;
		break;
	}

	/* Free the address. */
	freeaddrinfo(result);
}

#if !defined(CONFIG_USE_PROVISIONED_CERTIFICATES)
static int provision_certificates(void)
{
	{
		int err;

		/* Delete certificates */
		nrf_sec_tag_t sec_tag = CONFIG_CLOUD_CERT_SEC_TAG;

		for (nrf_key_mgnt_cred_type_t type = 0; type < 3; type++) {
			err = nrf_inbuilt_key_delete(sec_tag, type);
			printk("nrf_inbuilt_key_delete(%u, %d) => result=%d\n",
				sec_tag, type, err);
		}

		/* Provision CA Certificate. */
		err = nrf_inbuilt_key_write(CONFIG_CLOUD_CERT_SEC_TAG,
					NRF_KEY_MGMT_CRED_TYPE_CA_CHAIN,
					CLOUD_CA_CERTIFICATE,
					strlen(CLOUD_CA_CERTIFICATE));
		printk("nrf_inbuilt_key_write => result=%d\n", err);
		if (err) {
			printk("CLOUD_CA_CERTIFICATE err: %d", err);
			return err;
		}

		/* Provision Private Certificate. */
		err = nrf_inbuilt_key_write(
			CONFIG_CLOUD_CERT_SEC_TAG,
			NRF_KEY_MGMT_CRED_TYPE_PRIVATE_CERT,
			CLOUD_CLIENT_PRIVATE_KEY,
			strlen(CLOUD_CLIENT_PRIVATE_KEY));
		printk("nrf_inbuilt_key_write => result=%d\n", err);
		if (err) {
			printk("NRF_CLOUD_CLIENT_PRIVATE_KEY err: %d", err);
			return err;
		}

		/* Provision Public Certificate. */
		err = nrf_inbuilt_key_write(
			CONFIG_CLOUD_CERT_SEC_TAG,
			NRF_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
				 CLOUD_CLIENT_PUBLIC_CERTIFICATE,
				 strlen(CLOUD_CLIENT_PUBLIC_CERTIFICATE));
		printk("nrf_inbuilt_key_write => result=%d\n", err);
		if (err) {
			printk("CLOUD_CLIENT_PUBLIC_CERTIFICATE err: %d",
				err);
			return err;
		}
	}
	return 0;
}
#endif

static int client_id_get(char *id_buf)
{
#if !defined(CONFIG_CLOUD_CLIENT_ID)
	enum at_cmd_state at_state;
	char imei_buf[IMEI_LEN + 5];

	int err = at_cmd_write("AT+CGSN", imei_buf, (IMEI_LEN + 5), &at_state);
	if (err) {
		printk("Error when trying to do at_cmd_write: %d, at_state: %d", err, at_state);
	}

	snprintf(id_buf, CLIENT_ID_LEN + 1, "nrf-%s", imei_buf);
#else
	memcpy(id_buf, CONFIG_CLOUD_CLIENT_ID, CLIENT_ID_LEN + 1);
#endif /* !defined(NRF_CLOUD_CLIENT_ID) */
	return 0;
}

/**@brief Initialize the MQTT client structure */
static int client_init(struct mqtt_client *client, char *hostname)
{
	mqtt_client_init(client);
	broker_init(hostname);

	int ret = client_id_get(client_id_buf);
	printk("client_id: %s\n", client_id_buf);

	if (ret != 0) {
		return ret;
	}

	/* MQTT client configuration */
	client->broker = &broker_storage;
	client->evt_cb = mqtt_evt_handler;
	client->client_id.utf8 = client_id_buf;
	client->client_id.size = strlen(client_id_buf);
	client->password = NULL;
	client->user_name = NULL;
	client->protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);

	/* MQTT transport configuration */
	client->transport.type = MQTT_TRANSPORT_SECURE;

	static sec_tag_t sec_tag_list[] = {CONFIG_CLOUD_CERT_SEC_TAG};
	struct mqtt_sec_config *tls_config = &(client->transport).tls.config;

	tls_config->peer_verify = 2;
	tls_config->cipher_list = NULL;
	tls_config->cipher_count = 0;
	tls_config->sec_tag_count = ARRAY_SIZE(sec_tag_list);
	tls_config->sec_tag_list = sec_tag_list;
	tls_config->hostname = hostname;

	return 0;
}

/**@brief Initialize the file descriptor structure used by poll. */
static int fds_init(struct mqtt_client *c)
{
	fds.fd = c->transport.tls.sock;
	fds.events = POLLIN;
	return 0;
}

/**@brief Configures modem to provide LTE link.
 *
 * Blocks until link is successfully established.
 */
static void modem_configure(void)
{
#if defined(CONFIG_LTE_LINK_CONTROL)
	BUILD_ASSERT_MSG(!IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT),
			"This sample does not support auto init and connect");
	int err;

	err = at_cmd_init();
	__ASSERT(err == 0, "AT CMD could not be established.");
	printk("LTE Link Connecting ...\n");
	err = lte_lc_init_and_connect();
	__ASSERT(err == 0, "LTE link could not be established.");
	printk("LTE Link Connected!\n");
#endif
}

static void aws_fota_cb_handler(enum aws_fota_evt_id evt)
{
	switch (evt) {
	case AWS_FOTA_EVT_DONE:
		printk("AWS_FOTA_EVT_DONE, rebooting to apply update.\n");
		do_reboot = true;
		break;

	case AWS_FOTA_EVT_ERROR:
		printk("AWS_FOTA_EVT_ERROR\n");
		break;
	}
}

void main(void)
{
	int err;

	/* The mqtt client struct */
	struct mqtt_client client;

	printk("MQTT AWS Jobs FOTA Sample, version: %s\n", CONFIG_APP_VERSION);
	printk("Initializing bsdlib\n");
	err = bsdlib_init();
	switch (err) {
	case MODEM_DFU_RESULT_OK:
		printk("Modem firmware update successful!\n");
		printk("Modem will run the new firmware after reboot\n");
		k_thread_suspend(k_current_get());
		break;
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		printk("Modem firmware update failed\n");
		printk("Modem will run non-updated firmware on reboot.\n");
		break;
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		printk("Modem firmware update failed\n");
		printk("Fatal error.\n");
		break;

	default:
		break;
	}
	printk("Initialized bsdlib\n");

#if !defined(CONFIG_USE_PROVISIONED_CERTIFICATES)
	provision_certificates();
#endif /* CONFIG_USE_PROVISIONED_CERTIFICATES  */
	modem_configure();

	client_init(&client, CONFIG_MQTT_BROKER_HOSTNAME);

	err = aws_fota_init(&client, CONFIG_APP_VERSION, aws_fota_cb_handler);
	if (err != 0) {
		printk("ERROR: aws_fota_init %d\n", err);
		return;
	}

	err = mqtt_connect(&client);
	if (err != 0) {
		printk("ERROR: mqtt_connect %d\n", err);
		return;
	}

	err = fds_init(&client);
	if (err != 0) {
		printk("ERROR: fds_init %d\n", err);
		return;
	}

	/* All initializations were successful mark image as working so that we
	 * will not revert upon reboot. */
	boot_write_img_confirmed();

	while (1) {
		err = poll(&fds, 1, K_SECONDS(CONFIG_MQTT_KEEPALIVE));
		if (err < 0) {
			printk("ERROR: poll %d\n", errno);
			break;
		}

		err = mqtt_live(&client);
		if (err != 0) {
			printk("ERROR: mqtt_live %d\n", err);
			break;
		}

		if ((fds.revents & POLLIN) == POLLIN) {
			err = mqtt_input(&client);
			if (err != 0) {
				printk("ERROR: mqtt_input %d\n", err);
				break;
			}
		}

		if ((fds.revents & POLLERR) == POLLERR) {
			printk("POLLERR\n");
			break;
		}

		if ((fds.revents & POLLNVAL) == POLLNVAL) {
			printk("POLLNVAL\n");
			break;
		}

		if (do_reboot) {
			/* Teardown */
			mqtt_disconnect(&client);
			sys_reboot(0);
		}
	}

	printk("Disconnecting MQTT client...\n");

	err = mqtt_disconnect(&client);
	if (err) {
		printk("Could not disconnect MQTT client. Error: %d\n", err);
	}
}
