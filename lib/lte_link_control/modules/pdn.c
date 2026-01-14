/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/slist.h>
#include <zephyr/logging/log.h>
#include <nrf_errno.h>
#include <nrf_modem_at.h>
#include <modem/lte_lc.h>
#include <modem/at_monitor.h>
#include <modem/at_parser.h>
#include <modem/nrf_modem_lib.h>

#include "common/event_handler_list.h"
#include "modules/pdn.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

#define CID_UNASSIGNED (-1)
#define CP_ID_UNASSIGNED (-1)

#define PDN_ACT_REASON_NONE	 (-1)
#define PDN_ACT_REASON_IPV4_ONLY (0)
#define PDN_ACT_REASON_IPV6_ONLY (1)

#define APN_STR_MAX_LEN 64

#define MODEM_CFUN_POWER_OFF 0
#define MODEM_CFUN_NORMAL 1
#define MODEM_CFUN_ACTIVATE_LTE 21

#define AT_CMD_PDN_CONTEXT_READ_INFO  "AT+CGCONTRDP=%u"
#define AT_CMD_PDN_CONTEXT_READ_INFO_DNS_ADDR_PRIMARY_INDEX 6
#define AT_CMD_PDN_CONTEXT_READ_INFO_DNS_ADDR_SECONDARY_INDEX 7
#define AT_CMD_PDN_CONTEXT_READ_INFO_MTU_INDEX 12

#define AT_CMD_PDN_CONTEXT_READ_RSP_DELIM "\r\n"

     /* "+CGCONTRDP: 0,,"example.com","","","198.176.154.230","12.34.56.78",,,,,1464\r\n
      *  +CGCONTRDP: 0,,"example.com","","","1111:2222:3:FFF::55","1111:2222:3:FFF::55",,,,,1464"
      */
#define AT_CMD_PDN_CONTEXT_READ_INFO_PARSE_LINE1 \
	"+CGCONTRDP: %*u,,\"%*[^\"]\",\"\",\"\",\"%15[0-9.]\",\"%15[0-9.]\",,,,,%u"
#define AT_CMD_PDN_CONTEXT_READ_INFO_PARSE_LINE2 \
	"+%*[^+]"\
	"+CGCONTRDP: %*u,,\"%*[^\"]\",\"\",\"\",\"%39[0-9A-Fa-f:]\",\"%39[0-9A-Fa-f:]\",,,,,%u"

static K_MUTEX_DEFINE(list_mutex);

static sys_slist_t pdp_contexts = SYS_SLIST_STATIC_INIT(&pdp_contexts);

struct pdn {
	sys_snode_t node; /* list handling */
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
#ifndef CONFIG_UNITY
AT_MONITOR(pdn_cgev, "+CGEV", on_cgev);
AT_MONITOR(pdn_cnec_esm, "+CNEC_ESM", on_cnec_esm);
#endif

/* Expect enough added heap for the default PDN context */
BUILD_ASSERT(sizeof(struct pdn) <= CONFIG_HEAP_MEM_POOL_ADD_SIZE_LTE_LINK_CONTROL);

static void pdn_event_dispatch(uint8_t cid, enum lte_lc_pdn_evt_type pdn_event, int esm_err,
			       int8_t cp_id)
{
	struct lte_lc_evt evt = {
		.type = LTE_LC_EVT_PDN,
		.pdn = {
			.type = pdn_event,
			.cid = cid,
			.esm_err = esm_err,
			.cellular_profile_id = cp_id,
		}
	};

	event_handler_list_dispatch(&evt);
}

static struct pdn *pdn_find(int cid)
{
	struct pdn *pdn;

	SYS_SLIST_FOR_EACH_CONTAINER(&pdp_contexts, pdn, node) {
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

	pdn->context_id = INT8_MAX;

	k_mutex_lock(&list_mutex, K_FOREVER);
	sys_slist_append(&pdp_contexts, &pdn->node);
	k_mutex_unlock(&list_mutex);

	return pdn;
}

#ifdef CONFIG_UNITY
void on_cnec_esm(const char *notif)
#else
static void on_cnec_esm(const char *notif)
#endif
{
	char *p;
	uint32_t cid;
	uint32_t esm_err;
	struct pdn *pdn;
	struct lte_lc_evt evt = {
		.type = LTE_LC_EVT_PDN,
		.pdn = {
			.cellular_profile_id = CP_ID_UNASSIGNED,
		},
	};

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

	evt.pdn.type = LTE_LC_EVT_PDN_ESM_ERROR;
	evt.pdn.cid = cid;
	evt.pdn.esm_err = esm_err;

	event_handler_list_dispatch(&evt);

	if (cid == pdn_act_notif.cid) {
		pdn_act_notif.esm = esm_err;

		k_sem_give(&sem_cnec);
	}
}

static void parse_cgev(const char *notif)
{
	const char *response;

	/* we are on the system workqueue, let's not put this on the stack */
	static const struct {
		const char *notif;
		const enum lte_lc_pdn_evt_type type;
	} map[] = {
		/* +CGEV: ME PDN ACT <cid>[,<reason>] */
		{"ME PDN ACT ", LTE_LC_EVT_PDN_ACTIVATED},
		/* +CGEV: ME PDN DEACT <cid> */
		{"ME PDN DEACT ", LTE_LC_EVT_PDN_DEACTIVATED},
		/* +CGEV: NW PDN DEACT <cid> */
		{"NW PDN DEACT ", LTE_LC_EVT_PDN_DEACTIVATED},
		/* +CGEV: ME PDN SUSPENDED <cid> */
		{"ME PDN SUSPENDED ", LTE_LC_EVT_PDN_SUSPENDED},
		/* +CGEV: ME PDN RESUMED <cid> */
		{"ME PDN RESUMED ", LTE_LC_EVT_PDN_RESUMED},
		/* +CGEV: ME DETACH [<cp_id>] */
		{"ME DETACH",  LTE_LC_EVT_PDN_NETWORK_DETACH},
		/* +CGEV: NW DETACH [<cp_id>] */
		{"NW DETACH",  LTE_LC_EVT_PDN_NETWORK_DETACH},
		/* --- Order is important from here --- */
		/* +CGEV: IPV6 FAIL <cid> */
		{"IPV6 FAIL ",  LTE_LC_EVT_PDN_IPV6_DOWN},
		/* +CGEV: IPV6 <cid> */
		{"IPV6 ", LTE_LC_EVT_PDN_IPV6_UP},
	};

	/* Skip the '+CGEV:' prefix and any spaces */
	response = strchr(notif, ':');
	if (!response) {
		return;
	}

	response++;

	while (isspace((unsigned char)*response)) {
		response++;
	}

	/* Loop through the map and find the event */
	for (size_t i = 0; i < ARRAY_SIZE(map); i++) {
		size_t notif_len = strlen(map[i].notif);
		struct lte_lc_evt evt = {
			.type = LTE_LC_EVT_PDN,
			.pdn = {
				.type = map[i].type,
				.cellular_profile_id = CP_ID_UNASSIGNED,
			},
		};
		char *p;

		if (strncmp(response, map[i].notif, notif_len) != 0) {
			continue;
		}

		p = (char *)(response + notif_len);

		/* Parse <cid> or <cp_id> if present */
		if ((*p == ' ') || (*(p - 1) == ' ')) {
			switch (evt.pdn.type) {
			case LTE_LC_EVT_PDN_ACTIVATED:
			case LTE_LC_EVT_PDN_DEACTIVATED:
			case LTE_LC_EVT_PDN_IPV6_UP:
			case LTE_LC_EVT_PDN_IPV6_DOWN:
			case LTE_LC_EVT_PDN_SUSPENDED:
			case LTE_LC_EVT_PDN_RESUMED:
				evt.pdn.cid = (int8_t)strtoul(p, &p, 10);

				break;
			case LTE_LC_EVT_PDN_NETWORK_DETACH:
				evt.pdn.cellular_profile_id = (int8_t)strtoul(p, &p, 10);

				break;
			default:
				break;
			}
		}

		/* Handle special case where pdn_activate() is waiting for the ACTIVATED event */
		if (evt.pdn.type == LTE_LC_EVT_PDN_ACTIVATED && evt.pdn.cid == pdn_act_notif.cid) {
			if (*p == ',') {
				pdn_act_notif.reason = (int8_t)strtol(p + 1, NULL, 10);
			} else {
				pdn_act_notif.reason = PDN_ACT_REASON_NONE;
			}

			evt.pdn.esm_err = pdn_act_notif.reason;

			k_sem_give(&sem_cgev);
		}

		/* Handle network detach event, where all registered CIDs should be notified */
		if (evt.pdn.type == LTE_LC_EVT_PDN_NETWORK_DETACH) {
			/* Valid CIDs are in the range 0-9 for each cellular profile, starting from
			 * the cellular profile ID * 10 if present, or 0 if not present.
			 */
			size_t start_cid = 0;
			size_t end_cid = 9;
			struct pdn *pdn;

			if (evt.pdn.cellular_profile_id != CP_ID_UNASSIGNED) {
				start_cid = evt.pdn.cellular_profile_id * 10;
				end_cid = start_cid + 9;
			}

			SYS_SLIST_FOR_EACH_CONTAINER(&pdp_contexts, pdn, node) {
				if (pdn->context_id < start_cid || pdn->context_id > end_cid) {
					continue;
				}

				pdn_event_dispatch(pdn->context_id, evt.pdn.type, 0,
						   evt.pdn.cellular_profile_id);
			}

			return;
		}

		event_handler_list_dispatch(&evt);
	}
}

static void parse_cgev_apn_rate_ctrl(const char *notif)
{
	char *p;
	uint8_t apn_rate_ctrl_status;
	struct lte_lc_evt evt = { 0 };

	/* +CGEV: APNRATECTRL STAT <cid>,<status>,[<time_remaining>] */
	p = strstr(notif, "APNRATECTRL STAT");
	if (!p) {
		return;
	}

	/* Parse <cid> */
	p += strlen("APNRATECTRL STAT");
	__ASSERT(*p == ' ', "Bad APNRATECTRL parsing");

	evt.pdn.cid = (uint8_t)strtoul(p, &p, 10);
	__ASSERT(*p == ',', "Bad APNRATECTRL parsing");

	/* Parse <status> */
	apn_rate_ctrl_status = (uint8_t)strtoul(p + 1, NULL, 10);

	evt.type = LTE_LC_EVT_PDN;
	evt.pdn.type = apn_rate_ctrl_status == 1 ?
		       LTE_LC_EVT_PDN_APN_RATE_CONTROL_ON :
		       LTE_LC_EVT_PDN_APN_RATE_CONTROL_OFF;

	event_handler_list_dispatch(&evt);
}

#ifdef CONFIG_UNITY
void on_cgev(const char *notif)
#else
static void on_cgev(const char *notif)
#endif
{
	parse_cgev(notif);
	parse_cgev_apn_rate_ctrl(notif);
}

#if defined(CONFIG_LTE_LC_PDN_DEFAULTS_OVERRIDE)
static void pdn_defaults_override(void)
{
	int err;

	err = pdn_ctx_configure(0, CONFIG_LTE_LC_PDN_DEFAULT_APN,
				CONFIG_LTE_LC_PDN_DEFAULT_FAM, NULL);
	if (err) {
		LOG_ERR("Failed to configure default CID, err %d", err);
		return;
	}

#if defined(CONFIG_LTE_LC_PDN_DEFAULT_AUTH_PAP) || defined(CONFIG_LTE_LC_PDN_DEFAULT_AUTH_CHAP)
	/* +CGAUTH=<cid>[,<auth_prot>[,<userid>[,<password>]]] */
	BUILD_ASSERT(sizeof(CONFIG_LTE_LC_PDN_DEFAULT_USERNAME) > 1, "Username not defined");

	err = pdn_ctx_auth_set(0, CONFIG_LTE_LC_PDN_DEFAULT_AUTH,
			       CONFIG_LTE_LC_PDN_DEFAULT_USERNAME,
			       CONFIG_LTE_LC_PDN_DEFAULT_PASSWORD);
	if (err) {
		LOG_ERR("Failed to set auth params for default CID, err %d", err);
		return;
	}
#endif /* CONFIG_LTE_LC_PDN_DEFAULT_AUTH_PAP || CONFIG_LTE_LC_PDN_DEFAULT_AUTH_CHAP */
}
#endif /* CONFIG_LTE_LC_PDN_DEFAULTS_OVERRIDE */

#ifdef CONFIG_UNITY
void lte_lc_pdn_on_modem_init(int ret, void *ctx)
#else
NRF_MODEM_LIB_ON_INIT(pdn_modem_init_hook, lte_lc_pdn_on_modem_init, NULL);
static void lte_lc_pdn_on_modem_init(int ret, void *ctx)
#endif
{
	int err;

	if (ret != 0) {
		LOG_ERR("Modem library did not initialize: %d", ret);
		return;
	}

#if defined(CONFIG_LTE_LC_PDN_LEGACY_PCO)
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
#endif /* CONFIG_LTE_LC_PDN_LEGACY_PCO */

#if defined(CONFIG_LTE_LC_PDN_DEFAULTS_OVERRIDE)
	pdn_defaults_override();
#endif
}

static bool pdn_ctx_free(struct pdn *pdn)
{
	bool removed;

	if (!pdn) {
		return false;
	}

	k_mutex_lock(&list_mutex, K_FOREVER);

	removed = sys_slist_find_and_remove(&pdp_contexts, &pdn->node);

	k_mutex_unlock(&list_mutex);

	if (!removed) {
		return false;
	}

	k_free(pdn);

	return true;
}

int pdn_ctx_create(uint8_t *cid)
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
		(void)pdn_ctx_free(pdn);

		return err;
	} else if (err == 0) {
		/* no argument matched */
		(void)pdn_ctx_free(pdn);

		return -EBADMSG;
	}

	if (ctx_id_tmp > SCHAR_MAX || ctx_id_tmp < SCHAR_MIN) {
		LOG_ERR("Context ID (%d) out of bounds", ctx_id_tmp);
		(void)pdn_ctx_free(pdn);

		return -EFAULT;
	}

	pdn->context_id = (int8_t)ctx_id_tmp;

	*cid = pdn->context_id;

	return 0;
}

int pdn_ctx_configure(uint8_t cid, const char *apn, enum lte_lc_pdn_family family,
		      struct lte_lc_pdn_pdp_context_opts *opt)
{
	int err;
	char type_str[] = "IPV4V6"; /* longest type */

	if (apn != NULL && strlen(apn) >= APN_STR_MAX_LEN) {
		return -EINVAL;
	}

	switch (family) {
	case LTE_LC_PDN_FAM_IPV4:
		strncpy(type_str, "IP", sizeof(type_str));

		break;
	case LTE_LC_PDN_FAM_IPV6:
		strncpy(type_str, "IPV6", sizeof(type_str));

		break;
	case LTE_LC_PDN_FAM_IPV4V6:
		strncpy(type_str, "IPV4V6", sizeof(type_str));

		break;
	case LTE_LC_PDN_FAM_NONIP:
		strncpy(type_str, "Non-IP", sizeof(type_str));

		break;
	default:
		LOG_ERR("Unknown PDN address family %d", family);

		return -EINVAL;
	}

	if (opt) {
		/* NOTE: this is a workaround for an issue
		 * where passing default values does not work as intended.
		 * Only format these params when `opt` is passed explicitly.
		 */
		err = nrf_modem_at_printf("AT+CGDCONT=%u,%s,%s,,,,%u,,,,%u,%u",
			cid, type_str, apn,
			opt->ip4_addr_alloc,
			opt->nslpi,
			opt->secure_pco);
	} else {
		if (apn != NULL) {
			err = nrf_modem_at_printf("AT+CGDCONT=%u,%s,%s", cid, type_str, apn);
		} else {
			err = nrf_modem_at_printf("AT+CGDCONT=%u,%s", cid, type_str);
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

int pdn_ctx_auth_set(uint8_t cid, enum lte_lc_pdn_auth method,
		     const char *user, const char *password)
{
	int err;

	if (method == LTE_LC_PDN_AUTH_NONE) {
		if (user != NULL || password != NULL) {
			return -EINVAL;
		}

		return 0;
	}

	if (user == NULL || password == NULL) {
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

	(void)pdn_ctx_free(pdn);

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

int pdn_activate(uint8_t cid, int *esm, enum lte_lc_pdn_family *family)
{
	int err;
	int timeout;

	k_mutex_lock(&pdn_act_mutex, K_FOREVER);

	pdn_act_notif.cid = cid;
	pdn_act_notif.reason = PDN_ACT_REASON_NONE;

	err = cgact(cid, true);
	if (!err && family) {
		k_sem_take(&sem_cgev, K_FOREVER);

		if (pdn_act_notif.reason == PDN_ACT_REASON_IPV4_ONLY) {
			*family = LTE_LC_PDN_FAM_IPV4;
		} else if (pdn_act_notif.reason == PDN_ACT_REASON_IPV6_ONLY) {
			*family = LTE_LC_PDN_FAM_IPV6;
		}
	}

	if (esm) {
		timeout = k_sem_take(&sem_cnec, K_MSEC(CONFIG_LTE_LC_PDN_ESM_TIMEOUT));
		if (!timeout) {
			*esm = pdn_act_notif.esm;
		} else {
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

static int pdn_sa_family_from_ip_string(const char *src)
{
	char buf[NET_INET6_ADDRSTRLEN];

	if (zsock_inet_pton(NET_AF_INET, src, buf)) {
		return NET_AF_INET;
	} else if (zsock_inet_pton(NET_AF_INET6, src, buf)) {
		return NET_AF_INET6;
	}
	return -1;
}

/** @brief Fill PDN dynamic info with DNS addresses and mtu. */
static void pdn_dynamic_info_dns_addr_fill(struct lte_lc_pdn_dynamic_info *pdn_info, uint32_t mtu,
					   const char *dns_addr_str_primary,
					   const char *dns_addr_str_secondary)
{
	const int family = pdn_sa_family_from_ip_string(dns_addr_str_primary);

	if (family == NET_AF_INET) {
		(void)zsock_inet_pton(NET_AF_INET, dns_addr_str_primary,
				      &(pdn_info->dns_addr4_primary));
		(void)zsock_inet_pton(NET_AF_INET, dns_addr_str_secondary,
				      &(pdn_info->dns_addr4_secondary));

		pdn_info->ipv4_mtu = mtu;
	} else if (family == NET_AF_INET6) {
		(void)zsock_inet_pton(NET_AF_INET6, dns_addr_str_primary,
				      &(pdn_info->dns_addr6_primary));
		(void)zsock_inet_pton(NET_AF_INET6, dns_addr_str_secondary,
				      &(pdn_info->dns_addr6_secondary));
		pdn_info->ipv6_mtu = mtu;
	}
}

int pdn_dynamic_info_get(uint8_t cid, struct lte_lc_pdn_dynamic_info *pdn_info)
{
	int ret;
	char at_cmd_buf[sizeof("AT+CGCONTRDP=###")];
	char dns_addr_str_primary[NET_INET6_ADDRSTRLEN];
	char dns_addr_str_secondary[NET_INET6_ADDRSTRLEN];
	uint32_t mtu = 0;

	if (!pdn_info) {
		return -EINVAL;
	}

	/* Reset PDN dynamic info. */
	memset(pdn_info, 0, sizeof(struct lte_lc_pdn_dynamic_info));

	/* Clear secondary DNS address buffer. */
	memset(dns_addr_str_secondary, 0, sizeof(dns_addr_str_secondary));

	(void)snprintf(at_cmd_buf, sizeof(at_cmd_buf), AT_CMD_PDN_CONTEXT_READ_INFO, cid);

	ret = nrf_modem_at_scanf(at_cmd_buf, AT_CMD_PDN_CONTEXT_READ_INFO_PARSE_LINE1,
				 dns_addr_str_primary, dns_addr_str_secondary, &mtu);
	if (ret < 1) {
		/* Don't log an error if no PDN connections are active, this may not be considered
		 * an error by the caller.
		 */
		if (ret != -NRF_EBADMSG) {
			LOG_ERR("nrf_modem_at_scanf failed, ret: %d", ret);
		}

		return ret;
	}

	pdn_dynamic_info_dns_addr_fill(pdn_info, mtu, dns_addr_str_primary, dns_addr_str_secondary);

	/* Reset secondary DNS address buffer and mtu. */
	memset(dns_addr_str_secondary, 0, sizeof(dns_addr_str_secondary));

	mtu = 0;

	/* Scan second line if PDN has dual stack capabilities. */
	ret = nrf_modem_at_scanf(at_cmd_buf, AT_CMD_PDN_CONTEXT_READ_INFO_PARSE_LINE2,
				 dns_addr_str_primary, dns_addr_str_secondary, &mtu);
	if (ret < 1) {
		/* We only got one entry, but that is ok. */
		return 0;
	}

	pdn_dynamic_info_dns_addr_fill(pdn_info, mtu, dns_addr_str_primary, dns_addr_str_secondary);

	return 0;
}

int pdn_ctx_default_apn_get(char *buf, size_t len)
{
	int err;
	char apn[64] = {0};

	/* +CGDCONT: 0,"family","apn.name" */
	err = nrf_modem_at_scanf("AT+CGDCONT?",
		"+CGDCONT: 0,\"%*[^\"]\",\"%63[^\"]\"", apn);
	if (err < 0) {
		LOG_ERR("Failed to read PDP contexts, err %d", err);
		return err;
	} else if (err == 0) {
		/* no argument matched */
		return -EFAULT;
	}

	if (strlen(apn) + 1 > len) {
		return -E2BIG;
	}

	strcpy(buf, apn);

	return 0;
}

int pdn_default_ctx_events_enable(void)
{
	struct pdn *pdn;
	struct pdn *tmp;
	bool exists = false;

	k_mutex_lock(&list_mutex, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&pdp_contexts, pdn, tmp, node) {
		if (pdn->context_id == 0) {
			exists = true;

			break;
		}
	}

	k_mutex_unlock(&list_mutex);

	if (exists) {
		return 0;
	}

	pdn = pdn_ctx_new();
	if (!pdn) {
		return -ENOMEM;
	}

	pdn->context_id = 0;

	return 0;
}

int pdn_default_ctx_events_disable(void)
{
	struct pdn *pdn;
	struct pdn *tmp;
	bool removed = false;

	k_mutex_lock(&list_mutex, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&pdp_contexts, pdn, tmp, node) {
		if (pdn->context_id == 0) {
			removed = pdn_ctx_free(pdn);

			break;
		}
	}

	k_mutex_unlock(&list_mutex);

	if (!removed) {
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_UNITY)
void pdn_on_modem_cfun(int mode, void *ctx)
#else
NRF_MODEM_LIB_ON_CFUN(pdn_cfun_hook, pdn_on_modem_cfun, NULL);

static void pdn_on_modem_cfun(int mode, void *ctx)
#endif
{
	int err;
	struct pdn *pdn;
	struct pdn *tmp;

	if (mode == MODEM_CFUN_NORMAL ||
	    mode == MODEM_CFUN_ACTIVATE_LTE) {
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

	if (mode == MODEM_CFUN_POWER_OFF) {
		k_mutex_lock(&list_mutex, K_FOREVER);

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&pdp_contexts, pdn, tmp, node) {
			if (pdn->context_id == 0) {
				/* Default context is not destroyed */
				continue;
			}

			pdn_event_dispatch(pdn->context_id, LTE_LC_EVT_PDN_CTX_DESTROYED, 0,
					   CP_ID_UNASSIGNED);

			(void)pdn_ctx_free(pdn);
		}

		k_mutex_unlock(&list_mutex);

#if defined(CONFIG_LTE_LC_PDN_DEFAULTS_OVERRIDE)
		pdn_defaults_override();
#endif
	}
}
