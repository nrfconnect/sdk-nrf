/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* @file
 * @brief NRF Wi-Fi util shell module
 */
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <zephyr/kernel.h>
#include <common/fw_if/nrf71_wifi_ctrl.h>
#include <common/util.h>
#include <common/wifi_ipc.h>
#include <system/fmac_api.h>
#include <system/core.h>
#include <system/wifi_util.h>
#include <common/mac_addr.h>


extern struct nrf_wifi_drv_priv_zep rpu_drv_priv_zep;
struct nrf_wifi_ctx_zep *ctx = &rpu_drv_priv_zep.rpu_ctx_zep;

static bool check_valid_data_rate(const struct shell *sh,
				  unsigned char rate_flag,
				  unsigned int data_rate)
{
	bool ret = false;

	switch (rate_flag) {
	case RPU_TPUT_MODE_LEGACY:
		if ((data_rate == 1) ||
		    (data_rate == 2) ||
		    (data_rate == 55) ||
		    (data_rate == 11) ||
		    (data_rate == 6) ||
		    (data_rate == 9) ||
		    (data_rate == 12) ||
		    (data_rate == 18) ||
		    (data_rate == 24) ||
		    (data_rate == 36) ||
		    (data_rate == 48) ||
		    (data_rate == 54)) {
			ret = true;
		}
		break;
	case RPU_TPUT_MODE_HT:
	case RPU_TPUT_MODE_HE_SU:
	case RPU_TPUT_MODE_VHT:
		if ((data_rate >= 0) && (data_rate <= 7)) {
			ret = true;
		}
		break;
	case RPU_TPUT_MODE_HE_ER_SU:
		if (data_rate >= 0 && data_rate <= 2) {
			ret = true;
		}
		break;
	default:
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "%s: Invalid rate_flag %d\n",
			      __func__,
			      rate_flag);
		break;
	}

	return ret;
}


int nrf_wifi_util_conf_init(struct rpu_conf_params *conf_params)
{
	if (!conf_params) {
		return -ENOEXEC;
	}

	memset(conf_params, 0, sizeof(*conf_params));

	/* Initialize values which are other than 0 */
	conf_params->he_ltf = -1;
	conf_params->he_gi = -1;
	return 0;
}


static int nrf_wifi_util_set_he_ltf(const struct shell *sh,
				    size_t argc,
				    const char *argv[])
{
	char *ptr = NULL;
	unsigned long he_ltf = 0;

	if (ctx->conf_params.set_he_ltf_gi) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Disable 'set_he_ltf_gi', to set 'he_ltf'\n");
		return -ENOEXEC;
	}

	he_ltf = strtoul(argv[1], &ptr, 10);

	if (he_ltf > 2) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid HE LTF value(%lu).\n",
			      he_ltf);
		shell_help(sh);
		return -ENOEXEC;
	}

	ctx->conf_params.he_ltf = he_ltf;

	return 0;
}


static int nrf_wifi_util_set_he_gi(const struct shell *sh,
				   size_t argc,
				   const char *argv[])
{
	char *ptr = NULL;
	unsigned long he_gi = 0;

	if (ctx->conf_params.set_he_ltf_gi) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Disable 'set_he_ltf_gi', to set 'he_gi'\n");
		return -ENOEXEC;
	}

	he_gi = strtoul(argv[1], &ptr, 10);

	if (he_gi > 2) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid HE GI value(%lu).\n",
			      he_gi);
		shell_help(sh);
		return -ENOEXEC;
	}

	ctx->conf_params.he_gi = he_gi;

	return 0;
}


static int nrf_wifi_util_set_he_ltf_gi(const struct shell *sh,
				       size_t argc,
				       const char *argv[])
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	char *ptr = NULL;
	unsigned long val = 0;

	val = strtoul(argv[1], &ptr, 10);

	if ((val < 0) || (val > 1)) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid value(%lu).\n",
			      val);
		shell_help(sh);
		return -ENOEXEC;
	}

	status = nrf_wifi_sys_fmac_conf_ltf_gi(ctx->rpu_ctx,
					       ctx->conf_params.he_ltf,
					       ctx->conf_params.he_gi,
					       val);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Programming ltf_gi failed\n");
		return -ENOEXEC;
	}

	ctx->conf_params.set_he_ltf_gi = val;

	return 0;
}

#ifdef CONFIG_NRF71_STA_MODE
static int nrf_wifi_util_set_uapsd_queue(const struct shell *sh,
					 size_t argc,
					 const char *argv[])
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	char *ptr = NULL;
	unsigned long val = 0;

	val = strtoul(argv[1], &ptr, 10);

	if ((val < UAPSD_Q_MIN) || (val > UAPSD_Q_MAX)) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid value(%lu).\n",
			      val);
		shell_help(sh);
		return -ENOEXEC;
	}

	if (ctx->conf_params.uapsd_queue != val) {
		status = nrf_wifi_sys_fmac_set_uapsd_queue(ctx->rpu_ctx,
						       0,
						       val);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			shell_fprintf(sh,
				      SHELL_ERROR,
				      "Programming uapsd_queue failed\n");
			return -ENOEXEC;
		}

		ctx->conf_params.uapsd_queue = val;
	}

	return 0;
}
#endif /* CONFIG_NRF71_STA_MODE */


static int nrf_wifi_util_show_cfg(const struct shell *sh,
				  size_t argc,
				  const char *argv[])
{
	struct rpu_conf_params *conf_params = NULL;

	conf_params = &ctx->conf_params;

	shell_fprintf(sh,
		      SHELL_INFO,
		      "************* Configured Parameters ***********\n");
	shell_fprintf(sh,
		      SHELL_INFO,
		      "\n");

	shell_fprintf(sh,
		      SHELL_INFO,
		      "he_ltf = %d\n",
		      conf_params->he_ltf);

	shell_fprintf(sh,
		      SHELL_INFO,
		      "he_gi = %u\n",
		      conf_params->he_gi);

	shell_fprintf(sh,
		      SHELL_INFO,
		      "set_he_ltf_gi = %d\n",
		      conf_params->set_he_ltf_gi);

	shell_fprintf(sh,
		      SHELL_INFO,
		      "uapsd_queue = %d\n",
		      conf_params->uapsd_queue);

	shell_fprintf(sh,
		      SHELL_INFO,
		      "rate_flag = %d,  rate_val = %d\n",
		      ctx->conf_params.tx_pkt_tput_mode,
		      ctx->conf_params.tx_pkt_rate);

	shell_fprintf(sh,
		      SHELL_INFO,
		      "extended_sleep_sec = %u seconds\n",
		      ctx->extended_sleep_sec);
	return 0;
}

static int nrf_wifi_util_tx_rate(const struct shell *sh,
				 size_t argc,
				 const char *argv[])
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	char *ptr = NULL;
	long rate_flag = -1;
	long data_rate = -1;

	rate_flag = strtol(argv[1], &ptr, 10);

	if (rate_flag >= RPU_TPUT_MODE_MAX) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid value %ld for rate_flags\n",
			      rate_flag);
		shell_help(sh);
		return -ENOEXEC;
	}


	if (rate_flag == RPU_TPUT_MODE_HE_TB) {
		data_rate = -1;
	} else {
		if (argc < 3) {
			shell_fprintf(sh,
				      SHELL_ERROR,
				      "rate_val needed for rate_flag = %ld\n",
				      rate_flag);
			shell_help(sh);
			return -ENOEXEC;
		}

		data_rate = strtol(argv[2], &ptr, 10);

		if (!(check_valid_data_rate(sh,
					    rate_flag,
					    data_rate))) {
			shell_fprintf(sh,
				      SHELL_ERROR,
				      "Invalid data_rate %ld for rate_flag %ld\n",
				      data_rate,
				      rate_flag);
			return -ENOEXEC;
		}

	}

	status = nrf_wifi_sys_fmac_set_tx_rate(ctx->rpu_ctx,
					       rate_flag,
					       data_rate);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Programming tx_rate failed\n");
		return -ENOEXEC;
	}

	ctx->conf_params.tx_pkt_tput_mode = rate_flag;
	ctx->conf_params.tx_pkt_rate = data_rate;

	return 0;
}


static int nrf_wifi_util_show_vers(const struct shell *sh,
				  size_t argc,
				  const char *argv[])
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	unsigned int fw_ver;

	fmac_dev_ctx = ctx->rpu_ctx;

	shell_fprintf(sh, SHELL_INFO, "Driver version: %s\n",
				  NRF71_DRIVER_VERSION);

	status = nrf_wifi_fmac_ver_get(fmac_dev_ctx, &fw_ver);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		shell_fprintf(sh,
		 SHELL_INFO,
		 "Failed to get firmware version\n");
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_INFO,
		 "Firmware version: %d.%d.%d.%d\n",
		  NRF_WIFI_UMAC_VER(fw_ver),
		  NRF_WIFI_UMAC_VER_MAJ(fw_ver),
		  NRF_WIFI_UMAC_VER_MIN(fw_ver),
		  NRF_WIFI_UMAC_VER_EXTRA(fw_ver));

	return status;
}

#ifdef CONFIG_NRF_WIFI_RPU_RECOVERY
static int nrf_wifi_util_trigger_rpu_recovery(const struct shell *sh,
					      size_t argc,
					      const char *argv[])
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	int ret;

	k_mutex_lock(&ctx->rpu_lock, K_FOREVER);
	if (!ctx || !ctx->rpu_ctx) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "RPU context not initialized\n");
		ret = -ENOEXEC;
		goto unlock;
	}

	fmac_dev_ctx = ctx->rpu_ctx;

	status = nrf_wifi_sys_fmac_rpu_recovery_callback(fmac_dev_ctx, NULL, 0);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Failed to trigger RPU recovery\n");
		return -ENOEXEC;
	}

	shell_fprintf(sh,
		      SHELL_INFO,
		      "RPU recovery triggered\n");

	ret = 0;
unlock:
	k_mutex_unlock(&ctx->rpu_lock);
	return ret;
}

static int nrf_wifi_util_rpu_recovery_info(const struct shell *sh,
					   size_t argc,
					   const char *argv[])
{
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	unsigned long current_time_ms = k_uptime_get();
	int ret;

	k_mutex_lock(&ctx->rpu_lock, K_FOREVER);
	if (!ctx || !ctx->rpu_ctx) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "RPU context not initialized\n");
		ret = -ENOEXEC;
		goto unlock;
	}

	fmac_dev_ctx = ctx->rpu_ctx;
	if (!fmac_dev_ctx) {
		shell_fprintf(sh, SHELL_ERROR, "FMAC context not initialized\n");
		ret = -ENOEXEC;
		goto unlock;
	}

	if (!nrf_wifi_ipc_is_open(fmac_dev_ctx)) {
		shell_fprintf(sh, SHELL_ERROR, "Host-RPU transport not open\n");
		ret = -ENOEXEC;
		goto unlock;
	}

	shell_fprintf(sh, SHELL_INFO,
		      "wdt_irq_received: %d\n"
		      "wdt_irq_ignored: %d\n"
		      "last_wakeup_now_asserted_time_ms: %lu milliseconds\n"
		      "last_wakeup_now_deasserted_time_ms: %lu milliseconds\n"
		      "last_rpu_sleep_opp_time_ms: %lu milliseconds\n"
		      "current time: %lu milliseconds\n"
		      "rpu_recovery_success: %d\n"
		      "rpu_recovery_failure: %d\n\n",
		      ctx->wdt_irq_received, ctx->wdt_irq_ignored,
		      fmac_dev_ctx->wakeup_now_asserted_time_prev_ms,
		      fmac_dev_ctx->wakeup_now_deasserted_time_prev_ms,
		      fmac_dev_ctx->rpu_sleep_opp_time_prev_ms, current_time_ms,
		      ctx->rpu_recovery_success, ctx->rpu_recovery_failure);

	ret = 0;
unlock:
	k_mutex_unlock(&ctx->rpu_lock);
	return ret;
}
#endif /* CONFIG_NRF_WIFI_RPU_RECOVERY */

static void nrf_wifi_util_dump_mac_addr_slots(const struct shell *sh, bool uicr)
{
	const char *block = uicr ? "UICR" : "FICR";

	for (unsigned char slot = 0; slot < NRF_WIFI_XICR_MAC_ADDR_SLOTS; slot++) {
		uint32_t low, high;
		int ret;

		ret = nrf_wifi_xicr_mac_addr_slot_read(uicr, slot, &low, &high);
		if (ret == -ENOTSUP) {
			shell_print(sh, "%s: not accessible", block);
			return;
		} else if (ret) {
			shell_error(sh, "%s MACADDR[%u]: read failed (err %d)",
				    block, slot, ret);
			continue;
		}

		shell_print(sh, "%s MACADDR[%u].LOW  = 0x%08X", block, slot, low);
		shell_print(sh, "%s MACADDR[%u].HIGH = 0x%08X", block, slot, high);

		if (nrf_wifi_mac_addr_regs_empty(low, high)) {
			shell_print(sh, "%s MACADDR[%u]      = not programmed",
				    block, slot);
			continue;
		}

		/* HIGH holds bits [47:24] of the address, LOW bits [23:0], both
		 * MSB first, so 00F4CE36 / 00112233 is F4:CE:36:11:22:33.
		 */
		shell_print(sh, "%s MACADDR[%u]      = %02X:%02X:%02X:%02X:%02X:%02X",
			    block, slot,
			    (uint8_t)(high >> 16), (uint8_t)(high >> 8), (uint8_t)high,
			    (uint8_t)(low >> 16), (uint8_t)(low >> 8), (uint8_t)low);
	}
}

static int nrf_wifi_util_mac_addr(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	nrf_wifi_util_dump_mac_addr_slots(sh, true);
	nrf_wifi_util_dump_mac_addr_slots(sh, false);

	for (unsigned char vif_idx = 0; vif_idx < MAX_NUM_VIFS; vif_idx++) {
		uint8_t mac_addr[WIFI_MAC_ADDR_LEN];
		int ret;

		ret = nrf_wifi_xicr_mac_addr_get(vif_idx, mac_addr, NULL);
		if (ret) {
			shell_error(sh, "VIF%u: no MAC address available (err %d)",
				    vif_idx, ret);
			continue;
		}

		shell_print(sh, "VIF%u resolved    = %02X:%02X:%02X:%02X:%02X:%02X",
			    vif_idx,
			    mac_addr[0], mac_addr[1], mac_addr[2],
			    mac_addr[3], mac_addr[4], mac_addr[5]);
	}

	return 0;
}

static int nrf_wifi_util_req_extended_sleep(const struct shell *sh,
					    size_t argc,
					    const char *argv[])
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	char *ptr = NULL;
	unsigned long long val = 0;
	int ret = 0;

	if (argv[1][0] == '-') {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid value(%s).\n",
			      argv[1]);
		shell_help(sh);
		return -ENOEXEC;
	}

	val = strtoull(argv[1], &ptr, 10);

	if ((ptr == argv[1]) || (*ptr != '\0') || (val > UINT_MAX)) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid value(%s).\n",
			      argv[1]);
		shell_help(sh);
		return -ENOEXEC;
	}

	k_mutex_lock(&ctx->rpu_lock, K_FOREVER);
	if (!ctx->rpu_ctx) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "RPU context not initialized\n");
		ret = -ENOEXEC;
		goto unlock_sleep;
	}

	status = nrf_wifi_fmac_req_extended_sleep(ctx->rpu_ctx,
						  0,
						  (unsigned int)val);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Programming extended_sleep failed\n");
		ret = -ENOEXEC;
		goto unlock_sleep;
	}

	ctx->extended_sleep_sec = (unsigned int)val;

	shell_fprintf(sh,
		      SHELL_INFO,
		      "Requested extended sleep of %u seconds\n",
		      (unsigned int)val);

unlock_sleep:
	k_mutex_unlock(&ctx->rpu_lock);
	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	nrf71_util,
	SHELL_CMD_ARG(he_ltf,
		      NULL,
		      "0 - 1x HE LTF\n"
		      "1 - 2x HE LTF\n"
		      "2 - 4x HE LTF                                        ",
		      nrf_wifi_util_set_he_ltf,
		      2,
		      0),
	SHELL_CMD_ARG(he_gi,
		      NULL,
		      "0 - 0.8 us\n"
		      "1 - 1.6 us\n"
		      "2 - 3.2 us                                           ",
		      nrf_wifi_util_set_he_gi,
		      2,
		      0),
	SHELL_CMD_ARG(set_he_ltf_gi,
		      NULL,
		      "0 - Disable\n"
		      "1 - Enable",
		      nrf_wifi_util_set_he_ltf_gi,
		      2,
		      0),
#ifdef CONFIG_NRF71_STA_MODE
	SHELL_CMD_ARG(uapsd_queue,
		      NULL,
		      "<val> - 0 to 15",
		      nrf_wifi_util_set_uapsd_queue,
		      2,
		      0),
#endif /* CONFIG_NRF71_STA_MODE */
	SHELL_CMD_ARG(show_config,
		      NULL,
		      "Display the current configuration values",
		      nrf_wifi_util_show_cfg,
		      1,
		      0),
	SHELL_CMD_ARG(tx_rate,
		      NULL,
		      "Sets TX data rate to either a fixed value or AUTO\n"
		      "Parameters:\n"
		      "    <rate_flag> : The TX data rate type to be set, where:\n"
		      "        0 - LEGACY\n"
		      "        1 - HT\n"
		      "        2 - VHT\n"
		      "        3 - HE_SU\n"
		      "        4 - HE_ER_SU\n"
		      "        5 - AUTO\n"
		      "    <rate_val> : The TX data rate value to be set, valid values are:\n"
		      "        Legacy : <1, 2, 55, 11, 6, 9, 12, 18, 24, 36, 48, 54>\n"
		      "        Non-legacy: <MCS index value between 0 - 7>\n"
		      "        AUTO: <No value needed>\n",
		      nrf_wifi_util_tx_rate,
		      2,
		      1),
	SHELL_CMD_ARG(show_vers,
		      NULL,
		      "Display the driver and the firmware versions",
		      nrf_wifi_util_show_vers,
		      1,
		      0),
#ifdef CONFIG_NRF_WIFI_RPU_RECOVERY
	SHELL_CMD_ARG(rpu_recovery_test,
		      NULL,
		      "Trigger RPU recovery",
		      nrf_wifi_util_trigger_rpu_recovery,
		      1,
		      0),
	SHELL_CMD_ARG(rpu_recovery_info,
		      NULL,
		      "Dump RPU recovery information",
		      nrf_wifi_util_rpu_recovery_info,
		      1,
		      0),
#endif /* CONFIG_NRF_WIFI_RPU_RECOVERY */
	SHELL_CMD_ARG(extended_sleep,
		      NULL,
		      "<duration_sec> - Extended sleep interval in seconds.\n"
		      "During this interval the nRF71 remains in deep sleep without\n"
		      "waking for DTIM beacons. Inbound and outbound traffic will be lost.",
		      nrf_wifi_util_req_extended_sleep,
		      2,
		      0),
	SHELL_CMD_ARG(mac_addr,
		      NULL,
		      "Dump the MAC addresses programmed in the xICR registers\n"
		      "and the address each interface resolves to",
		      nrf_wifi_util_mac_addr,
		      1,
		      0),
	SHELL_SUBCMD_SET_END);


SHELL_SUBCMD_ADD((nrf71), util, &nrf71_util, "nRF71 utility commands\n", NULL, 0, 0);


static int nrf_wifi_util_init(void)
{

	if (nrf_wifi_util_conf_init(&ctx->conf_params) < 0) {
		return -1;
	}

	return 0;
}


SYS_INIT(nrf_wifi_util_init,
	 APPLICATION,
	 CONFIG_APPLICATION_INIT_PRIORITY);
