/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing SoftAP specific definitions for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#include "fmac_ap.h"
#include "fmac_peer.h"
#include "queue.h"
#include "fmac_tx.h"

enum nrf_wifi_status sap_client_ps_get_frames(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					      struct nrf_wifi_sap_ps_get_frames *config)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct peers_info *peer = NULL;
	void *wakeup_client_q = NULL;
	int id = -1;
	int ac = 0;
	int desc = 0;

	if (!fmac_dev_ctx || !config) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid params\n",
				      __func__);
		goto out;
	}

	nrf_wifi_osal_spinlock_take(fmac_dev_ctx->fpriv->opriv,
				    fmac_dev_ctx->tx_config.tx_lock);

	id = nrf_wifi_fmac_peer_get_id(fmac_dev_ctx, config->mac_addr);

	if (id == -1) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid Peer_ID, Mac Addr =%pM\n",
				      __func__,
				      config->mac_addr);

		nrf_wifi_osal_spinlock_rel(fmac_dev_ctx->fpriv->opriv,
					   fmac_dev_ctx->tx_config.tx_lock);
		goto out;
	}


	peer = &fmac_dev_ctx->tx_config.peers[id];
	peer->ps_token_count = config->num_frames;

	wakeup_client_q = fmac_dev_ctx->tx_config.wakeup_client_q;

	if (wakeup_client_q) {
		nrf_wifi_utils_q_enqueue(fmac_dev_ctx->fpriv->opriv,
					 wakeup_client_q,
					 peer);
	}

	for (ac = NRF_WIFI_FMAC_AC_VO; ac >= 0; --ac) {
		desc = tx_desc_get(fmac_dev_ctx, ac);

		if (desc < fmac_dev_ctx->fpriv->num_tx_tokens) {
			tx_pending_process(fmac_dev_ctx, desc, ac);
		}
	}

	nrf_wifi_osal_spinlock_rel(fmac_dev_ctx->fpriv->opriv,
				   fmac_dev_ctx->tx_config.tx_lock);

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}


enum nrf_wifi_status sap_client_update_pmmode(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					      struct nrf_wifi_sap_client_pwrsave *config)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct peers_info *peer = NULL;
	void *wakeup_client_q = NULL;
	int id = -1;
	int ac = 0;
	int desc = 0;

	if (!fmac_dev_ctx || !config) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid params\n",
				      __func__);
		goto out;
	}

	nrf_wifi_osal_spinlock_take(fmac_dev_ctx->fpriv->opriv,
				    fmac_dev_ctx->tx_config.tx_lock);

	id = nrf_wifi_fmac_peer_get_id(fmac_dev_ctx,
				       config->mac_addr);

	if (id == -1) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid Peer_ID, Mac address = %pM\n",
				      __func__,
				      config->mac_addr);

		nrf_wifi_osal_spinlock_rel(fmac_dev_ctx->fpriv->opriv,
					   fmac_dev_ctx->tx_config.tx_lock);

		goto out;
	}


	peer = &fmac_dev_ctx->tx_config.peers[id];
	peer->ps_state = config->sta_ps_state;

	if (peer->ps_state == NRF_WIFI_CLIENT_ACTIVE) {
		wakeup_client_q = fmac_dev_ctx->tx_config.wakeup_client_q;

		if (wakeup_client_q) {
			nrf_wifi_utils_q_enqueue(fmac_dev_ctx->fpriv->opriv,
						 wakeup_client_q,
						 peer);
		}

		for (ac = NRF_WIFI_FMAC_AC_VO; ac >= 0; --ac) {
			desc = tx_desc_get(fmac_dev_ctx, ac);

			if (desc < fmac_dev_ctx->fpriv->num_tx_tokens) {
				tx_pending_process(fmac_dev_ctx, desc, ac);
			}
		}
	}

	nrf_wifi_osal_spinlock_rel(fmac_dev_ctx->fpriv->opriv,
				   fmac_dev_ctx->tx_config.tx_lock);

	status = NRF_WIFI_STATUS_SUCCESS;

out:
	return status;
}
