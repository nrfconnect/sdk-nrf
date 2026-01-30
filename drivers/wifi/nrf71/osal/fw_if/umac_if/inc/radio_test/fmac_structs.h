/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 *
 * @addtogroup nrf_wifi_api_radio_test FMAC radio test API
 * @{
 *
 * TODO: This file is not added doxygen to avoid duplicate warnings.
 *
 * @brief Header containing declarations for utility functions for
 * FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_STRUCTS_H__
#define __FMAC_STRUCTS_H__

#include "osal_api.h"

#include <nrf71_wifi_ctrl.h>

#include "common/fmac_structs_common.h"

/**
 * @brief  Structure to hold per device context information for the UMAC IF layer.
 *
 * This structure maintains the context information necessary for
 * a single instance of a FullMAC-based RPU.
 */
struct nrf_wifi_rt_fmac_dev_ctx {
	/** Firmware RF test command type. */
	enum nrf_wifi_rf_test rf_test_type;
	/** Firmware RF test capability data. */
	void *rf_test_cap_data;
	/** Firmware RF test capability data size. */
	unsigned int rf_test_cap_sz;
	/** Firmware RF test command is completed. */
	bool radio_cmd_done;
	/** Firmware RF test command status. */
	enum nrf_wifi_cmd_status radio_cmd_status;
	/** Firmware RF test RX capture event status */
	unsigned char capture_status;
};


/**
 * @brief - Structure to hold per device host and firmware statistics.
 *
 */
struct rpu_rt_op_stats {
	/** Host statistics. */
	struct rpu_host_stats host;
	/** Firmware statistics. */
	struct rpu_rt_fw_stats fw;
};

/**
 * @}
 */
#endif /* __FMAC_STRUCTS_H__ */
