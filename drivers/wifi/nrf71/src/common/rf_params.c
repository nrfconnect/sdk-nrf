/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing RF parameters for the Wi-Fi driver.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

#include <common/mem_mgmt.h>
#include <common/fw_if/nrf71_wifi_rf.h>
#include <common/rf_params.h>
#include <util.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);


/* The RF front-end description lives on the Wi-Fi node, the parent of the wlan
 * interface nodes. Look it up by compatible so it cannot silently bind to the
 * wrong node.
 */
#define NRF71_WIFI_NODE DT_INST(0, nordic_nrf7120_wifi)

BUILD_ASSERT(DT_SAME_NODE(NRF71_WIFI_NODE, DT_PARENT(DT_INST(0, nordic_wlan))),
	     "RF front-end parameters must come from the Wi-Fi node this driver is bound to");

/* The UMAC structure uses 0.25 dBm, while DTS and the RF parameter strings
 * both use 1 dBm.
 */
#define MAX_TX_PWR(label) (DT_PROP(NRF71_WIFI_NODE, label) * 4)
#define MAX_TX_PWR_DBM(label) DT_PROP(NRF71_WIFI_NODE, label)

/* Antenna gain in the UMAC structure is unsigned, so a negative net gain is
 * passed as 0. The RF parameter fields are signed and take it as it is.
 */
#define ANT_GAIN_UMAC(label) \
	(DT_PROP(NRF71_WIFI_NODE, label) < 0 ? 0 : DT_PROP(NRF71_WIFI_NODE, label))

void configure_tx_pwr_settings(struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl_params,
			       struct nrf_wifi_tx_pwr_ceil_params *tx_pwr_ceil_params)
{
	memset(tx_pwr_ctrl_params, 0, sizeof(*tx_pwr_ctrl_params));

	tx_pwr_ctrl_params->ant_gain_2g = ANT_GAIN_UMAC(nordic_wifi_ant_gain_2g);
	tx_pwr_ctrl_params->ant_gain_5g_band1 = ANT_GAIN_UMAC(nordic_wifi_ant_gain_5g_band1);
	tx_pwr_ctrl_params->ant_gain_5g_band2 = ANT_GAIN_UMAC(nordic_wifi_ant_gain_5g_band2);
	tx_pwr_ctrl_params->ant_gain_5g_band3 = ANT_GAIN_UMAC(nordic_wifi_ant_gain_5g_band3);
	tx_pwr_ctrl_params->ant_gain_6g_band1 = ANT_GAIN_UMAC(nordic_wifi_ant_gain_6g_band1);
	tx_pwr_ctrl_params->ant_gain_6g_band2 = ANT_GAIN_UMAC(nordic_wifi_ant_gain_6g_band2);
	tx_pwr_ctrl_params->ant_gain_6g_band3 = ANT_GAIN_UMAC(nordic_wifi_ant_gain_6g_band3);
	tx_pwr_ctrl_params->ant_gain_6g_band4 = ANT_GAIN_UMAC(nordic_wifi_ant_gain_6g_band4);
	tx_pwr_ctrl_params->ant_gain_6g_band5 = ANT_GAIN_UMAC(nordic_wifi_ant_gain_6g_band5);
	tx_pwr_ctrl_params->ant_gain_6g_band6 = ANT_GAIN_UMAC(nordic_wifi_ant_gain_6g_band6);

	tx_pwr_ceil_params->max_pwr_2g_dsss = MAX_TX_PWR(wifi_max_tx_pwr_2g_dsss);
	tx_pwr_ceil_params->max_pwr_2g_mcs0 = MAX_TX_PWR(wifi_max_tx_pwr_2g_mcs0);
	tx_pwr_ceil_params->max_pwr_2g_mcs7 = MAX_TX_PWR(wifi_max_tx_pwr_2g_mcs7);
	tx_pwr_ceil_params->max_pwr_5g_low_mcs0 = MAX_TX_PWR(wifi_max_tx_pwr_5g_low_mcs0);
	tx_pwr_ceil_params->max_pwr_5g_low_mcs7 = MAX_TX_PWR(wifi_max_tx_pwr_5g_low_mcs7);
	tx_pwr_ceil_params->max_pwr_5g_mid_mcs0 = MAX_TX_PWR(wifi_max_tx_pwr_5g_mid_mcs0);
	tx_pwr_ceil_params->max_pwr_5g_mid_mcs7 = MAX_TX_PWR(wifi_max_tx_pwr_5g_mid_mcs7);
	tx_pwr_ceil_params->max_pwr_5g_high_mcs0 = MAX_TX_PWR(wifi_max_tx_pwr_5g_high_mcs0);
	tx_pwr_ceil_params->max_pwr_5g_high_mcs7 = MAX_TX_PWR(wifi_max_tx_pwr_5g_high_mcs7);
}

static struct rf_hex_param rf_params[NUM_WIFI_PARAMS] = {
	{NRF_WIFI_PARAMS1, NULL, 0},  {NRF_WIFI_PARAMS2, NULL, 0},  {NRF_WIFI_PARAMS3, NULL, 0},
	{NRF_WIFI_PARAMS4, NULL, 0},  {NRF_WIFI_PARAMS5, NULL, 0},  {NRF_WIFI_PARAMS6, NULL, 0},
	{NRF_WIFI_PARAMS7, NULL, 0},  {NRF_WIFI_PARAMS8, NULL, 0},  {NRF_WIFI_PARAMS9, NULL, 0},
	{NRF_WIFI_PARAMS10, NULL, 0}, {NRF_WIFI_PARAMS11, NULL, 0}, {NRF_WIFI_PARAMS12, NULL, 0},
	{NRF_WIFI_PARAMS13, NULL, 0}, {NRF_WIFI_PARAMS14, NULL, 0}, {NRF_WIFI_PARAMS15, NULL, 0},
	{NRF_WIFI_PARAMS16, NULL, 0}, {NRF_WIFI_PARAMS17, NULL, 0}, {NRF_WIFI_PARAMS18, NULL, 0},
	{NRF_WIFI_PARAMS19, NULL, 0}, {NRF_WIFI_PARAMS20, NULL, 0}, {NRF_WIFI_PARAMS21, NULL, 0},
	{NRF_WIFI_PARAMS22, NULL, 0},
};

/* Flat byte offsets of the RF parameter categories that carry board dependent
 * values, taken from the RF parameter definition. The ANT_GAIN_OFFSETS,
 * EDGE_BACKOFF_OFFSETS and PCB_LOSS_BYTE_OFFSETS enums in the firmware
 * interface headers describe an older flat layout and must not be used: their
 * offsets land inside the RF and calibration category.
 */
#define RF_PARAMS_TX_PWR_CEIL_OFST 8
#define RF_PARAMS_EDGE_CEIL_OFST   243
#define RF_PARAMS_ANT_GAIN_OFST    278

/* Indices within the TX power ceiling category. MCS9 is unsupported, so those
 * entries keep the values the default parameter strings carry.
 */
#define TX_PWR_CEIL_2G_DSSS    0
#define TX_PWR_CEIL_2G_MCS7    2
#define TX_PWR_CEIL_2G_MCS0    3
#define TX_PWR_CEIL_5G_MCS7    7
#define TX_PWR_CEIL_5G_MCS0    10
#define TX_PWR_CEIL_6G_MCS7    19
#define TX_PWR_CEIL_6G_MCS0    25

/* Number of 6 GHz sub-bands in the TX power ceiling and antenna gain
 * categories. The edge ceilings use two, UNII-5 and UNII-6, instead.
 */
#define NUM_6G_SUB_BANDS       6

/* Return the byte at a flat offset in the RF parameter set, which the firmware
 * receives as NUM_WIFI_PARAMS separate buffers.
 */
static uint8_t *rf_params_byte(unsigned int flat_ofst)
{
	unsigned int base = 0;
	int index;

	for (index = 0; index < NUM_WIFI_PARAMS; index++) {
		if (!rf_params[index].bytes) {
			continue;
		}

		if (flat_ofst < base + (unsigned int)rf_params[index].bytes_len) {
			return &rf_params[index].bytes[flat_ofst - base];
		}

		base += (unsigned int)rf_params[index].bytes_len;
	}

	return NULL;
}

static enum nrf_wifi_status rf_params_set(unsigned int flat_ofst, uint8_t val)
{
	uint8_t *byte = rf_params_byte(flat_ofst);

	if (!byte) {
		LOG_ERR("%s: RF parameter offset %u out of range", __func__, flat_ofst);
		return NRF_WIFI_STATUS_FAIL;
	}

	*byte = val;

	return NRF_WIFI_STATUS_SUCCESS;
}

/* Antenna gain fields are signed 4-bit, two per byte, low nibble first. */
static enum nrf_wifi_status rf_params_set_nibble(unsigned int index, int8_t val)
{
	uint8_t *byte = rf_params_byte(RF_PARAMS_ANT_GAIN_OFST + index / 2);

	if (!byte) {
		LOG_ERR("%s: antenna gain index %u out of range", __func__, index);
		return NRF_WIFI_STATUS_FAIL;
	}

	if (index % 2) {
		*byte = (*byte & 0x0F) | (uint8_t)((val & 0x0F) << 4);
	} else {
		*byte = (*byte & 0xF0) | (uint8_t)(val & 0x0F);
	}

	return NRF_WIFI_STATUS_SUCCESS;
}

/* Apply the board dependent RF front-end values from the Wi-Fi node in the
 * board devicetree on top of the default parameter strings. MCS9 is
 * unsupported, so those entries are left as the defaults have them.
 *
 * The category 18 CRC is deliberately not recomputed: the firmware ignores it.
 */
static enum nrf_wifi_status rf_params_apply_board_values(void)
{
	static const uint8_t tx_pwr_ceil[] = {
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_2g_dsss),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_2g_mcs7),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_2g_mcs0),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_5g_low_mcs7),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_5g_mid_mcs7),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_5g_high_mcs7),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_5g_low_mcs0),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_5g_mid_mcs0),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_5g_high_mcs0),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_6g_band1_mcs7),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_6g_band2_mcs7),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_6g_band3_mcs7),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_6g_band4_mcs7),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_6g_band5_mcs7),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_6g_band6_mcs7),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_6g_band1_mcs0),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_6g_band2_mcs0),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_6g_band3_mcs0),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_6g_band4_mcs0),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_6g_band5_mcs0),
		MAX_TX_PWR_DBM(wifi_max_tx_pwr_6g_band6_mcs0),
	};
	static const uint8_t tx_pwr_ceil_idx[] = {
		TX_PWR_CEIL_2G_DSSS,
		TX_PWR_CEIL_2G_MCS7,
		TX_PWR_CEIL_2G_MCS0,
		TX_PWR_CEIL_5G_MCS7, TX_PWR_CEIL_5G_MCS7 + 1, TX_PWR_CEIL_5G_MCS7 + 2,
		TX_PWR_CEIL_5G_MCS0, TX_PWR_CEIL_5G_MCS0 + 1, TX_PWR_CEIL_5G_MCS0 + 2,
		TX_PWR_CEIL_6G_MCS7 + 0, TX_PWR_CEIL_6G_MCS7 + 1, TX_PWR_CEIL_6G_MCS7 + 2,
		TX_PWR_CEIL_6G_MCS7 + 3, TX_PWR_CEIL_6G_MCS7 + 4, TX_PWR_CEIL_6G_MCS7 + 5,
		TX_PWR_CEIL_6G_MCS0 + 0, TX_PWR_CEIL_6G_MCS0 + 1, TX_PWR_CEIL_6G_MCS0 + 2,
		TX_PWR_CEIL_6G_MCS0 + 3, TX_PWR_CEIL_6G_MCS0 + 4, TX_PWR_CEIL_6G_MCS0 + 5,
	};
	static const uint8_t edge_ceil[] = {
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_2g_lower_edge_ceiling_dsss),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_2g_lower_edge_ceiling_ht),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_2g_lower_edge_ceiling_he),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_2g_upper_edge_ceiling_dsss),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_2g_upper_edge_ceiling_ht),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_2g_upper_edge_ceiling_he),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_1_lower_edge_ceiling_ht),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_1_lower_edge_ceiling_he),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_1_upper_edge_ceiling_ht),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_1_upper_edge_ceiling_he),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_2a_lower_edge_ceiling_ht),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_2a_lower_edge_ceiling_he),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_2a_upper_edge_ceiling_ht),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_2a_upper_edge_ceiling_he),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_2c_lower_edge_ceiling_ht),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_2c_lower_edge_ceiling_he),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_2c_upper_edge_ceiling_ht),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_2c_upper_edge_ceiling_he),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_3_lower_edge_ceiling_ht),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_3_lower_edge_ceiling_he),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_3_upper_edge_ceiling_ht),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_3_upper_edge_ceiling_he),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_4_lower_edge_ceiling_ht),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_4_lower_edge_ceiling_he),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_4_upper_edge_ceiling_ht),
			DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_4_upper_edge_ceiling_he),
		DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_5_lower_edge_ceiling_ht),
		DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_5_lower_edge_ceiling_he),
		DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_5_upper_edge_ceiling_ht),
		DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_5_upper_edge_ceiling_he),
		DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_6_lower_edge_ceiling_ht),
		DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_6_lower_edge_ceiling_he),
		DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_6_upper_edge_ceiling_ht),
		DT_PROP(NRF71_WIFI_NODE, nordic_wifi_unii_6_upper_edge_ceiling_he),
	};
	static const int8_t ant_gain[] = {
		DT_PROP(NRF71_WIFI_NODE, nordic_wifi_ant_gain_2g),
		DT_PROP(NRF71_WIFI_NODE, nordic_wifi_ant_gain_5g_band1),
		DT_PROP(NRF71_WIFI_NODE, nordic_wifi_ant_gain_5g_band2),
		DT_PROP(NRF71_WIFI_NODE, nordic_wifi_ant_gain_5g_band3),
		DT_PROP(NRF71_WIFI_NODE, nordic_wifi_ant_gain_6g_band1),
		DT_PROP(NRF71_WIFI_NODE, nordic_wifi_ant_gain_6g_band2),
		DT_PROP(NRF71_WIFI_NODE, nordic_wifi_ant_gain_6g_band3),
		DT_PROP(NRF71_WIFI_NODE, nordic_wifi_ant_gain_6g_band4),
		DT_PROP(NRF71_WIFI_NODE, nordic_wifi_ant_gain_6g_band5),
		DT_PROP(NRF71_WIFI_NODE, nordic_wifi_ant_gain_6g_band6),
	};
	unsigned int i;

	BUILD_ASSERT(ARRAY_SIZE(tx_pwr_ceil) == ARRAY_SIZE(tx_pwr_ceil_idx),
		     "TX power ceiling values and their indices must stay in step");

	for (i = 0; i < ARRAY_SIZE(tx_pwr_ceil); i++) {
		if (rf_params_set(RF_PARAMS_TX_PWR_CEIL_OFST + tx_pwr_ceil_idx[i],
				  tx_pwr_ceil[i]) != NRF_WIFI_STATUS_SUCCESS) {
			return NRF_WIFI_STATUS_FAIL;
		}
	}

	for (i = 0; i < ARRAY_SIZE(edge_ceil); i++) {
		if (rf_params_set(RF_PARAMS_EDGE_CEIL_OFST + i,
				  edge_ceil[i]) != NRF_WIFI_STATUS_SUCCESS) {
			return NRF_WIFI_STATUS_FAIL;
		}
	}

	for (i = 0; i < ARRAY_SIZE(ant_gain); i++) {
		if (rf_params_set_nibble(i, ant_gain[i]) != NRF_WIFI_STATUS_SUCCESS) {
			return NRF_WIFI_STATUS_FAIL;
		}
	}

	return NRF_WIFI_STATUS_SUCCESS;
}

enum nrf_wifi_status nrf_wifi_fmac_config_rf_params(void *dev_ctx, unsigned int *rf_params_addr)
{
	int index;
	int cleanup_idx;
	size_t str_len;
	int ret;

	for (index = 0; index < NUM_WIFI_PARAMS; index++) {
		if (!rf_params[index].hex_str) {
			continue;
		}
		str_len = strlen(rf_params[index].hex_str);
		rf_params[index].bytes = nrf_wifi_mem_alloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, str_len);
		if (!rf_params[index].bytes) {
			LOG_ERR("%s: Unable to allocate %zu bytes", __func__, str_len);
			goto cleanup;
		}

		ret = nrf_wifi_utils_hex_str_to_val(rf_params[index].bytes, (unsigned int)str_len,
						    (unsigned char *)rf_params[index].hex_str);
		if (ret < 0) {
			LOG_ERR("%s: hex_str_to_val failed", __func__);
			nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, rf_params[index].bytes);
			rf_params[index].bytes = NULL;
			goto cleanup;
		}

		rf_params[index].bytes_len = ret;
		rf_params_addr[index] = (unsigned int)rf_params[index].bytes;
	}
	if (rf_params_apply_board_values() != NRF_WIFI_STATUS_SUCCESS) {
		goto cleanup;
	}

	return NRF_WIFI_STATUS_SUCCESS;

cleanup:
	for (cleanup_idx = 0; cleanup_idx < index; cleanup_idx++) {
		if (rf_params[cleanup_idx].bytes) {
			nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL,
					  rf_params[cleanup_idx].bytes);
			rf_params[cleanup_idx].bytes = NULL;
		}
	}
	return NRF_WIFI_STATUS_FAIL;
}
