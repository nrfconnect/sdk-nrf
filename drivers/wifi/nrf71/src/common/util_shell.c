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
#include <system/fmac_tx.h>
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

#ifdef CONFIG_NRF71_STA_MODE
static const char *ac_str(int ac)
{
	static const char * const names[NRF_WIFI_FMAC_AC_MAX] = {
		"BK", "BE", "VI", "VO", "MC"
	};

	if ((ac < 0) || (ac >= NRF_WIFI_FMAC_AC_MAX)) {
		return "??";
	}

	return names[ac];
}

/* One line per topic: what the tokens carried, and what stopped them. */
static void tx_token_stats_summary(const struct shell *sh,
				   struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx,
				   struct nrf_wifi_sys_fmac_priv *sys_fpriv)
{
	struct tx_token_stats *ts = &sys_dev_ctx->tx_config.token_stats;
	unsigned int cap = sys_fpriv->avail_ampdu_len_per_token;
	unsigned int tokens_used = 0;
	unsigned int max_in_flight = 0;
	unsigned int busy = 0;
	unsigned int starved = 0;
	unsigned int q_full = 0;
	unsigned int avg_pkts_x100;
	unsigned int avg_bytes;
	unsigned int stops;
	const char *limiter = "none";
	unsigned int limiter_cnt = 0;
	unsigned int i;

	for (i = 0; i < CONFIG_NRF71_MAX_TX_TOKENS; i++) {
		if (ts->token_cmds[i]) {
			tokens_used++;
		}
	}

	for (i = 0; i < NRF_WIFI_FMAC_AC_MAX; i++) {
		if (ts->max_outstanding_descs[i] > max_in_flight) {
			max_in_flight = ts->max_outstanding_descs[i];
		}
		busy += ts->token_get_busy[i];
		starved += ts->token_get_starved[i];
		q_full += ts->pend_q_full_drops[i];
	}

	avg_pkts_x100 = ts->tx_cmds ?
		(unsigned int)((ts->tx_cmd_pkts * 100ULL) / ts->tx_cmds) : 0U;
	avg_bytes = ts->tx_cmds ? (unsigned int)(ts->tx_cmd_bytes / ts->tx_cmds) : 0U;

	stops = ts->aggr_stop_size + ts->aggr_stop_count + ts->aggr_stop_mismatch +
		ts->aggr_stop_twt + ts->aggr_stop_q_empty;

	/* aggr_stop_q_empty only means the pending queue was drained by this
	 * command, which is the normal case; the host is only really failing to
	 * keep up if the token had to be returned to the free pool for lack of
	 * packets (token_acq) or if the pipe ran idle.
	 */
	if (ts->aggr_stop_count > limiter_cnt) {
		limiter_cnt = ts->aggr_stop_count;
		limiter = "aggr count cap";
	}
	if (ts->aggr_stop_size > limiter_cnt) {
		limiter_cnt = ts->aggr_stop_size;
		limiter = "token size cap";
	}
	if (ts->aggr_stop_mismatch > limiter_cnt) {
		limiter_cnt = ts->aggr_stop_mismatch;
		limiter = "SA/RA mismatch";
	}
	if (ts->aggr_stop_twt > limiter_cnt) {
		limiter_cnt = ts->aggr_stop_twt;
		limiter = "TWT sleep";
	}

	if (ts->pipe_idle_us > ts->pipe_busy_us) {
		limiter = "host out of pkts";
		limiter_cnt = stops;
	} else if (limiter_cnt == 0) {
		limiter = "RPU turnaround";
		limiter_cnt = stops;
	}

	shell_fprintf(sh, SHELL_INFO,
		      "TX: %u cmds, %u pkts, %u dones | %u.%02u pkts/cmd (cap %u), "
		      "%u B/cmd (%u%% of %u B cap), %u B/pkt\n",
		      ts->tx_cmds,
		      ts->tx_cmd_pkts,
		      ts->tx_dones,
		      avg_pkts_x100 / 100U,
		      avg_pkts_x100 % 100U,
		      sys_fpriv->data_config.max_tx_aggregation,
		      avg_bytes,
		      cap ? ((avg_bytes * 100U) / cap) : 0U,
		      cap,
		      ts->tx_cmd_pkts ?
			      (unsigned int)(ts->tx_cmd_data_bytes / ts->tx_cmd_pkts) : 0U);
	shell_fprintf(sh, SHELL_INFO,
		      "    tokens: %u of %u used, %u max in flight, %u busy, %u starved | "
		      "limiter: %s (%u%%)\n",
		      tokens_used,
		      sys_fpriv->num_tx_tokens,
		      max_in_flight,
		      busy,
		      starved,
		      limiter,
		      stops ? ((limiter_cnt * 100U) / stops) : 0U);
	if (ts->tx_cmds) {
		unsigned long long inflight_us = 0;
		unsigned long long busy_us = ts->pipe_busy_us;
		unsigned int window_us = ts->window_start_cyc ?
			k_cyc_to_us_floor32(k_cycle_get_32() - ts->window_start_cyc) : 0U;

		for (i = 0; i < CONFIG_NRF71_MAX_TX_TOKENS; i++) {
			inflight_us += ts->token_inflight_us[i];
		}

		/* Concurrency and rate are computed over the time the pipe was
		 * actually busy, so an idle period before the traffic started
		 * does not dilute them; busy%% is against the window since the
		 * counters were cleared, so clear right before a run.
		 */
		shell_fprintf(sh, SHELL_INFO,
			      "    turnaround: %u us/cmd, %u.%02u cmds in flight while busy "
			      "(max %u), %u KB/s in flight\n",
			      (unsigned int)(inflight_us / ts->tx_cmds),
			      busy_us ? (unsigned int)(inflight_us / busy_us) : 0U,
			      busy_us ? (unsigned int)((inflight_us * 100ULL / busy_us) % 100U) :
					0U,
			      ts->max_cmds_in_flight,
			      busy_us ? (unsigned int)((ts->tx_cmd_bytes * 1000ULL) / busy_us) :
					0U);
		shell_fprintf(sh, SHELL_INFO,
			      "    pipe: busy %llu ms, idle %llu ms, busy %u%% of the "
			      "%u ms window since clear\n",
			      ts->pipe_busy_us / 1000ULL,
			      ts->pipe_idle_us / 1000ULL,
			      window_us ? (unsigned int)((ts->pipe_busy_us * 100ULL) /
							 window_us) : 0U,
			      window_us / 1000U);
	}

	shell_fprintf(sh, SHELL_INFO,
		      "    host: %u pkts in, %u held on pend_q (%u%%), drops: %u q_full, "
		      "%u other%s\n",
		      ts->if_send_calls,
		      ts->pkts_queued,
		      ts->if_send_accepted ?
			      ((ts->pkts_queued * 100U) / ts->if_send_accepted) : 0U,
		      q_full,
		      ts->if_drop_no_nbuf + ts->if_drop_unknown_peer + ts->if_drop_not_ready,
		      starved ? " | STARVED, see -v" : "");
}

static void tx_token_stats_dump(const struct shell *sh,
				struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx,
				struct nrf_wifi_sys_fmac_priv *sys_fpriv)
{
	struct tx_token_stats *ts = &sys_dev_ctx->tx_config.token_stats;
	unsigned int num_tx_tokens = sys_fpriv->num_tx_tokens;
	unsigned int num_tx_tokens_per_ac = sys_fpriv->num_tx_tokens_per_ac;
	unsigned int cap = sys_fpriv->avail_ampdu_len_per_token;
	unsigned int max_aggr = sys_fpriv->data_config.max_tx_aggregation;
	unsigned int total_gets = 0;
	unsigned int total_busy = 0;
	unsigned int total_starved = 0;
	unsigned int aggr_stops;
	unsigned int i;

	shell_fprintf(sh, SHELL_INFO,
		      "\n--- Per-token efficiency ---\n"
		      "tokens: %u (reserved/AC: %u, spare: %u), "
		      "caps: %u B/token, %u pkts/token\n",
		      num_tx_tokens,
		      num_tx_tokens_per_ac,
		      num_tx_tokens - (num_tx_tokens_per_ac * NRF_WIFI_FMAC_AC_MAX),
		      cap,
		      max_aggr);
	shell_fprintf(sh, SHELL_INFO,
		      "bytes = packet data + %u B headroom per packet, i.e. what is\n"
		      "budgeted against the per-token size cap; acq = times the token was\n"
		      "taken from the free pool (a busy token is re-used in TX done and is\n"
		      "not returned, so acq stays low while cmds keeps rising)\n",
		      TX_BUF_HEADROOM);

	shell_fprintf(sh, SHELL_INFO,
		      "%-4s %6s %8s %9s %8s %9s %5s %8s %9s %8s %8s %6s %s\n",
		      "tok", "acq", "cmds", "pkts", "pkts/cmd", "bytes/cmd",
		      "fill", "max_pkts", "max_bytes", "us/cmd", "max_us", "KB/s",
		      "type");

	for (i = 0; i < num_tx_tokens && i < CONFIG_NRF71_MAX_TX_TOKENS; i++) {
		unsigned int cmds = ts->token_cmds[i];
		unsigned int pkts_x100 = cmds ? ((ts->token_pkts[i] * 100U) / cmds) : 0U;
		unsigned int bytes_per_cmd = cmds ?
			(unsigned int)(ts->token_bytes[i] / cmds) : 0U;
		bool spare = (i >= (num_tx_tokens_per_ac * NRF_WIFI_FMAC_AC_MAX));

		unsigned int us_per_cmd = cmds ?
			(unsigned int)(ts->token_inflight_us[i] / cmds) : 0U;

		shell_fprintf(sh, SHELL_INFO,
			      "%-4u %6u %8u %9u %5u.%02u %9u %4u%% %8u %9u %8u %8u %6u %s\n",
			      i,
			      ts->token_acq[i],
			      cmds,
			      ts->token_pkts[i],
			      pkts_x100 / 100U,
			      pkts_x100 % 100U,
			      bytes_per_cmd,
			      cap ? ((bytes_per_cmd * 100U) / cap) : 0U,
			      ts->token_max_pkts[i],
			      ts->token_max_bytes[i],
			      us_per_cmd,
			      ts->token_max_inflight_us[i],
			      ts->token_inflight_us[i] ?
				      (unsigned int)((ts->token_bytes[i] * 1000ULL) /
						     ts->token_inflight_us[i]) : 0U,
			      spare ? "spare" : ac_str(i % NRF_WIFI_FMAC_AC_MAX));
	}

	shell_fprintf(sh, SHELL_INFO,
		      "\n--- Aggregation: packets per TX command ---\n");

	for (i = 0; i < MAX_TX_AGG_SIZE; i++) {
		if (ts->pkts_per_cmd[i] == 0) {
			continue;
		}

		shell_fprintf(sh, SHELL_INFO,
			      "%2u pkt(s) - %u cmd(s) (%u.%02u%%)\n",
			      i + 1,
			      ts->pkts_per_cmd[i],
			      ts->tx_cmds ?
				      ((ts->pkts_per_cmd[i] * 100U) / ts->tx_cmds) : 0U,
			      ts->tx_cmds ?
				      (((ts->pkts_per_cmd[i] * 10000U) / ts->tx_cmds) % 100U) :
				      0U);
	}

	shell_fprintf(sh, SHELL_INFO,
		      "\n--- Aggregation: token fill level (%% of %u B cap) ---\n",
		      cap);

	for (i = 0; i < TX_TOKEN_FILL_BUCKETS; i++) {
		if (ts->fill_hist[i] == 0) {
			continue;
		}

		shell_fprintf(sh, SHELL_INFO,
			      "%3u-%3u%% - %u cmd(s)\n",
			      i * (100U / TX_TOKEN_FILL_BUCKETS),
			      (i + 1) * (100U / TX_TOKEN_FILL_BUCKETS),
			      ts->fill_hist[i]);
	}

	shell_fprintf(sh, SHELL_INFO,
		      "\nTX cmds: %u, pkts: %u, data: %llu B, occupancy: %llu B, "
		      "max cmd: %u B\n",
		      ts->tx_cmds,
		      ts->tx_cmd_pkts,
		      ts->tx_cmd_data_bytes,
		      ts->tx_cmd_bytes,
		      ts->max_cmd_bytes);
	shell_fprintf(sh, SHELL_INFO,
		      "TX dones: %u, pipe busy: %llu us, idle: %llu us, "
		      "max cmds in flight: %u\n",
		      ts->tx_dones,
		      ts->pipe_busy_us,
		      ts->pipe_idle_us,
		      ts->max_cmds_in_flight);

	if (ts->tx_cmds) {
		unsigned int avg_pkts_x100 = (unsigned int)((ts->tx_cmd_pkts * 100ULL) /
							    ts->tx_cmds);
		unsigned int avg_bytes = (unsigned int)(ts->tx_cmd_bytes / ts->tx_cmds);
		unsigned int avg_pkt_len = ts->tx_cmd_pkts ?
			(unsigned int)(ts->tx_cmd_data_bytes / ts->tx_cmd_pkts) : 0U;

		shell_fprintf(sh, SHELL_INFO,
			      "avg_tx pkts/TX cmd: %u.%02u (of %u), "
			      "avg bytes/TX cmd: %u (%u%% of cap), avg pkt: %u B\n",
			      avg_pkts_x100 / 100U,
			      avg_pkts_x100 % 100U,
			      max_aggr,
			      avg_bytes,
			      cap ? ((avg_bytes * 100U) / cap) : 0U,
			      avg_pkt_len);
	} else {
		shell_fprintf(sh, SHELL_INFO, "avg_tx pkts/TX cmd: n/a\n");
	}

	aggr_stops = ts->aggr_stop_size + ts->aggr_stop_count + ts->aggr_stop_mismatch +
		     ts->aggr_stop_twt + ts->aggr_stop_q_empty;

	shell_fprintf(sh, SHELL_INFO,
		      "\n--- Why aggregation stopped (%u events) ---\n",
		      aggr_stops);
	shell_fprintf(sh, SHELL_INFO,
		      "size cap hit   : %u\n"
		      "count cap hit  : %u\n"
		      "SA/RA mismatch : %u\n"
		      "TWT sleep      : %u\n"
		      "pend_q empty   : %u\n"
		      "forced single  : %u\n",
		      ts->aggr_stop_size,
		      ts->aggr_stop_count,
		      ts->aggr_stop_mismatch,
		      ts->aggr_stop_twt,
		      ts->aggr_stop_q_empty,
		      ts->aggr_forced_single);

	shell_fprintf(sh, SHELL_INFO,
		      "\n--- Per-AC token accounting ---\n"
		      "a token is requested for every packet handed to the driver, so once\n"
		      "the tokens of an AC are in flight every further request is 'busy'\n"
		      "and the packet waits on the pending queue for a TX done; only\n"
		      "'starved' (no token while the AC has nothing in flight) is a fault\n");
	shell_fprintf(sh, SHELL_INFO,
		      "%-4s %9s %7s %10s %8s %8s %9s %7s %7s\n",
		      "ac", "reserved", "spare", "busy", "starved", "in_fl",
		      "max_in_fl", "max_q", "q_full");

	for (i = 0; i < NRF_WIFI_FMAC_AC_MAX; i++) {
		total_gets += ts->reserved_token_get[i] + ts->spare_token_get[i];
		total_busy += ts->token_get_busy[i];
		total_starved += ts->token_get_starved[i];

		shell_fprintf(sh, SHELL_INFO,
			      "%-4s %9u %7u %10u %8u %8u %9u %7u %7u\n",
			      ac_str(i),
			      ts->reserved_token_get[i],
			      ts->spare_token_get[i],
			      ts->token_get_busy[i],
			      ts->token_get_starved[i],
			      sys_dev_ctx->tx_config.outstanding_descs[i],
			      ts->max_outstanding_descs[i],
			      ts->max_pending_qlen[i],
			      ts->pend_q_full_drops[i]);
	}

	shell_fprintf(sh, SHELL_INFO,
		      "token gets: %u, busy: %u, starved: %u, spare AC switches: %u\n",
		      total_gets,
		      total_busy,
		      total_starved,
		      ts->spare_token_ac_switch);
	shell_fprintf(sh, SHELL_INFO,
		      "token re-use: %u.%02u TX done(s) per token get\n",
		      total_gets ? (ts->tx_dones / total_gets) : 0U,
		      total_gets ? (((ts->tx_dones * 100U) / total_gets) % 100U) : 0U);

	shell_fprintf(sh, SHELL_INFO,
		      "\n--- Host TX path ---\n");
	shell_fprintf(sh, SHELL_INFO,
		      "if_send calls: %u, accepted: %u\n",
		      ts->if_send_calls,
		      ts->if_send_accepted);
	shell_fprintf(sh, SHELL_INFO,
		      "of which held on the pending queue (no token/aggregating/PS): %u "
		      "(%u%%)\n",
		      ts->pkts_queued,
		      ts->if_send_accepted ?
			      ((ts->pkts_queued * 100U) / ts->if_send_accepted) : 0U);
	shell_fprintf(sh, SHELL_INFO,
		      "if_send drops: no_nbuf: %u, unknown_peer: %u, not_ready: %u, "
		      "fmac_fail: %u\n",
		      ts->if_drop_no_nbuf,
		      ts->if_drop_unknown_peer,
		      ts->if_drop_not_ready,
		      ts->if_drop_fmac_fail);
	shell_fprintf(sh, SHELL_INFO,
		      "host pkts: tx: %llu, tx_done: %llu, tx_drop: %llu\n",
		      (unsigned long long)sys_dev_ctx->host_stats.total_tx_pkts,
		      (unsigned long long)sys_dev_ctx->host_stats.total_tx_done_pkts,
		      (unsigned long long)sys_dev_ctx->host_stats.total_tx_drop_pkts);
}

static int nrf_wifi_util_tx_stats(const struct shell *sh,
				  size_t argc,
				  const char *argv[])
{
	int vif_index = -1;
	int peer_index = 0;
	int max_vif_index = MAX(MAX_NUM_APS, MAX_NUM_STAS);
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;
	unsigned int tx_pending_pkts = 0;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	bool clear = false;
	bool verbose = false;
	int ret;

	for (size_t i = 2; i < argc; i++) {
		if (strcmp(argv[i], "clear") == 0) {
			clear = true;
		} else if (strcmp(argv[i], "-v") == 0) {
			verbose = true;
		} else {
			shell_fprintf(sh,
				      SHELL_ERROR,
				      "Invalid argument \"%s\".\n",
				      argv[i]);
			shell_help(sh);
			return -ENOEXEC;
		}
	}

	vif_index = atoi(argv[1]);
	if ((vif_index < 0) || (vif_index >= max_vif_index)) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid vif index(%d).\n",
			      vif_index);
		shell_help(sh);
		return -ENOEXEC;
	}

	k_mutex_lock(&ctx->rpu_lock, K_FOREVER);
	if (!ctx->rpu_ctx) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "RPU context not initialized\n");
		ret = -ENOEXEC;
		goto unlock;
	}

	fmac_dev_ctx = ctx->rpu_ctx;
	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	sys_fpriv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	if (clear) {
		memset(&sys_dev_ctx->tx_config.token_stats,
		       0,
		       sizeof(sys_dev_ctx->tx_config.token_stats));
		sys_dev_ctx->tx_config.token_stats.window_start_cyc = k_cycle_get_32();
		shell_fprintf(sh,
			      SHELL_INFO,
			      "TX token stats cleared\n");
		ret = 0;
		goto unlock;
	}

	if (!verbose) {
		tx_token_stats_summary(sh, sys_dev_ctx, sys_fpriv);
		ret = 0;
		goto unlock;
	}

	/* TODO: Get peer_index from shell once AP mode is supported */
	shell_fprintf(sh,
		SHELL_INFO,
		"************* Tx Stats: vif(%d) peer(0) ***********\n",
		vif_index);

	for (int i = 0; i < NRF_WIFI_FMAC_AC_MAX ; i++) {
		tx_pending_pkts = (unsigned int)sys_dlist_len(
			&sys_dev_ctx->tx_config.pend_pkt_q[peer_index][i]);

		shell_fprintf(
			sh,
			SHELL_INFO,
			"Outstanding tokens: ac: %d (%s) -> %d (pending_q_len: %d)\n",
			i,
			ac_str(i),
			sys_dev_ctx->tx_config.outstanding_descs[i],
			tx_pending_pkts);
	}

	tx_token_stats_dump(sh, sys_dev_ctx, sys_fpriv);

	ret = 0;

unlock:
	k_mutex_unlock(&ctx->rpu_lock);
	return ret;
}
#endif /* CONFIG_NRF71_STA_MODE */


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
#ifdef CONFIG_NRF71_STA_MODE
	SHELL_CMD_ARG(tx_stats,
		      NULL,
		      "Displays a TX statistics summary: aggregation, token usage\n"
		      "and what is limiting the TX path\n"
		      "Parameters:\n"
		      "    vif_index: 0 - 1\n"
		      "    -v       : (optional) full per-token, histogram and per-AC dump\n"
		      "    clear    : (optional) reset the TX token counters\n",
		      nrf_wifi_util_tx_stats,
		      2,
		      2),
#endif /* CONFIG_NRF71_STA_MODE */
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
