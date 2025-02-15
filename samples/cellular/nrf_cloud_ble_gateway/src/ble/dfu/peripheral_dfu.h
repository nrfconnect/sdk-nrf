/* Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PERIPHERAL_DFU_H_
#define PERIPHERAL_DFU_H_

int peripheral_dfu_init(void);
int peripheral_dfu_config(const char *addr, int size, const char *version,
			   uint32_t crc, bool init_pkt, bool use_printk);
int peripheral_dfu_start(const char *host, const char *file, int sec_tag,
			 const char *apn, size_t fragment_size);
int peripheral_dfu_cleanup(void);

#endif
