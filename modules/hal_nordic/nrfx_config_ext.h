/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRFX_CONFIG_EXT_H__
#define NRFX_CONFIG_EXT_H__

/* Include nrfx_config_nrf*.h for products which are yet to be upstreamed. */

#if (defined(NRF7120_ENGA_XXAA) && defined(NRF_APPLICATION))
    #include "templates/nrfx_config_nrf7120_enga_application.h"
#elif (defined(NRF7120_ENGA_XXAA)) && defined(NRF_FLPR)
    #include"templates/nrfx_config_nrf7120_enga_flpr.h"
#elif (defined(NRF54LS05B_XXAA)) && defined(NRF_APPLICATION)
    #include"templates/nrfx_config_nrf54ls05b_application.h"
#else
    #error "Unknown device."
#endif

#endif /* NRFX_CONFIG_EXT_H__ */
