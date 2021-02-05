/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <nrf_modem.h>
#include <modem/lte_lc.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include <net/mqtt.h>
#include <net/socket.h>
#include <modem/nrf_modem_lib.h>
#include <net/aws_jobs.h>
#include <net/aws_fota.h>
#include <dfu/mcuboot.h>
#include <power/reboot.h>
#include <random/rand32.h>

BUILD_ASSERT(!IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT),
			"This sample does not support auto init and connect");

#if defined(CONFIG_USE_NRF_CLOUD)
#define NRF_CLOUD_SECURITY_TAG 16842753
#endif

#define MQTT_KEEPALIVE (CONFIG_MQTT_KEEPALIVE * MSEC_PER_SEC)

#if defined(CONFIG_PROVISION_CERTIFICATES)
#include "certificates.h"
#if defined(CONFIG_MODEM_KEY_MGMT)
#include <modem/modem_key_mgmt.h>
#endif
#endif

#if !defined(CONFIG_CLOUD_CLIENT_ID)
/* Define the length of the IMEI AT COMMAND response buffer */
#define CGSN_RESP_LEN 19
#define IMEI_LEN 15
#define CLIENT_ID_LEN (sizeof(CONFIG_NRF_CLOUD_CLIENT_ID_PREFIX) + IMEI_LEN)
#else
#define CLIENT_ID_LEN sizeof(CONFIG_CLOUD_CLIENT_ID)
#endif
static uint8_t client_id_buf[CLIENT_ID_LEN];
static uint8_t current_job_id[AWS_JOBS_JOB_ID_MAX_LEN];

/* Buffers for MQTT client. */
static uint8_t rx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t tx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t payload_buf[CONFIG_MQTT_PAYLOAD_BUFFER_SIZE];

/* MQTT Broker details. */
static struct sockaddr_storage broker_storage;

/* File descriptor */
static struct pollfd fds;

/* Set to true when application should teardown and reboot */
static bool do_reboot;

/* Set to true when application should reconnect the LTE link*/
static bool link_offline;

#if defined(CONFIG_NRF_MODEM_LIB)
/**@brief Recoverable modem library error. */
void nrf_modem_recoverable_error_handler(uint32_t err)
{
	printk("Modem library recoverable error: %u\n", err);
}

#endif /* defined(CONFIG_NRF_MODEM_LIB) */

#if !defined(CONFIG_USE_NRF_CLOUD)
/* Topic for updating shadow topic with version number */
#define AWS "$aws/things/"
#define UPDATE_DELTA_TOPIC AWS "%s/shadow/update"
#define SHADOW_STATE_UPDATE \
"{\"state\":{\"reported\":{\"nrfcloud__dfu_v1__app_v\":\"%s\"}}}"

static int update_device_shadow_version(struct mqtt_client *const client)
{
	struct mqtt_publish_param param;
	char update_delta_topic[strlen(AWS) + strlen("/shadow/update") +
				sizeof(client_id_buf)];
	uint8_t shadow_update_payload[CONFIG_DEVICE_SHADOW_PAYLOAD_SIZE];

	int ret = snprintf(update_delta_topic,
			   sizeof(update_delta_topic),
			   UPDATE_DELTA_TOPIC,
			   client->client_id.utf8);
	uint32_t update_delta_topic_len = ret;

	if (ret >= sizeof(update_delta_topic)) {
		return -ENOMEM;
	} else if (ret < 0) {
		return ret;
	}

	ret = snprintf(shadow_update_payload,
		       sizeof(shadow_update_payload),
		       SHADOW_STATE_UPDATE,
		       CONFIG_APP_VERSION);
	uint32_t shadow_update_payload_len = ret;

	if (ret >= sizeof(shadow_update_payload)) {
		return -ENOMEM;
	} else if (ret < 0) {
		return ret;
	}

	param.message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE;
	param.message.topic.topic.utf8 = update_delta_topic;
	param.message.topic.topic.size = update_delta_topic_len;
	param.message.payload.data = shadow_update_payload;
	param.message.payload.len = shadow_update_payload_len;
	param.message_id = sys_rand32_get();
	param.dup_flag = 0;
	param.retain_flag = 0;

	return mqtt_publish(client, &param);
}
#endif /* !defined(CONFIG_USE_NRF_CLOUD) */

/**@brief Function to print strings without null-termination. */
static void data_print(uint8_t *prefix, uint8_t *data, size_t len)
{
	char buf[len + 1];

	memcpy(buf, data, len);
	buf[len] = 0;
	printk("%s%s\n", prefix, buf);
}

/**@brief Function to read the published payload.
 */
static int publish_get_payload(struct mqtt_client *c,
			       uint8_t *write_buf,
			       size_t length)
{
	uint8_t *buf = write_buf;
	uint8_t *end = buf + length;

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
	if (err == 0) {
		/* Event handled by FOTA library so we can skip it */
		return;
	} else if (err < 0) {
		printk("aws_fota_mqtt_evt_handler: Failed! %d\n", err);
	}

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			printk("MQTT connect failed %d\n", evt->result);
			break;
		}

		printk("[%s:%d] MQTT client connected!\n", __func__, __LINE__);
#if !defined(CONFIG_USE_NRF_CLOUD)
		err = update_device_shadow_version(c);
		if (err) {
			printk("Unable to update device shadow err: %d\n", err);
		}
#endif
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
	char addr_str[INET6_ADDRSTRLEN];
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

			inet_ntop(AF_INET, &broker->sin_addr, addr_str,
				  sizeof(addr_str));
			printk("IPv4 Address %s\n", addr_str);
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

			inet_ntop(AF_INET6, &broker->sin6_addr, addr_str,
				  sizeof(addr_str));
			printk("IPv6 Address %s\n", addr_str);
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
#if defined(CONFIG_PROVISION_CERTIFICATES)
#warning Not for prodcution use. This should only be used once to provisioning the certificates please deselect the provision certificates configuration and compile again.
#define MAX_OF_2 MAX(sizeof(CLOUD_CA_CERTIFICATE),\
		     sizeof(CLOUD_CLIENT_PRIVATE_KEY))
#define MAX_LEN MAX(MAX_OF_2, sizeof(CLOUD_CLIENT_PUBLIC_CERTIFICATE))
static uint8_t certificates[][MAX_LEN] = {{CLOUD_CA_CERTIFICATE},
				       {CLOUD_CLIENT_PRIVATE_KEY},
				       {CLOUD_CLIENT_PUBLIC_CERTIFICATE} };
static const size_t cert_len[] = {
	sizeof(CLOUD_CA_CERTIFICATE) - 1, sizeof(CLOUD_CLIENT_PRIVATE_KEY) - 1,
	sizeof(CLOUD_CLIENT_PUBLIC_CERTIFICATE) - 1
};
static int provision_certificates(void)
{
	int err;

	printk("************************* WARNING *************************\n");
	printk("%s called do not use this in production!\n", __func__);
	printk("This will store the certificates in readable flash and leave\n");
	printk("them exposed on modem_traces. Only use this once for\n");
	printk("provisioning certificates for development to reduce flash tear."
		"\n");
	printk("************************* WARNING *************************\n");
	nrf_sec_tag_t sec_tag = CONFIG_CLOUD_CERT_SEC_TAG;
	enum modem_key_mgmt_cred_type cred[] = {
		MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
		MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT,
		MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
	};

	/* Delete certificates */
	for (enum modem_key_mgmt_cred_type type = 0; type < 3; type++) {
		err = modem_key_mgmt_delete(sec_tag, type);
		printk("modem_key_mgmt_delete(%u, %d) => result=%d\n",
				sec_tag, type, err);
	}

	/* Write certificates */
	for (enum modem_key_mgmt_cred_type type = 0; type < 3; type++) {
		err = modem_key_mgmt_write(sec_tag, cred[type],
				certificates[type], cert_len[type]);
		printk("modem_key_mgmt_write => result=%d\n", err);
	}
	return 0;
}
#endif

static int client_id_get(char *id_buf, size_t len)
{
#if !defined(CONFIG_CLOUD_CLIENT_ID)
	enum at_cmd_state at_state;
	char imei_buf[CGSN_RESP_LEN];
	int err = at_cmd_write("AT+CGSN", imei_buf, sizeof(imei_buf),
				&at_state);

	if (err) {
		printk("Error when trying to do at_cmd_write: %d, at_state: %d",
			err, at_state);
		return err;
	}
	snprintf(id_buf, len, "%s%.*s", CONFIG_NRF_CLOUD_CLIENT_ID_PREFIX,
		 IMEI_LEN, imei_buf);
#else
	memcpy(id_buf, CONFIG_CLOUD_CLIENT_ID, len);
#endif /* !defined(NRF_CLOUD_CLIENT_ID) */
	return 0;
}

/**@brief Initialize the MQTT client structure */
static int client_init(struct mqtt_client *client, char *hostname)
{
	mqtt_client_init(client);
	broker_init(hostname);
	int ret = client_id_get(client_id_buf, sizeof(client_id_buf));

	if (ret != 0) {
		return ret;
	}
	printk("client_id: %s\n", client_id_buf);

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

	static sec_tag_t sec_tag_list[] = {
#ifdef CONFIG_USE_NRF_CLOUD
		NRF_CLOUD_SECURITY_TAG
#else
		CONFIG_CLOUD_CERT_SEC_TAG
#endif
	};
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

/**@brief Configures AT Command interface to the modem link. */
static void at_configure(void)
{
	int err;

	err = at_notif_init();
	__ASSERT(err == 0, "AT Notify could not be initialized.");
	err = at_cmd_init();
	__ASSERT(err == 0, "AT CMD could not be established.");
}

static void aws_fota_cb_handler(struct aws_fota_event *fota_evt)
{
	int err;

	if (fota_evt == NULL) {
		return;
	}

	switch (fota_evt->id) {
	case AWS_FOTA_EVT_START:
		if (aws_fota_get_job_id(current_job_id,
					sizeof(current_job_id)) <= 0) {
			snprintf(current_job_id, sizeof(current_job_id), "N/A");
		}
		printk("AWS_FOTA_EVT_START, job id = %s\n", current_job_id);
		break;

	case AWS_FOTA_EVT_DL_PROGRESS:
		/* CONFIG_FOTA_DOWNLOAD_PROGRESS_EVT must be enabled */
		/* to receive progress events */
		printk("AWS_FOTA_EVT_DL_PROGRESS, %d%% downloaded\n",
			fota_evt->dl.progress);
		break;

	case AWS_FOTA_EVT_DONE:
		printk("AWS_FOTA_EVT_DONE, rebooting to apply update\n");
		do_reboot = true;
		break;

	case AWS_FOTA_EVT_ERASE_PENDING:
		printk("AWS_FOTA_EVT_ERASE_PENDING, reboot or disconnect the "
		       "LTE link\n");
		err = lte_lc_offline();
		if (err) {
			printk("Error turning off the LTE link\n");
			break;
		}
		link_offline = true;
		break;

	case AWS_FOTA_EVT_ERASE_DONE:
		printk("AWS_FOTA_EVT_ERASE_DONE\n");

		if (link_offline) {
			printk("Reconnecting the LTE link\n");

			err = lte_lc_connect();
			if (err) {
				printk("Error reconnecting the LTE link\n");
				break;
			}
			link_offline = false;
		}
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
	printk("Initializing modem library\n");
	err = nrf_modem_lib_init();
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
	case -1:
		printk("Could not initialize modem library.\n");
		printk("Fatal error.\n");
		return;
	default:
		break;
	}
	printk("Initialized modem library\n");

	at_configure();
#if defined(CONFIG_PROVISION_CERTIFICATES)
	provision_certificates();
#endif /* CONFIG_PROVISION_CERTIFICATES */
	printk("LTE Link Connecting ...\n");
	err = lte_lc_init_and_connect();
	__ASSERT(err == 0, "LTE link could not be established.");
	printk("LTE Link Connected!\n");

	client_init(&client, CONFIG_MQTT_BROKER_HOSTNAME);

	err = aws_fota_init(&client, aws_fota_cb_handler);
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
	 * will not revert upon reboot.
	 */
	boot_write_img_confirmed();

	while (1) {
		err = poll(&fds, 1, MQTT_KEEPALIVE);
		if (err < 0) {
			printk("ERROR: poll %d\n", errno);
			break;
		}

		err = mqtt_live(&client);
		if ((err != 0) && (err != -EAGAIN)) {
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
