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
#include <zephyr.h>
#include <init.h>
#include <modem/pdn.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(pdn, CONFIG_PDN_LOG_LEVEL);

/* Buffer size for AT command and responses.
 * Longest AT command is:
 *   AT+CGDCONT=20,"IPV4V6","<APN>",0,0,0,0,0,0,0,0,0
 * where the APN can be up to 100 chars.
 * That's 43 chars, plus 100, which we round to 160.
 */
#define AT_BUF_SIZE (160)
/* Global buffer for AT commands and responses. */
static char at_buf[AT_BUF_SIZE];

#define CID_UNASSIGNED (-1)

struct pdn {
	pdn_event_handler_t callback;
	int8_t context_id;
};

static int esm_from_notif;
static struct k_sem notif_sem;
static struct pdn pdn_contexts[CONFIG_PDN_CONTEXTS_MAX];
static pdn_event_handler_t default_callback;

static struct pdn *pdn_find(int cid)
{
	for (size_t i = 0; i < ARRAY_SIZE(pdn_contexts); i++) {
		struct pdn *pdn = &pdn_contexts[i];

		if (pdn->context_id == cid) {
			return pdn;
		}
	}

	return NULL;
}

static struct pdn *pdn_avail_get(void)
{
	return pdn_find(CID_UNASSIGNED);
}

static void at_notif_handler(void *context, const char *notif)
{
	char *p;
	uint8_t cid;
	uint8_t esm_err;
	struct pdn *pdn;

	/* These notifications are at least 10 characters long,
	 * skip shorter strings.
	 */
	if (strlen(notif) < 10) {
		return;
	}

	/* AT notif: +CNEC_ESM: <cause>,<cid>
	 * Handle this by itself, we need to extract both CID and ESM
	 * TODO: check before all other notifs?
	 */
	p = strstr(notif, "CNEC_ESM: ");
	if (p) {
		esm_err = strtoul(p + strlen("CNEC_ESM: "), NULL, 10);

		p = strchr(p, ',');
		if (!p) {
			return;
		}

		cid = strtoul(p + 1, NULL, 10);

		pdn = pdn_find(cid);
		if (!pdn) {
			return;
		}

		if (pdn->callback) {
			pdn->callback((intptr_t)cid, PDN_EVENT_CNEC_ESM,
				      esm_err);
		}

		esm_from_notif = esm_err;
		k_sem_give(&notif_sem);

		return;
	}

	const struct {
		const char *notif;
		const enum pdn_event event;
	} map[] = {
		{"CGEV: ME PDN ACT",	PDN_EVENT_ACTIVATED},	/* +CGEV: ME PDN ACT <cid> */
		{"CGEV: ME PDN DEACT",	PDN_EVENT_DEACTIVATED},	/* +CGEV: ME PDN DEACT <cid> */
		/* Order is important */
		{"CGEV: IPV6 FAIL",	PDN_EVENT_IPV6_DOWN},	/* +CGEV: IPV6 FAIL <cid> */
		{"CGEV: IPV6",		PDN_EVENT_IPV6_UP},	/* +CGEV: IPV6 <cid> */
	};

	for (size_t i = 0; i < ARRAY_SIZE(map); i++) {
		p = strstr(notif, map[i].notif);
		if (!p) {
			continue;
		}

		cid = strtoul(p + strlen(map[i].notif), NULL, 10);

		if (default_callback && cid == 0) {
			default_callback(0, map[i].event, 0);
			return;
		}

		pdn = pdn_find(cid);
		if (!pdn) {
			return;
		}

		if (pdn->callback) {
			pdn->callback((intptr_t)cid, map[i].event, 0);
			return;
		}
	}
}

int pdn_init(void)
{
	int err;
	static bool has_init;

	if (has_init) {
		return 0;
	}

	k_sem_init(&notif_sem, 0, 1);

#if defined(CONFIG_PDN_LEGACY_PCO)
	err = at_cmd_write("AT%XEPCO=0", NULL, 0, NULL);
	if (err) {
		LOG_WRN("Failed to set legacy PCO mode, err %d", err);
	}
#endif

	for (size_t i = 0; i < ARRAY_SIZE(pdn_contexts); i++) {
		pdn_contexts[i].context_id = CID_UNASSIGNED;
	}

	err = at_notif_register_handler(NULL, at_notif_handler);
	if (err) {
		return err;
	}

	has_init = true;
	return 0;
}

int pdn_default_callback_set(pdn_event_handler_t cb)
{
	if (!cb) {
		return -EINVAL;
	}

	default_callback = cb;

	return 0;
}

int pdn_ctx_create(uint8_t *cid, pdn_event_handler_t cb)
{
	int err;
	char *p;
	struct pdn *pdn;

	if (!cid) {
		return -EFAULT;
	}

	pdn = pdn_avail_get();
	if (!pdn) {
		return -ENOMEM;
	}

	err = at_cmd_write("AT%XNEWCID?", at_buf, sizeof(at_buf), NULL);
	if (err) {
		return err;
	}

	p = strchr(at_buf, ':');
	if (!p) {
		return -EBADMSG;
	}

	pdn->context_id = strtoul(p + 1, NULL, 10);

	*cid = pdn->context_id;

	if (cb) {
		pdn->callback = cb;
	}

	return 0;
}

int pdn_ctx_configure(uint8_t cid, const char *apn, enum pdn_fam fam,
		      struct pdn_pdp_opt *opt)
{
	int err;
	char family[] = "IPV4V6"; /* longest family */

	if (!apn) {
		return -EFAULT;
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
	}

	if (opt) {
		/* NOTE: this is a workaround for an issue
		 * where passing default values does not work as intended.
		 * Only format these params when `opt` is passed explicitly.
		 */
		err = snprintf(at_buf, sizeof(at_buf),
			"AT+CGDCONT=%u,%s,%s,,,,%u,,,,%u,%u",
			cid, family, apn,
			opt->ip4_addr_alloc,
			opt->nslpi,
			opt->secure_pco);
	} else {
		err = snprintf(at_buf, sizeof(at_buf),
			"AT+CGDCONT=%u,%s,%s", cid, family, apn);
	}

	if (err < 0 || err >= sizeof(at_buf)) {
		return -ENOBUFS;
	}

	err = at_cmd_write(at_buf, NULL, 0, NULL);
	if (err) {
		LOG_ERR("Failed to configure CID %d, err, %d", cid, err);
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

	err = snprintf(at_buf, sizeof(at_buf),
		       "AT+CGAUTH=%u,%s,%s", cid, user, password);

	if (err < 0 || err >= sizeof(at_buf)) {
		return -ENOBUFS;
	}

	err = at_cmd_write(at_buf, NULL, 0, NULL);
	if (err) {
		LOG_ERR("Failed to configure auth for CID %d, err, %d", cid, err);
		return err;
	}

	return 0;
}

int pdn_ctx_destroy(uint8_t cid)
{
	int err;
	struct pdn *pdn;

	pdn = pdn_find(cid);
	if (!pdn) {
		return -EINVAL;
	}

	err = snprintf(at_buf, sizeof(at_buf), "AT+CGDCONT=%u", cid);
	if (err < 0 || err >= sizeof(at_buf)) {
		return -ENOBUFS;
	}

	err = at_cmd_write(at_buf, NULL, 0, NULL);
	if (err) {
		return err;
	}

	pdn->context_id = CID_UNASSIGNED;
	pdn->callback = NULL;

	return 0;
}

static int cgact(uint8_t cid, bool activate)
{
	int err;

	err = snprintf(at_buf, sizeof(at_buf), "AT+CGACT=%u,%u",
		       activate ? 1 : 0, cid);
	if (err < 0 || err >= sizeof(at_buf)) {
		return -ENOBUFS;
	}

	err = at_cmd_write(at_buf, NULL, 0, NULL);
	if (err) {
		LOG_WRN("Failed to activate PDN for CID %d, err %d", cid, err);
		return err;
	}

	return 0;
}

int pdn_activate(uint8_t cid, int *esm)
{
	int err;
	int timeout;

	err = cgact(cid, true);
	if (esm) {
		timeout = k_sem_take(&notif_sem, K_MSEC(CONFIG_PDN_ESM_TIMEOUT));
		if (!timeout) {
			*esm = esm_from_notif;
		} else if (timeout == -EAGAIN) {
			*esm = 0;
		}
		esm_from_notif = 0;
	}

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

	err = snprintf(at_buf, sizeof(at_buf), "AT%%XGETPDNID=%u", cid);
	if (err < 0 || err >= sizeof(at_buf)) {
		return -ENOBUFS;
	}

	err = at_cmd_write(at_buf, resp, sizeof(resp), NULL);
	if (err) {
		LOG_ERR("Failed to read PDN ID for CID %d, err %d", cid, err);
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
	char *p, *end;

	err = at_cmd_write("AT+CGDCONT?", at_buf, sizeof(at_buf), NULL);
	if (err) {
		LOG_ERR("Failed to read PDP contexts, err %d", err);
		return err;
	}

	/* CGDCONT: #,"Family","APN" */
	p = strstr(at_buf, "CGDCONT: 0");
	if (!p) {
		return -EBADMSG;
	}

	/* Skip to the third double quote `"` */
	for (size_t i = 0; i < 3; i++) {
		p = strchr(p, '\"');
		if (!p) {
			return -EBADMSG;
		}
		p++;
	}

	/* Find closing quote and replace with \0 */
	end = strchr(p, '\"');
	if (!end) {
		return -EBADMSG;
	}
	*end = '\0';

	if (end - p + sizeof('\0') > len) {
		return -ENOBUFS;
	}

	memcpy(buf, p, end - p + sizeof('\0'));

	return 0;
}

#if defined(CONFIG_PDN_SYS_INIT)
static int pdn_sys_init(const struct device *unused)
{
	int err;

#if defined(CONFIG_PDN_DEFAULTS_OVERRIDE)
	err = pdn_ctx_configure(0, CONFIG_PDN_DEFAULT_APN,
				CONFIG_PDN_DEFAULT_FAM, NULL);
	if (err) {
		LOG_ERR("Failed to configure default CID, err %d", err);
		return -1;
	}

	err = pdn_ctx_auth_set(0, CONFIG_PDN_DEFAULT_AUTH,
			       CONFIG_PDN_DEFAULT_USERNAME,
			       CONFIG_PDN_DEFAULT_PASSWORD);
	if (err) {
		LOG_ERR("Failed to set auth params for default CID, err %d",
			err);
		return -1;
	}
#endif /* CONFIG_PDN_DEFAULTS_OVERRIDE */

	err = pdn_init();
	if (err) {
		LOG_ERR("Failed to initialize PDN library, err %d", err);
		return -1;
	}

	return 0;
}

SYS_INIT(pdn_sys_init, APPLICATION, CONFIG_PDN_INIT_PRIORITY);
#endif /* CONFIG_PDN_SYS_INIT */
