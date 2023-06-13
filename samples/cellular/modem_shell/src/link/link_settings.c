/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <assert.h>
#include <zephyr/shell/shell.h>
#include <zephyr/settings/settings.h>

#include <nrf_modem_at.h>

#include <modem/lte_lc.h>
#include <modem/pdn.h>

#include "mosh_defines.h"
#include "mosh_print.h"

#include "link_shell_print.h"
#include "link_shell_pdn.h"
#include "link_settings.h"

#define LINK_SETT_KEY "mosh_link_settings"

/* ****************************************************************************/

#define LINK_SETT_DEFCONT_ENABLED "defcont_enabled"
#define LINK_SETT_DEFCONT_APN_KEY "defcont_apn"
#define LINK_SETT_DEFCONT_IP_FAMILY_KEY "defcont_ip_family"

#define LINK_SETT_DEFCONT_MAX_IP_FAMILY_STR_LEN 6
#define LINK_SETT_DEFCONT_DEFAULT_APN "internet"

/* ****************************************************************************/

#define LINK_SETT_DEFCONTAUTH_ENABLED "defcontauth_enabled"
#define LINK_SETT_DEFCONTAUTH_USERNAME_KEY "defcontauth_username"
#define LINK_SETT_DEFCONTAUTH_PASSWORD_KEY "defcontauth_password"
#define LINK_SETT_DEFCONTAUTH_PROTOCOL_KEY "defcontauth_prot"

#define LINK_SETT_DEFCONTAUTH_MAX_UNAME_STR_LEN 32
#define LINK_SETT_DEFCONTAUTH_MAX_PWORD_STR_LEN 32

#define LINK_SETT_DEFCONTAUTH_DEFAULT_USERNAME "username"
#define LINK_SETT_DEFCONTAUTH_DEFAULT_PASSWORD "password"

/* ****************************************************************************/

#define LINK_SETT_DNSADDR_MAX_IP_LEN 46

#define LINK_SETT_DNSADDR_ENABLED "dnsaddr_enabled"
#define LINK_SETT_DNSADDR_IP_KEY "dnsaddr_ip"

/* ****************************************************************************/

#define LINK_SETT_SYSMODE_KEY "sysmode"
#define LINK_SETT_SYSMODE_LTE_PREFERENCE_KEY "sysmode_lte_pref"

/* ****************************************************************************/

#define LINK_SETT_NORMAL_MODE_AT_CMD_1_KEY "funmmode_normal_at_cmd_1"
#define LINK_SETT_NORMAL_MODE_AT_CMD_2_KEY "funmmode_normal_at_cmd_2"
#define LINK_SETT_NORMAL_MODE_AT_CMD_3_KEY "funmmode_normal_at_cmd_3"

/* ****************************************************************************/

#define LINK_SETT_NORMAL_MODE_AUTOCONN_ENABLED "normal_mode_autoconn_enabled"
#define LINK_SETT_NORMAL_MODE_AUTOCONN_REL14_USED "normal_mode_autoconn_rel14_used"

/* ****************************************************************************/
struct link_sett_t {
	char defcont_apn_str[MOSH_APN_STR_MAX_LEN + 1];
	bool defcont_enabled;

	char defcontauth_uname_str[LINK_SETT_DEFCONTAUTH_MAX_UNAME_STR_LEN + 1];
	char defcontauth_pword_str[LINK_SETT_DEFCONTAUTH_MAX_PWORD_STR_LEN + 1];
	bool defcontauth_enabled;

	char dnsaddr_ip_str[LINK_SETT_DNSADDR_MAX_IP_LEN + 1];
	bool dnsaddr_enabled;

	enum lte_lc_system_mode sysmode;
	enum lte_lc_system_mode_preference sysmode_lte_preference;
	enum pdn_fam pdn_family;
	enum pdn_auth defcontauth_prot;

	/* Note: if adding more memory slots, remember also update
	 * LINK_SETT_NMODEAT_MEM_SLOT_INDEX_START/END accordingly.
	 */
	char normal_mode_at_cmd_str_1
	[CONFIG_MOSH_LINK_SETT_NORMAL_MODE_AT_CMD_STR_LEN + 1];
	char normal_mode_at_cmd_str_2
	[CONFIG_MOSH_LINK_SETT_NORMAL_MODE_AT_CMD_STR_LEN + 1];
	char normal_mode_at_cmd_str_3
	[CONFIG_MOSH_LINK_SETT_NORMAL_MODE_AT_CMD_STR_LEN + 1];

	bool normal_mode_autoconn_enabled;
	bool normal_mode_autoconn_rel14_used;
};
static struct link_sett_t link_settings;

/* ****************************************************************************/

/**@brief Callback when settings_load() is called. */
static int link_sett_handler(const char *key, size_t len,
			      settings_read_cb read_cb, void *cb_arg)
{
	int err;

	if (strcmp(key, LINK_SETT_DEFCONT_ENABLED) == 0) {
		err = read_cb(cb_arg, &link_settings.defcont_enabled,
			      sizeof(link_settings.defcont_enabled));
		if (err < 0) {
			mosh_error("Failed to read defcont enabled, error: %d",	err);
			return err;
		}
		return 0;
	} else if (strcmp(key, LINK_SETT_DEFCONT_APN_KEY) == 0) {
		err = read_cb(cb_arg, &link_settings.defcont_apn_str,
			      sizeof(link_settings.defcont_apn_str));
		if (err < 0) {
			mosh_error("Failed to read defcont APN, error: %d", err);
			return err;
		}
		return 0;
	} else if (strcmp(key, LINK_SETT_DEFCONT_IP_FAMILY_KEY) == 0) {
		err = read_cb(cb_arg, &link_settings.pdn_family,
			      sizeof(link_settings.pdn_family));
		if (err < 0) {
			mosh_error("Failed to read defcont PDN family, error: %d", err);
			return err;
		}
		return 0;
	}
	if (strcmp(key, LINK_SETT_DEFCONTAUTH_ENABLED) == 0) {
		err = read_cb(cb_arg, &link_settings.defcontauth_enabled,
			      sizeof(link_settings.defcontauth_enabled));
		if (err < 0) {
			mosh_error("Failed to read defcontauth enabled, error: %d", err);
			return err;
		}
		return 0;
	} else if (strcmp(key, LINK_SETT_DEFCONTAUTH_USERNAME_KEY) == 0) {
		err = read_cb(cb_arg, &link_settings.defcontauth_uname_str,
			      sizeof(link_settings.defcontauth_uname_str));
		if (err < 0) {
			mosh_error("Failed to read defcontauth username, error: %d", err);
			return err;
		}
		return 0;
	} else if (strcmp(key, LINK_SETT_DEFCONTAUTH_PASSWORD_KEY) == 0) {
		err = read_cb(cb_arg, &link_settings.defcontauth_pword_str,
			      sizeof(link_settings.defcontauth_pword_str));
		if (err < 0) {
			mosh_error("Failed to read defcontauth password, error: %d", err);
			return err;
		}
		return 0;
	} else if (strcmp(key, LINK_SETT_DEFCONTAUTH_PROTOCOL_KEY) == 0) {
		err = read_cb(cb_arg, &link_settings.defcontauth_prot,
			      sizeof(link_settings.defcontauth_prot));
		if (err < 0) {
			mosh_error("Failed to read defcontauth password, error: %d", err);
			return err;
		}
		return 0;
	} else if (strcmp(key, LINK_SETT_DNSADDR_ENABLED) == 0) {
		err = read_cb(cb_arg, &link_settings.dnsaddr_enabled,
			      sizeof(link_settings.dnsaddr_enabled));
		if (err < 0) {
			mosh_error("Failed to read dnsaddr enabled, error: %d", err);
			return err;
		}
		return 0;
	} else if (strcmp(key, LINK_SETT_DNSADDR_IP_KEY) == 0) {
		err = read_cb(cb_arg, &link_settings.dnsaddr_ip_str,
			      sizeof(link_settings.dnsaddr_ip_str));
		if (err < 0) {
			mosh_error("Failed to read dnsaddr IP, error: %d", err);
			return err;
		}
		return 0;
	} else if (strcmp(key, LINK_SETT_SYSMODE_KEY) == 0) {
		err = read_cb(cb_arg, &link_settings.sysmode,
			      sizeof(link_settings.sysmode));
		if (err < 0) {
			mosh_error("Failed to read sysmode, error: %d", err);
			return err;
		}
		return 0;
	} else if (strcmp(key, LINK_SETT_SYSMODE_LTE_PREFERENCE_KEY) == 0) {
		err = read_cb(cb_arg, &link_settings.sysmode_lte_preference,
			      sizeof(link_settings.sysmode_lte_preference));
		if (err < 0) {
			mosh_error("Failed to read LTE preference, error: %d", err);
			return err;
		}
		return 0;
	} else if (strcmp(key, LINK_SETT_NORMAL_MODE_AT_CMD_1_KEY) == 0) {
		err = read_cb(cb_arg, &link_settings.normal_mode_at_cmd_str_1,
			      sizeof(link_settings.normal_mode_at_cmd_str_1));
		if (err < 0) {
			mosh_error("Failed to read normal mode at cmd 1, error: %d", err);
			return err;
		}
		return 0;
	} else if (strcmp(key, LINK_SETT_NORMAL_MODE_AT_CMD_2_KEY) == 0) {
		err = read_cb(cb_arg, &link_settings.normal_mode_at_cmd_str_2,
			      sizeof(link_settings.normal_mode_at_cmd_str_2));
		if (err < 0) {
			mosh_error("Failed to read normal mode at cmd 2, error: %d", err);
			return err;
		}
		return 0;
	} else if (strcmp(key, LINK_SETT_NORMAL_MODE_AT_CMD_3_KEY) == 0) {
		err = read_cb(cb_arg, &link_settings.normal_mode_at_cmd_str_3,
			      sizeof(link_settings.normal_mode_at_cmd_str_3));
		if (err < 0) {
			mosh_error("Failed to read normal mode at cmd 3, error: %d", err);
			return err;
		}
		return 0;
	} else if (strcmp(key, LINK_SETT_NORMAL_MODE_AUTOCONN_ENABLED) == 0) {
		err = read_cb(
			cb_arg, &link_settings.normal_mode_autoconn_enabled,
			sizeof(link_settings.normal_mode_autoconn_enabled));
		if (err < 0) {
			mosh_error("Failed to read normal mode autoconnect, error: %d", err);
			return err;
		}
		return 0;
	} else if (strcmp(key, LINK_SETT_NORMAL_MODE_AUTOCONN_REL14_USED) == 0) {
		err = read_cb(
			cb_arg, &link_settings.normal_mode_autoconn_rel14_used,
			sizeof(link_settings.normal_mode_autoconn_rel14_used));
		if (err < 0) {
			mosh_error("Failed to read normal mode autoconnect rel14, error: %d", err);
			return err;
		}
		return 0;
	}

	return 0;
}

/* ****************************************************************************/

int link_sett_save_defcont_enabled(bool enabled)
{
	const char *key = LINK_SETT_KEY "/" LINK_SETT_DEFCONT_ENABLED;
	int err;

	link_settings.defcont_enabled = enabled;
	mosh_print("link defcont %s", ((enabled == true) ? "enabled" : "disabled"));

	err = settings_save_one(key, &link_settings.defcont_enabled,
				sizeof(link_settings.defcont_enabled));
	if (err) {
		mosh_error("%s: err %d from settings_save_one()", __func__, err);
		return err;
	}
	return 0;
}

bool link_sett_is_defcont_enabled(void)
{
	return link_settings.defcont_enabled;
}

enum pdn_fam link_sett_defcont_pdn_family_get(void)
{
	return link_settings.pdn_family;
}

int link_sett_save_defcont_pdn_family(enum pdn_fam family)
{
	int err;
	const char *key = LINK_SETT_KEY "/" LINK_SETT_DEFCONT_IP_FAMILY_KEY;
	char tmp_str[8];

	err = settings_save_one(key, &family, sizeof(enum pdn_fam));

	if (err) {
		mosh_error("Saving of family failed with err %d", err);
		return err;
	}

	link_settings.pdn_family = family;

	mosh_print(
		"Key \"%s\" with value \"%d\" / \"%s\" saved",
		key, family,
		link_pdn_lib_family_to_string(family, tmp_str));

	return 0;
}

char *link_sett_defcont_apn_get(void)
{
	return link_settings.defcont_apn_str;
}

int link_sett_save_defcont_apn(const char *defcont_apn_str)
{
	int err;
	const char *key = LINK_SETT_KEY "/" LINK_SETT_DEFCONT_APN_KEY;
	int len = strlen(defcont_apn_str);

	assert(len <= MOSH_APN_STR_MAX_LEN);

	err = settings_save_one(key, defcont_apn_str, len + 1);
	if (err) {
		mosh_error("%s: err %d from settings_save_one()", (__func__), err);
		return err;
	}
	mosh_print("%s: key %s with value %s saved", (__func__), key, defcont_apn_str);

	strcpy(link_settings.defcont_apn_str, defcont_apn_str);

	return 0;
}

void link_sett_defcont_conf_shell_print(void)
{
	char tmp_str[16];

	mosh_print("link defcont config:");
	mosh_print("  Enabled: %s", link_settings.defcont_enabled ? "true" : "false");
	mosh_print("  APN: %s", link_settings.defcont_apn_str);
	mosh_print(
		"  PDN family: %d (\"%s\")",
		link_settings.pdn_family,
		link_pdn_lib_family_to_string(link_settings.pdn_family, tmp_str));
}

/* ****************************************************************************/

int link_sett_save_defcontauth_enabled(bool enabled)
{
	const char *key = LINK_SETT_KEY "/" LINK_SETT_DEFCONTAUTH_ENABLED;
	int err;

	link_settings.defcontauth_enabled = enabled;
	mosh_print("link defcontauth  %s", ((enabled == true) ? "enabled" : "disabled"));

	err = settings_save_one(key, &link_settings.defcontauth_enabled,
				sizeof(link_settings.defcontauth_enabled));
	if (err) {
		mosh_error("%s: erro %d from settings_save_one()", (__func__), err);
		return err;
	}

	return 0;
}

bool link_sett_is_defcontauth_enabled(void)
{
	return link_settings.defcontauth_enabled;
}

int link_sett_save_defcontauth_username(const char *username_str)
{
	int err;
	const char *key =
		LINK_SETT_KEY "/" LINK_SETT_DEFCONTAUTH_USERNAME_KEY;
	int len = strlen(username_str);

	if (len > LINK_SETT_DEFCONTAUTH_MAX_UNAME_STR_LEN) {
		mosh_error(
			"%s: username length over the limit %d",
			__func__, LINK_SETT_DEFCONTAUTH_MAX_UNAME_STR_LEN);
		return -EINVAL;
	}

	err = settings_save_one(key, username_str, len + 1);
	if (err) {
		mosh_error("Saving of authentication username failed with err %d", err);
		return err;
	}
	mosh_print("Key \"%s\" with value \"%s\" saved", key, username_str);

	strcpy(link_settings.defcontauth_uname_str, username_str);

	return 0;
}

int link_sett_save_defcontauth_password(const char *password_str)
{
	int err;
	const char *key = LINK_SETT_KEY "/" LINK_SETT_DEFCONTAUTH_PASSWORD_KEY;
	int len = strlen(password_str);

	if (len > LINK_SETT_DEFCONTAUTH_MAX_PWORD_STR_LEN) {
		mosh_error(
			"%s: username length over the limit %d",
			__func__, LINK_SETT_DEFCONTAUTH_MAX_PWORD_STR_LEN);
		return -EINVAL;
	}

	err = settings_save_one(key, password_str, len + 1);
	if (err) {
		mosh_error("Saving of authentication password failed with err %d", err);
		return err;
	}
	mosh_print("Key \"%s\" with value \"%s\" saved", key, password_str);

	strcpy(link_settings.defcontauth_pword_str, password_str);

	return 0;
}

int link_sett_save_defcontauth_prot(int auth_prot)
{
	int err;
	const char *key =
		LINK_SETT_KEY "/" LINK_SETT_DEFCONTAUTH_PROTOCOL_KEY;
	enum pdn_auth method;

	err = link_shell_pdn_auth_prot_to_pdn_lib_method_map(auth_prot,
							     &method);
	if (err) {
		mosh_error("Uknown auth protocol %d", auth_prot);
		return -EINVAL;
	}

	err = settings_save_one(key, &auth_prot, sizeof(enum pdn_auth));
	if (err) {
		mosh_error("Saving of authentication protocol failed with err %d", err);
		return err;
	}
	link_settings.defcontauth_prot = method;

	mosh_print("Key \"%s\" with value \"%d\" saved", key, method);

	return 0;
}

enum pdn_auth link_sett_defcontauth_prot_get(void)
{
	int prot = link_settings.defcontauth_prot;

	return prot;
}

char *link_sett_defcontauth_username_get(void)
{
	return link_settings.defcontauth_uname_str;
}

char *link_sett_defcontauth_password_get(void)
{
	return link_settings.defcontauth_pword_str;
}

void link_sett_defcontauth_conf_shell_print(void)
{
	static const char *const prot_type_str[] = { "None", "PAP", "CHAP" };

	mosh_print("link defcontauth config:");
	mosh_print("  Enabled: %s",
			link_settings.defcontauth_enabled ? "true" : "false");
	mosh_print("  Username: %s", link_settings.defcontauth_uname_str);
	mosh_print("  Password: %s", link_settings.defcontauth_pword_str);
	mosh_print("  Authentication protocol: %s",
			prot_type_str[link_settings.defcontauth_prot]);
}

/* ****************************************************************************/

int link_sett_save_dnsaddr_enabled(bool enabled)
{
	const char *key = LINK_SETT_KEY "/" LINK_SETT_DNSADDR_ENABLED;
	int err;

	link_settings.dnsaddr_enabled = enabled;
	mosh_print("link dnsaddr %s", ((enabled == true) ? "enabled" : "disabled"));

	err = settings_save_one(key, &link_settings.dnsaddr_enabled,
				sizeof(link_settings.dnsaddr_enabled));
	if (err) {
		mosh_error("%s: err %d from settings_save_one()", __func__, err);
		return err;
	}
	return 0;
}

bool link_sett_is_dnsaddr_enabled(void)
{
	return link_settings.dnsaddr_enabled;
}

int link_sett_save_dnsaddr_ip(const char *dnsaddr_ip_str)
{
	int err;
	const char *key = LINK_SETT_KEY "/" LINK_SETT_DNSADDR_IP_KEY;
	int len = strlen(dnsaddr_ip_str);

	assert(len <= LINK_SETT_DNSADDR_MAX_IP_LEN);

	err = settings_save_one(key, dnsaddr_ip_str, len + 1);
	if (err) {
		mosh_error("%s: err %d from settings_save_one()", (__func__), err);
		return err;
	}
	mosh_print("%s: key %s with value %s saved", (__func__), key, dnsaddr_ip_str);

	strcpy(link_settings.dnsaddr_ip_str, dnsaddr_ip_str);

	return 0;
}

const char *link_sett_dnsaddr_ip_get(void)
{
	return link_settings.dnsaddr_ip_str;
}

void link_sett_dnsaddr_conf_shell_print(void)
{
	mosh_print("link dnsaddr config:");
	mosh_print("  Enabled: %s", link_settings.dnsaddr_enabled ? "true" : "false");
	mosh_print("  IP address: %s", link_settings.dnsaddr_ip_str);
}

/* ****************************************************************************/

int link_sett_sysmode_save(enum lte_lc_system_mode mode,
			   enum lte_lc_system_mode_preference lte_pref)
{
	const char *key = LINK_SETT_KEY "/" LINK_SETT_SYSMODE_KEY;
	const char *lte_pref_key = LINK_SETT_KEY "/" LINK_SETT_SYSMODE_LTE_PREFERENCE_KEY;
	int err;

	err = settings_save_one(key, &mode, sizeof(mode));
	if (err) {
		mosh_error("link_sett_save_sysmode: erro %d from settings_save_one()", err);
		return err;
	}
	link_settings.sysmode = mode;
	mosh_print("sysmode %d saved successfully to settings", mode);

	err = settings_save_one(lte_pref_key, &lte_pref, sizeof(lte_pref));
	if (err) {
		mosh_error(
			"link_sett_save_sysmode for lte pref: erro %d from settings_save_one()",
			err);
		return err;
	}
	link_settings.sysmode_lte_preference = lte_pref;
	mosh_print("LTE preference %d saved successfully to settings", lte_pref);

	return 0;
}

void link_sett_sysmode_print(void)
{
	char snum[64];

	mosh_print("link sysmode config:");
	mosh_print("  mode: %s", link_shell_sysmode_to_string(link_sett_sysmode_get(), snum));
	mosh_print(
		"  LTE preference: %s",
		link_shell_sysmode_preferred_to_string(
			link_sett_sysmode_lte_preference_get(), snum));
}

int link_sett_sysmode_get(void)
{
	return link_settings.sysmode;
}

int link_sett_sysmode_lte_preference_get(void)
{
	return link_settings.sysmode_lte_preference;
}

int link_sett_sysmode_default_set(void)
{
	return link_sett_sysmode_save(LTE_LC_SYSTEM_MODE_NONE,
				       CONFIG_LTE_MODE_PREFERENCE);
}

/* ****************************************************************************/

char *link_sett_normal_mode_at_cmd_str_get(uint8_t mem_slot)
{
	if (mem_slot == 1) {
		return link_settings.normal_mode_at_cmd_str_1;
	} else if (mem_slot == 2) {
		return link_settings.normal_mode_at_cmd_str_2;
	} else if (mem_slot == 3) {
		return link_settings.normal_mode_at_cmd_str_3;
	}

	mosh_error("%s:unsupported memory slot %d", __func__, mem_slot);
	return NULL;
}

int link_sett_save_normal_mode_at_cmd_str(const char *at_str, uint8_t mem_slot)
{
	int err;
	const char *key;
	int len = strlen(at_str);
	char *at_cmd_ram_storage_ptr;

	if (len > CONFIG_MOSH_LINK_SETT_NORMAL_MODE_AT_CMD_STR_LEN) {
		mosh_error(
			"%s: at command string length (%d) over the limit %d",
			(__func__), len, CONFIG_MOSH_LINK_SETT_NORMAL_MODE_AT_CMD_STR_LEN);
		return -EINVAL;
	}

	if (mem_slot == 1) {
		at_cmd_ram_storage_ptr =
			link_settings.normal_mode_at_cmd_str_1;
		key = LINK_SETT_KEY "/" LINK_SETT_NORMAL_MODE_AT_CMD_1_KEY;
	} else if (mem_slot == 2) {
		at_cmd_ram_storage_ptr =
			link_settings.normal_mode_at_cmd_str_2;
		key = LINK_SETT_KEY "/" LINK_SETT_NORMAL_MODE_AT_CMD_2_KEY;
	} else if (mem_slot == 3) {
		at_cmd_ram_storage_ptr =
			link_settings.normal_mode_at_cmd_str_3;
		key = LINK_SETT_KEY "/" LINK_SETT_NORMAL_MODE_AT_CMD_3_KEY;
	} else {
		mosh_error("%s: unsupported memory slot %d", (__func__), mem_slot);
		return -EINVAL;
	}

	err = settings_save_one(key, at_str, len + 1);
	if (err) {
		mosh_error(
			"Saving of normal mode at cmd %d to settings failed with err %d",
			mem_slot, err);
		return err;
	}

	mosh_print("Key \"%s\" with value \"%s\" saved", key, at_str);

	strcpy(at_cmd_ram_storage_ptr, at_str);
	return 0;
}

int link_sett_clear_normal_mode_at_cmd_str(uint8_t mem_slot)
{
	int err;
	const char *key;
	char *at_cmd_ram_storage_ptr;

	if (mem_slot == 1) {
		at_cmd_ram_storage_ptr =
			link_settings.normal_mode_at_cmd_str_1;
		key = LINK_SETT_KEY "/" LINK_SETT_NORMAL_MODE_AT_CMD_1_KEY;
	} else if (mem_slot == 2) {
		at_cmd_ram_storage_ptr =
			link_settings.normal_mode_at_cmd_str_2;
		key = LINK_SETT_KEY "/" LINK_SETT_NORMAL_MODE_AT_CMD_2_KEY;
	} else if (mem_slot == 3) {
		at_cmd_ram_storage_ptr =
			link_settings.normal_mode_at_cmd_str_3;
		key = LINK_SETT_KEY "/" LINK_SETT_NORMAL_MODE_AT_CMD_3_KEY;
	} else {
		mosh_error(
			"link_sett_delete_normal_mode_at_cmd_str: unsupported memory slot %d",
			mem_slot);
		return -EINVAL;
	}

	err = settings_save_one(key, '\0', 1);
	if (err) {
		mosh_error(
			"Clearing of normal mode at cmd %d to settings failed with err %d",
			mem_slot, err);
		return err;
	}

	mosh_print("Key \"%s\" cleared", key);

	at_cmd_ram_storage_ptr[0] = '\0';

	return 0;
}

void link_sett_normal_mode_at_cmds_shell_print(void)
{
	mosh_print("link normal mode at commands:");
	mosh_print("  Memory slot 1: \"%s\"", link_settings.normal_mode_at_cmd_str_1);
	mosh_print("  Memory slot 2: \"%s\"", link_settings.normal_mode_at_cmd_str_2);
	mosh_print("  Memory slot 3: \"%s\"", link_settings.normal_mode_at_cmd_str_3);
}

/* ****************************************************************************/

int link_sett_save_normal_mode_autoconn_enabled(bool enabled, bool use_rel14)
{
	const char *key_enabled =
		LINK_SETT_KEY "/" LINK_SETT_NORMAL_MODE_AUTOCONN_ENABLED;
	const char *key_rel14 =
		LINK_SETT_KEY "/" LINK_SETT_NORMAL_MODE_AUTOCONN_REL14_USED;
	int err;

	link_settings.normal_mode_autoconn_enabled = enabled;
	link_settings.normal_mode_autoconn_rel14_used = use_rel14;

	mosh_print("link nmodeauto %s %s",
		((enabled == true) ? "enabled" : "disabled"),
		((use_rel14 == true) ? "with REL14" : "without REL14"));

	err = settings_save_one(
		key_enabled, &link_settings.normal_mode_autoconn_enabled,
		sizeof(link_settings.normal_mode_autoconn_enabled));
	if (err) {
		mosh_error("normal_mode_autoconn_enabled: err %d from settings_save_one()", err);
		return err;
	}

	err = settings_save_one(
		key_rel14, &link_settings.normal_mode_autoconn_rel14_used,
		sizeof(link_settings.normal_mode_autoconn_rel14_used));
	if (err) {
		mosh_error("normal_mode_autoconn_rel14_used: err %d from settings_save_one()", err);
		return err;
	}

	return 0;
}

bool link_sett_is_normal_mode_autoconn_enabled(void)
{
	return link_settings.normal_mode_autoconn_enabled;
}

bool link_sett_is_normal_mode_autoconn_rel14_used(void)
{
	return link_settings.normal_mode_autoconn_rel14_used;
}

void link_sett_normal_mode_autoconn_shell_print(void)
{
	mosh_print("link nmodeauto settings:");
	mosh_print(
		"  Autoconnect enabled: %s",
		link_settings.normal_mode_autoconn_enabled ? "true" : "false");
	mosh_print(
		"  Release 14 features enabled: %s",
		link_settings.normal_mode_autoconn_rel14_used ? "true" : "false");

}

/* ****************************************************************************/

static void link_sett_ram_data_init(void)
{
	memset(&link_settings, 0, sizeof(link_settings));

	link_settings.normal_mode_autoconn_enabled = true;
	link_settings.sysmode = LTE_LC_SYSTEM_MODE_NONE;
	link_settings.pdn_family = PDN_FAM_IPV4V6;

	strcpy(link_settings.defcont_apn_str, LINK_SETT_DEFCONT_DEFAULT_APN);
	strcpy(link_settings.defcontauth_uname_str, LINK_SETT_DEFCONTAUTH_DEFAULT_USERNAME);
	strcpy(link_settings.defcontauth_pword_str, LINK_SETT_DEFCONTAUTH_DEFAULT_PASSWORD);
}

/* ****************************************************************************/

void link_sett_all_print(void)
{
	link_sett_sysmode_print();
	link_sett_defcont_conf_shell_print();
	link_sett_defcontauth_conf_shell_print();
	link_sett_dnsaddr_conf_shell_print();
	link_sett_normal_mode_at_cmds_shell_print();
	link_sett_normal_mode_autoconn_shell_print();
}

void link_sett_defaults_set(void)
{
	link_sett_ram_data_init();

	link_sett_save_defcont_enabled(false);
	link_sett_save_defcont_pdn_family(PDN_FAM_IPV4V6);
	link_sett_save_defcont_apn(LINK_SETT_DEFCONT_DEFAULT_APN);

	link_sett_save_defcontauth_enabled(false);
	link_sett_save_defcontauth_username(LINK_SETT_DEFCONTAUTH_DEFAULT_USERNAME);
	link_sett_save_defcontauth_password(LINK_SETT_DEFCONTAUTH_DEFAULT_PASSWORD);
	link_sett_save_defcontauth_prot(PDN_AUTH_NONE);

	link_sett_save_dnsaddr_enabled(false);
	link_sett_save_dnsaddr_ip("");

	link_sett_sysmode_default_set();

	link_sett_clear_normal_mode_at_cmd_str(1);
	link_sett_clear_normal_mode_at_cmd_str(2);
	link_sett_clear_normal_mode_at_cmd_str(3);

	link_sett_save_normal_mode_autoconn_enabled(true, true);

	mosh_print("link settings reseted");
}

void link_sett_modem_factory_reset(enum lte_lc_factory_reset_type type)
{
	int err;

	/* Reset modem factory settings by type */
	err = lte_lc_factory_reset(type);
	if (err) {
		mosh_error("Resetting modem factory settings failed, err %d", err);
	} else {
		mosh_print("link settings: modem factory reset done");
	}
}

static struct settings_handler cfg = { .name = LINK_SETT_KEY,
				       .h_set = link_sett_handler };

int link_sett_init(void)
{
	int err;

	/* Set the defaults: */
	link_sett_ram_data_init();

	err = settings_subsys_init();
	if (err) {
		mosh_error("Failed to initialize settings subsystem, error: %d", err);
		return err;
	}
	err = settings_register(&cfg);
	if (err) {
		mosh_error("Cannot register settings handler %d", err);
		return err;
	}
	err = settings_load();
	if (err) {
		mosh_error("Cannot load settings %d", err);
		return err;
	}
	return 0;
}
