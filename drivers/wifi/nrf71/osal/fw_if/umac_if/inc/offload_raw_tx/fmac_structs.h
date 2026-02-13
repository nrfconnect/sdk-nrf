/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 *
 * @addtogroup nrf_wifi_fmac_off_raw_tx_api FMAC offloaded raw tx API
 * @{
 *
 * TODO: This file is not added doxygen to avoid duplicate warnings.
 *
 * @brief Header containing declarations for utility functions for
 * FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_STRUCTS_H__
#define __FMAC_STRUCTS_H__

#include <nrf71_wifi_ctrl.h>
#include "common/fmac_structs_common.h"
#define NRF_WIFI_FMAC_PARAMS_RECV_TIMEOUT 100 /* ms */

/**
 * @brief  Structure to hold per device context information for the UMAC IF layer.
 *
 * This structure maintains the context information necessary for
 * a single instance of a FullMAC-based RPU.
 */
struct nrf_wifi_off_raw_tx_fmac_dev_ctx {
	enum nrf_wifi_cmd_status off_raw_tx_cmd_status;
	bool off_raw_tx_cmd_done;
};


/**
 * @brief - Structure to hold per device host and firmware statistics.
 *
 */
struct rpu_off_raw_tx_op_stats {
	/** Host statistics. */
	struct rpu_host_stats host;
	/** Firmware statistics. */
	struct rpu_off_raw_tx_fw_stats fw;
};


/**
 * @}
 */
#endif /* __FMAC_STRUCTS_H__ */
