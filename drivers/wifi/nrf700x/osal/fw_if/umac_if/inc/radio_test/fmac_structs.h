/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *
 * @addtogroup nrf700x_fmac_api_radio_test FMAC radio test API
 * @{
 *
 * TODO: This file is not added doxygen to avoid duplicate warnings.
 *
 * @brief Header containing declarations for utility functions for
 * FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_STRUCTS_H__
#define __FMAC_STRUCTS_H__

#include <stdbool.h>

#include "osal_api.h"
#include "host_rpu_umac_if.h"
#if !defined(__DOXYGEN__)
#include "fmac_structs_common.h"
#endif

/**
 * @brief  Structure to hold context information for the UMAC IF layer.
 *
 * This structure maintains the context information necessary for the
 * operation of the UMAC IF layer.
 */
struct wifi_nrf_fmac_priv {
	/* To fix duplicate warning */
#if !defined(__DOXYGEN__)
	/** Handle to the OSAL layer. */
	struct wifi_nrf_osal_priv *opriv;
	/** Handle to the HAL layer. */
	struct wifi_nrf_hal_priv *hpriv;
#endif /* !defined(__DOXYGEN__) */
};


/**
 * @brief  Structure to hold per device context information for the UMAC IF layer.
 *
 * This structure maintains the context information necessary for the
 * a single instance of an FullMAC based RPU.
 */
struct wifi_nrf_fmac_dev_ctx {
	/* To fix duplicate warning */
#if !defined(__DOXYGEN__)
	/** Handle to the FMAC IF layer. */
	struct wifi_nrf_fmac_priv *fpriv;
	/** Handle to the OSAL layer. */
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
#endif /* !defined(__DOXYGEN__) */
	/** Firmware RF test command type. */
	enum nrf_wifi_rf_test rf_test_type;
	/** Firmware RF test capability data. */
	void *rf_test_cap_data;
	/** Firmware RF test capability data size. */
	unsigned int rf_test_cap_sz;
	/** Firmware RF test command is completed. */
	bool radio_cmd_done;
	/** Firmware RF test command status. */
	enum nrf_wifi_radio_test_err_status radio_cmd_status;
};

/**
 * @}
 */
#endif /* __FMAC_STRUCTS_H__ */
