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

/* An arbitrary max backoff interval if connection to server times out [s] */
#define SRV_TIMEOUT_BACKOFF_MAX_S 86400
#define SETTINGS_STORAGE_PREFIX	  CONFIG_NRF_PROVISIONING_SETTINGS_STORAGE_PATH

/* nRF Provisioning context */
static struct nrf_provisioning_http_context rest_ctx = {
	.connect_socket = REST_CLIENT_SCKT_CONNECT,
};

/* nRF Provisioning context */
static struct nrf_provisioning_coap_context coap_ctx = {
	.connect_socket = -1,
	.rx_buf = NULL,
	.rx_buf_len = 0,
};

/* Variables */
static bool initialized;
static time_t provisioning_interval;
static nrf_provisioning_event_cb_t callback_local;
static bool nw_connected = true;
static bool reschedule;
static unsigned int backoff;
static struct k_work_q provisioning_work_q;
static struct k_work_queue_config work_q_config = {
	.name = "nrf_provisioning_work_q",
};

/* Forward declarations */
static void nrf_provisioning_work(struct k_work *work);
static void schedule_next_work(unsigned int seconds);
static void init_work_fn(struct k_work *work);
static void trigger_reschedule(void);
static int cert_provision(void);

K_WORK_DEFINE(init_work, init_work_fn);
K_WORK_DELAYABLE_DEFINE(provisioning_work, nrf_provisioning_work);
K_THREAD_STACK_DEFINE(nrf_provisioning_stack, CONFIG_NRF_PROVISIONING_STACK_SIZE);

#if defined(CONFIG_NRF_PROVISIONING_WITH_CERT)
NRF_MODEM_LIB_ON_INIT(nrf_provisioning_on_modem_init, nrf_provisioning_on_modem_init, NULL);

static void nrf_provisioning_on_modem_init(int ret, void *ctx)
{
	ARG_UNUSED(ctx);

	if (ret) {
		LOG_ERR("Modem library init error: %d", ret);
		return;
	}

	ret = cert_provision();
	if (ret) {
		__ASSERT(false, "Failed to provision certificate, err %d", ret);
		return;
	}
}
#endif /* CONFIG_NRF_PROVISIONING_WITH_CERT */

static void schedule_next_work(unsigned int seconds)
{
	if (seconds == 0) {
		k_work_reschedule_for_queue(&provisioning_work_q, &provisioning_work, K_NO_WAIT);
	} else {
		k_work_reschedule_for_queue(&provisioning_work_q, &provisioning_work,
					    K_SECONDS(seconds));
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
		return -ENFILE;
	}

	ret = modem_key_mgmt_exists(CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG,
				    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);
	if (ret) {
		LOG_ERR("Failed to check for certificates err %d", ret);
		return ret;
	}

	/* Don't overwrite certificate if one has been provisioned */
	if (exists) {
		LOG_DBG("Certificate already provisioned");
		return 0;
	}

	if (!cert_size) {
		LOG_ERR("Provided certificate size is 0");
		return -EINVAL;
	}

	LOG_DBG("Provisioning new certificate");
	LOG_HEXDUMP_DBG(cert, cert_size, "New certificate: ");

	ret = modem_key_mgmt_write(CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   cert, cert_size);
	if (ret) {
		LOG_ERR("Failed to provision certificate, err %d", ret);
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
		if (len_rd >= sizeof(time_str)) {
			return -ENOMEM;
		}

		len = read_cb(cb_arg, &time_str, len_rd);
		if (len < 0) {
			LOG_ERR("Unable to read the timestamp of next provisioning");
			return len;
		}

		time_str[len] = 0;

		if (len == 0) {
			provisioning_interval = CONFIG_NRF_PROVISIONING_INTERVAL_S;
			LOG_DBG("Initial provisioning");
		} else {
			errno = 0;
			time_t interval = (time_t) strtoll(time_str, NULL, 0);

			if (interval < 0 || errno != 0) {
				LOG_ERR("Invalid interval value: %s", time_str);
				provisioning_interval = CONFIG_NRF_PROVISIONING_INTERVAL_S;
				return -EINVAL;
			}

			LOG_DBG("Stored interval: \"%jd\"", interval);
			if (provisioning_interval != interval) {
				provisioning_interval = interval;
				/* Mark rescheduling, but don't trigger as settings are scanned from
				 * event handler just before checking.
				 */
				reschedule = true;
			}
		}

		return 0;
	} else if (strncmp(key, NRF_PROVISIONING_CORRELATION_ID_KEY, key_len) == 0) {
		/* Loaded only once */
		if (strcmp(latest_cmd_id, "") != 0) {
			return 0;
		}

		if (len_rd >= NRF_PROVISIONING_CORRELATION_ID_SIZE) {
			return -ENOMEM;
		}
		memset(latest_cmd_id, 0, NRF_PROVISIONING_CORRELATION_ID_SIZE);
		len = read_cb(cb_arg, latest_cmd_id, len_rd);
		if (len < 0) {
			return len;
		}

		if (len == 0) {
			LOG_DBG("No provisioning information stored");
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
				/* Some LTE networks assign IPv6 addresses asynchronously after LTE
				 * attach. The address configuration (via Router Advertisement) can
				 * occur 1–2 seconds after registration completes.
				 * Waiting 2 seconds before performing provisioning (IP traffic)
				 * increases the likelihood that an IPv6 address is available.
				 */
				k_sleep(K_SECONDS(2));
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

	init = true;

	return 0;
}

static void lte_lc_event_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_PDN:
		switch (evt->pdn.type) {
		case LTE_LC_EVT_PDN_ACTIVATED:
		case LTE_LC_EVT_PDN_RESUMED:

			LOG_DBG("Connected to network");
			nw_connected = true;
			if (backoff && IS_ENABLED(CONFIG_NRF_PROVISIONING_SCHEDULED)) {
				/* If network resumed while waiting, resume immediately */
				schedule_next_work(0);
			}

			break;
		case LTE_LC_EVT_PDN_DEACTIVATED:
		case LTE_LC_EVT_PDN_NETWORK_DETACH:
		case LTE_LC_EVT_PDN_SUSPENDED:

			LOG_DBG("Disconnected from network");
			nw_connected = false;

			break;
		default:
			/* Don't care about other PDN events */
			break;
		}
		break;
	default:
		/* Don't care about other events */
		break;
	}
}

int nrf_provisioning_init(nrf_provisioning_event_cb_t callback_handler)
{
	int ret;

	if (!callback_handler) {
		LOG_ERR("Callback handler is NULL");
		return -EINVAL;
	}

	if (initialized) {
		LOG_ERR("Already initialized");
		return -EALREADY;
	}

	callback_local = callback_handler;

	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_HTTP)) {
		ret = nrf_provisioning_http_init(callback_local);
	} else {
		ret = nrf_provisioning_coap_init(callback_local);
	}

	if (ret) {
		LOG_ERR("Failed to initialize nRF Provisioning transport, err %d", ret);
		return ret;
	}

	k_work_queue_init(&provisioning_work_q);
	k_work_queue_start(&provisioning_work_q,
		nrf_provisioning_stack, K_THREAD_STACK_SIZEOF(nrf_provisioning_stack),
		K_LOWEST_APPLICATION_THREAD_PRIO, &work_q_config);

	/* Offload time consuming work to the work queue to avoid blocking the caller. */
	k_work_submit_to_queue(&provisioning_work_q, &init_work);

	initialized = true;

	return 0;
}

static void init_work_fn(struct k_work *work)
{
	int ret;
	struct nrf_provisioning_callback_data event_data = { 0 };

	ret = settings_init();
	if (ret == -EALREADY) {
		ret = 0;
	} else if (ret != 0) {
		LOG_ERR("Can't initialize settings");
		event_data.type = NRF_PROVISIONING_EVENT_FATAL_ERROR;
		callback_local(&event_data);
		return;
	}

	/* Setup handler for LTE Link Control events. */
	lte_lc_register_handler(lte_lc_event_handler);

	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_AUTO_START_ON_INIT)) {
		LOG_DBG("Starting provisioning on init");

		trigger_reschedule();
	}
}

int nrf_provisioning_trigger_manually(void)
{
	if (!initialized) {
		return -EFAULT;
	}

	schedule_next_work(0);
	LOG_DBG("Externally initiated provisioning");

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

	if (!initialized) {
		LOG_ERR("Library not initialized");
		return -EFAULT;
	}

	if (first) {
		first = false;
		provisioning_interval = (provisioning_interval > 0)
						? provisioning_interval
						: CONFIG_NRF_PROVISIONING_INTERVAL_S;

		LOG_DBG("First provisioning, setting interval to %lld seconds",
			(int64_t)provisioning_interval);
		/* Delay spread does not need high-entropy randomness */
		srand(k_uptime_get_32());
		goto out;
	}

	ret = date_time_now(&now_s);
	if (ret < 0) {
		if (ret != -ENODATA) {
			LOG_ERR("Getting time failed, error: %d", ret);
			return ret;
		}

		/* Backoff... */
		retry_s = retry_s * 2;

		/* ...up to a degree */
		if (retry_s > CONFIG_NRF_PROVISIONING_INTERVAL_S) {
			retry_s = CONFIG_NRF_PROVISIONING_INTERVAL_S;
		}

		goto out;
	}

	now_s /= 1000; /* date_time_now() is ms */

	if (now_s > deadline_s || reschedule) {
		/* Interval set by the server takes precedence */
		deadline_s = now_s + (provisioning_interval ? provisioning_interval
							    : CONFIG_NRF_PROVISIONING_INTERVAL_S);
	}

	retry_s = deadline_s - now_s;
out:
	spread_s = rand() %
		(CONFIG_NRF_PROVISIONING_SPREAD_S ? CONFIG_NRF_PROVISIONING_SPREAD_S : 1);

	/* To even the load on server side */
	retry_s += spread_s;

	LOG_DBG("Checking for provisioning commands in %lld seconds", (int64_t)retry_s);
	reschedule = false;

	return retry_s;
}

static void commit_latest_cmd_id(void)
{
	int ret = settings_save_one(SETTINGS_STORAGE_PREFIX "/" NRF_PROVISIONING_CORRELATION_ID_KEY,
		nrf_provisioning_codec_get_latest_cmd_id(),
		strlen(nrf_provisioning_codec_get_latest_cmd_id()));

	if (ret) {
		LOG_ERR("Unable to store key: %s; value: %s; err: %d",
			NRF_PROVISIONING_CORRELATION_ID_KEY,
			nrf_provisioning_codec_get_latest_cmd_id(), ret);
	} else {
		LOG_DBG("%s", nrf_provisioning_codec_get_latest_cmd_id());
	}
}

static int wait_for_valid_datetime(int timeout_seconds)
{
	int waited = 0;

	while (waited < timeout_seconds) {
		if (date_time_is_valid()) {
			LOG_DBG("Valid date time obtained");
			return 0;
		}

		k_sleep(K_SECONDS(1));
		waited++;
	}

	return -ETIMEDOUT;
}

int nrf_provisioning_set_interval(int interval)
{
	if (!initialized) {
		return -EFAULT;
	}

	if (interval < 0) {
		LOG_ERR("Invalid interval %d", interval);
		return -EINVAL;
	}

	if (!IS_ENABLED(CONFIG_NRF_PROVISIONING_SCHEDULED)) {
		return -ENOTSUP;
	}

	LOG_DBG("Provisioning interval set to %d", interval);

	if (interval != provisioning_interval) {
		char time_str[sizeof(STRINGIFY(2147483647))] = {0};
		int ret;

		provisioning_interval = interval;

		ret = snprintf(time_str, sizeof(time_str), "%d", interval);
		if (ret < 0) {
			LOG_ERR("Unable to convert interval to string");
			return -EFAULT;
		}

		ret = settings_save_one(SETTINGS_STORAGE_PREFIX "/interval-sec",
			time_str, strlen(time_str));
		if (ret) {
			LOG_ERR("Unable to store interval, err: %d", ret);
			return -ENOSTR;
		}

		LOG_DBG("Stored interval: \"%s\"", time_str);
		trigger_reschedule();
	}

	return 0;
}

static void schedule_backoff(void)
{
	if (backoff == 0) {
		backoff = CONFIG_NRF_PROVISIONING_INITIAL_BACKOFF;
	}

	LOG_DBG("Scheduling backoff for %d seconds", backoff);
	schedule_next_work(backoff);
	backoff = backoff * 2;
	if (backoff > SRV_TIMEOUT_BACKOFF_MAX_S) {
		backoff = SRV_TIMEOUT_BACKOFF_MAX_S;
	}
}

static void trigger_reschedule(void)
{
	LOG_DBG("Triggering reschedule");
	reschedule = true;
	backoff = 0;
	schedule_next_work(0);
}

static void check_return_code_and_notify(int ret)
{
	struct nrf_provisioning_callback_data event_data = { 0 };

	switch (ret) {
	case -EBUSY:
		LOG_WRN("Provisioning client busy");

		event_data.type = NRF_PROVISIONING_EVENT_FAILED;
		callback_local(&event_data);

		if (IS_ENABLED(CONFIG_NRF_PROVISIONING_SCHEDULED)) {
			schedule_backoff();
			LOG_WRN("Retrying in %d seconds", backoff);
			return;
		}

		break;
	case -ETIMEDOUT:
		LOG_WRN("Provisioning timed out");

		event_data.type = NRF_PROVISIONING_EVENT_FAILED;
		callback_local(&event_data);

		if (IS_ENABLED(CONFIG_NRF_PROVISIONING_SCHEDULED)) {
			schedule_backoff();
			LOG_WRN("Retrying in %d seconds", backoff);
			return;
		}

		break;
	case -EACCES:
		LOG_WRN("Unauthorized access: device is not yet claimed.");

		event_data.type = NRF_PROVISIONING_EVENT_FAILED_DEVICE_NOT_CLAIMED;

		if (IS_ENABLED(CONFIG_NRF_PROVISIONING_PROVIDE_ATTESTATION_TOKEN)) {
			struct nrf_attestation_token token = { 0 };
			int err;

			err = modem_attest_token_get(&token);
			if (err) {
				LOG_ERR("Failed to get token, err %d", err);
				callback_local(&event_data);
			} else {
				event_data.token = &token;
				callback_local(&event_data);

				modem_attest_token_free(&token);
			}

		} else {
			callback_local(&event_data);
		}

		break;
	case -ENOMEM:
		LOG_ERR("Not enough memory to process the incoming list of commands");

		event_data.type = NRF_PROVISIONING_EVENT_FAILED_TOO_MANY_COMMANDS;
		callback_local(&event_data);

		break;
	case -EINVAL:
		LOG_ERR("Invalid exchange");

		event_data.type = NRF_PROVISIONING_EVENT_FAILED;
		callback_local(&event_data);

		break;
	case -ECONNREFUSED:
		LOG_ERR("Connection refused");
		LOG_WRN("Please check the CA certificate stored in sectag "
			STRINGIFY(CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG)"");

		event_data.type = NRF_PROVISIONING_EVENT_FAILED_WRONG_ROOT_CA;
		callback_local(&event_data);

		break;
	case -ENODATA:
		LOG_DBG("No commands to process");

		event_data.type = NRF_PROVISIONING_EVENT_NO_COMMANDS;
		callback_local(&event_data);

		break;
	default:
		if (ret < 0) {
			LOG_ERR("Provisioning failed, error: %d", ret);

			event_data.type = NRF_PROVISIONING_EVENT_FAILED;
			callback_local(&event_data);

		} else if (ret > 0) {
			/* Provisioning is finished */

			if (IS_ENABLED(CONFIG_NRF_PROVISIONING_SAVE_CMD_ID)) {
				LOG_DBG("Saving the latest command id");
				commit_latest_cmd_id();
			}

			event_data.type = NRF_PROVISIONING_EVENT_DONE;
			callback_local(&event_data);
		}
		break;
	}

	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_SCHEDULED)) {
		trigger_reschedule();
	}
}

void nrf_provisioning_work(struct k_work *work)
{
	int ret;
	struct nrf_provisioning_callback_data event_data = { 0 };

	LOG_DBG("nrf_provisioning_work() called");

	/* Check if settings have been changed */
	settings_load_subtree(settings.name);

	if (reschedule && IS_ENABLED(CONFIG_NRF_PROVISIONING_SCHEDULED)) {
		ret = nrf_provisioning_schedule();
		if (ret < 0) {
			event_data.type = NRF_PROVISIONING_EVENT_FATAL_ERROR;
			callback_local(&event_data);
			return;
		}

		LOG_DBG("Next provisioning in %d seconds", ret);

		event_data.type = NRF_PROVISIONING_EVENT_SCHEDULED_PROVISIONING;
		event_data.next_attempt_time_seconds = ret;
		callback_local(&event_data);

		schedule_next_work(ret);
		return;
	}

	/* Backoff as long as there's no network */
	if (!nw_connected && IS_ENABLED(CONFIG_NRF_PROVISIONING_SCHEDULED)) {
		schedule_backoff();
		return;
	}

	event_data.type = NRF_PROVISIONING_EVENT_START;
	callback_local(&event_data);

	ret = wait_for_valid_datetime(CONFIG_NRF_PROVISIONING_VALID_DATE_TIME_TIMEOUT_SECONDS);
	if (ret) {
		LOG_ERR("Failed to get valid date time, err %d", ret);
		event_data.type = NRF_PROVISIONING_EVENT_FAILED_NO_VALID_DATETIME;
		callback_local(&event_data);

		event_data.type = NRF_PROVISIONING_EVENT_STOP;
		callback_local(&event_data);
		return;
	}

	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_HTTP)) {
		ret = nrf_provisioning_http_req(&rest_ctx);
	} else {
		ret = nrf_provisioning_coap_req(&coap_ctx);
	}

	check_return_code_and_notify(ret);

	event_data.type = NRF_PROVISIONING_EVENT_STOP;
	callback_local(&event_data);
}
