/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* @file
 * @brief NRF Wi-Fi radio test shell module
 */

#include <zephyr_fmac_main.h>
#include <nrf_wifi_radio_test_shell.h>
#include <util.h>

extern struct wifi_nrf_drv_priv_zep rpu_drv_priv_zep;
struct wifi_nrf_ctx_zep *ctx = &rpu_drv_priv_zep.rpu_ctx_zep;


static bool check_test_in_prog(const struct shell *shell)
{
	if (ctx->conf_params.rx) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Disable RX\n");
		return false;
	}

	if (ctx->conf_params.tx) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Disable TX\n");
		return false;
	}

	if (ctx->rf_test_run) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Disable RF Test (%d)\n",
			      ctx->rf_test);
		return false;
	}
	return true;
}


static bool check_valid_data_rate(const struct shell *shell,
				  unsigned char tput_mode,
				  unsigned int nss,
				  int dr)
{
	bool is_mcs = dr & 0x80;
	bool ret = false;

	if (dr == -1)
		return true;

	if (is_mcs) {
		dr = dr & 0x7F;

		if (tput_mode & RPU_TPUT_MODE_HT) {
			if (nss == 2) {
				if ((dr >= 8) && (dr <= 15)) {
					ret = true;
				} else {
					shell_fprintf(shell,
						      SHELL_ERROR,
						      "Invalid MIMO HT MCS: %d\n",
						       dr);
				}
			}

			if (nss == 1) {
				if ((dr >= 0) && (dr <= 7)) {
					ret = true;
				} else {
					shell_fprintf(shell,
						      SHELL_ERROR,
						      "Invalid SISO HT MCS: %d\n",
						      dr);
				}
			}

		} else if (tput_mode == RPU_TPUT_MODE_VHT) {
			if ((dr >= 0 && dr <= 9)) {
				ret = true;
			} else {
				shell_fprintf(shell,
					      SHELL_ERROR,
					      "Invalid VHT MCS value: %d\n",
					      dr);
			}
		} else if (tput_mode == RPU_TPUT_MODE_HE_SU) {
			if ((dr >= 0 && dr <= 7)) {
				ret = true;
			} else {
				shell_fprintf(shell,
					      SHELL_ERROR,
					      "Invalid HE_SU MCS value: %d\n",
					      dr);
			}
		} else if (tput_mode == RPU_TPUT_MODE_HE_ER_SU) {
			if ((dr >= 0 && dr <= 2)) {
				ret = true;
			} else {
				shell_fprintf(shell,
					      SHELL_ERROR,
					      "Invalid HE_ER_SU MCS value: %d\n",
					      dr);
			}
		} else {
			shell_fprintf(shell,
				      SHELL_ERROR,
				      "%s: Invalid throughput mode: %d\n", __func__,
				      dr);
		}
	} else {
		if (tput_mode != RPU_TPUT_MODE_LEGACY) {
			ret = false;

			shell_fprintf(shell,
				      SHELL_ERROR,
				      "Invalid rate_flags for legacy: %d\n",
				      dr);
		}

		if ((dr == 1) ||
		    (dr == 2) ||
		    (dr == 55) ||
		    (dr == 11) ||
		    (dr == 6) ||
		    (dr == 9) ||
		    (dr == 12) ||
		    (dr == 18) ||
		    (dr == 24) ||
		    (dr == 36) ||
		    (dr == 48) ||
		    (dr == 54) ||
		    (dr == -1)) {
			ret = true;
		} else {
			shell_fprintf(shell,
				      SHELL_ERROR,
				      "Invalid Legacy Rate value: %d\n",
				      dr);
		}
	}

	return ret;
}


static bool check_valid_channel(unsigned char chan_num)
{
	if (((chan_num >= 1) && (chan_num <= 14)) ||
		(chan_num == 32) ||
		(chan_num == 36) ||
		(chan_num == 40) ||
		(chan_num == 44) ||
		(chan_num == 48) ||
		(chan_num == 52) ||
		(chan_num == 56) ||
		(chan_num == 60) ||
		(chan_num == 64) ||
		(chan_num == 68) ||
		(chan_num == 96) ||
		(chan_num == 100) ||
		(chan_num == 104) ||
		(chan_num == 108) ||
		(chan_num == 112) ||
		(chan_num == 116) ||
		(chan_num == 120) ||
		(chan_num == 124) ||
		(chan_num == 128) ||
		(chan_num == 132) ||
		(chan_num == 136) ||
		(chan_num == 140) ||
		(chan_num == 144) ||
		(chan_num == 149) ||
		(chan_num == 153) ||
		(chan_num == 157) ||
		(chan_num == 159) ||
		(chan_num == 161) ||
		(chan_num == 163) ||
		(chan_num == 165) ||
		(chan_num == 167) ||
		(chan_num == 169) ||
		(chan_num == 171) ||
		(chan_num == 173) ||
		(chan_num == 175) ||
		(chan_num == 177)) {
		return true;
	}

	return false;
}


static int check_channel_settings(const struct shell *shell,
				  unsigned char tput_mode,
				  struct chan_params *chan)
{
	if (tput_mode == RPU_TPUT_MODE_LEGACY) {
		if (chan->bw != RPU_CH_BW_20) {
			shell_fprintf(shell,
				      SHELL_ERROR,
				      "Invalid bandwidth setting for legacy channel = %d\n",
				      chan->primary_num);
			return -1;
		}
	} else if (tput_mode == RPU_TPUT_MODE_HT) {
		if (chan->bw != RPU_CH_BW_20) {
			shell_fprintf(shell,
				      SHELL_ERROR,
				      "Invalid bandwidth setting for HT channel = %d\n",
				      chan->primary_num);
			return -1;
		}
	} else if (tput_mode == RPU_TPUT_MODE_VHT) {
		if ((chan->primary_num >= 1) &&
		    (chan->primary_num <= 14)) {
			shell_fprintf(shell,
				      SHELL_ERROR,
				      "VHT setting not allowed in 2.4GHz band\n");
			return -1;
		}

		if (chan->bw != RPU_CH_BW_20) {
			shell_fprintf(shell,
				      SHELL_ERROR,
				      "Invalid bandwidth setting for VHT channel = %d\n",
				      chan->primary_num);
			return -1;
		}
	} else if ((tput_mode == RPU_TPUT_MODE_HE_SU) ||
#ifndef CONFIG_NRF700X_REV_A
		   (tput_mode == RPU_TPUT_MODE_HE_TB) ||
#endif /* !CONFIG_NRF700X_REV_A */
		   (tput_mode == RPU_TPUT_MODE_HE_ER_SU)) {
		if (chan->bw != RPU_CH_BW_20) {
			shell_fprintf(shell,
				      SHELL_ERROR,
				      "Invalid bandwidth setting for HE channel = %d\n",
				      chan->primary_num);
			return -1;
		}
	} else {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid throughput mode = %d\n",
			      tput_mode);
		return -1;
	}

	return 0;
}


void nrf_wifi_radio_test_conf_init(struct rpu_conf_params *conf_params)
{
	memset(conf_params,
	       0,
	       sizeof(*conf_params));

	/* Initialize values which are other than 0 */
	conf_params->op_mode = RPU_OP_MODE_RADIO_TEST;

	memset(conf_params->rf_params,
	       0xFF,
	       sizeof(conf_params->rf_params));

	nrf_wifi_utils_hex_str_to_val(rpu_drv_priv_zep.fmac_priv->opriv,
				      conf_params->rf_params,
				      sizeof(conf_params->rf_params),
				      NRF_WIFI_DEF_RF_PARAMS);

	conf_params->tx_pkt_nss = 1;
	conf_params->tx_pkt_mcs = -1;
	conf_params->tx_pkt_rate = -1;
	conf_params->tx_pkt_gap_us = 200;

	conf_params->chan.primary_num = 1;
	conf_params->tx_mode = 1;
	conf_params->tx_pkt_num = -1;
	conf_params->tx_pkt_len = 1400;
	conf_params->tx_pkt_tput_mode = RPU_TPUT_MODE_MAX;
	conf_params->aux_adc_input_chain_id = 1;
	conf_params->set_he_ltf_gi = 0;
	conf_params->phy_calib = NRF_WIFI_DEF_PHY_CALIB;
}


static int nrf_wifi_radio_test_set_defaults(const struct shell *shell,
					    size_t argc,
					    const char *argv[])
{
	if (!check_test_in_prog(shell)) {
		return -ENOEXEC;
	}

	nrf_wifi_radio_test_conf_init(&ctx->conf_params);

	return 0;
}


static int nrf_wifi_radio_test_set_phy_calib_rxdc(const struct shell *shell,
						  size_t argc,
						  const char *argv[])
{
	char *ptr = NULL;
	unsigned long phy_calib_rxdc = 0;

	phy_calib_rxdc = strtoul(argv[1], &ptr, 10);

	if (phy_calib_rxdc > 1) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid PHY RX DC calibration value(%lu).\n",
			      phy_calib_rxdc);
		shell_help(shell);
		return -ENOEXEC;
	}

	if (!check_test_in_prog(shell)) {
		return -ENOEXEC;
	}

	if (phy_calib_rxdc)
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib |
					     NRF_WIFI_PHY_CALIB_FLAG_RXDC);
	else
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib &
					     ~(NRF_WIFI_PHY_CALIB_FLAG_RXDC));

	return 0;
}


static int nrf_wifi_radio_test_set_phy_calib_txdc(const struct shell *shell,
						  size_t argc,
						  const char *argv[])
{
	char *ptr = NULL;
	unsigned long phy_calib_txdc = 0;

	phy_calib_txdc = strtoul(argv[1], &ptr, 10);

	if (phy_calib_txdc > 1) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid PHY TX DC calibration value(%lu).\n",
			      phy_calib_txdc);
		shell_help(shell);
		return -ENOEXEC;
	}

	if (!check_test_in_prog(shell)) {
		return -ENOEXEC;
	}

	if (phy_calib_txdc)
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib |
					     NRF_WIFI_PHY_CALIB_FLAG_TXDC);
	else
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib &
					     ~(NRF_WIFI_PHY_CALIB_FLAG_TXDC));

	return 0;
}


static int nrf_wifi_radio_test_set_phy_calib_txpow(const struct shell *shell,
						   size_t argc,
						   const char *argv[])
{
	char *ptr = NULL;
	unsigned long phy_calib_txpow = 0;

	phy_calib_txpow = strtoul(argv[1], &ptr, 10);

	if (phy_calib_txpow > 1) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid PHY TX power calibration value(%lu).\n",
			      phy_calib_txpow);
		shell_help(shell);
		return -ENOEXEC;
	}

	if (!check_test_in_prog(shell)) {
		return -ENOEXEC;
	}

	if (phy_calib_txpow)
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib |
					     NRF_WIFI_PHY_CALIB_FLAG_TXPOW);
	else
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib &
					     ~(NRF_WIFI_PHY_CALIB_FLAG_TXPOW));

	return 0;
}


static int nrf_wifi_radio_test_set_phy_calib_rxiq(const struct shell *shell,
						  size_t argc,
						  const char *argv[])
{
	char *ptr = NULL;
	unsigned long phy_calib_rxiq = 0;

	phy_calib_rxiq = strtoul(argv[1], &ptr, 10);

	if (phy_calib_rxiq > 1) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid PHY RX IQ calibration value(%lu).\n",
			      phy_calib_rxiq);
		shell_help(shell);
		return -ENOEXEC;
	}

	if (!check_test_in_prog(shell)) {
		return -ENOEXEC;
	}

	if (phy_calib_rxiq)
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib |
					     NRF_WIFI_PHY_CALIB_FLAG_RXIQ);
	else
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib &
					     ~(NRF_WIFI_PHY_CALIB_FLAG_RXIQ));

	return 0;
}


static int nrf_wifi_radio_test_set_phy_calib_txiq(const struct shell *shell,
						  size_t argc,
						  const char *argv[])
{
	char *ptr = NULL;
	unsigned long phy_calib_txiq = 0;

	phy_calib_txiq = strtoul(argv[1], &ptr, 10);

	if (phy_calib_txiq > 1) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid PHY TX IQ calibration value(%lu).\n",
			      phy_calib_txiq);
		shell_help(shell);
		return -ENOEXEC;
	}

	if (!check_test_in_prog(shell)) {
		return -ENOEXEC;
	}

	if (phy_calib_txiq)
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib |
					     NRF_WIFI_PHY_CALIB_FLAG_TXIQ);
	else
		ctx->conf_params.phy_calib = (ctx->conf_params.phy_calib &
					     ~(NRF_WIFI_PHY_CALIB_FLAG_TXIQ));
	return 0;
}


static int nrf_wifi_radio_test_set_he_ltf(const struct shell *shell,
					  size_t argc,
					  const char *argv[])
{
	char *ptr = NULL;
	unsigned long he_ltf = 0;

	he_ltf = strtoul(argv[1], &ptr, 10);

	if (he_ltf > 2) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid HE LTF value(%lu).\n",
			      he_ltf);
		shell_help(shell);
		return -ENOEXEC;
	}

	ctx->conf_params.he_ltf = he_ltf;

	return 0;
}


static int nrf_wifi_radio_test_set_he_gi(const struct shell *shell,
					 size_t argc,
					 const char *argv[])
{
	char *ptr = NULL;
	unsigned long he_gi = 0;

	he_gi = strtoul(argv[1], &ptr, 10);

	if (he_gi > 2) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid HE GI value(%lu).\n",
			      he_gi);
		shell_help(shell);
		return -ENOEXEC;
	}

	ctx->conf_params.he_gi = he_gi;

	return 0;
}


static int nrf_wifi_radio_test_set_rf_params(const struct shell *shell,
					     size_t argc,
					     const char *argv[])
{
	if (!check_test_in_prog(shell)) {
		return -ENOEXEC;
	}

	memset(ctx->conf_params.rf_params,
	       0xFF,
	       NRF_WIFI_RF_PARAMS_CONF_SIZE);

	nrf_wifi_utils_hex_str_to_val(rpu_drv_priv_zep.fmac_priv->opriv,
				      ctx->conf_params.rf_params,
				      NRF_WIFI_RF_PARAMS_CONF_SIZE,
				      (char *)argv[1]);

	return 0;
}


static int nrf_wifi_radio_test_set_tx_pkt_tput_mode(const struct shell *shell,
						    size_t argc,
						    const char *argv[])
{
	char *ptr = NULL;
	unsigned long val = 0;

	val = strtoul(argv[1], &ptr, 10);

	if (val >= RPU_TPUT_MODE_MAX) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid value %lu\n",
			      val);
		shell_help(shell);
		return -ENOEXEC;
	}

	if (!check_test_in_prog(shell)) {
		return -ENOEXEC;
	}

	ctx->conf_params.tx_pkt_tput_mode = val;

	return 0;
}


static int nrf_wifi_radio_test_set_tx_pkt_sgi(const struct shell *shell,
					      size_t argc,
					      const char *argv[])
{
	char *ptr = NULL;
	unsigned long val = 0;

	val = strtoul(argv[1], &ptr, 10);

	if (val > 1) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid value %lu\n",
			      val);
		shell_help(shell);
		return -ENOEXEC;
	}

	if (!check_test_in_prog(shell)) {
		return -ENOEXEC;
	}

	ctx->conf_params.tx_pkt_sgi = val;

	return 0;
}


static int nrf_wifi_radio_test_set_tx_pkt_preamble(const struct shell *shell,
						   size_t argc,
						   const char *argv[])
{
	char *ptr = NULL;
	unsigned long val = 0;

	val = strtoul(argv[1], &ptr, 10);

	if (val >= RPU_PKT_PREAMBLE_MAX) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid value %lu\n",
			      val);
		shell_help(shell);
		return -ENOEXEC;
	}

	if (!check_test_in_prog(shell)) {
		return -ENOEXEC;
	}

	if (ctx->conf_params.tx_pkt_tput_mode == 0) {
		if ((val != RPU_PKT_PREAMBLE_SHORT) &&
		    (val != RPU_PKT_PREAMBLE_LONG)) {
			shell_fprintf(shell,
				      SHELL_ERROR,
				      "Invalid value %lu for legacy mode\n",
				      val);
			return -ENOEXEC;
		}
	} else {
		if (val != RPU_PKT_PREAMBLE_MIXED) {
			shell_fprintf(shell,
				      SHELL_ERROR,
				      "Only mixed preamble mode supported in HT, VHT and HE modes\n");
			return -ENOEXEC;
		}
	}

	ctx->conf_params.tx_pkt_preamble = val;

	return 0;
}


static int nrf_wifi_radio_test_set_tx_pkt_mcs(const struct shell *shell,
					      size_t argc,
					      const char *argv[])
{
	char *ptr = NULL;
	long val = -1;

	val = strtol(argv[1], &ptr, 10);

	if (!check_test_in_prog(shell)) {
		return -ENOEXEC;
	}

	if (ctx->conf_params.tx_pkt_rate != -1) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "tx_pkt_rate is set\n");
		return -ENOEXEC;
	}

	if (!(check_valid_data_rate(shell,
				    ctx->conf_params.tx_pkt_tput_mode,
				    ctx->conf_params.tx_pkt_nss,
				    (val | 0x80)))) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid value %lu\n",
			      val);
		return -ENOEXEC;
	}

	ctx->conf_params.tx_pkt_mcs = val;

	return 0;
}


static int nrf_wifi_radio_test_set_tx_pkt_rate(const struct shell *shell,
					       size_t argc,
					       const char *argv[])
{
	char *ptr = NULL;
	long val = -1;

	val = strtol(argv[1], &ptr, 10);

	if (!check_test_in_prog(shell)) {
		return -ENOEXEC;
	}

	if (ctx->conf_params.tx_pkt_mcs != -1) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "tx_pkt_mcs is set\n");
		return -ENOEXEC;
	}

	if (!(check_valid_data_rate(shell,
				    ctx->conf_params.tx_pkt_tput_mode,
				    ctx->conf_params.tx_pkt_nss,
				    val))) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid value %lu\n",
			      val);
		return -ENOEXEC;
	}

	ctx->conf_params.tx_pkt_rate = val;

	return 0;
}


static int nrf_wifi_radio_test_set_tx_pkt_gap(const struct shell *shell,
					      size_t argc,
					      const char *argv[])
{
	char *ptr = NULL;
	unsigned long val = 0;

	val = strtoul(argv[1], &ptr, 10);

	if ((val < 200) || (val > 200000)) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid value %lu\n",
			      val);
		shell_help(shell);
		return -ENOEXEC;
	}

	if (!check_test_in_prog(shell)) {
		return -ENOEXEC;
	}

	ctx->conf_params.tx_pkt_gap_us = val;

	return 0;
}


static int nrf_wifi_radio_test_set_chnl_primary(const struct shell *shell,
						size_t argc,
						const char *argv[])
{
	char *ptr = NULL;
	unsigned long val = 0;

	val = strtoul(argv[1], &ptr, 10);

	if (!(check_valid_channel(val))) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid value %lu\n",
			      val);
		return -ENOEXEC;
	}


	if (!check_test_in_prog(shell)) {
		return -ENOEXEC;
	}

	ctx->conf_params.chan.primary_num = val;

	return 0;
}


static int nrf_wifi_radio_test_set_tx_pkt_num(const struct shell *shell,
					      size_t argc,
					      const char *argv[])
{
	char *ptr = NULL;
	long val = 0;

	val = strtol(argv[1], &ptr, 10);

	if ((val < -1) || (val == 0)) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid value %lu\n",
			      val);
		shell_help(shell);
		return -ENOEXEC;
	}

	if (!check_test_in_prog(shell)) {
		return -ENOEXEC;
	}

	ctx->conf_params.tx_pkt_num = val;

	return 0;
}


static int nrf_wifi_radio_test_set_tx_pkt_len(const struct shell *shell,
					      size_t argc,
					      const char *argv[])
{
	char *ptr = NULL;
	unsigned long val = 0;

	val = strtoul(argv[1], &ptr, 10);

	if (val == 0) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid value %lu\n",
			      val);
		shell_help(shell);
		return -ENOEXEC;
	}

	if (ctx->conf_params.tx_pkt_tput_mode == RPU_TPUT_MODE_MAX) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Throughput mode not set\n");
		return -ENOEXEC;
	}

#ifndef CONFIG_NRF700X_REV_A
	if (ctx->conf_params.tx_pkt_tput_mode == RPU_TPUT_MODE_HE_TB) {
		if (ctx->conf_params.ru_tone == 26) {
			if (val >= 350) {
				shell_fprintf(shell,
					      SHELL_ERROR,
					      "'tx_pkt_len' has to be less than 350 bytes\n");
				return -ENOEXEC;
			}
		} else if (ctx->conf_params.ru_tone == 52) {
			if (val >= 800) {
				shell_fprintf(shell,
					      SHELL_ERROR,
					      "'tx_pkt_len' has to be less than 800 bytes\n");
				return -ENOEXEC;
			}
		} else if (ctx->conf_params.ru_tone == 106) {
			if (val >= 1800) {
				shell_fprintf(shell,
					      SHELL_ERROR,
					      "'tx_pkt_len' has to be less than 1800 bytes\n");
				return -ENOEXEC;
			}
		} else if (ctx->conf_params.ru_tone == 242) {
			if (val >= 4000) {
				shell_fprintf(shell,
					      SHELL_ERROR,
					      "'tx_pkt_len' has to be less than 4000 bytes\n");
				return -ENOEXEC;
			}
		} else {
			shell_fprintf(shell,
				      SHELL_ERROR,
				      "'ru_tone' not set\n");
			return -ENOEXEC;
		}
	}
#endif /* !CONFIG_NRF700X_REV_A */

	if (!check_test_in_prog(shell)) {
		return -ENOEXEC;
	}

	ctx->conf_params.tx_pkt_len = val;

	return 0;
}


static int nrf_wifi_radio_test_set_tx_power(const struct shell *shell,
					    size_t argc,
					    const char *argv[])
{
	char *ptr = NULL;
	unsigned long val = 0;

	val = strtoul(argv[1], &ptr, 10);

	if (!check_test_in_prog(shell)) {
		return -ENOEXEC;
	}

	ctx->conf_params.tx_power = val;

	return 0;
}


#ifndef CONFIG_NRF700X_REV_A
static int nrf_wifi_radio_test_set_ru_tone(const struct shell *shell,
					   size_t argc,
					   const char *argv[])
{
	char *ptr = NULL;
	unsigned long val = 0;

	val = strtoul(argv[1], &ptr, 10);

	if ((val != 26) &&
	    (val != 52) &&
	    (val != 106) &&
	    (val != 242)) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid value %lu\n",
			      val);
		shell_help(shell);
		return -ENOEXEC;
	}

	if (!check_test_in_prog(shell)) {
		return -ENOEXEC;
	}

	ctx->conf_params.ru_tone = val;

	return 0;
}


static int nrf_wifi_radio_test_set_ru_index(const struct shell *shell,
					    size_t argc,
					    const char *argv[])
{
	char *ptr = NULL;
	unsigned long val = 0;

	val = strtoul(argv[1], &ptr, 10);

	if (ctx->conf_params.ru_tone == 26) {
		if ((val < 1) || (val > 9)) {
			shell_fprintf(shell,
				      SHELL_ERROR,
				      "Invalid value %lu\n",
				      val);
			shell_help(shell);
			return -ENOEXEC;
		}
	} else if (ctx->conf_params.ru_tone == 52) {
		if ((val < 1) || (val > 4)) {
			shell_fprintf(shell,
				      SHELL_ERROR,
				      "Invalid value %lu\n",
				      val);
			shell_help(shell);
			return -ENOEXEC;
		}
	} else if (ctx->conf_params.ru_tone == 106) {
		if ((val < 1) || (val > 2)) {
			shell_fprintf(shell,
				      SHELL_ERROR,
				      "Invalid value %lu\n",
				      val);
			shell_help(shell);
			return -ENOEXEC;
		}
	} else if (ctx->conf_params.ru_tone == 242) {
		if ((val != 1)) {
			shell_fprintf(shell,
				      SHELL_ERROR,
				      "Invalid value %lu\n",
				      val);
			shell_help(shell);
			return -ENOEXEC;
		}
	} else {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "RU tone not set\n");
		shell_help(shell);
		return -ENOEXEC;
	}


	if (!check_test_in_prog(shell)) {
		return -ENOEXEC;
	}

	ctx->conf_params.ru_index = val;

	return 0;
}
#endif /* !CONFIG_NRF700X_REV_A */


static int nrf_wifi_radio_test_set_tx(const struct shell *shell,
				      size_t argc,
				      const char *argv[])
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	char *ptr = NULL;
	unsigned long val = 0;

	val = strtoul(argv[1], &ptr, 10);

	if (val > 1) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid value %lu\n",
			      val);
		shell_help(shell);
		return -ENOEXEC;
	}

	if (val == 1) {
		if (!check_test_in_prog(shell)) {
			return -ENOEXEC;
		}

		if (check_channel_settings(shell,
					   ctx->conf_params.tx_pkt_tput_mode,
					   &ctx->conf_params.chan) != 0) {
			shell_fprintf(shell,
				      SHELL_ERROR,
				      "Invalid channel settings\n");
			return -ENOEXEC;
		}

		if ((ctx->conf_params.tx_pkt_rate != -1) &&
		    (ctx->conf_params.tx_pkt_mcs != -1)) {
			shell_fprintf(shell,
				      SHELL_ERROR,
				      "'tx_pkt_rate' & 'tx_pkt_mcs' cannot be set simultaneously\n");
			return -ENOEXEC;
		}
	}

	ctx->conf_params.tx = val;

	status = wifi_nrf_fmac_radio_test_prog_tx(ctx->rpu_ctx,
						  &ctx->conf_params);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Programming TX failed\n");
		return -ENOEXEC;
	}

	return 0;
}


static int nrf_wifi_radio_test_set_rx(const struct shell *shell,
				      size_t argc,
				      const char *argv[])
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	char *ptr = NULL;
	unsigned long val = 0;

	val = strtoul(argv[1], &ptr, 10);

	if (val > 1) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid value %lu\n",
			      val);
		shell_help(shell);
		return -ENOEXEC;
	}

	if (val == 1) {
		if (!check_test_in_prog(shell)) {
			return -ENOEXEC;
		}
	}

	ctx->conf_params.rx = val;

	status = wifi_nrf_fmac_radio_test_prog_rx(ctx->rpu_ctx,
						  &ctx->conf_params);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Programming RX failed\n");
		return -ENOEXEC;
	}

	return 0;
}

#ifdef CONFIG_BOARD_NRF7002DK_NRF5340
static int nrf_wifi_radio_test_ble_ant_switch_ctrl(const struct shell *shell,
					     size_t argc,
					     const char *argv[])
{
	unsigned int val;

	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "invalid # of args : %d\n", argc);
		return -ENOEXEC;
	}

	val  = strtoul(argv[1], NULL, 0);

	ctx->conf_params.ble_ant_switch_ctrl = val;

	return ble_ant_switch(val);
}
#endif /* CONFIG_BOARD_NRF7002DK_NRF5340 */


static int nrf_wifi_radio_test_rx_adc_cap(const struct shell *shell,
					  size_t argc,
					  const char *argv[])
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	char *ptr = NULL;
	unsigned long capture_length = 0;
	unsigned int *rx_adc_cap = NULL;
	unsigned int i = 0;
	int ret = -ENOEXEC;
	unsigned char lna_gain;
	unsigned char bb_gain;

	capture_length = strtoul(argv[1], &ptr, 10);
	lna_gain = strtoul(argv[2], &ptr, 10);
	bb_gain  = strtoul(argv[3], &ptr, 10);

	if (capture_length >= 1) {
		if (!check_test_in_prog(shell)) {
			goto out;
		}
	} else {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "%s: Invalid capture_length %ld\n", __func__, capture_length);
		shell_help(shell);
		goto out;
	}

	rx_adc_cap = k_calloc((capture_length * 4), sizeof(char));

	if (!rx_adc_cap) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "%s: Unable to allocate (%ld) bytes for RX ADC capture\n",
				   __func__, (capture_length * 4));
		goto out;
	}

	ctx->rf_test_run = true;
	ctx->rf_test = NRF_WIFI_RF_TEST_RX_ADC_CAP;

	status = nrf_wifi_fmac_rf_test_rx_cap(ctx->rpu_ctx,
					      NRF_WIFI_RF_TEST_RX_ADC_CAP,
					      rx_adc_cap,
					      capture_length,
						  lna_gain,
						  bb_gain);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "RX ADC capture programming failed\n");
		goto out;
	}

	shell_fprintf(shell,
		      SHELL_INFO,
		      "************* RX ADC capture data ***********\n");

	for (i = 0; i < capture_length/4; i++)
		shell_fprintf(shell,
			      SHELL_INFO,
			      "%08X\n",
				  rx_adc_cap[i]);

	ret = 0;
out:
	if (rx_adc_cap)
		k_free(rx_adc_cap);

	ctx->rf_test_run = false;
	ctx->rf_test = NRF_WIFI_RF_TEST_MAX;

	return ret;
}


static int nrf_wifi_radio_test_rx_stat_pkt_cap(const struct shell *shell,
					       size_t argc,
					       const char *argv[])
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	char *ptr = NULL;
	unsigned long capture_length = 0;
	unsigned int *rx_stat_pkt_cap = NULL;
	unsigned int i = 0;
	int ret = -ENOEXEC;
	unsigned char lna_gain;
	unsigned char bb_gain;

	capture_length = strtoul(argv[1], &ptr, 10);
	lna_gain = strtoul(argv[2], &ptr, 10);
	bb_gain  = strtoul(argv[3], &ptr, 10);

	if (capture_length >= 1) {
		if (!check_test_in_prog(shell)) {
			goto out;
		}
	} else {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "%s: Invalid capture_length %ld\n", __func__, capture_length);
		shell_help(shell);
		goto out;
	}

	rx_stat_pkt_cap = k_calloc((capture_length * 4), sizeof(char));

	if (!rx_stat_pkt_cap) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "%s: Unable to allocate (%ld) bytes for RX static packet capture\n",
				  __func__, (capture_length * 4));
		goto out;
	}

	ctx->rf_test_run = true;
	ctx->rf_test = NRF_WIFI_RF_TEST_RX_STAT_PKT_CAP;

	status = nrf_wifi_fmac_rf_test_rx_cap(ctx->rpu_ctx,
					      NRF_WIFI_RF_TEST_RX_STAT_PKT_CAP,
					      rx_stat_pkt_cap,
					      capture_length,
						  lna_gain,
						  bb_gain);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "RX static packet capture programming failed\n");
		goto out;
	}

	shell_fprintf(shell,
		      SHELL_INFO,
		      "************* RX static packet capture data ***********\n");

	for (i = 0; i < capture_length/4; i++)
		shell_fprintf(shell,
			      SHELL_INFO,
			      "%08X\n",
			      rx_stat_pkt_cap[i]);

	ret = 0;
out:
	if (rx_stat_pkt_cap)
		k_free(rx_stat_pkt_cap);

	ctx->rf_test_run = false;
	ctx->rf_test = NRF_WIFI_RF_TEST_MAX;

	return ret;
}


static int nrf_wifi_radio_test_rx_dyn_pkt_cap(const struct shell *shell,
					      size_t argc,
					      const char *argv[])
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	char *ptr = NULL;
	unsigned long capture_length = 0;
	unsigned int *rx_dyn_pkt_cap = NULL;
	unsigned int i = 0;
	int ret = -ENOEXEC;
	unsigned char lna_gain = 0;
	unsigned char bb_gain = 0;

	capture_length = strtoul(argv[1], &ptr, 10);

	if (capture_length >= 1) {
		if (!check_test_in_prog(shell)) {
			goto out;
		}
	} else {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "%s: Invalid capture_length %ld\n", __func__, capture_length);
		shell_help(shell);
		goto out;
	}

	rx_dyn_pkt_cap = k_calloc((capture_length * 4), sizeof(char));

	if (!rx_dyn_pkt_cap) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "%s: Unable to allocate (%ld) bytes for RX dynamic packet capture\n",
				  __func__, (capture_length * 4));
		goto out;
	}

	ctx->rf_test_run = true;
	ctx->rf_test = NRF_WIFI_RF_TEST_RX_DYN_PKT_CAP;

	status = nrf_wifi_fmac_rf_test_rx_cap(ctx->rpu_ctx,
					      NRF_WIFI_RF_TEST_RX_DYN_PKT_CAP,
					      rx_dyn_pkt_cap,
					      capture_length,
						  lna_gain,
						  bb_gain);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "RX dynamic packet capture programming failed\n");
		goto out;
	}

	shell_fprintf(shell,
		      SHELL_INFO,
		      "************* RX dynamic packet capture data ***********\n");

	for (i = 0; i < capture_length/4; i++)
		shell_fprintf(shell,
			      SHELL_INFO,
			      "%08X\n",
			      rx_dyn_pkt_cap[i]);

	ret = 0;
out:
	if (rx_dyn_pkt_cap)
		k_free(rx_dyn_pkt_cap);

	ctx->rf_test_run = false;
	ctx->rf_test = NRF_WIFI_RF_TEST_MAX;

	return ret;
}


static int nrf_wifi_radio_test_tx_tone(const struct shell *shell,
				       size_t argc,
				       const char *argv[])
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	char *ptr = NULL;
	unsigned long val = 0;
	int ret = -ENOEXEC;
	signed int norm_frequency;
	unsigned short int tone_amplitude;
	unsigned char tx_power;

	val = strtoul(argv[1], &ptr, 10);
	norm_frequency = strtoul(argv[2], &ptr, 10);
	tone_amplitude = strtoul(argv[3], &ptr, 10);
	tx_power = strtoul(argv[4], &ptr, 10);

	if (val > 1) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid value %lu\n",
			      val);
		shell_help(shell);
		goto out;
	}

	if (norm_frequency == 0)	{
		shell_fprintf(shell,
			      SHELL_ERROR,
				  "Tone frequency is not configured\n");
	}

	if (val == 1) {
		if (!check_test_in_prog(shell)) {
			goto out;
		}
	}

	ctx->rf_test_run = true;
	ctx->rf_test = NRF_WIFI_RF_TEST_TX_TONE;

	status = nrf_wifi_fmac_rf_test_tx_tone(ctx->rpu_ctx,
					       (unsigned char)val,
						   norm_frequency,
						   tone_amplitude,
						   tx_power);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "TX tone programming failed\n");
		goto out;
	}

	ret = 0;
out:
	ctx->rf_test_run = false;
	ctx->rf_test = NRF_WIFI_RF_TEST_MAX;

	return ret;
}


static int nrf_wifi_radio_test_dpd(const struct shell *shell,
				   size_t argc,
				   const char *argv[])
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	char *ptr = NULL;
	unsigned long val = 0;
	int ret = -ENOEXEC;

	val = strtoul(argv[1], &ptr, 10);

	if (val > 1) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid value %lu\n",
			      val);
		shell_help(shell);
		goto out;
	}

	if (val == 1) {
		if (!check_test_in_prog(shell)) {
			goto out;
		}
	}

	ctx->rf_test_run = true;
	ctx->rf_test = NRF_WIFI_RF_TEST_DPD;

	status = nrf_wifi_fmac_rf_test_dpd(ctx->rpu_ctx,
					   val);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "DPD programming failed\n");
		goto out;
	}

	ret = 0;
out:
	ctx->rf_test_run = false;
	ctx->rf_test = NRF_WIFI_RF_TEST_MAX;

	return ret;
}

static int nrf_wifi_radio_get_temperature(const struct shell *shell,
				   size_t argc,
				   const char *argv[])
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	char *ptr = NULL;
	unsigned long val = 0;
	int ret = -ENOEXEC;

	val = strtoul(argv[1], &ptr, 10);

	if (val > 1) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid value %lu\n",
			      val);
		shell_help(shell);
		goto out;
	}

	if (val == 1) {
		if (!check_test_in_prog(shell)) {
			goto out;
		}
	}

	ctx->rf_test_run = true;
	ctx->rf_test = NRF_WIFI_RF_TEST_GET_TEMPERATURE;

	status = nrf_wifi_fmac_rf_get_temp(ctx->rpu_ctx);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "DPD programming failed\n");
		goto out;
	}

	ret = 0;
out:
	ctx->rf_test_run = false;
	ctx->rf_test = NRF_WIFI_RF_TEST_MAX;

	return ret;
}

static int nrf_wifi_radio_get_rf_rssi(const struct shell *shell,
				   size_t argc,
				   const char *argv[])
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	char *ptr = NULL;
	unsigned long val = 0;
	int ret = -ENOEXEC;

	val = strtoul(argv[1], &ptr, 10);

	if (val > 1) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid value %lu\n",
			      val);
		shell_help(shell);
		goto out;
	}

	if (val == 1) {
		if (!check_test_in_prog(shell)) {
			goto out;
		}
	}

	ctx->rf_test_run = true;
	ctx->rf_test = NRF_WIFI_RF_TEST_RF_RSSI;

	status = nrf_wifi_fmac_rf_get_rf_rssi(ctx->rpu_ctx);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "DPD programming failed\n");
		goto out;
	}

	ret = 0;
out:
	ctx->rf_test_run = false;
	ctx->rf_test = NRF_WIFI_RF_TEST_MAX;

	return ret;
}


static int nrf_wifi_radio_set_xo_val(const struct shell *shell,
				   size_t argc,
				   const char *argv[])
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	char *ptr = NULL;
	unsigned long val = 0;
	int ret = -ENOEXEC;

	val = strtoul(argv[1], &ptr, 10);

	if (val > 0x7f) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid value XO value should be <= 0x7f\n");
		shell_help(shell);
		goto out;
	}

	if (val < 0) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid value XO value should be >= 0\n");
		shell_help(shell);
		goto out;
	}

	if (val == 1) {
		if (!check_test_in_prog(shell)) {
			goto out;
		}
	}

	ctx->rf_test_run = true;
	ctx->rf_test = NRF_WIFI_RF_TEST_XO_CALIB;

	status = nrf_wifi_fmac_set_xo_val(ctx->rpu_ctx,
					       (unsigned char)val);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "XO value programming failed\n");
		goto out;
	}

	ret = 0;
out:
	ctx->rf_test_run = false;
	ctx->rf_test = NRF_WIFI_RF_TEST_MAX;

	return ret;
}

static int nrf_wifi_radio_get_xo_value(const struct shell *shell,
				   size_t argc,
				   const char *argv[])
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	char *ptr = NULL;
	unsigned int tone_frequency = 0;
	int ret = -ENOEXEC;

	tone_frequency = strtoul(argv[1], &ptr, 10);

	if (tone_frequency < 1) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid value %d\n",
			      tone_frequency);
		shell_help(shell);
		goto out;
	} else {
		if (!check_test_in_prog(shell)) {
			goto out;
		}
	}

	ctx->rf_test_run = true;
	ctx->rf_test = NRF_WIFI_RF_TEST_XO_TUNE;

	status = nrf_wifi_fmac_rf_test_get_xo_value(ctx->rpu_ctx, tone_frequency);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "XO value programming failed\n");
		goto out;
	}

	ret = 0;
out:
	ctx->rf_test_run = false;
	ctx->rf_test = NRF_WIFI_RF_TEST_MAX;

	return ret;
}


static int nrf_wifi_radio_test_show_cfg(const struct shell *shell,
					size_t argc,
					const char *argv[])
{
	int i = 0;
	struct rpu_conf_params *conf_params = NULL;

	conf_params = &ctx->conf_params;

	shell_fprintf(shell,
		      SHELL_INFO,
		      "************* Configured Parameters ***********\n");
	shell_fprintf(shell,
		      SHELL_INFO,
		      "rf_params =");

	for (i = 0; i < NRF_WIFI_RF_PARAMS_CONF_SIZE; i++)
		shell_fprintf(shell,
			      SHELL_INFO,
			      " %02X",
			      conf_params->rf_params[i]);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "\n");

	shell_fprintf(shell,
		      SHELL_INFO,
		      "tx_pkt_tput_mode = %d\n",
		      conf_params->tx_pkt_tput_mode);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "tx_pkt_sgi = %d\n",
		      conf_params->tx_pkt_sgi);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "tx_pkt_preamble = %d\n",
		      conf_params->tx_pkt_preamble);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "tx_pkt_mcs = %d\n",
		      conf_params->tx_pkt_mcs);

	if (conf_params->tx_pkt_rate == 5)
		shell_fprintf(shell,
			      SHELL_INFO,
			      "tx_pkt_rate = 5.5\n");
	else
		shell_fprintf(shell,
			      SHELL_INFO,
			      "tx_pkt_rate = %d\n",
			      conf_params->tx_pkt_rate);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "tx_pkt_gap = %d\n",
		      conf_params->tx_pkt_gap_us);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "phy_calib_rxdc = %d\n",
		      (conf_params->phy_calib &
		       NRF_WIFI_PHY_CALIB_FLAG_RXDC) ? 1:0);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "phy_calib_txdc = %d\n",
		      (conf_params->phy_calib &
		       NRF_WIFI_PHY_CALIB_FLAG_TXDC) ? 1:0);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "phy_calib_txpow = %d\n",
		      (conf_params->phy_calib &
		       NRF_WIFI_PHY_CALIB_FLAG_TXPOW) ? 1:0);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "phy_calib_rxiq = %d\n",
		      (conf_params->phy_calib &
		       NRF_WIFI_PHY_CALIB_FLAG_RXIQ) ? 1:0);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "phy_calib_txiq = %d\n",
		      (conf_params->phy_calib &
		       NRF_WIFI_PHY_CALIB_FLAG_TXIQ) ? 1:0);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "chnl_primary = %d\n",
		      conf_params->chan.primary_num);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "tx_pkt_num = %d\n",
		      conf_params->tx_pkt_num);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "tx_pkt_len = %d\n",
		      conf_params->tx_pkt_len);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "tx_power = %d\n",
		      conf_params->tx_power);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "he_ltf = %d\n",
		      conf_params->he_ltf);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "he_gi = %d\n",
		      conf_params->he_gi);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "tx = %d\n",
		      conf_params->tx);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "rx = %d\n",
		      conf_params->rx);

#ifdef CONFIG_BOARD_NRF7002DK_NRF5340
	shell_fprintf(shell,
		      SHELL_INFO,
		      "ble_ant_switch_ctrl = %d\n",
		      conf_params->ble_ant_switch_ctrl);
#endif /* CONFIG_BOARD_NRF7002DK_NRF5340 */

#ifndef CONFIG_NRF700X_REV_A
	shell_fprintf(shell,
		      SHELL_INFO,
		      "wlan_ant_switch_ctrl = %d\n",
		      conf_params->wlan_ant_switch_ctrl);
#endif /* ! CONFIG_NRF700X_REV_A */

	return 0;
}


static int nrf_wifi_radio_test_get_stats(const struct shell *shell,
					 size_t argc,
					 const char *argv[])
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct rpu_op_stats stats;

	memset(&stats,
	       0,
	       sizeof(stats));

	status = wifi_nrf_fmac_stats_get(ctx->rpu_ctx,
					 ctx->conf_params.op_mode,
					 &stats);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "wifi_nrf_fmac_stats_get failed\n");
		return -ENOEXEC;
	}

	shell_fprintf(shell,
		      SHELL_INFO,
		      "************* PHY STATS ***********\n");

	shell_fprintf(shell,
		      SHELL_INFO,
		      "rssi_avg = %d dBm\n",
		      stats.fw.phy.rssi_avg);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "ofdm_crc32_pass_cnt=%d\n",
		      stats.fw.phy.ofdm_crc32_pass_cnt);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "ofdm_crc32_fail_cnt=%d\n",
		      stats.fw.phy.ofdm_crc32_fail_cnt);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "dsss_crc32_pass_cnt=%d\n",
		      stats.fw.phy.dsss_crc32_pass_cnt);

	shell_fprintf(shell,
		      SHELL_INFO,
		      "dsss_crc32_fail_cnt=%d\n",
		      stats.fw.phy.dsss_crc32_fail_cnt);

	return 0;
}

#ifndef CONFIG_NRF700X_REV_A
/* See enum CD2CM_MSG_ID_T in RPU Coexistence Manager API */
#define CD2CM_UPDATE_SWITCH_CONFIG 0x7
static int nrf_wifi_radio_test_wlan_switch_ctrl(const struct shell *shell,
				 size_t argc,
				 const char *argv[])
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	char *ptr = NULL;
	struct rpu_btcoex params = { 0 };

	if (argc < 2) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid number of parameters\n");
		shell_help(shell);
		return -ENOEXEC;
	}

	params.rpu_msg_id = CD2CM_UPDATE_SWITCH_CONFIG;
	params.switch_A = strtoul(argv[1], &ptr, 10);

	if (params.switch_A > 1) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "Invalid WLAN switch config (%d)\n",
			      params.switch_A);
		shell_help(shell);
		return -ENOEXEC;
	}

	ctx->conf_params.wlan_ant_switch_ctrl = params.switch_A;

	status = wifi_nrf_fmac_conf_btcoex(ctx->rpu_ctx,
					   &params);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		shell_fprintf(shell,
			      SHELL_ERROR,
			      "WLAN switch configuration failed\n");
		return -ENOEXEC;
	}

	return 0;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(
	nrf_wifi_radio_test_subcmds,
	SHELL_CMD_ARG(set_defaults,
		      NULL,
		      "Reset configuration parameter to their default values",
		      nrf_wifi_radio_test_set_defaults,
		      1,
		      0),
	SHELL_CMD_ARG(phy_calib_rxdc,
		      NULL,
		      "0 - Disable RX DC calibration\n"
		      "1 - Enable RX DC calibration",
		      nrf_wifi_radio_test_set_phy_calib_rxdc,
		      2,
		      0),
	SHELL_CMD_ARG(phy_calib_txdc,
		      NULL,
		      "0 - Disable TX DC calibration\n"
		      "1 - Enable TX DC calibration",
		      nrf_wifi_radio_test_set_phy_calib_txdc,
		      2,
		      0),
	SHELL_CMD_ARG(phy_calib_txpow,
		      NULL,
		      "0 - Disable TX power calibration\n"
		      "1 - Enable TX power calibration",
		      nrf_wifi_radio_test_set_phy_calib_txpow,
		      2,
		      0),
	SHELL_CMD_ARG(phy_calib_rxiq,
		      NULL,
		      "0 - Disable RX IQ calibration\n"
		      "1 - Enable RX IQ calibration",
		      nrf_wifi_radio_test_set_phy_calib_rxiq,
		      2,
		      0),
	SHELL_CMD_ARG(phy_calib_txiq,
		      NULL,
		      "0 - Disable TX IQ calibration\n"
		      "1 - Enable TX IQ calibration",
		      nrf_wifi_radio_test_set_phy_calib_txiq,
		      2,
		      0),
	SHELL_CMD_ARG(he_ltf,
		      NULL,
		      "0 - 1x HE LTF\n"
		      "1 - 2x HE LTF\n"
		      "2 - 4x HE LTF                                        ",
		      nrf_wifi_radio_test_set_he_ltf,
		      2,
		      0),
	SHELL_CMD_ARG(he_gi,
		      NULL,
		      "0 - 0.8 us\n"
		      "1 - 1.6 us\n"
		      "2 - 3.2 us                                           ",
		      nrf_wifi_radio_test_set_he_gi,
		      2,
		      0),
	SHELL_CMD_ARG(rf_params,
		      NULL,
		      "RF parameters in the form of a 42 byte hex value string",
		      nrf_wifi_radio_test_set_rf_params,
		      2,
		      0),
#ifdef CONFIG_NRF700X_REV_A
	SHELL_CMD_ARG(tx_pkt_tput_mode,
		      NULL,
		      "0 - Legacy mode\n"
		      "1 - HT mode\n"
		      "2 - VHT mode\n"
		      "3 - HE(SU) mode\n"
		      "4 - HE(ER SU) mode\n                             ",
		      nrf_wifi_radio_test_set_tx_pkt_tput_mode,
		      2,
		      0),
#else
	SHELL_CMD_ARG(tx_pkt_tput_mode,
		      NULL,
		      "0 - Legacy mode\n"
		      "1 - HT mode\n"
		      "2 - VHT mode\n"
		      "3 - HE(SU) mode\n"
		      "4 - HE(ER SU) mode\n"
		      "5 - HE_TB mode                                   ",
		      nrf_wifi_radio_test_set_tx_pkt_tput_mode,
		      2,
		      0),
#endif /* !CONFIG_NRF700X_REV_A */
	SHELL_CMD_ARG(tx_pkt_sgi,
		      NULL,
		      "0 - Disable\n"
		      "1 - Enable",
		      nrf_wifi_radio_test_set_tx_pkt_sgi,
		      2,
		      0),
	SHELL_CMD_ARG(tx_pkt_preamble,
		      NULL,
		      "0 - Short preamble\n"
		      "1 - Long preamble\n"
		      "2 - Mixed preamble                                   ",
		      nrf_wifi_radio_test_set_tx_pkt_preamble,
		      2,
		      0),
	SHELL_CMD_ARG(tx_pkt_mcs,
		      NULL,
		      "-1    - Not being used\n"
		      "<val> - MCS index to be used",
		      nrf_wifi_radio_test_set_tx_pkt_mcs,
		      2,
		      0),
	SHELL_CMD_ARG(tx_pkt_rate,
		      NULL,
		      "-1    - Not being used\n"
		      "<val> - Legacy rate to be used in Mbps (1, 2, 5.5, 11, 6, 9, 12, 18, 24, 36, 48, 54)",
		      nrf_wifi_radio_test_set_tx_pkt_rate,
		      2,
		      0),
	SHELL_CMD_ARG(tx_pkt_gap,
		      NULL,
		      "<val> - Interval between TX packets in us (Min: 200, Max: 200000, Default: 200)",
		      nrf_wifi_radio_test_set_tx_pkt_gap,
		      2,
		      0),
	SHELL_CMD_ARG(chnl_primary,
		      NULL,
		      "<val> - Primary channel number (Default: 1)",
		      nrf_wifi_radio_test_set_chnl_primary,
		      2,
		      0),
	SHELL_CMD_ARG(tx_pkt_num,
		      NULL,
		      "-1    - Transmit infinite packets\n"
		      "<val> - Number of packets to transmit",
		      nrf_wifi_radio_test_set_tx_pkt_num,
		      2,
		      0),
	SHELL_CMD_ARG(tx_pkt_len,
		      NULL,
		      "<val> - Length of the packet (in bytes) to be transmitted (Default: 1400)",
		      nrf_wifi_radio_test_set_tx_pkt_len,
		      2,
		      0),
	SHELL_CMD_ARG(tx_power,
		      NULL,
		      "<val> - Value in db",
		      nrf_wifi_radio_test_set_tx_power,
		      2,
		      0),
#ifndef CONFIG_NRF700X_REV_A
	SHELL_CMD_ARG(ru_tone,
		      NULL,
		      "<val> - Resource unit (RU) size (26,52,106 or 242)",
		      nrf_wifi_radio_test_set_ru_tone,
		      2,
		      0),
	SHELL_CMD_ARG(ru_index,
		      NULL,
		      "<val> - Location of resource unit (RU) in 20 MHz spectrum\n"
		      "        Valid Values:\n"
		      "           For 26 ru_tone: 1 to 9\n"
		      "           For 52 ru_tone:  1 to 4\n"
		      "           For 106 ru_tone:  1 to 2\n"
		      "           For 242 ru_tone:  1",
		      nrf_wifi_radio_test_set_ru_index,
		      2,
		      0),
#endif /* !CONFIG_NRF700X_REV_A */
	SHELL_CMD_ARG(tx,
		      NULL,
		      "0 - Disable TX\n"
		      "1 - Enable TX",
		      nrf_wifi_radio_test_set_tx,
		      2,
		      0),
	SHELL_CMD_ARG(rx,
		      NULL,
		      "0 - Disable RX\n"
		      "1 - Enable RX",
		      nrf_wifi_radio_test_set_rx,
		      2,
		      0),
#ifdef CONFIG_BOARD_NRF7002DK_NRF5340
	SHELL_CMD_ARG(ble_ant_switch_ctrl,
		      NULL,
		      "0 - Switch set to use the BLE antenna\n"
		      "1 - Switch set to use the shared Wi-Fi antenna",
		      nrf_wifi_radio_test_ble_ant_switch_ctrl,
		      2,
		      0),
#endif /* CONFIG_BOARD_NRF7002DK_NRF5340 */
	SHELL_CMD_ARG(rx_adc_cap,
		      NULL,
		      "\n<CAPTURE LENGTH> - Number of RX ADC samples to be captured\n"
		      "\n<LNA GAIN> - 0: 24dB, 1: 18dB, 2: 12dB, 3: 0dB & 4: -12dB\n"
		      "\n<BASEBAND GAIN> - valid range 0 to 31.It supports 64dB range."
			  "The increment happens lineraly 2dB/step\n",
		      nrf_wifi_radio_test_rx_adc_cap,
		      4,
		      0),
	SHELL_CMD_ARG(rx_stat_pkt_cap,
		      NULL,
			  "\n<CAPTURE LENGTH> - Number of RX ADC samples to be captured\n"
		      "\n<LNA GAIN> - 0: 24dB, 1: 18dB, 2: 12dB, 3: 0dB & 4: -12dB\n"
		      "\n<BASEBAND GAIN> - valid range 0 to 31.It supports 64dB range."
			  "The increment happens lineraly 2dB/step\n",
		      nrf_wifi_radio_test_rx_stat_pkt_cap,
		      4,
		      0),
	SHELL_CMD_ARG(rx_dyn_pkt_cap,
		      NULL,
		      "<val> - Number of RX dynamic pkt samples to be captured",
		      nrf_wifi_radio_test_rx_dyn_pkt_cap,
		      2,
		      0),
	SHELL_CMD_ARG(tx_tone,
		      NULL,
		      "\n<TONE CONTROL> - 0: Disable Tone 1: Enable tone\n"
		      "\n<NORMALIZED FREQUENCY> - Compute the normalized frequency for the tone to be transmitted as\n"
					   "\tnormFreq = round(toneFreq * ((1/(DAC sampling rate /2))*(2^25)))\n"
		      "\n<TONE AMPLITUDE> - Value between 0 to 1023\n"
		      "\n<TX POWER> - TX power in the range -16dBm to +24dBm\n"
			  "Example to transmit 5MHz tone: wifi_radio_test  wifi_radio_test 1 4194304 255 10\n",
		      nrf_wifi_radio_test_tx_tone,
		      5,
		      0),
	SHELL_CMD_ARG(dpd,
		      NULL,
		      "0 - DPD bypass\n"
		      "1 - Enable DPD",
		      nrf_wifi_radio_test_dpd,
		      2,
		      0),
	SHELL_CMD_ARG(get_temperature,
		      NULL,
		      "No arguments required\n",
		      nrf_wifi_radio_get_temperature,
		      1,
		      0),
	SHELL_CMD_ARG(get_rf_rssi,
		      NULL,
		      "No arguments required\n",
		      nrf_wifi_radio_get_rf_rssi,
		      1,
		      0),
	SHELL_CMD_ARG(set_xo_val,
		      NULL,
		      "<val> - XO value",
		      nrf_wifi_radio_set_xo_val,
		      2,
			  0),
	SHELL_CMD_ARG(get_xo_val,
		      NULL,
		      "\n<TONE FREQUENCY> - Default is 0.5MHz(4194304)\n"
			  "The range supported is -1MHz to +1MHz\n"
			  "Compute the tone frequency for the tone to be transmitted as\n"
			  "tone frequency = round(tone_frequency * 2^23)",
		      nrf_wifi_radio_get_xo_value,
		      2,
			  0),
	SHELL_CMD_ARG(show_config,
		      NULL,
		      "Display the current configuration values",
		      nrf_wifi_radio_test_show_cfg,
		      1,
		      0),
	SHELL_CMD_ARG(get_stats,
		      NULL,
		      "Display statistics",
		      nrf_wifi_radio_test_get_stats,
		      1,
		      0),
#ifndef CONFIG_NRF700X_REV_A
	SHELL_CMD_ARG(wlan_ant_switch_ctrl,
		      NULL,
		      "Configure WLAN Antenna switch (0-separate/1-shared)",
		      nrf_wifi_radio_test_wlan_switch_ctrl,
		      2,
		      0),
#endif
	SHELL_SUBCMD_SET_END);


SHELL_CMD_REGISTER(wifi_radio_test,
		   &nrf_wifi_radio_test_subcmds,
		   "nRF Wi-Fi radio test commands",
		   NULL);


static int nrf_wifi_radio_test_shell_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	nrf_wifi_radio_test_conf_init(&ctx->conf_params);

	return 0;
}


SYS_INIT(nrf_wifi_radio_test_shell_init,
	 APPLICATION,
	 CONFIG_APPLICATION_INIT_PRIORITY);
