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
 * @brief  Structure to hold per device context information for the UMAC IF layer.
 *
 * This structure maintains the context information necessary for the
 * a single instance of an FullMAC based RPU.
 */
struct nrf_wifi_fmac_dev_ctx_rt {
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
