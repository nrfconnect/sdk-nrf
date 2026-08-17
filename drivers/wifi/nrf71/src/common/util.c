/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing utility function definitions for the
 * Wi-Fi driver.
 */

#include <common/mem_mgmt.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <common/util.h>
#include <common/fmac_structs_common.h>
#include <nrf71_wifi_ctrl.h>
#ifdef NRF71_SYSTEM_MODE
#include <common/nbuf_mgmt.h>
#include <system/fmac_api.h>
#endif /* NRF71_SYSTEM_MODE */
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

int nrf_wifi_utils_hex_str_to_val(unsigned char *hex_arr,
				  unsigned int hex_arr_sz,
				  unsigned char *str)
{
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned char ch = 0;
	unsigned char val = 0;
	unsigned int len = 0;
	int ret = -1;

	len = strlen((const char *)str);

	if (len / 2 > hex_arr_sz) {
		LOG_ERR("%s: String length (%d) greater than array size (%d)",
				      __func__,
				      len,
				      hex_arr_sz);
		goto out;
	}

	if (len % 2) {
		LOG_ERR("%s:String length = %d, is not a multiple of 2",
				      __func__,
				      len);
		goto out;
	}

	for (i = 0; i < len; i++) {
		/* Convert each character to lower case */
		ch = ((str[i] >= 'A' && str[i] <= 'Z') ? str[i] + 32 : str[i]);

		if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f')) {
			LOG_ERR("%s: Invalid hex character in string %d",
					      __func__,
					      ch);
			goto out;
		}

		if (ch >= '0' && ch <= '9') {
			ch = ch - '0';
		} else {
			ch = ch - 'a' + 10;
		}

		val += ch;

		if (!(i % 2)) {
			val <<= 4;
		} else {
			hex_arr[j] = val;
			j++;
			val = 0;
		}
	}

	ret = j;
out:
	return ret;
}


bool nrf_wifi_utils_is_mac_addr_valid(const char *mac_addr)
{
	unsigned char zero_addr[NRF_WIFI_ETH_ADDR_LEN] = {0};

	return (mac_addr &&
		(nrf_wifi_mem_cmp(mac_addr,
				       zero_addr,
				       sizeof(zero_addr)) != 0) &&
		!(mac_addr[0] & 0x1));
}


int nrf_wifi_utils_chan_to_freq(enum nrf_wifi_band band,
				unsigned short chan)
{
	int freq = -1;
	unsigned short valid_5g_chans[] = {32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 96, 100, 104,
		108, 112, 116, 120, 124, 128, 132, 136, 140, 144, 149, 153, 157, 159, 161, 163,
		165, 167, 169, 171, 173, 175, 177};
	unsigned char i = 0;

	switch (band) {
	case NRF_WIFI_BAND_2GHZ:
		if ((chan >= 1) && (chan <= 13)) {
			freq = (((chan - 1) * 5) + 2412);
		} else if (chan == 14) {
			freq = 2484;
		} else {
			LOG_ERR("%s: Invalid channel value %d",
					      __func__,
					      chan);
			goto out;
		}
		break;
	case NRF_WIFI_BAND_5GHZ:
		for (i = 0; i < ARRAY_SIZE(valid_5g_chans); i++) {
			if (chan == valid_5g_chans[i]) {
				freq = (chan * 5) + 5000;
				break;
			}
		}

		break;
	default:
		LOG_ERR("%s: Invalid band value %d",
				      __func__,
				      band);
		goto out;
	}
out:
	return freq;

}

/**
 * @brief Get operating band bitmap from Kconfig (see NRF_WIFI_OP_BAND_* in
 * nrf71_wifi_ctrl.h / nrf71_wifi_common.h).
 *
 * @return Bitmap of bands (NRF_WIFI_OP_BAND_2GHZ, NRF_WIFI_OP_BAND_5GHZ,
 *         NRF_WIFI_OP_BAND_6GHZ).
 */
unsigned char nrf_wifi_utils_get_op_band(void)
{
	if (IS_ENABLED(CONFIG_NRF_WIFI_2G_BAND)) {
		return NRF_WIFI_OP_BAND_2GHZ;
	}
	if (IS_ENABLED(CONFIG_NRF_WIFI_5G_BAND)) {
		return NRF_WIFI_OP_BAND_5GHZ;
	}
	if (IS_ENABLED(CONFIG_NRF_WIFI_6G_BAND)) {
		return NRF_WIFI_OP_BAND_6GHZ;
	}
	if (IS_ENABLED(CONFIG_NRF_WIFI_DUAL_BAND)) {
		return NRF_WIFI_OP_BAND_2GHZ | NRF_WIFI_OP_BAND_5GHZ;
	}
	return NRF_WIFI_OP_BAND_2GHZ | NRF_WIFI_OP_BAND_5GHZ | NRF_WIFI_OP_BAND_6GHZ;
}

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
	unsigned char *payload = (unsigned char *)nwb;

	return payload[6] << 8 | payload[7];
}

unsigned short nrf_wifi_util_tx_get_eth_type(void *nwb)
{
	unsigned char *payload = (unsigned char *)nwb;

	return payload[12] << 8 | payload[13];
}

enum nrf_wifi_status nrf_wifi_check_mode_validity(unsigned char mode)
{
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
	for (unsigned int i = 0; i < arr_sz; i++) {
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

	return nrf_wifi_nbuf_data_get(nwb);
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
