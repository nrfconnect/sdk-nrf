/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *
 * @addtogroup nrf700x_fmac_api_common FMAC API common
 * @{
 *
 * @brief Header containing declarations for utility functions for
 * FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_STRUCTS_COMMON_H__
#define __FMAC_STRUCTS_COMMON_H__

#include <stdbool.h>

#include "osal_api.h"
#include "host_rpu_umac_if.h"

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
 * @brief - Structure to hold per device host and firmware statistics.
 *
 */
struct rpu_op_stats {
	/** Host statistics. */
	struct rpu_host_stats host;
	/** Firmware statistics. */
	struct rpu_fw_stats fw;
};

/**
 * @brief Structure to hold FW patch information.
 *
 */
struct wifi_nrf_fw_info {
	/** Pointer to the FW patch data. */
	const void *data;
	/** Size of the FW patch data. */
	unsigned int size;
};


/**
 * @brief Structure to hold FW patch information for LMAC and UMAC.
 *
 */
struct wifi_nrf_fmac_fw_info {
	/** Primary LMAC FW patch information. */
	struct wifi_nrf_fw_info lmac_patch_pri;
	/** Secondary LMAC FW patch information. */
	struct wifi_nrf_fw_info lmac_patch_sec;
	/** Primary UMAC FW patch information. */
	struct wifi_nrf_fw_info umac_patch_pri;
	/** Secondary UMAC FW patch information. */
	struct wifi_nrf_fw_info umac_patch_sec;
};


/**
 * @brief Structure to hold OTP region information.
 *
 */
struct wifi_nrf_fmac_otp_info {
	/** OTP region information. */
	struct host_rpu_umac_info info;
	/** Flags indicating which OTP regions are valid. */
	unsigned int flags;
};

/**
 * @brief Structure to hold Regulatory parameter data.
 *
 */
struct wifi_nrf_fmac_reg_info {
	/** ISO IEC Country code. */
	unsigned char alpha2[NRF_WIFI_COUNTRY_CODE_LEN];
	 /** Forcefully set regulatory. */
	bool force;
};

/**
 * @brief Structure to hold common fmac priv parameter data.
 *
 */
struct wifi_nrf_fmac_priv {
	/** Handle to the OS abstraction layer. */
	struct wifi_nrf_osal_priv *opriv;
	/** Handle to the HAL layer. */
	struct wifi_nrf_hal_priv *hpriv;
	/** Data pointer to mode specific parameters */
	char priv[];
};

/**
 * @brief Structure to hold common fmac dev context parameter data.
 *
 */
struct wifi_nrf_fmac_dev_ctx {
	/** Handle to the FMAC IF abstraction layer. */
	struct wifi_nrf_fmac_priv *fpriv;
	/** Handle to the OS abstraction layer. */
	void *os_dev_ctx;
	/** Handle to the HAL layer. */
	void *hal_dev_ctx;
	/** Firmware statistics. */
	struct rpu_fw_stats *fw_stats;
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
	/** Data pointer to mode specific parameters */
	char priv[];
};
/**
 * @}
 */
#endif /* __FMAC_STRUCTS_COMMON_H__ */
