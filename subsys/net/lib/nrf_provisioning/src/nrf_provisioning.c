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
static time_t provisioning_interval;
static struct nrf_provisioning_mm_change mm;
static struct nrf_provisioning_dm_change dm;
static bool nw_connected = true;
static bool reschedule;
static unsigned int backoff;
static struct k_work_q provisioning_work_q;
static struct k_work_queue_config work_q_config = {
	.name = "nrf_provisioning_work_q",
};

static void nrf_provisioning_work(struct k_work *work);
static void schedule_next_work(unsigned int seconds);
static void init_work_fn(struct k_work *work);
static void trigger_reschedule(void);

K_WORK_DEFINE(init_work, init_work_fn);
K_WORK_DELAYABLE_DEFINE(provisioning_work, nrf_provisioning_work);
K_THREAD_STACK_DEFINE(nrf_provisioning_stack, CONFIG_NRF_PROVISIONING_STACK_SIZE);

NRF_MODEM_LIB_ON_INIT(nrf_provisioning_on_modem_init, nrf_provisioning_on_modem_init, NULL);
static void nrf_provisioning_on_modem_init(int ret, void *ctx)
{
	int err;

	if (ret != 0) {
		LOG_ERR("Modem library did not initialize: %d", ret);
		return;
	}

	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_AUTO_INIT)) {
		err = nrf_provisioning_init(NULL, NULL);
		if (err) {
			LOG_ERR("Failed to initialize provisioning client");
		}
	}
}

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

#if defined(CONFIG_NRF_PROVISIONING_WITH_CERT)
	bool exists;
	int prev_mode;

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
		goto exit;
	}

	ret = modem_key_mgmt_exists(CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG,
					MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);
	if (ret) {
		__ASSERT_NO_MSG(false);
		LOG_ERR("Failed to check for certificates err %d", ret);
		goto exit;
	}

	/* Don't overwrite certificate if one has been provisioned */
	if (exists || !cert_size) {
		ret = 0;
		goto exit;
	}

	prev_mode = mm.cb(LTE_LC_FUNC_MODE_OFFLINE, mm.user_data);

	if (prev_mode < 0) {
		LOG_ERR("Can't put modem to offline modem for writing certificate");
		ret = prev_mode;
		goto exit;
	}

	LOG_INF("Provisioning new certificate");
	LOG_HEXDUMP_DBG(cert, cert_size, "New certificate: ");


	ret = modem_key_mgmt_write(CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   cert, cert_size);
	if (ret) {
		__ASSERT_NO_MSG(false);
		LOG_ERR("Failed to provision certificate, err %d", ret);
	}

	prev_mode = mm.cb(prev_mode, mm.user_data);
	if (prev_mode < 0) {
		LOG_ERR("Can't restore modem mode after certificate write");
		ret = ret ? ret : -prev_mode;
	}
exit:
#endif /* CONFIG_NRF_PROVISIONING_WITH_CERT */
	return ret;
}

static int nrf_provisioning_set(const char *key, size_t len_rd,
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
			LOG_INF("No provisioning information stored");
		}

		return 0;
	}

	return -ENOENT;
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

static int nrf_provisioning_modem_mode_cb(enum lte_lc_func_mode new_mode, void *user_data)
{
	enum lte_lc_func_mode fmode;
	char time_buf[64];
	int ret;

	(void)user_data;

	if (lte_lc_func_mode_get(&fmode)) {
		LOG_ERR("Failed to read modem functional mode");
		ret = -EFAULT;
		return ret;
	}

	if (fmode == new_mode) {
		ret = fmode;
	} else if (new_mode == LTE_LC_FUNC_MODE_NORMAL) {
		/* I need to use the blocking call, because in next step
		 * the service will create a socket and call connect()
		 */
		ret = lte_lc_connect();

		if (ret) {
			LOG_ERR("lte_lc_connect() failed %d", ret);
			return ret;
		}
		LOG_INF("Modem connection restored");

		LOG_INF("Waiting for modem to acquire network time...");

		do {
			k_sleep(K_SECONDS(3));
			ret = nrf_provisioning_at_time_get(time_buf, sizeof(time_buf));
		} while (ret != 0);

		LOG_INF("Network time obtained");
		ret = fmode;
	} else {
		ret = lte_lc_func_mode_set(new_mode);
		if (ret == 0) {
			LOG_DBG("Modem set to requested state %d", new_mode);
			ret = fmode;
		}
	}

	return ret;
}

static void nrf_provisioning_device_mode_cb(enum nrf_provisioning_event event, void *user_data)
{
	(void)user_data;

#if !CONFIG_UNITY
	if (event == NRF_PROVISIONING_EVENT_DONE) {
		/* Disconnect from network gracefully */
		int ret = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE);

		if (ret != 0) {
			LOG_ERR("Unable to set modem offline, error %d", ret);
		}

		LOG_INF("Provisioning done, rebooting...");
		while (log_process()) {
			;
		}

		sys_reboot(SYS_REBOOT_WARM);
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
		if (backoff) {
			/* If network resumed while waiting, resume immediately */
			schedule_next_work(0);
		}
		break;
	default:
		break;
	}
}

int nrf_provisioning_init(struct nrf_provisioning_mm_change *mmode,
				struct nrf_provisioning_dm_change *dmode)
{
	int ret;

	/* Restore the default if not a callback function */
	if (!mmode) {
		mm.cb = nrf_provisioning_modem_mode_cb;
		mm.user_data = NULL;
	} else {
		mm.cb = mmode->cb;
		mm.user_data = mmode->user_data;
	}

	if (!dmode) {
		dm.cb = nrf_provisioning_device_mode_cb;
		dm.user_data = NULL;
	} else {
		dm.cb = dmode->cb;
		dm.user_data = dmode->user_data;
	}

	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_HTTP)) {
		ret = nrf_provisioning_http_init(&mm);
	} else {
		ret = nrf_provisioning_coap_init(&mm);
	}
	if (ret) {
		goto exit;
	}
	if (initialized) {
		return 0;
	}
	initialized = true;
	k_work_queue_init(&provisioning_work_q);
	k_work_queue_start(&provisioning_work_q,
		nrf_provisioning_stack, K_THREAD_STACK_SIZEOF(nrf_provisioning_stack),
		K_LOWEST_APPLICATION_THREAD_PRIO, &work_q_config);

	k_work_submit_to_queue(&provisioning_work_q, &init_work);

exit:
	if (ret) {
		LOG_ERR("Provisioning client initialization failed, error: %d", ret);
	}
	return 0;
}

static void init_work_fn(struct k_work *work)
{
	int ret;

	/* Provision certificates now when it's possible to put modem offline */
	ret = cert_provision();
	if (ret) {
		goto exit;
	}

	ret = settings_init();
	if (ret == -EALREADY) {
		ret = 0;
	} else if (ret != 0) {
		LOG_ERR("Can't initialize settings");
		goto exit;
	}

	lte_lc_register_handler(nrf_provisioning_lte_handler);

	/* Let the provisioning thread run */
	trigger_reschedule();
exit:
	if (ret) {
		LOG_ERR("Provisioning client initialization failed, error: %d", ret);
	}
}

int nrf_provisioning_trigger_manually(void)
{
	if (!initialized) {
		return -EFAULT;
	}

	schedule_next_work(0);
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
		provisioning_interval = (provisioning_interval > 0)
						? provisioning_interval
						: CONFIG_NRF_PROVISIONING_INTERVAL_S;
		LOG_DBG("First provisioning, setting interval to %lld seconds",
			(int64_t)provisioning_interval);
		goto out;
	}

	ret = date_time_now(&now_s);
	if (ret < 0) {
		if (ret != -ENODATA) {
			__ASSERT(false, "Getting time failed, error: %d", ret);
			LOG_ERR("Getting time failed, error: %d", ret);
		}

		if (provisioning_interval) {
			retry_s = provisioning_interval;
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

	now_s /= 1000; /* date_time_now() is ms */
	if (now_s > deadline_s || reschedule) {

		/* Provision now */
		if (!provisioning_interval && !deadline_s) {
			deadline_s = now_s + CONFIG_NRF_PROVISIONING_INTERVAL_S;
			retry_s = 0;
			goto out;
		}

		/* Interval set by the server takes precedence */
		deadline_s = provisioning_interval ? now_s + provisioning_interval :
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
		strlen(nrf_provisioning_codec_get_latest_cmd_id()));

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

	if (interval < 0) {
		LOG_ERR("Invalid interval %d", interval);
		return;
	}

	LOG_DBG("Provisioning interval set to %d", interval);

	if (interval != provisioning_interval) {
		char time_str[sizeof(STRINGIFY(2147483647))] = {0};
		int ret;

		provisioning_interval = interval;

		ret = snprintf(time_str, sizeof(time_str), "%d", interval);
		if (ret < 0) {
			LOG_ERR("Unable to convert interval to string");
			return;
		}

		ret = settings_save_one(SETTINGS_STORAGE_PREFIX "/interval-sec",
			time_str, strlen(time_str));
		if (ret) {
			LOG_ERR("Unable to store interval, err: %d", ret);
			return;
		}
		LOG_DBG("Stored interval: \"%s\"", time_str);
		trigger_reschedule();
	}
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
	schedule_next_work(0);
}

void nrf_provisioning_work(struct k_work *work)
{
	int ret;

	LOG_DBG("nrf_provisioning_work() called");

	/* Check if settings have been changed */
	settings_load_subtree(settings.name);

	if (reschedule) {
		ret = nrf_provisioning_schedule();
		if (ret < 0) {
			LOG_ERR("Provisioning client terminated");
			__ASSERT(false, "Provisioning client terminated");
			return;
		}

		schedule_next_work(ret);
		return;
	}

	/* Backoff as long as there's no network */
	if (!nw_connected) {
		schedule_backoff();
		return;
	}

	dm.cb(NRF_PROVISIONING_EVENT_START, dm.user_data);
	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_HTTP)) {
		ret = nrf_provisioning_http_req(&rest_ctx);
	} else {
		ret = nrf_provisioning_coap_req(&coap_ctx);
	}
	dm.cb(NRF_PROVISIONING_EVENT_STOP, dm.user_data);


	switch (ret) {
	case -EBUSY:
		schedule_backoff();
		LOG_WRN("Busy, retrying in %d seconds", backoff);
		return;
	case -ETIMEDOUT:
		schedule_backoff();
		LOG_WRN("Timeout, retrying in %d seconds", backoff);
		return;
	case -EACCES:
		backoff = 0;
		LOG_WRN("Unauthorized access: device is not yet claimed.");
		if (IS_ENABLED(CONFIG_NRF_PROVISIONING_PRINT_ATTESTATION_TOKEN)) {
			struct nrf_attestation_token token = {0};
			int err;

			err = modem_attest_token_get(&token);
			if (err) {
				LOG_ERR("Failed to get token, err %d", err);
			} else {
				printk("\nAttestation token "
					   "for claiming device on nRFCloud:\n");
				printk("%.*s.%.*s\n\n", token.attest_sz, token.attest,
					   token.cose_sz, token.cose);
				modem_attest_token_free(&token);
			}
		}
		return;
	case -ECONNREFUSED:
		backoff = 0;
		LOG_ERR("Connection refused");
		LOG_WRN("Please check the CA certificate stored in sectag "
			STRINGIFY(CONFIG_NRF_PROVISIONING_ROOT_CA_SEC_TAG)"");
		return;
	default:
		backoff = 0;
		if (ret < 0) {
			LOG_ERR("Provisioning failed, error: %d", ret);
		} else if (ret > 0) {
			/* Provisioning finished */
			if (IS_ENABLED(CONFIG_NRF_PROVISIONING_SAVE_CMD_ID)) {
				LOG_DBG("Saving the latest command id");
				commit_latest_cmd_id();
			}
			dm.cb(NRF_PROVISIONING_EVENT_DONE, dm.user_data);
		}
		trigger_reschedule();
		return;
	}

}
