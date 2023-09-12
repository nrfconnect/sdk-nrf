/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zephyr/logging/log.h>
#include <nrf_modem_at.h>
#include <modem/pdn.h>
#include <modem/at_monitor.h>
#include <modem/nrf_modem_lib.h>
#if defined(CONFIG_LTE_LINK_CONTROL)
#include <modem/lte_lc.h>
#endif

LOG_MODULE_REGISTER(pdn, CONFIG_PDN_LOG_LEVEL);

#define CID_UNASSIGNED (-1)

#define PDN_ACT_REASON_NONE	 (-1)
#define PDN_ACT_REASON_IPV4_ONLY (0)
#define PDN_ACT_REASON_IPV6_ONLY (1)

#define APN_STR_MAX_LEN 64

static K_MUTEX_DEFINE(list_mutex);

static sys_slist_t pdn_contexts = SYS_SLIST_STATIC_INIT(&pdn_context);

struct pdn {
	sys_snode_t node; /* list handling */
	pdn_event_handler_t callback;
	int8_t context_id;
};

/* Variables used for synchronizing pdn_activate() with AT notifications. */
static struct {
	int8_t cid;
	int8_t esm;
	int8_t reason;
} pdn_act_notif = {
	.cid = CID_UNASSIGNED
};

static K_MUTEX_DEFINE(pdn_act_mutex);
static K_SEM_DEFINE(sem_cgev, 0, 1);
static K_SEM_DEFINE(sem_cnec, 0, 1);

/* Use one monitor for all CGEV events and distinguish
 * between the different type of events later (ME, IPV6, etc).
 */
AT_MONITOR(pdn_cgev, "+CGEV", on_cgev);
AT_MONITOR(pdn_cnec_esm, "+CNEC_ESM", on_cnec_esm);

static struct pdn *pdn_find(int cid)
{
	struct pdn *pdn;

	SYS_SLIST_FOR_EACH_CONTAINER(&pdn_contexts, pdn, node) {
		if (pdn->context_id == cid) {
			return pdn;
		}
	}

	return NULL;
}

static struct pdn *pdn_ctx_new(void)
{
	struct pdn *pdn;

	pdn = k_malloc(sizeof(struct pdn));
	if (!pdn) {
		return NULL;
	}

	k_mutex_lock(&list_mutex, K_FOREVER);
	sys_slist_append(&pdn_contexts, &pdn->node);
	k_mutex_unlock(&list_mutex);

	return pdn;
}

static void on_cnec_esm(const char *notif)
{
	char *p;
	uint32_t cid;
	uint32_t esm_err;
	struct pdn *pdn;

	/* +CNEC_ESM: <cause>,<cid> */
	esm_err = strtoul(notif + strlen("+CNEC_ESM: "), &p, 10);

	if (!p || *p != ',') {
		return;
	}

	cid = strtoul(p + 1, NULL, 10);

	pdn = pdn_find(cid);
	if (!pdn) {
		return;
	}

	if (pdn->callback) {
		pdn->callback((intptr_t)cid, PDN_EVENT_CNEC_ESM, esm_err);
	}

	if (cid == pdn_act_notif.cid) {
		pdn_act_notif.esm = esm_err;
		k_sem_give(&sem_cnec);
	}
}

static void parse_cgev(const char *notif)
{
	char *p;
	int8_t cid;
	struct pdn *pdn;

	/* we are on the system workqueue, let's not put this on the stack */
	static const struct {
		const char *notif;
		const enum pdn_event event;
	} map[] = {
		/* +CGEV: ME PDN ACT <cid>[,<reason>] */
		{"ME PDN ACT", PDN_EVENT_ACTIVATED},
		/* +CGEV: ME PDN DEACT <cid> */
		{"ME PDN DEACT", PDN_EVENT_DEACTIVATED},
		/* +CGEV: NW PDN DEACT <cid> */
		{"NW PDN DEACT", PDN_EVENT_DEACTIVATED},
		/* +CGEV: ME DETACH */
		{"ME DETACH",  PDN_EVENT_NETWORK_DETACH},
		/* +CGEV: NW DETACH */
		{"NW DETACH",  PDN_EVENT_NETWORK_DETACH},
		/* --- Order is important from here --- */
		/* +CGEV: IPV6 FAIL <cid> */
		{"IPV6 FAIL",  PDN_EVENT_IPV6_DOWN},
		/* +CGEV: IPV6 <cid> */
		{"IPV6", PDN_EVENT_IPV6_UP},
	};

	for (size_t i = 0; i < ARRAY_SIZE(map); i++) {
		p = strstr(notif, map[i].notif);
		if (!p) {
			continue;
		}

		/* parse <cid> */
		p += strlen(map[i].notif);
		if (*p == ' ') {
			cid = (uint8_t)strtoul(p, &p, 10);
		} else {
			cid = CID_UNASSIGNED;
		}

		if (map[i].event == PDN_EVENT_ACTIVATED && cid == pdn_act_notif.cid) {
			/* parse <reason> */
			if (*p == ',') {
				pdn_act_notif.reason = (int8_t)strtol(p + 1, NULL, 10);
			} else {
				pdn_act_notif.reason = PDN_ACT_REASON_NONE;
			}
			k_sem_give(&sem_cgev);
		}

		SYS_SLIST_FOR_EACH_CONTAINER(&pdn_contexts, pdn, node) {
			if (pdn->callback && (pdn->context_id == cid || cid == CID_UNASSIGNED)) {
				pdn->callback(pdn->context_id, map[i].event, 0);
			}
		}
		return;
	}
}

static void parse_cgev_apn_rate_ctrl(const char *notif)
{
	char *p;
	int8_t cid;
	struct pdn *pdn;
	uint8_t apn_rate_ctrl_status;

	/* +CGEV: APNRATECTRL STAT <cid>,<status>,[<time_remaining>] */
	p = strstr(notif, "APNRATECTRL STAT");
	if (!p) {
		return;
	}

	/* parse <cid> */
	p += strlen("APNRATECTRL STAT");
	__ASSERT(*p == ' ', "Bad APNRATECTRL parsing");

	cid = (uint8_t)strtoul(p, &p, 10);
	__ASSERT(*p == ',', "Bad APNRATECTRL parsing");

	/* parse <status> */
	apn_rate_ctrl_status = (uint8_t)strtol(p + 1, NULL, 10);

	SYS_SLIST_FOR_EACH_CONTAINER(&pdn_contexts, pdn, node) {
		if (pdn->callback && pdn->context_id == cid) {
			pdn->callback(pdn->context_id,
				      apn_rate_ctrl_status ? PDN_EVENT_APN_RATE_CONTROL_ON
							   : PDN_EVENT_APN_RATE_CONTROL_OFF,
				      0);
		}
	}
}

static void on_cgev(const char *notif)
{
	parse_cgev(notif);
	parse_cgev_apn_rate_ctrl(notif);
}

NRF_MODEM_LIB_ON_INIT(modem_init_hook, on_modem_init, NULL);

static void on_modem_init(int ret, void *ctx)
{
	int err;
	(void) err;

	if (ret != 0) {
		/* Return if modem initialization failed */
		return;
	}

#if defined(CONFIG_PDN_LEGACY_PCO)
	err = nrf_modem_at_printf("AT%%XEPCO=0");
	if (err) {
		LOG_ERR("Failed to set legacy PCO mode, err %d", err);
		return;
	}
#else
	err = nrf_modem_at_printf("AT%%XEPCO=1");
	if (err) {
		LOG_ERR("Failed to set ePCO mode, err %d", err);
		return;
	}
#endif /* CONFIG_PDN_LEGACY_PCO */

#if defined(CONFIG_PDN_DEFAULTS_OVERRIDE)
	err = pdn_ctx_configure(0, CONFIG_PDN_DEFAULT_APN,
				CONFIG_PDN_DEFAULT_FAM, NULL);
	if (err) {
		LOG_ERR("Failed to configure default CID, err %d", err);
		return;
	}

#if defined(CONFIG_PDN_DEFAULT_AUTH_PAP) || defined(CONFIG_PDN_DEFAULT_AUTH_CHAP)
	/* +CGAUTH=<cid>[,<auth_prot>[,<userid>[,<password>]]] */
	BUILD_ASSERT(sizeof(CONFIG_PDN_DEFAULT_USERNAME) > 1, "Username not defined");

	err = pdn_ctx_auth_set(0, CONFIG_PDN_DEFAULT_AUTH,
			       CONFIG_PDN_DEFAULT_USERNAME,
			       CONFIG_PDN_DEFAULT_PASSWORD);
	if (err) {
		LOG_ERR("Failed to set auth params for default CID, err %d", err);
		return;
	}
#endif /* CONFIG_PDN_DEFAULT_AUTH_PAP || CONFIG_PDN_DEFAULT_AUTH_CHAP */
#endif /* CONFIG_PDN_DEFAULTS_OVERRIDE */
}

int pdn_default_ctx_cb_reg(pdn_event_handler_t cb)
{
	struct pdn *pdn;

	if (!cb) {
		return -EFAULT;
	}

	pdn = pdn_ctx_new();
	if (!pdn) {
		return -ENOMEM;
	}

	pdn->callback = cb;
	pdn->context_id = 0;

	LOG_DBG("Default PDP ctx callback %p registered", cb);

	return 0;
}

int pdn_default_ctx_cb_dereg(pdn_event_handler_t cb)
{
	bool remvd;
	struct pdn *pdn;
	struct pdn *tmp;

	remvd = false;
	k_mutex_lock(&list_mutex, K_FOREVER);
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&pdn_contexts, pdn, tmp, node) {
		if (pdn->callback == cb) {
			remvd = sys_slist_find_and_remove(&pdn_contexts, &pdn->node);
			break;
		}
	}
	k_mutex_unlock(&list_mutex);

	if (!remvd) {
		return -EINVAL;
	}

	LOG_DBG("Default PDP ctx callback %p unregistered", pdn->callback);
	k_free(pdn);

	return 0;
}

static void pdn_ctx_free(struct pdn *pdn)
{
	if (!pdn) {
		return;
	}

	k_mutex_lock(&list_mutex, K_FOREVER);
	sys_slist_find_and_remove(&pdn_contexts, &pdn->node);
	k_mutex_unlock(&list_mutex);

	k_free(pdn);
}

int pdn_ctx_create(uint8_t *cid, pdn_event_handler_t cb)
{
	int err;
	struct pdn *pdn;
	int ctx_id_tmp;

	if (!cid) {
		return -EFAULT;
	}

	pdn = pdn_ctx_new();
	if (!pdn) {
		return -ENOMEM;
	}

	err = nrf_modem_at_scanf("AT%XNEWCID?", "%%XNEWCID: %d", &ctx_id_tmp);
	if (err < 0) {
		pdn_ctx_free(pdn);
		return err;
	}

	if (ctx_id_tmp > SCHAR_MAX || ctx_id_tmp < SCHAR_MIN) {
		LOG_ERR("Context ID (%d) out of bounds", ctx_id_tmp);
		pdn_ctx_free(pdn);
		return -EFAULT;
	}

	pdn->context_id = (int8_t)ctx_id_tmp;

	*cid = pdn->context_id;

	/* The callback is uninitialized, assign regardless of whether the input is NULL. */
	pdn->callback = cb;

	return 0;
}

int pdn_ctx_configure(uint8_t cid, const char *apn, enum pdn_fam fam,
		      struct pdn_pdp_opt *opt)
{
	int err;
	char family[] = "IPV4V6"; /* longest family */

	if (apn != NULL && strlen(apn) >= APN_STR_MAX_LEN) {
		return -EINVAL;
	}

	switch (fam) {
	case PDN_FAM_IPV4:
		strncpy(family, "IP", sizeof(family));
		break;
	case PDN_FAM_IPV6:
		strncpy(family, "IPV6", sizeof(family));
		break;
	case PDN_FAM_IPV4V6:
		strncpy(family, "IPV4V6", sizeof(family));
		break;
	case PDN_FAM_NONIP:
		strncpy(family, "Non-IP", sizeof(family));
		break;
	default:
		LOG_ERR("Wrong family %d", fam);
		return -EINVAL;
	}

	if (opt) {
		/* NOTE: this is a workaround for an issue
		 * where passing default values does not work as intended.
		 * Only format these params when `opt` is passed explicitly.
		 */
		err = nrf_modem_at_printf("AT+CGDCONT=%u,%s,%s,,,,%u,,,,%u,%u",
			cid, family, apn,
			opt->ip4_addr_alloc,
			opt->nslpi,
			opt->secure_pco);
	} else {
		if (apn != NULL) {
			err = nrf_modem_at_printf("AT+CGDCONT=%u,%s,%s", cid, family, apn);
		} else {
			err = nrf_modem_at_printf("AT+CGDCONT=%u,%s", cid, family);
		}
	}

	if (err) {
		LOG_ERR("Failed to configure CID %d, err, %d", cid, err);
		if (err > 0) {
			err = -ENOEXEC;
		}

		return err;
	}

	return 0;
}

int pdn_ctx_auth_set(uint8_t cid, enum pdn_auth method,
		    const char *user, const char *password)
{
	int err;

	if (method != PDN_AUTH_NONE && (user == NULL || password == NULL)) {
		return -EINVAL;
	}

	err = nrf_modem_at_printf("AT+CGAUTH=%u,%d,%s,%s", cid, method, user, password);
	if (err) {
		LOG_ERR("Failed to configure auth for CID %d, err, %d", cid, err);
		if (err > 0) {
			err = -ENOEXEC;
		}

		return err;
	}

	return 0;
}

int pdn_ctx_destroy(uint8_t cid)
{
	int err;
	struct pdn *pdn;

	/* Default PDP context cannot be destroyed */
	if (cid == 0) {
		return -EINVAL;
	}

	pdn = pdn_find(cid);
	if (!pdn) {
		return -EINVAL;
	}

	err = nrf_modem_at_printf("AT+CGDCONT=%u", cid);
	if (err) {
		LOG_ERR("Failed to remove auth for CID %d, err, %d", cid, err);
		if (err > 0) {
			err = -ENOEXEC;
		}
		/* cleanup regardless */
	}

	pdn_ctx_free(pdn);

	return err;
}

static int cgact(uint8_t cid, bool activate)
{
	int err;

	err = nrf_modem_at_printf("AT+CGACT=%u,%u", activate ? 1 : 0, cid);
	if (err) {
		LOG_WRN("Failed to activate PDN for CID %d, err %d", cid, err);
		if (err > 0) {
			err = -ENOEXEC;
		}

		return err;
	}

	return 0;
}

int pdn_activate(uint8_t cid, int *esm, enum pdn_fam *fam)
{
	int err;
	int timeout;

	k_mutex_lock(&pdn_act_mutex, K_FOREVER);
	pdn_act_notif.cid = cid;

	err = cgact(cid, true);
	if (!err && fam) {
		k_sem_take(&sem_cgev, K_FOREVER);
		if (pdn_act_notif.reason == PDN_ACT_REASON_IPV4_ONLY) {
			*fam = PDN_FAM_IPV4;
		} else if (pdn_act_notif.reason == PDN_ACT_REASON_IPV6_ONLY) {
			*fam = PDN_FAM_IPV6;
		}
	}
	if (esm) {
		timeout = k_sem_take(&sem_cnec, K_MSEC(CONFIG_PDN_ESM_TIMEOUT));
		if (!timeout) {
			*esm = pdn_act_notif.esm;
		} else if (timeout == -EAGAIN) {
			*esm = 0;
		}
	}

	pdn_act_notif.cid = CID_UNASSIGNED;
	k_mutex_unlock(&pdn_act_mutex);

	return err;
}

int pdn_deactivate(uint8_t cid)
{
	int err;

	err = cgact(cid, false);
	if (err) {
		return err;
	}

	return 0;
}

int pdn_id_get(uint8_t cid)
{
	int err;
	char *p;
	char resp[32];

	err = nrf_modem_at_cmd(resp, sizeof(resp), "AT%%XGETPDNID=%u", cid);
	if (err) {
		LOG_ERR("Failed to read PDN ID for CID %d, err %d", cid, err);
		if (err > 0) {
			err = -ENOEXEC;
		}

		return err;
	}

	p = strchr(resp, ':');
	if (!p) {
		return -EBADMSG;
	}

	return strtoul(p + 1, NULL, 10);
}

int pdn_default_apn_get(char *buf, size_t len)
{
	int err;
	char apn[64] = {0};

	/* +CGDCONT: 0,"family","apn.name" */
	err = nrf_modem_at_scanf("AT+CGDCONT?",
		"+CGDCONT: 0,\"%*[^\"]\",\"%64[^\"]\"", apn);
	if (err < 0) {
		LOG_ERR("Failed to read PDP contexts, err %d", err);
		return err;
	}

	if (strlen(apn) + 1 > len) {
		return -E2BIG;
	}

	strcpy(buf, apn);
	return 0;
}

#if defined(CONFIG_LTE_LINK_CONTROL)
LTE_LC_ON_CFUN(pdn_cfun_hook, on_cfun, NULL);

static void on_cfun(enum lte_lc_func_mode mode, void *ctx)
{
	int err;

	if (mode == LTE_LC_FUNC_MODE_NORMAL ||
	    mode == LTE_LC_FUNC_MODE_ACTIVATE_LTE) {
		LOG_DBG("Subscribing to +CNEC=16 and +CGEREP=1");
		err = nrf_modem_at_printf("AT+CNEC=16");
		if (err) {
			LOG_ERR("Unable to subscribe to +CNEC=16, err %d", err);
		}
		err = nrf_modem_at_printf("AT+CGEREP=1");
		if (err) {
			LOG_ERR("Unable to subscribe to +CGEREP=1, err %d", err);
		}
	}
}
#endif /* CONFIG_LTE_LINK_CONTROL */
