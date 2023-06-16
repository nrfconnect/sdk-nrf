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
#include <zephyr/random/rand32.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>

#include <modem/lte_lc.h>
#include <modem/modem_key_mgmt.h>
#include <modem/nrf_modem_lib.h>
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

K_MUTEX_DEFINE(np_mtx);
K_CONDVAR_DEFINE(np_cond);

/* An arbitrary max backoff interval if connection to server times out [s] */
#define SRV_TIMEOUT_BACKOFF_MAX_S 86400
#define SETTINGS_STORAGE_PREFIX CONFIG_NRF_PROVISIONING_SETTINGS_STORAGE_PATH

static bool nw_connected = true;


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

static struct nrf_provisioning_mm_change mm;
static struct nrf_provisioning_dm_change dm;

NRF_MODEM_LIB_ON_INIT(nrf_provisioning_init_hook, nrf_provisioning_on_modem_init, NULL);

static void nrf_provisioning_on_modem_init(int ret, void *ctx)
{
	int err;

	if (IS_ENABLED(CONFIG_NRF_PROVISIONING_AUTO_INIT)) {
		err = nrf_provisioning_init(NULL, NULL);
		if (err) {
			LOG_ERR("Failed to initialize provisioning client");
		}
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

static void nrf_provisioning_device_mode_cb(void *user_data)
{
	(void)user_data;

#if !CONFIG_UNITY
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


int nrf_provisioning_init(struct nrf_provisioning_mm_change *mmode,
				struct nrf_provisioning_dm_change *dmode)
{
	int ret;

	k_mutex_lock(&np_mtx, K_FOREVER);

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
		goto exit;
	}

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

	initialized = true;

	lte_lc_register_handler(nrf_provisioning_lte_handler);

	/* Let the provisioning thread run */
	k_condvar_signal(&np_cond);
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

	LOG_DBG("Connecting in %llds", retry_s);
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


int nrf_provisioning_req(void)
{
	int ret;
	int backoff;

	k_condvar_wait(&np_cond, &np_mtx, K_FOREVER);
	k_mutex_unlock(&np_mtx);

	while (true) {
		backoff = 1; /* Backoff start interval */
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
		} while (!nw_connected);

		if (IS_ENABLED(CONFIG_NRF_PROVISIONING_HTTP)) {
			ret = nrf_provisioning_http_req(&rest_ctx);
		} else {
			ret = nrf_provisioning_coap_req(&coap_ctx);
		}

		while (ret == -EBUSY) {
			/* Backoff */
			k_condvar_wait(&np_cond, &np_mtx, K_SECONDS(backoff));
			k_mutex_unlock(&np_mtx);

			backoff = backoff * 2;
			if (backoff > SRV_TIMEOUT_BACKOFF_MAX_S) {
				backoff = SRV_TIMEOUT_BACKOFF_MAX_S;
			}
			if (IS_ENABLED(CONFIG_NRF_PROVISIONING_HTTP)) {
				ret = nrf_provisioning_http_req(&rest_ctx);
			} else {
				ret = nrf_provisioning_coap_req(&coap_ctx);
			}
		}

		if (ret == -EINVAL) {
			__ASSERT(false, "Invalid exchange, abort");
			LOG_ERR("Invalid exchange");
		} else if (ret == -ECONNREFUSED) {
			LOG_ERR("Connection refused, client exits");
			k_mutex_lock(&np_mtx, K_FOREVER);
			initialized = false;
			k_mutex_unlock(&np_mtx);
			break;
		} else if (ret < 0) {
			LOG_ERR("Provisioning failed, error: %d", ret);
		} else if (ret > 0) {
			/* Provisioning finished */
			if (IS_ENABLED(CONFIG_NRF_PROVISIONING_SAVE_CMD_ID)) {
				LOG_DBG("Saving the latest command id");
				commit_latest_cmd_id();
			}
			dm.cb(dm.user_data);
		}

#if CONFIG_UNITY
		break;
#endif
	}

	return ret;
}

#define NRF_PROVISIONING_STACK_SIZE 3072
#define NRF_PROVISIONING_PRIORITY 5

K_THREAD_DEFINE(nrf_provisioning, NRF_PROVISIONING_STACK_SIZE,
		nrf_provisioning_req, NULL, NULL, NULL,
		NRF_PROVISIONING_PRIORITY, 0, 0);
