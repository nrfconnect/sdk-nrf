/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing utility function definitions for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#include "osal_api.h"
#include "common/fmac_api_common.h"
#include "common/fmac_util.h"

#include <nrf71_wifi_ctrl.h>
#ifdef NRF71_SYSTEM_MODE
#include <system/fmac_structs.h>
#endif /* NRF71_SYSTEM_MODE */

bool nrf_wifi_util_is_multicast_addr(const unsigned char *addr)
{
	return (0x01 & *addr);
}


bool nrf_wifi_util_is_unicast_addr(const unsigned char *addr)
{
	return !nrf_wifi_util_is_multicast_addr(addr);
}


bool nrf_wifi_util_ether_addr_equal(const unsigned char *addr_1,
				    const unsigned char *addr_2)
{
	const unsigned short *a = (const unsigned short *)addr_1;
	const unsigned short *b = (const unsigned short *)addr_2;

	return ((a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2])) == 0;
}

unsigned short nrf_wifi_util_rx_get_eth_type(void *nwb)
{
	unsigned char *payload = NULL;

	payload = (unsigned char *)nwb;

	return payload[6] << 8 | payload[7];
}


unsigned short nrf_wifi_util_tx_get_eth_type(void *nwb)
{
	unsigned char *payload = NULL;

	payload = (unsigned char *)nwb;

	return payload[12] << 8 | payload[13];
}


enum nrf_wifi_status nrf_wifi_check_mode_validity(unsigned char mode)
{
	/**
	 * We validate the currently supported driver and lower layer
	 * modes only
	 */
	if ((mode ^ NRF_WIFI_STA_MODE) == 0) {
		return NRF_WIFI_STATUS_SUCCESS;
	}
#ifdef NRF71_RAW_DATA_RX
	else if ((mode ^ NRF_WIFI_MONITOR_MODE) == 0) {
		return NRF_WIFI_STATUS_SUCCESS;
	}
#endif /* NRF71_RAW_DATA_RX */
	return NRF_WIFI_STATUS_FAIL;
}

bool nrf_wifi_util_is_arr_zero(unsigned char *arr,
			       unsigned int arr_sz)
{
	unsigned int i = 0;

	for (i = 0; i < arr_sz; i++) {
		if (arr[i] != 0) {
			return false;
		}
	}

	return true;
}

#ifdef NRF71_SYSTEM_MODE
unsigned char *nrf_wifi_util_get_ra(struct nrf_wifi_fmac_vif_ctx *vif,
				    void *nwb)
{
	if ((vif->if_type == NRF_WIFI_IFTYPE_STATION)
#ifdef NRF71_RAW_DATA_TX
	    || (vif->if_type == NRF_WIFI_STA_TX_INJECTOR)
#endif /* NRF71_RAW_DATA_TX */
#ifdef NRF71_PROMISC_DATA_RX
	    || (vif->if_type == NRF_WIFI_STA_PROMISC)
	    || (vif->if_type == NRF_WIFI_STA_PROMISC_TX_INJECTOR)
#endif
	    ) {
		return vif->bssid;
	}

	return nrf_wifi_osal_nbuf_data_get(nwb);
}
#endif /* NRF71_SYSTEM_MODE */

void *wifi_fmac_priv(struct nrf_wifi_fmac_priv *def)
{
	return &def->priv;
}

void *wifi_dev_priv(struct nrf_wifi_fmac_dev_ctx *def)
{
	return &def->priv;
}
