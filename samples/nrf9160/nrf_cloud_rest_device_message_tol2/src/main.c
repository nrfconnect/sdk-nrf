/*
 * Author: tol2@nordicsemi.no
 * Date: 07/28/2022
 * NRFCloud rest message sample app replicate
 */

#include <zephyr/kernel.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#include <modem/modem_info.h>
#include <zephyr/settings/settings.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_rest.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(nrf_cloud_rest_device_message_sample,
		    CONFIG_NRF_CLOUD_REST_DEVICE_MESSAGE_SAMPLE_LOG_LEVEL);

#define WAIT_TIME_BETWEEN_MESSAGES_SEC 3
#define TIME_TO_RETRY 5
#define MAX_TIME_OUT 60000
#define REST_RX_BUF_SIZE 2100

char device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1];
K_SEM_DEFINE(lte_connect, 0, 1);
char rx_buf[REST_RX_BUF_SIZE];
struct nrf_cloud_rest_context rest_context = {
						   .connect_socket = -1,
						   .timeout_ms = MAX_TIME_OUT,
						   .rx_buf = rx_buf,
						   .rx_buf_len = sizeof(rx_buf),
						   .fragment_size = 0};

void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		LOG_DBG("LTE_LC_EVT_NW_REG_STATUS: %d", evt->nw_reg_status);
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		    (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			break;
		}

		if (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME) {
			LOG_DBG("Network registration status: Connected - Home");
		} else {
			LOG_DBG("Network registration status: Connected - Roaming");
		}

		k_sem_give(&lte_connect);
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		LOG_DBG("LTE changed: Cell ID: %d, Tracking area: %d", evt->cell.id, evt->cell.tac);
		break;
	default:
		break;
	}
}

int lte_connnecting(void)
{
	LOG_INF("Waiting for LTE connection");
	k_sem_reset(&lte_connect);

	int err = lte_lc_init_and_connect_async(lte_handler);

	if (err) {
		LOG_ERR("Failed to get LTE connection, error: %d", err);
		return err;
	}

	k_sem_take(&lte_connect, K_FOREVER);
	LOG_INF("Connected to LTE");
	return 0;
}

void get_date_time_lte(void)
{
	char time_buf[64];
	int err;

	LOG_INF("Waiting for modem to acquire time from network");

	do {
		err = nrf_modem_at_cmd(time_buf, sizeof(time_buf), "AT%%CCLK?");
		if (err) {
			LOG_ERR("Failed to acquire time from network, error: %d. ", err);
			LOG_ERR("Retry in %d seconds", TIME_TO_RETRY);
		}
		k_sleep(K_SECONDS(TIME_TO_RETRY));
	} while (err != 0);

	LOG_INF("Network time acquisition completed, %s", time_buf);
}

int setup(void)
{
	int err;

	err = nrf_cloud_client_id_get(device_id, sizeof(device_id));
	if (err) {
		LOG_ERR("Failed to get device id, error: %d", err);
		return err;
	}
	LOG_INF("Device ID: %s", log_strdup(device_id));

	err = lte_connnecting();
	if (err) {
		LOG_ERR("Failed to establish LTE connection, error: %d", err);
		return err;
	}

	get_date_time_lte();
	return 0;
}

int send_message(const char *const message)
{
	int err;

	err = nrf_cloud_rest_send_device_message(&rest_context, device_id, message, false, NULL);
	if (err) {
		LOG_ERR("Failed to send message to the cloud, error: %d", err);
		return err;
	}

	return 0;
}

void main(void)
{
	int err;
	static const char sample_msg[] = "{\"sample message\":\"Hello World from REST PDX\"}";

	err = setup();
	if (err) {
		LOG_ERR("Setup failed, exiting");
		return;
	}

	LOG_INF("Sending a message to NRF Cloud every %d seconds for %d seconds",
		WAIT_TIME_BETWEEN_MESSAGES_SEC, MAX_TIME_OUT / 1000);
	while (1) {
		err = send_message(sample_msg);
		k_sleep(K_SECONDS(WAIT_TIME_BETWEEN_MESSAGES_SEC));
		if (err) {
			LOG_ERR("Message sending failed, exiting");
			return;
		}
	}
}
