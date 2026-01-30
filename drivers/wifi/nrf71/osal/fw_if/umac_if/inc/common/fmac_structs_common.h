/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 *
 * @addtogroup nrf_wifi_api_common FMAC API common
 * @{
 *
 * @brief Header containing declarations for utility functions for
 * FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_STRUCTS_COMMON_H__
#define __FMAC_STRUCTS_COMMON_H__

#include "osal_api.h"
#include <nrf71_wifi_ctrl.h>

#define NRF_WIFI_FW_CHUNK_ID_STR_LEN 16

/** @brief Device operation modes. */
enum nrf_wifi_op_mode {
	/** System mode. */
	NRF_WIFI_OP_MODE_SYS = 0,
	/** Radio test mode */
	NRF_WIFI_OP_MODE_RT,
	/** Offloaded raw TX mode */
	NRF_WIFI_OP_MODE_OFF_RAW_TX,

/** @cond INTERNAL_HIDDEN */
	__NRF_WIFI_OP_MODE_AFTER_LAST,
	NRF_WIFI_OP_MODE_MAX = __NRF_WIFI_OP_MODE_AFTER_LAST - 1,
	NRF_WIFI_OP_MODE_UNKNOWN
/** @endcond */
};

/**
 * @brief Structure to hold host specific statistics.
 *
 */
struct rpu_host_stats {
	/** Total number of TX frames transmitted. */
	unsigned long long total_tx_pkts;
	/** Total number of TX dones received. */
	unsigned long long total_tx_done_pkts;
	/** Total number of TX frames dropped. */
	unsigned long long total_tx_drop_pkts;
	/** Total number of RX frames received. */
	unsigned long long total_rx_pkts;
	/** Total number of RX frames dropped. */
	unsigned long long total_rx_drop_pkts;
};


/**
 * @brief Structure to hold FW patch information.
 *
 */
struct nrf_wifi_fw_info {
	/** Pointer to the FW patch data. */
	const void *data;
	/** Size of the FW patch data. */
	unsigned int size;
};


/**
 * @brief Structure to hold FW patch information for LMAC and UMAC.
 *
 */
struct nrf_wifi_fmac_fw_info {
	/** Primary LMAC FW patch information. */
	struct nrf_wifi_fw_info lmac_patch_pri;
	/** Secondary LMAC FW patch information. */
	struct nrf_wifi_fw_info lmac_patch_sec;
	/** Primary UMAC FW patch information. */
	struct nrf_wifi_fw_info umac_patch_pri;
	/** Secondary UMAC FW patch information. */
	struct nrf_wifi_fw_info umac_patch_sec;
};

/**
 * @brief Structure to hold FW patch chunk information.
 *
 */
struct nrf_wifi_fmac_fw_chunk_info {
	/** Pointer to the FW patch chunk ID string. */
	char id_str[NRF_WIFI_FW_CHUNK_ID_STR_LEN];
	/** Pointer to the FW patch chunk data. */
	const void *data;
	/** Size of the FW patch chunk data. */
	unsigned int size;
	/** Destination address of the FW patch chunk data. */
	unsigned int dest_addr;
};

/* Maximum number of channels supported in a regulatory
 * currently set to 42 as hardware supports 2.4GHz and 5GHz.
 * In 2.4 GHz band maximum 14 channels and
 * in 5 GHz band maximum 28 channels are supported.
 * Maximum number of channels will change if more number
 * of channels are enabled in different bands or
 * different bands are supported in hardware.
 */
#define MAX_NUM_REG_CHANELS 42

/**
 * @brief Structure to hold Regulatory parameter data.
 *
 */
struct nrf_wifi_fmac_reg_info {
	/** ISO IEC Country code. */
	unsigned char alpha2[NRF_WIFI_COUNTRY_CODE_LEN];
	 /** Forcefully set regulatory. */
	bool force;
	/** Regulatory channels count*/
	unsigned int reg_chan_count;
	/** Regulatory channel attributes */
	struct nrf_wifi_get_reg_chn_info reg_chan_info[MAX_NUM_REG_CHANELS];
};

/**
 * @brief Structure to hold common fmac priv parameter data.
 *
 */
struct nrf_wifi_fmac_priv {
	/** Handle to the HAL layer. */
	struct nrf_wifi_hal_priv *hpriv;
	/** Operation mode. \ref nrf_wifi_op_mode */
	int op_mode;
	/** Data pointer to mode specific parameters */
	char priv[];
};

/**
 * @brief Structure to hold common fmac dev context parameter data.
 *
 */
struct nrf_wifi_fmac_dev_ctx {
	/** Handle to the FMAC IF abstraction layer. */
	struct nrf_wifi_fmac_priv *fpriv;
	/** Handle to the OS abstraction layer. */
	void *os_dev_ctx;
	/** Handle to the HAL layer. */
	void *hal_dev_ctx;
	/** Operation mode. \ref nrf_wifi_op_mode */
	int op_mode;
	/** Firmware statistics. */
	void *fw_stats;
	/** Firmware statistics requested. */
	bool stats_req;
	/** Firmware boot done. */
	bool fw_boot_done;
	/** Firmware init done. */
	bool fw_init_done;
	/** Firmware deinit done. */
	bool fw_deinit_done;
	/** Alpha2 valid. */
	bool alpha2_valid;
	/** Alpha2 country code, last byte is reserved for null character. */
	unsigned char alpha2[3];
	/** Regulatory channels count*/
	unsigned int reg_chan_count;
	/** Regulatory channel attributes */
	struct nrf_wifi_get_reg_chn_info *reg_chan_info;
	/** To determine if event is solicited or not */
	bool waiting_for_reg_event;
	/** Regulatory set status */
	int reg_set_status;
	/** Regulatory change event */
	struct nrf_wifi_event_regulatory_change *reg_change;
	/** TX power ceiling parameters */
	struct nrf_wifi_tx_pwr_ceil_params *tx_pwr_ceil_params;
	/** Data pointer to mode specific parameters */
	char priv[];
};
/**
 * @}
 */
#endif /* __FMAC_STRUCTS_COMMON_H__ */
