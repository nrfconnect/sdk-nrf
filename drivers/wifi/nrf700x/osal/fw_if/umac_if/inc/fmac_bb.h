/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing bounce buffer specific declarations for the FMAC IF Layer
 * of the Wi-Fi driver.
 */

#ifndef __FMAC_BB_H__
#define __FMAC_BB_H__

#define HOST_PKTRAM_BB_START 0x02C00000
#define HOST_PKTRAM_BB_LEN (4 * 1024 * 1024)

#ifndef VIRT_TO_PHYS
#define VIRT_TO_PHYS(addr) (HOST_PKTRAM_BB_START + (addr - fmac_ctx->base_addr_host_pktram_bb))
#endif

#define TX_MAX_DATA_SIZE (1600)

void bb_init(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx);

#endif /* __FMAC_BB_H__ */
