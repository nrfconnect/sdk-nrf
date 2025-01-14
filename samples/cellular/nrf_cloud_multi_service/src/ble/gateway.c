/* Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/reboot.h>
#include <nrf_modem.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <modem/lte_lc.h>
#include <nrf9160.h>
#include <hal/nrf_gpio.h>
#if defined(CONFIG_NRF_MODEM_LIB)
#include <nrf_socket.h>
#endif
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_codec.h>

#include "gateway.h"
#include "nrf_cloud_mem.h"

#include "cJSON.h"
#include "cJSON_os.h"
#include "ble.h"
#include "ble_codec.h"
#include "ble_conn_mgr.h"

LOG_MODULE_REGISTER(gateway, CONFIG_NRFCLOUD_BLE_GATEWAY_LOG_LEVEL);

#define AT_CMNG_READ_LEN 97

#define CLOUD_PROC_STACK_SIZE 2048
#define CLOUD_PROC_PRIORITY 5

#define GET_PSK_ID "AT%CMNG=2,16842753,4"
#define GET_PSK_ID_LEN (sizeof(GET_PSK_ID)-1)
#define GET_PSK_ID_ERR "ERROR"

/* Uncomment below to enable writing characteristics from cloud_data_process()
 * rather than from gateway_handler(). However, writing added too much data
 * to the fifo. There was no memory to unsubscribe from memory intensive
 * subscribes.
 */
#define QUEUE_CHAR_WRITES
#define QUEUE_CHAR_READS

#define VALUE_BUF_SIZE 256
static uint8_t value_buf[VALUE_BUF_SIZE];

char gateway_id[NRF_CLOUD_CLIENT_ID_LEN+1];

struct cloud_data_t {
	void *fifo_reserved;
	char addr[BT_ADDR_STR_LEN];
	char uuid[UUID_STR_LEN];
	uint8_t client_char_config;
	bool read;
	bool ccc;
	bool sub;
#if defined(QUEUE_CHAR_WRITES)
	bool write;
	size_t data_len;
	uint8_t data[VALUE_BUF_SIZE];
#endif
};

static atomic_t queued_cloud_data;
K_FIFO_DEFINE(cloud_data_fifo);

static int gateway_handler(cJSON *json);

int gateway_shadow_delta_handler(const struct nrf_cloud_obj_shadow_delta *delta)
{
	cJSON *root_obj = delta->state.json;
	cJSON *desired_connections_obj = cJSON_GetObjectItem(root_obj, "desiredConnections");

	if (!desired_connections_obj) {
		LOG_DBG("No desired connections provided");
		return 0;
	}

	return desired_conns_handler(desired_connections_obj);
}

int gateway_shadow_accepted_handler(const struct nrf_cloud_obj *desired)
{
	ARG_UNUSED(desired);

	int err = nrf_cloud_shadow_transform_request("state.desired.desiredConnections", 1024);

	if (err) {
		LOG_ERR("Error requesting shadow tf: %d", err);
	}
	return err;
}

int gateway_shadow_transform_handler(const struct nrf_cloud_obj *desired_conns)
{
	if (!desired_conns || (desired_conns->type != NRF_CLOUD_OBJ_TYPE_JSON) ||
	    !(desired_conns->json)) {
		return -EINVAL;
	}
	cJSON *desired_tf = desired_conns->json;
	cJSON *desired = cJSON_GetObjectItem(desired_tf, "tf");

	return desired_conns_handler(desired);
}

static void cloud_data_process(int unused1, int unused2, int unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);
	struct k_mutex lock;
	int ret;

	k_mutex_init(&lock);

	while (1) {
		LOG_DBG("Waiting for cloud_data_fifo");
		struct cloud_data_t *cloud_data = k_fifo_get(&cloud_data_fifo, K_FOREVER);

		atomic_dec(&queued_cloud_data);
		LOG_DBG("Dequeued cloud_data_fifo element; still queued: %ld",
			atomic_get(&queued_cloud_data));

		if (cloud_data != NULL) {
			k_mutex_lock(&lock, K_FOREVER);

			if (cloud_data->sub) {
				LOG_DBG("Dequeued sub msg");
				ble_subscribe(cloud_data->addr,
					      cloud_data->uuid,
					      cloud_data->client_char_config);
			}
#if defined(QUEUE_CHAR_READS)
			else if (cloud_data->read) {
				LOG_DBG("Dequeued gatt_read request %s, %s, %u",
					cloud_data->addr,
					cloud_data->uuid,
					cloud_data->ccc);
				ret = gatt_read(cloud_data->addr,
						cloud_data->uuid,
						cloud_data->ccc);
				if (ret) {
					LOG_ERR("Error on gatt_read(%s, %s, %u): %d",
						cloud_data->addr,
						cloud_data->uuid,
						cloud_data->ccc,
						ret);
				}
			}
#endif
#if defined(QUEUE_CHAR_WRITES)
			else if (cloud_data->write) {
				LOG_DBG("Dequeued gatt write request");
				ret = gatt_write(cloud_data->addr,
						 cloud_data->uuid,
						 cloud_data->data,
						 cloud_data->data_len, NULL);
				if (ret) {
					LOG_ERR("Error on gatt_write(%s, %s): %d",
						cloud_data->addr,
						cloud_data->uuid,
						ret);
				}
			}
#endif
			k_free(cloud_data);
			k_mutex_unlock(&lock);
		}
	}
}

K_THREAD_DEFINE(cloud_proc_thread, CLOUD_PROC_STACK_SIZE,
		cloud_data_process, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

static cJSON *json_object_decode(cJSON *obj, const char *str)
{
	return obj ? cJSON_GetObjectItem(obj, str) : NULL;
}

static bool compare(const char *s1, const char *s2)
{
	return !strncmp(s1, s2, strlen(s2));
}

int gateway_data_handler(const struct nrf_cloud_data *const dev_msg)
{
	struct nrf_cloud_obj msg_obj;
	int err = nrf_cloud_obj_input_decode(&msg_obj, dev_msg);

	if (err) {
		/* The message isn't JSON or otherwise couldn't be parsed. */
		LOG_DBG("A general topic device message of length %d could not be parsed.",
			dev_msg->len);
		return err;
	}

	if (msg_obj.type != NRF_CLOUD_OBJ_TYPE_JSON) {
		LOG_DBG("Wrong message type for gateway: %d", msg_obj.type);
		err = -EINVAL;
	} else {
		err = gateway_handler(msg_obj.json);
	}

	(void)nrf_cloud_obj_free(&msg_obj);
	return err;
}

static int gateway_handler(cJSON *root_obj)
{
	int ret = 0;
	cJSON *desired_obj;

	cJSON *type_obj;
	cJSON *operation_obj;

	cJSON *ble_address;

	cJSON *chrc_uuid;
	cJSON *service_uuid;
	cJSON *desc_arr;
	uint8_t desc_buf[2] = {0};
	uint8_t desc_len = 0;

	cJSON *value_arr;
	uint8_t value_len = 0;

	if (root_obj == NULL) {
		return -ENOENT;
	}

	type_obj = json_object_decode(root_obj, "type");

	if (type_obj == NULL) {
		ret = -ENOENT;
		goto exit_handler;
	}

	const char *type_str = type_obj->valuestring;

	if (!compare(type_str, "operation")) {
		ret = -EINVAL;
		goto exit_handler;
	}

	operation_obj = json_object_decode(root_obj, "operation");
	desired_obj = json_object_decode(operation_obj, "type");

	if (compare(desired_obj->valuestring, "scan")) {
		desired_obj = json_object_decode(operation_obj, "scanType");
		switch (desired_obj->valueint) {
		case 0:
			/* submit k_work to respond */
			LOG_INF("Cloud requested BLE scan");
			scan_start(false);
			break;
		case 1:
			/* submit k_work to respond */
			/* TODO: Add beacon support */
			break;
		default:
			break;
		}

	} else if (compare(desired_obj->valuestring,
			   "device_characteristic_value_read")) {
		ble_address = json_object_decode(operation_obj,
						 "deviceAddress");
		chrc_uuid = json_object_decode(operation_obj,
					       "characteristicUUID");

		LOG_INF("Got device_characteristic_value_read: %s",
			ble_address->valuestring);
		if ((ble_address != NULL) && (chrc_uuid != NULL)) {
#if defined(QUEUE_CHAR_READS)
			struct cloud_data_t cloud_data = {
				.read = true,
				.ccc = false,
				.sub = false
			};

			memcpy(&cloud_data.addr,
			       ble_address->valuestring,
			       strlen(ble_address->valuestring));
			memcpy(&cloud_data.uuid,
			       chrc_uuid->valuestring,
			       strlen(chrc_uuid->valuestring));

			size_t size = sizeof(struct cloud_data_t);
			char *mem_ptr = k_malloc(size);

			if (mem_ptr == NULL) {
				LOG_ERR("Out of memory!");
				ret = -ENOMEM;
				goto exit_handler;
			}

			memcpy(mem_ptr, &cloud_data, size);
			atomic_inc(&queued_cloud_data);
			k_fifo_put(&cloud_data_fifo, mem_ptr);
			LOG_DBG("Queued device_characteristic_value_read addr:%s, uuid:%s; "
				"queued: %ld",
				cloud_data.addr, cloud_data.uuid, atomic_get(&queued_cloud_data));
#else
			ret = gatt_read(ble_address->valuestring, chrc_uuid->valuestring, false);
			if (ret) {
				LOG_ERR("Error on gatt_read(%s, %s, 0): %d",
					ble_address->valuestring,
					chrc_uuid->valuestring,
					ret);
			}
#endif
		}

	} else if (compare(desired_obj->valuestring,
			   "device_descriptor_value_read")) {

		ble_address = json_object_decode(operation_obj,
						 "deviceAddress");
		chrc_uuid = json_object_decode(operation_obj,
					       "characteristicUUID");

		LOG_INF("Got device_descriptor_value_read: %s",
			ble_address->valuestring);
		if ((ble_address != NULL) && (chrc_uuid != NULL)) {
#if defined(QUEUE_CHAR_READS)
			struct cloud_data_t cloud_data = {
				.read = true,
				.ccc = true,
				.sub = false
			};

			memcpy(&cloud_data.addr,
			       ble_address->valuestring,
			       strlen(ble_address->valuestring));
			memcpy(&cloud_data.uuid,
			       chrc_uuid->valuestring,
			       strlen(chrc_uuid->valuestring));

			size_t size = sizeof(struct cloud_data_t);
			char *mem_ptr = k_malloc(size);

			if (mem_ptr == NULL) {
				LOG_ERR("Out of memory!");
				ret = -ENOMEM;
				goto exit_handler;
			}

			memcpy(mem_ptr, &cloud_data, size);
			atomic_inc(&queued_cloud_data);
			k_fifo_put(&cloud_data_fifo, mem_ptr);
			LOG_DBG("Queued device_descriptor_value_read addr:%s, uuid:%s, queued: %ld",
				cloud_data.addr, cloud_data.uuid, atomic_get(&queued_cloud_data));
#else
			ret = gatt_read(ble_address->valuestring, chrc_uuid->valuestring, true);
			if (ret) {
				LOG_ERR("Error on gatt_read(%s, %s, 1): %d",
					ble_address->valuestring,
					chrc_uuid->valuestring,
					ret);
			}
#endif
		}

	} else if (compare(desired_obj->valuestring,
			   "device_descriptor_value_write")) {
		ble_address = json_object_decode(operation_obj,
						 "deviceAddress");
		chrc_uuid = json_object_decode(operation_obj,
					       "characteristicUUID");
		desc_arr = json_object_decode(operation_obj,
					      "descriptorValue");

		desc_len = cJSON_GetArraySize(desc_arr);
		for (int i = 0; i < desc_len; i++) {
			cJSON *item = cJSON_GetArrayItem(desc_arr, i);

			desc_buf[i] = item->valueint;
		}

		LOG_DBG("Got device_descriptor_value_write "
			"addr: %s, uuid: %s, value: (0x%02X, 0x%02X)",
			ble_address->valuestring, chrc_uuid->valuestring,
			desc_buf[0], desc_buf[1]);

		if ((ble_address != NULL) && (chrc_uuid != NULL)) {
#if defined(QUEUE_CHAR_WRITES)
			struct cloud_data_t cloud_data = {
				.sub = true,
				.read = false
			};

			memcpy(&cloud_data.addr,
			       ble_address->valuestring,
			       strlen(ble_address->valuestring));
			memcpy(&cloud_data.uuid,
			       chrc_uuid->valuestring,
			       strlen(chrc_uuid->valuestring));

			cloud_data.client_char_config = desc_buf[0];

			size_t size = sizeof(struct cloud_data_t);
			char *mem_ptr = k_malloc(size);

			if (mem_ptr == NULL) {
				LOG_ERR("Out of memory!");
				ret = -ENOMEM;
				goto exit_handler;
			}

			memcpy(mem_ptr, &cloud_data, size);
			atomic_inc(&queued_cloud_data);
			k_fifo_put(&cloud_data_fifo, mem_ptr);
			LOG_DBG("Queued device_descriptor_value_write addr:%s, uuid:%s, "
				"ccc:%d, sub:%d, conf:%d, queued:%ld",
				cloud_data.addr,
				cloud_data.uuid,
				cloud_data.ccc, cloud_data.sub,
				cloud_data.client_char_config,
				atomic_get(&queued_cloud_data));
#else
			ret = gatt_write(ble_address->valuestring,
					 chrc_uuid->valuestring,
					 desc_buf, desc_len, NULL);
			if (ret) {
				LOG_ERR("Error on gatt_write(%s, %s): %d",
					ble_address->valuestring,
					chrc_uuid->valuestring,
					ret);
			}
#endif
		}

	} else if (compare(desired_obj->valuestring,
			   "device_characteristic_value_write")) {
		ble_address = json_object_decode(operation_obj,
						 "deviceAddress");
		chrc_uuid = json_object_decode(operation_obj,
					       "characteristicUUID");
		service_uuid = json_object_decode(operation_obj,
						  "serviceUUID");
		value_arr = json_object_decode(operation_obj,
					       "characteristicValue");

		if (cJSON_IsString(value_arr)) {
			char *str = cJSON_GetStringValue(value_arr);

			strncpy(value_buf, str, sizeof(value_buf) - 1);
			value_len = strlen(value_buf);
		} else {
			value_len = MIN(cJSON_GetArraySize(value_arr), VALUE_BUF_SIZE);

			for (int i = 0; i < value_len; i++) {
				cJSON *item = cJSON_GetArrayItem(value_arr, i);

				value_buf[i] = item->valueint;
			}
		}

		LOG_DBG("Got device_characteristic_value_write "
			"addr: %s, svc uuid:%s, chrc uuid:%s",
			ble_address->valuestring,
			service_uuid ? service_uuid->valuestring : "n/a",
			chrc_uuid ? chrc_uuid->valuestring : "n/a");
		LOG_HEXDUMP_DBG(value_buf, value_len, "value");

		if ((ble_address != NULL) && (chrc_uuid != NULL)) {
#if defined(QUEUE_CHAR_WRITES)
			struct cloud_data_t cloud_data = {
				.write = true,
				.data_len = value_len
			};

			memcpy(&cloud_data.addr,
			       ble_address->valuestring,
			       strlen(ble_address->valuestring));
			memcpy(&cloud_data.uuid,
			       chrc_uuid->valuestring,
			       strlen(chrc_uuid->valuestring));
			memcpy(&cloud_data.data,
			       value_buf,
			       value_len);

			size_t size = sizeof(struct cloud_data_t);
			char *mem_ptr = k_malloc(size);

			if (mem_ptr == NULL) {
				LOG_ERR("Out of memory!");
				ret = -ENOMEM;
				goto exit_handler;
			}

			memcpy(mem_ptr, &cloud_data, size);
			atomic_inc(&queued_cloud_data);
			k_fifo_put(&cloud_data_fifo, mem_ptr);
			LOG_DBG("Queued device_characteristic_value_write "
				"addr:%s, uuid:%s, queued:%ld",
				cloud_data.addr, cloud_data.uuid, atomic_get(&queued_cloud_data));
#else
			ret = gatt_write(ble_address->valuestring,
					 chrc_uuid->valuestring,
					 value_buf, value_len, NULL);
			if (ret) {
				LOG_ERR("Error on gatt_write(%s, %s): %d",
					ble_address->valuestring,
					chrc_uuid->valuestring,
					ret);
			}
#endif
		}

	} else if (compare(desired_obj->valuestring, "device_discover")) {
		ble_address = json_object_decode(operation_obj,
						 "deviceAddress");
		if (ble_address != NULL) {
			LOG_INF("Cloud requested device_discover: %s",
				ble_address->valuestring);
			ble_conn_mgr_rediscover(ble_address->valuestring);
		}
	}

exit_handler:
	return ret;
}

void set_log_panic(void)
{
	LOG_PANIC();
}

static void starting_button_handler(void)
{
#if defined(CONFIG_ENTER_52840_MCUBOOT_VIA_BUTTON)
	/* the active board file need to include the functions:
	 * nrf52840_reset_to_mcuboot() and nrf52840_wait_boot_low()
	 */
	if (ui_button_is_active(1)) {
		printk("BOOT BUTTON HELD\n");
		/* ui_led_set_pattern(UI_BLE_BUTTON, PWM_DEV_1); */
		while (ui_button_is_active(1)) {
			k_sleep(K_MSEC(500));
		}
		printk("BOOT BUTTON RELEASED\n");
		if (!is_boot_selected()) {
			printk("Boot held after reset but not long enough"
			       " to select the nRF52840 bootloader!\n");
			/* ui_led_set_pattern(UI_BLE_OFF, PWM_DEV_1); */
		} else {
			/* User wants to update the 52840 */
			nrf52840_reset_to_mcuboot();

			/* wait forever for signal from other side so we can
			 * continue
			 */
			int err;

			err = nrf52840_wait_boot_complete(WAIT_BOOT_TIMEOUT_MS);
			if (err == 0) {
				/* ui_led_set_pattern(UI_BLE_OFF, PWM_DEV_1); */
				k_sleep(K_SECONDS(1));
				printk("nRF52840 update complete\n");
			} else {
				/* unable to monitor; just halt */
				printk("Error waiting for nrf52840 reboot: %d\n",
				       err);
				for (;;) {
					k_sleep(K_MSEC(500));
				}
			}
		}
	}
#endif
}

void init_gateway(void)
{
	int err;

	starting_button_handler();
	err = ble_init();
	if (err) {
		LOG_ERR("Error initializing BLE: %d", err);
	} else {
		/* Set BLE is disconnected LED pattern */
	}

	ble_codec_init();

#if CONFIG_GATEWAY_BLE_FOTA
	ret = peripheral_dfu_init();
	if (ret) {
		LOG_ERR("Error initializing BLE DFU: %d", ret);
	}
#endif
}

void reset_gateway(void)
{
	ble_conn_mgr_init();
	ble_conn_mgr_clear_desired(true);
	ble_stop_activity();
}

int g2c_send(const struct nrf_cloud_data *output)
{
	struct nrf_cloud_tx_data msg;

	msg.data.ptr = output->ptr;
	msg.data.len = output->len;
	msg.topic_type = NRF_CLOUD_TOPIC_MESSAGE;
	msg.qos = MQTT_QOS_1_AT_LEAST_ONCE;
	msg.id = 0;
	msg.obj = NULL;

	return nrf_cloud_send(&msg);
}

int gw_shadow_publish(const struct nrf_cloud_data *output)
{
	struct nrf_cloud_tx_data msg;

	msg.data.ptr = output->ptr;
	msg.data.len = output->len;
	msg.topic_type = NRF_CLOUD_TOPIC_STATE;
	msg.qos = MQTT_QOS_1_AT_LEAST_ONCE;
	msg.id = 0;
	msg.obj = NULL;

	return nrf_cloud_send(&msg);
}

void device_shutdown(bool reboot)
{
	int err;

	LOG_PANIC();
	if (!reboot) {
		LOG_INF("Shutting down...");
#if defined(CONFIG_MODEM_WAKEUP_PIN)
		nrf_gpio_cfg_input(CONFIG_MODEM_WAKEUP_PIN,
				   NRF_GPIO_PIN_PULLUP);
		nrf_gpio_cfg_sense_set(CONFIG_MODEM_WAKEUP_PIN,
				       NRF_GPIO_PIN_SENSE_LOW);
#endif
	}

	/* Set BLE shutdown pattern */

	LOG_INF("Disconnect from cloud...");
	err = nrf_cloud_disconnect();
	if (err) {
		LOG_ERR("Error closing cloud: %d",
			err);
	}

	LOG_INF("Power off LTE...");
	err = lte_lc_power_off();
	if (err) {
		LOG_ERR("Error powering off LTE: %d",
			err);
	}

	LOG_INF("Shutdown modem...");
	err = nrf_modem_shutdown();
	if (err) {
		LOG_ERR("Error on bsd_shutdown(): %d",
			err);
	}

	if (reboot) {
		LOG_INF("Rebooting...");
		k_sleep(K_SECONDS(1));

#if defined(CONFIG_REBOOT)
		sys_reboot(SYS_REBOOT_COLD);
#else
		LOG_ERR("sys_reboot not defined: "
			"enable CONFIG_REBOOT and rebuild");
#endif
	} else {
		LOG_INF("Power down.");
		k_sleep(K_SECONDS(1));
		NRF_REGULATORS_NS->SYSTEMOFF = 1;
	}
}
