/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <time.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/init.h>
#include <zephyr/random/random.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>

#include <modem/lte_lc.h>
#include <modem/modem_key_mgmt.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_attest_token.h>
#include <net/nrf_provisioning.h>
#include <net/rest_client.h>
#include <date_time.h>

#include "nrf_provisioning_at.h"
#include "nrf_provisioning_http.h"
#include "nrf_provisioning_codec.h"
#include "nrf_provisioning_internal.h"

#include "nrf_provisioning_coap.h"

#include "cert_amazon_root_ca1.h"
#include "cert_coap_root_ca.h"

LOG_MODULE_REGISTER(nrf_provisioning, CONFIG_NRF_PROVISIONING_LOG_LEVEL);

K_MUTEX_DEFINE(np_mtx);
K_CONDVAR_DEFINE(np_cond);

/* An arbitrary max backoff interval if connection to server times out [s] */
#define SRV_TIMEOUT_BACKOFF_MAX_S 86400
#define SETTINGS_STORAGE_PREFIX CONFIG_NRF_PROVISIONING_SETTINGS_STORAGE_PATH

static bool nw_connected = true;
static bool reschedule;

/* nRF Provisioning context */
static struct nrf_provisioning_http_context rest_ctx = {
	.connect_socket = REST_CLIENT_SCKT_CONNECT,
	.keep_alive = false,
	.rx_buf = NULL,
	.rx_buf_len = 0,
	.auth = NULL,
};

/* nRF Provisioning context */
static struct nrf_provisioning_coap_context coap_ctx = {
	.connect_socket = -1,
	.rx_buf = NULL,
	.rx_buf_len = 0,
};

static bool initialized;

static time_t nxt_provisioning;

static nrf_provisioning_event_cb_t callback_local;

NRF_MODEM_LIB_ON_INIT(nrf_provisioning_init_hook, nrf_provisioning_on_modem_init, NULL);

static void nrf_provisioning_on_modem_init(int ret, void *ctx)
{
	int err;

	if (ret != 0) {
		LOG_ERR("Modem library did not initialize: %d", ret);
		return;
	}

	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_AUTO_INIT)) {
		err = nrf_provisioning_init(NULL);
		if (err) {
			LOG_ERR("Failed to initialize provisioning client");
		}
	}
}

static int cert_provision(void)
{
	int ret = 0;
	bool exists;
#if defined(CONFIG_NRF_PROVISIONING_HTTP)
		/* Certificate for nRF HTTP Provisioning */
		const uint8_t *cert = cert_amazon_root_ca1_pem;
		const size_t cert_size = cert_amazon_root_ca1_pem_size;
#else
		/* Certificate for nRF COAP Provisioning */
		const uint8_t *cert = cert_coap_root_ca_pem;
		const size_t cert_size = cert_coap_root_ca_pem_size;
#endif

#if !defined(CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG)
#error CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG not defined!
#endif

	if (cert_size > KB(4)) {
		LOG_ERR("Certificate too large");
		__ASSERT(false, "Certificate too large");
		return -ENFILE;
	}

	ret = modem_key_mgmt_exists(CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG,
				    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);
	if (ret) {
		__ASSERT_NO_MSG(false);
		LOG_ERR("Failed to check for certificates err %d", ret);
		return ret;
	}

	/* Don't overwrite certificate if one has been provisioned */
	if (exists || !cert_size) {
		ret = 0;
		return 0;
	}

	ret = nrf_provisioning_notify_event_and_wait_for_modem_state(
		120, NRF_PROVISIONING_EVENT_NEED_LTE_DEACTIVATED, callback_local);
	if (ret) {
		LOG_ERR("Failed to set modem online, err %d", ret);
		return ret;
	}

	LOG_INF("Provisioning new certificate");
	LOG_HEXDUMP_DBG(cert, cert_size, "New certificate: ");

	ret = modem_key_mgmt_write(CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   cert, cert_size);
	if (ret) {
		__ASSERT_NO_MSG(false);
		LOG_ERR("Failed to provision certificate, err %d", ret);
		return ret;
	}

	ret = nrf_provisioning_notify_event_and_wait_for_modem_state(
		120, NRF_PROVISIONING_EVENT_NEED_LTE_ACTIVATED, callback_local);
	if (ret) {
		LOG_ERR("Failed to set modem online, err %d", ret);
		return ret;
	}

	return 0;
}

int nrf_provisioning_set(const char *key, size_t len_rd,
			 settings_read_cb read_cb, void *cb_arg)
{
	int len;
	int key_len;
	const char *next;
	char time_str[sizeof(STRINGIFY(9223372036854775807))] = {0};
	char * const latest_cmd_id = nrf_provisioning_codec_get_latest_cmd_id();

	if (!key) {
		return -ENOENT;
	}

	key_len = settings_name_next(key, &next);

	if (strncmp(key, "interval-sec", key_len) == 0) {
		len = read_cb(cb_arg, &time_str, sizeof(time_str));
		if (len <= 0) {
			LOG_ERR("Unable to read the timestamp of next provisioning");
			memset(&time_str, 0, sizeof(time_str));
		}

		if (strlen(time_str) == 0) {
			nxt_provisioning = 0;
			LOG_INF("Initial provisioning");
		} else {
			nxt_provisioning = (time_t) strtoll(time_str, NULL, 10);
			LOG_DBG("Stored interval: \"%jd\"", nxt_provisioning);
		}

		return 0;
	} else if (strncmp(key, NRF_PROVISIONING_CORRELATION_ID_KEY, key_len) == 0) {
		/* Loaded only once */
		if (strcmp(latest_cmd_id, "") != 0) {
			return 0;
		}

		len = read_cb(cb_arg, latest_cmd_id,
			NRF_PROVISIONING_CORRELATION_ID_SIZE);
		if (len <= 0) {
			LOG_INF("No provisioning information stored");
		}

		if (strlen(latest_cmd_id) == 0) {
			LOG_INF("Initial provisioning");
		}

		return 0;
	}


	return -ENOENT;
}

int nrf_provisioning_notify_event_and_wait_for_modem_state(
				int timeout_seconds,
				enum nrf_provisioning_event event,
				nrf_provisioning_event_cb_t callback)
{
	int ret;
	enum lte_lc_func_mode fmode;

	if (callback == NULL) {
		LOG_ERR("Callback data is NULL");
		return -EINVAL;
	}

	if ((event != NRF_PROVISIONING_EVENT_NEED_LTE_DEACTIVATED) &&
	    (event != NRF_PROVISIONING_EVENT_NEED_LTE_ACTIVATED)) {
		LOG_ERR("Invalid event");
		return -EINVAL;
	}

	struct nrf_provisioning_callback_data event_data = {
		.type = event,
	};

	callback(&event_data);

	for (int i = 0; i < timeout_seconds; i++) {
		ret = lte_lc_func_mode_get(&fmode);
		if (ret) {
			LOG_ERR("Failed to read modem functional mode");
			return ret;
		}

		if (event == NRF_PROVISIONING_EVENT_NEED_LTE_DEACTIVATED) {
			if (fmode == LTE_LC_FUNC_MODE_OFFLINE) {
				LOG_DBG("Modem is offline");
				return 0;
			}
		} else if (event == NRF_PROVISIONING_EVENT_NEED_LTE_ACTIVATED) {
			if (nw_connected) {
				LOG_DBG("Modem is registered to LTE network");
				return 0;
			}
		}

		k_sleep(K_SECONDS(1));
	}

	LOG_ERR("Timeout waiting for the desired functional mode");

	return -ETIMEDOUT;
}

static struct settings_handler settings = {
	.name = SETTINGS_STORAGE_PREFIX,
	.h_set = nrf_provisioning_set,
};

static int settings_init(void)
{
	int err;
	static bool init;

	if (init) {
		return -EALREADY;
	}

	err = settings_subsys_init();
	if (err) {
		LOG_ERR("settings_subsys_init failed, error: %d", err);
		return err;
	}

	err = settings_register(&settings);
	if (err) {
		LOG_ERR("settings_register failed, error: %d", err);
		return err;
	}

	err = settings_load_subtree(settings.name);
	if (err) {
		LOG_ERR("settings_load_subtree failed, error: %d", err);
		return err;
	}

	init = true;

	return 0;
}

static void nrf_provisioning_callback(const struct nrf_provisioning_callback_data *event)
{

#if !CONFIG_UNITY
	switch (event->type) {
	case NRF_PROVISIONING_EVENT_FATAL_ERROR:
		LOG_ERR("Provisioning error");
		break;
	case NRF_PROVISIONING_EVENT_DONE:
		/* Disconnect from network gracefully */
		int ret = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE);

		if (ret != 0) {
			LOG_ERR("Unable to set modem offline, error %d", ret);
		}

		LOG_INF("Provisioning done, rebooting...");
		LOG_PANIC();

		sys_reboot(SYS_REBOOT_WARM);
		break;
	case NRF_PROVISIONING_EVENT_NEED_LTE_DEACTIVATED:
		LOG_INF("Modem offline needed");

		enum lte_lc_func_mode mode = LTE_LC_FUNC_MODE_OFFLINE;

		ret = lte_lc_func_mode_set(mode);
		if (ret) {
			LOG_ERR("Unable to set modem offline, error: %d", ret);
			return;
		}

		break;
	case NRF_PROVISIONING_EVENT_NEED_LTE_ACTIVATED: {
		LOG_INF("Modem online needed");

		char time_buf[64];

		ret = lte_lc_connect();
		if (ret) {
			LOG_ERR("lte_lc_connect() failed %d", ret);
		}

		LOG_INF("Modem connection restored");
		LOG_INF("Waiting for modem to acquire network time...");

		do {
			k_sleep(K_SECONDS(3));
			ret = nrf_provisioning_at_time_get(time_buf, sizeof(time_buf));

		} while (ret != 0);

		LOG_INF("Network time obtained");
	};
		break;
	default:
		break;
	}
#endif

}

static void nrf_provisioning_lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		LOG_DBG("LTE_LC_EVT_NW_REG_STATUS: %d", evt->nw_reg_status);
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		    (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			if (initialized) {
				nw_connected = false;
				LOG_INF("Disconnected from network - provisioning paused");
			}
			break;
		}

		LOG_INF("%s - provisioning resumed",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
				"Connected; home network" :
				"Connected; roaming");
		nw_connected = true;
		break;
	default:
		break;
	}
}

int nrf_provisioning_init(nrf_provisioning_event_cb_t callback_handler)
{
	int ret;

	k_mutex_lock(&np_mtx, K_FOREVER);

	if (!callback_handler) {
		callback_local = nrf_provisioning_callback;
	} else {
		callback_local = callback_handler;
	}

	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_HTTP)) {
		ret = nrf_provisioning_http_init(callback_local);
	} else {
		ret = nrf_provisioning_coap_init(callback_local);
	}

	if (ret) {
		goto exit;
	}

	if (initialized) {
		goto exit;
	}

	ret = settings_init();
	if (ret == -EALREADY) {
		ret = 0;
	} else if (ret != 0) {
		LOG_ERR("Can't initialize settings");
		goto exit;
	}

	initialized = true;

	lte_lc_register_handler(nrf_provisioning_lte_handler);

	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_AUTO_START_ON_INIT)) {
		/* Let the provisioning thread run */
		k_condvar_signal(&np_cond);
	}
exit:
	k_mutex_unlock(&np_mtx);

	return ret;
}

int nrf_provisioning_trigger_manually(void)
{
	int ret = k_mutex_lock(&np_mtx, K_NO_WAIT);

	if (ret < 0) {
		return ret;
	}

	if (!initialized) {
		LOG_ERR("Not initialized");
		k_mutex_unlock(&np_mtx);
		return -EFAULT;
	}

	/* Let the provisioning thread run */
	k_condvar_signal(&np_cond);
	k_mutex_unlock(&np_mtx);

	LOG_INF("Externally initiated provisioning");

	return 0;
}

/**
 * @brief Time to next provisioning.
 *
 * @return <0 on error, >=0 denotes how many seconds till next provisioning.
 */
int nrf_provisioning_schedule(void)
{
	int ret;
	int64_t now_s;
	static time_t retry_s = 1;
	static time_t deadline_s;
	time_t spread_s;
	static bool first = true;

	if (first) {
		first = false;
		nxt_provisioning = CONFIG_NRF_PROVISIONING_INTERVAL_S;
		goto out;
	}

	ret = date_time_now(&now_s);
	if (ret < 0) {
		if (ret != -ENODATA) {
			__ASSERT(false, "Getting time failed, error: %d", ret);
			LOG_ERR("Getting time failed, error: %d", ret);
		}

		if (nxt_provisioning) {
			retry_s = nxt_provisioning;
		} else {
			/* Backoff... */
			retry_s = retry_s * 2;

			/* ...up to a degree */
			if (retry_s > CONFIG_NRF_PROVISIONING_INTERVAL_S) {
				retry_s = CONFIG_NRF_PROVISIONING_INTERVAL_S;
			}
		}

		goto out;
	}

	if (now_s > deadline_s) {
		/* Provision now */
		if (!nxt_provisioning && !deadline_s) {
			deadline_s = now_s + CONFIG_NRF_PROVISIONING_INTERVAL_S;
			retry_s = 0;
			goto out;
		}

		/* Interval set by the server takes precedence */
		deadline_s = nxt_provisioning ? now_s + nxt_provisioning :
			now_s + CONFIG_NRF_PROVISIONING_INTERVAL_S;
	}

	retry_s = deadline_s - now_s;
out:
	/* Delay spread does not need high-entropy randomness */
	srand(now_s);
	spread_s = rand() %
		(CONFIG_NRF_PROVISIONING_SPREAD_S ? CONFIG_NRF_PROVISIONING_SPREAD_S : 1);

	/* To even the load on server side */
	retry_s += spread_s;

	LOG_INF("Checking for provisioning commands in %lld seconds", retry_s);
	reschedule = false;

	return retry_s;
}

static void commit_latest_cmd_id(void)
{
	int ret = settings_save_one(SETTINGS_STORAGE_PREFIX "/" NRF_PROVISIONING_CORRELATION_ID_KEY,
		nrf_provisioning_codec_get_latest_cmd_id(),
		strlen(nrf_provisioning_codec_get_latest_cmd_id()) + 1);

	if (ret) {
		LOG_ERR("Unable to store key: %s; value: %s; err: %d",
			NRF_PROVISIONING_CORRELATION_ID_KEY,
			nrf_provisioning_codec_get_latest_cmd_id(), ret);
	} else {
		LOG_DBG("%s", nrf_provisioning_codec_get_latest_cmd_id());
	}
}

void nrf_provisioning_set_interval(int interval)
{
	LOG_DBG("Provisioning interval set to %d", interval);

	if (interval != nxt_provisioning) {
		char time_str[sizeof(STRINGIFY(2147483647))] = {0};
		int ret;

		nxt_provisioning = interval;
		reschedule = true;

		ret = k_mutex_lock(&np_mtx, K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("Unable to lock mutex, err: %d", ret);
			return;
		}
		/* Let the provisioning thread run */
		k_condvar_signal(&np_cond);
		k_mutex_unlock(&np_mtx);

		ret = snprintf(time_str, sizeof(time_str), "%d", interval);
		if (ret < 0) {
			LOG_ERR("Unable to convert interval to string");
			return;
		}

		ret = settings_save_one(SETTINGS_STORAGE_PREFIX "/interval-sec",
			time_str, strlen(time_str) + 1);
		if (ret) {
			LOG_ERR("Unable to store interval, err: %d", ret);
			return;
		}
		LOG_DBG("Stored interval: \"%s\"", time_str);
	}
}

int nrf_provisioning_req(void)
{
	int ret;
	struct nrf_provisioning_callback_data event_data = { 0 };

	while (true) {

		k_condvar_wait(&np_cond, &np_mtx, K_FOREVER);
		k_mutex_unlock(&np_mtx);

		if (IS_ENABLED(CONFIG_NRF_PROVISIONING_WITH_CERT)) {
			ret = cert_provision();
			if (ret) {
				LOG_ERR("Failed to provision certificate, err %d", ret);
				event_data.type = NRF_PROVISIONING_EVENT_FATAL_ERROR;
				callback_local(&event_data);
				return ret;
			}
		}

		if (IS_ENABLED(IS_ENABLED(CONFIG_NRF_PROVISIONING_SCHEDULED))) {
			settings_load_subtree(settings.name); /* Get the provisioning interval */

			/* Reschedule as long as there's no network */
			do {
				ret = nrf_provisioning_schedule();
				if (ret < 0) {
					LOG_ERR("Provisioning client terminated");
					__ASSERT(false, "Provisioning client terminated");
					break; /* Terminates the thread */
				}

				k_condvar_wait(&np_cond, &np_mtx, K_SECONDS(ret));
				k_mutex_unlock(&np_mtx);
			} while (!nw_connected || reschedule);
		}

		event_data.type = NRF_PROVISIONING_EVENT_START;
		callback_local(&event_data);

		if (IS_ENABLED(CONFIG_NRF_PROVISIONING_HTTP)) {
			ret = nrf_provisioning_http_req(&rest_ctx);
		} else {
			ret = nrf_provisioning_coap_req(&coap_ctx);
		}

		event_data.type = NRF_PROVISIONING_EVENT_STOP;
		callback_local(&event_data);

		if (IS_ENABLED(CONFIG_NRF_PROVISIONING_SCHEDULED)) {
			while (ret == -EBUSY || ret == -ETIMEDOUT) {
				int backoff = CONFIG_NRF_PROVISIONING_INITIAL_BACKOFF;

				if (ret == -EBUSY) {
					LOG_WRN("Busy, retrying in %d seconds", backoff);
				} else {
					LOG_WRN("Timeout, retrying in %d seconds", backoff);
				}

				/* Backoff */
				k_condvar_wait(&np_cond, &np_mtx, K_SECONDS(backoff));
				k_mutex_unlock(&np_mtx);

				backoff = backoff * 2;
				if (backoff > SRV_TIMEOUT_BACKOFF_MAX_S) {
					backoff = SRV_TIMEOUT_BACKOFF_MAX_S;
				}

				event_data.type = NRF_PROVISIONING_EVENT_START;
				callback_local(&event_data);

				if (IS_ENABLED(CONFIG_NRF_PROVISIONING_HTTP)) {
					ret = nrf_provisioning_http_req(&rest_ctx);
				} else {
					ret = nrf_provisioning_coap_req(&coap_ctx);
				}

				event_data.type = NRF_PROVISIONING_EVENT_STOP;
				callback_local(&event_data);
			}
		}

		if (ret == -EACCES) {
			LOG_WRN("Unauthorized access: device is not yet claimed.");

			event_data.type = NRF_PROVISIONING_EVENT_FAILED_DEVICE_NOT_CLAIMED;

			if (IS_ENABLED(CONFIG_NRF_PROVISIONING_PROVIDE_ATTESTATION_TOKEN)) {
				struct nrf_attestation_token token = { 0 };
				int err;

				err = modem_attest_token_get(&token);
				if (err) {
					LOG_ERR("Failed to get token, err %d", err);
				} else {
					event_data.token = &token;
					callback_local(&event_data);

					modem_attest_token_free(&token);
				}

			} else {
				callback_local(&event_data);
			}

		} else if (ret == -EINVAL) {
			__ASSERT(false, "Invalid exchange, abort");
			LOG_ERR("Invalid exchange");

			event_data.type = NRF_PROVISIONING_EVENT_FAILED;
			callback_local(&event_data);

		} else if (ret == -ECONNREFUSED) {
			LOG_ERR("Connection refused");
			LOG_WRN("Please check the CA certificate stored in sectag "
				STRINGIFY(CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG)"");

			event_data.type = NRF_PROVISIONING_EVENT_FAILED_WRONG_ROOT_CA;
			callback_local(&event_data);

		} else if (ret < 0) {
			LOG_ERR("Provisioning failed, error: %d", ret);

			event_data.type = NRF_PROVISIONING_EVENT_FAILED;
			callback_local(&event_data);

		} else if (ret > 0) {
			/* Provisioning finished */

			if (IS_ENABLED(CONFIG_NRF_PROVISIONING_SAVE_CMD_ID)) {
				LOG_DBG("Saving the latest command id");
				commit_latest_cmd_id();
			}

			event_data.type = NRF_PROVISIONING_EVENT_DONE;
			callback_local(&event_data);
		}

#if CONFIG_UNITY
		break;
#endif
	}

	return ret;
}

K_THREAD_DEFINE(nrf_provisioning, CONFIG_NRF_PROVISIONING_STACK_SIZE,
		nrf_provisioning_req, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
