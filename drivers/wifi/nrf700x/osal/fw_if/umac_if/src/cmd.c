/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing command specific definitions for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#include "host_rpu_umac_if.h"
#include "hal_api.h"
#include "fmac_structs.h"

struct host_rpu_msg *umac_cmd_alloc(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				    int type,
				    int len)
{
	struct host_rpu_msg *umac_cmd = NULL;

	umac_cmd = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					    sizeof(*umac_cmd) + len);

	if (!umac_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Failed to allocate UMAC cmd\n",
				      __func__);
		goto out;
	}

	umac_cmd->type = type;
	umac_cmd->hdr.len = sizeof(*umac_cmd) + len;

out:
	return umac_cmd;
}


enum wifi_nrf_status umac_cmd_cfg(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				void *params,
				int len)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;

	if (!fmac_dev_ctx->fw_init_done) {
		struct nrf_wifi_umac_hdr *umac_hdr = NULL;

		umac_hdr = (struct nrf_wifi_umac_hdr *)params;
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: UMAC buff config not yet done(%d)\n",
				      __func__,
				      umac_hdr->cmd_evnt);
		goto out;
	}

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_UMAC,
				  len);

	if (!umac_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: umac_cmd_alloc failed\n",
				      __func__);
		goto out;
	}

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      umac_cmd->msg,
			      params,
			      len);

	status = wifi_nrf_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}


enum wifi_nrf_status umac_cmd_init(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
#ifndef CONFIG_NRF700X_RADIO_TEST
				   unsigned char *rf_params,
				   bool rf_params_valid,
				   struct nrf_wifi_data_config_params *config,
#endif /* !CONFIG_NRF700X_RADIO_TEST */
#ifdef CONFIG_NRF_WIFI_LOW_POWER
				   int sleep_type,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
				   unsigned int phy_calib)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_sys_init *umac_cmd_data = NULL;
	unsigned int len = 0;

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: umac_cmd_alloc failed\n",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_sys_init *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_INIT;
	umac_cmd_data->sys_head.len = len;

#ifndef CONFIG_NRF700X_RADIO_TEST
	umac_cmd_data->sys_params.rf_params_valid = rf_params_valid;

	if (rf_params_valid) {
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      umac_cmd_data->sys_params.rf_params,
				      rf_params,
				      NRF_WIFI_RF_PARAMS_SIZE);
	}
#endif /* !CONFIG_NRF700X_RADIO_TEST */

	umac_cmd_data->sys_params.phy_calib = phy_calib;
	umac_cmd_data->sys_params.hw_bringup_time = HW_DELAY;
	umac_cmd_data->sys_params.sw_bringup_time = SW_DELAY;
	umac_cmd_data->sys_params.bcn_time_out = BCN_TIMEOUT;
	umac_cmd_data->sys_params.calib_sleep_clk = CALIB_SLEEP_CLOCK_ENABLE;
#ifdef CONFIG_NRF_WIFI_LOW_POWER
	umac_cmd_data->sys_params.sleep_enable = sleep_type;
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
#ifdef CONFIG_NRF700X_TCP_IP_CHECKSUM_OFFLOAD
	umac_cmd_data->sys_params.tcp_ip_checksum_offload = 1;
#endif /* CONFIG_NRF700X_TCP_IP_CHECKSUM_OFFLOAD */
	wifi_nrf_osal_log_info(fmac_dev_ctx->fpriv->opriv, "RPU LPM type: %s\n",
		umac_cmd_data->sys_params.sleep_enable == 2 ? "HW" :
		umac_cmd_data->sys_params.sleep_enable == 1 ? "SW" : "DISABLED");
#ifndef CONFIG_NRF700X_RADIO_TEST
	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      umac_cmd_data->rx_buf_pools,
			      fmac_dev_ctx->fpriv->rx_buf_pools,
			      sizeof(umac_cmd_data->rx_buf_pools));

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &umac_cmd_data->data_config_params,
			      config,
			      sizeof(umac_cmd_data->data_config_params));

	umac_cmd_data->temp_vbat_config_params.temp_based_calib_en = NRF_WIFI_TEMP_CALIB_ENABLE;
	umac_cmd_data->temp_vbat_config_params.temp_calib_bitmap = NRF_WIFI_DEF_PHY_TEMP_CALIB;
	umac_cmd_data->temp_vbat_config_params.vbat_calibp_bitmap = NRF_WIFI_DEF_PHY_VBAT_CALIB;
	umac_cmd_data->temp_vbat_config_params.temp_vbat_mon_period = NRF_WIFI_TEMP_CALIB_PERIOD;
	umac_cmd_data->temp_vbat_config_params.vth_low = NRF_WIFI_VBAT_LOW;
	umac_cmd_data->temp_vbat_config_params.vth_hi = NRF_WIFI_VBAT_HIGH;
	umac_cmd_data->temp_vbat_config_params.temp_threshold = NRF_WIFI_TEMP_CALIB_THRESHOLD;
	umac_cmd_data->temp_vbat_config_params.vth_very_low = NRF_WIFI_VBAT_VERYLOW;
#endif /* !CONFIG_NRF700X_RADIO_TEST */

	status = wifi_nrf_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}


enum wifi_nrf_status umac_cmd_deinit(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_sys_deinit *umac_cmd_data = NULL;
	unsigned int len = 0;

	len = sizeof(*umac_cmd_data);
	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);
	if (!umac_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: umac_cmd_alloc failed\n",
				      __func__);
		goto out;
	}
	umac_cmd_data = (struct nrf_wifi_cmd_sys_deinit *)(umac_cmd->msg);
	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_DEINIT;
	umac_cmd_data->sys_head.len = len;
	status = wifi_nrf_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));
out:
	return status;
}


enum wifi_nrf_status umac_cmd_btcoex(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				void *cmd, unsigned int cmd_len)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_coex_config *umac_cmd_data = NULL;
	int len = 0;

	len = sizeof(*umac_cmd_data)+cmd_len;

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: umac_cmd_alloc failed\n",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_coex_config *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_BTCOEX;
	umac_cmd_data->sys_head.len = len;
	umac_cmd_data->coex_config_info.len = cmd_len;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      umac_cmd_data->coex_config_info.coex_cmd,
			      cmd,
			      cmd_len);

	status = wifi_nrf_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));
out:
	return status;


}

enum wifi_nrf_status umac_cmd_he_ltf_gi(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					unsigned char he_ltf,
					unsigned char he_gi,
					unsigned char enabled)
{
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_he_gi_ltf_config *umac_cmd_data = NULL;
	int len = 0;
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: umac_cmd_alloc failed\n",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_he_gi_ltf_config *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_HE_GI_LTF_CONFIG;
	umac_cmd_data->sys_head.len = len;

	if (enabled) {
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      &umac_cmd_data->he_ltf,
				      &he_ltf,
				      sizeof(he_ltf));
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      &umac_cmd_data->he_gi_type,
				      &he_gi,
				      sizeof(he_gi));
	}

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &umac_cmd_data->enable,
			      &enabled,
			      sizeof(enabled));

	status = wifi_nrf_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));
out:
	return status;
}

#ifdef CONFIG_NRF700X_RADIO_TEST
enum wifi_nrf_status umac_cmd_prog_init(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					struct nrf_wifi_radio_test_init_info *init_params)
{
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_radio_test_init *umac_cmd_data = NULL;
	int len = 0;
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: umac_cmd_alloc failed\n",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_radio_test_init *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_RADIO_TEST_INIT;
	umac_cmd_data->sys_head.len = len;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &umac_cmd_data->conf,
			      init_params,
			      sizeof(umac_cmd_data->conf));

	status = wifi_nrf_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));
out:
	return status;
}


enum wifi_nrf_status umac_cmd_prog_tx(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				      struct rpu_conf_params *params)
{
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_mode_params *umac_cmd_data = NULL;
	int len = 0;
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: umac_cmd_alloc failed\n",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_mode_params *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_TX;
	umac_cmd_data->sys_head.len = len;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &umac_cmd_data->conf,
			      params,
			      sizeof(umac_cmd_data->conf));

	status = wifi_nrf_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}


enum wifi_nrf_status umac_cmd_prog_rx(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				      struct rpu_conf_rx_radio_test_params *rx_params)
{
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_rx *umac_cmd_data = NULL;
	int len = 0;
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: umac_cmd_alloc failed\n",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_rx *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_RX;
	umac_cmd_data->sys_head.len = len;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &umac_cmd_data->conf,
			      rx_params,
			      sizeof(umac_cmd_data->conf));

	status = wifi_nrf_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}


enum wifi_nrf_status umac_cmd_prog_rf_test(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					   void *rf_test_params,
					   unsigned int rf_test_params_sz)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_rftest *umac_cmd_data = NULL;
	int len = 0;

	len = (sizeof(*umac_cmd_data) + rf_test_params_sz);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: umac_cmd_alloc failed\n",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_rftest *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_RF_TEST;
	umac_cmd_data->sys_head.len = len;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      (void *)umac_cmd_data->rf_test_info.rfcmd,
			      rf_test_params,
			      rf_test_params_sz);

	umac_cmd_data->rf_test_info.len = rf_test_params_sz;

	status = wifi_nrf_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}
#endif /* CONFIG_NRF700X_RADIO_TEST */


enum wifi_nrf_status umac_cmd_prog_stats_get(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
#ifdef CONFIG_NRF700X_RADIO_TEST
					     int op_mode,
#endif /* CONFIG_NRF700X_RADIO_TEST */
					     int stats_type)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_get_stats *umac_cmd_data = NULL;
	int len = 0;

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: umac_cmd_alloc failed\n",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_get_stats *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_GET_STATS;
	umac_cmd_data->sys_head.len = len;
	umac_cmd_data->stats_type = stats_type;
#ifdef CONFIG_NRF700X_RADIO_TEST
	umac_cmd_data->op_mode = op_mode;
#endif /* CONFIG_NRF700X_RADIO_TEST */

	status = wifi_nrf_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}
