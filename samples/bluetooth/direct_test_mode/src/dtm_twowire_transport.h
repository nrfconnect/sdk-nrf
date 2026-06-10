/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DTM_TWOWIRE_TRANSPORT_H_
#define DTM_TWOWIRE_TRANSPORT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the 2-wire UART transport.
 *
 * @note This function must succeed before any other function in this module is called.
 *
 * @return 0 on success, negative error code on failure.
 */
int dtm_tw_transport_init(void);

/**
 * @brief Read a 2-wire UART packet.
 *
 * @note This function blocks until a complete 2-wire packet is read.
 *
 * @return The 2-wire packet read.
 */
uint16_t dtm_tw_transport_read(void);

/**
 * @brief Write a 2-wire UART packet.
 *
 * @param[in] tw_packet The 2-wire packet to write.
 */
void dtm_tw_transport_write(uint16_t tw_packet);

#ifdef __cplusplus
}
#endif

#endif /* DTM_TWOWIRE_TRANSPORT_H_ */
