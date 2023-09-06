/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing VIF handling specific definitions for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#include "fmac_vif.h"
#include "host_rpu_umac_if.h"
#include "fmac_util.h"


int wifi_nrf_fmac_vif_check_if_limit(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				     int if_type)
{
	struct wifi_nrf_fmac_dev_ctx_def *def_dev_ctx;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	switch (if_type) {
	case NRF_WIFI_IFTYPE_STATION:
	case NRF_WIFI_IFTYPE_P2P_CLIENT:
		if (def_dev_ctx->num_sta >= MAX_NUM_STAS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Maximum STA Interface type exceeded\n",
					      __func__);
			return -1;
		}
		break;
	case NRF_WIFI_IFTYPE_AP:
	case NRF_WIFI_IFTYPE_P2P_GO:
		if (def_dev_ctx->num_ap >= MAX_NUM_APS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Maximum AP Interface type exceeded\n",
					      __func__);
			return -1;
		}
		break;
	default:
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Interface type not supported\n",
				      __func__);
		return -1;
	}

	return 0;
}


void wifi_nrf_fmac_vif_incr_if_type(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				    int if_type)
{
	struct wifi_nrf_fmac_dev_ctx_def *def_dev_ctx;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	switch (if_type) {
	case NRF_WIFI_IFTYPE_STATION:
	case NRF_WIFI_IFTYPE_P2P_CLIENT:
		def_dev_ctx->num_sta++;
		break;
	case NRF_WIFI_IFTYPE_AP:
	case NRF_WIFI_IFTYPE_P2P_GO:
		def_dev_ctx->num_ap++;
		break;
	default:
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s:Unsupported VIF type\n",
				      __func__);
	}
}


void wifi_nrf_fmac_vif_decr_if_type(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				    int if_type)
{
	struct wifi_nrf_fmac_dev_ctx_def *def_dev_ctx;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	switch (if_type) {
	case NRF_WIFI_IFTYPE_STATION:
	case NRF_WIFI_IFTYPE_P2P_CLIENT:
		def_dev_ctx->num_sta--;
		break;
	case NRF_WIFI_IFTYPE_AP:
	case NRF_WIFI_IFTYPE_P2P_GO:
		def_dev_ctx->num_ap--;
		break;
	default:
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s:Unsupported VIF type\n",
				      __func__);
	}
}


void wifi_nrf_fmac_vif_clear_ctx(void *dev_ctx,
				 unsigned char if_idx)
{
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;
	struct wifi_nrf_fmac_dev_ctx_def *def_dev_ctx;

	fmac_dev_ctx = dev_ctx;
	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	vif_ctx = def_dev_ctx->vif_ctx[if_idx];

	wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
			       vif_ctx);
	def_dev_ctx->vif_ctx[if_idx] = NULL;
}


void wifi_nrf_fmac_vif_update_if_type(void *dev_ctx,
				      unsigned char if_idx,
				      int if_type)
{
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;
	struct wifi_nrf_fmac_dev_ctx_def *def_dev_ctx = NULL;

	fmac_dev_ctx = dev_ctx;
	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	vif_ctx = def_dev_ctx->vif_ctx[if_idx];

	wifi_nrf_fmac_vif_decr_if_type(fmac_dev_ctx,
				       vif_ctx->if_type);

	wifi_nrf_fmac_vif_incr_if_type(fmac_dev_ctx,
				       if_type);

	vif_ctx->if_type = if_type;
}

unsigned int wifi_nrf_fmac_get_num_vifs(void *dev_ctx)
{
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = dev_ctx;
	struct wifi_nrf_fmac_dev_ctx_def *def_dev_ctx = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (!fmac_dev_ctx) {
		/* Don't return error here, as FMAC might be initialized based
		 * the number of VIFs.
		 */
		return 0;
	}

	return def_dev_ctx->num_sta + def_dev_ctx->num_ap;
}
