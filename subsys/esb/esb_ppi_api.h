/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ESB_FEM_H__
#define ESB_FEM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/** @brief Set PPI/DPPI for Tx or Rx radio operations.
 *
 * Connections created by this function in a PPI variant:
 *                       1         2
 *       RADIO_DISABLED ---> EGU -----> radio ramp up task
 *                                \---> self disable
 *
 *       if (start_timer)
 *                    3
 *            EGU ---------> TIMER_START
 *
 * Connection created by this function in a DPPI variant:
 *                      1          2
 *      RADIO_DISABLED ---> EGU1 -----> EGU2
 *                               |
 *                        if (start_timer)
 *                               \----> TIMER_START
 *             3
 *      EGU2 -----> radio ramp up task
 *           |
 *           \----> self disable
 *
 * @param[in] rx Radio Rx mode, otherwise Tx mode.
 * @param[in] timer_start Indicates whether the timer is to be started on the EGU event.
 */
void esb_ppi_for_txrx_set(bool rx, bool timer_start);

/** @brief Clear PPI/DPPI connection for Tx or Rx radio operations
 *
 * @param[in] rx Radio Rx mode, otherwise Tx mode.
 * @param[in] timer_start Clear timer connections if the timer was set using
 *                        the @ref esb_ppi_for_txrx_set function.
 */
void esb_ppi_for_txrx_clear(bool rx, bool timer_start);

/** @brief Configure PPIs/DPPIs for the external front-end module. The EGU event will be connected
 *         to the TIMER_START event. As a result, the front-end module ramp-up will be scheduled\
 *         during the radio ramp-up period, with timing based on the FEM implementation used.
 */
void esb_ppi_for_fem_set(void);

/** @brief Clear PPIs/DDPIs for the external front-end module.
 */
void esb_ppi_for_fem_clear(void);

/** @brief Configure PPIs/DPPIs for packet retransmission.
 *
 * Connections created by this function:
 *
 *                      1
 *      RADIO_DISABLED ---> EGU
 *                       4
 *      TIMER_COMPARE_1 ---> RADIO_TXEN
 *
 */
void esb_ppi_for_retransmission_set(void);

/** @brief Clear PPIs/DPPIs configuration for packet retransmission.
 */
void esb_ppi_for_retransmission_clear(void);

/** @brief Configure PPIs/DPPIs for an ACK packet transmission.
 *
 * Connections created by this function:
 *
 *                     5
 *      RADIO_ADDRESS ---> TIMER_SHUTDOWN
 *                       6
 *      TIMER_COMPARE_0 ---> RADIO_DISABLE
 *
 */
void esb_ppi_for_wait_for_ack_set(void);

/** @brief Clear PPIs/DPPIs configuration for an ACK packet transmission.
 */
void esb_ppi_for_wait_for_ack_clear(void);

/** @brief Configure PPIs/DPPIs for a packet with ACK disabled and
 *         CONFIG_ESB_NEVER_DISABLE_TX Kconfig option enabled.
 */
void esb_ppi_for_wait_for_rx_set(void);

/** @brief Clear PPIs/DPPIs configuration for a packet with ACK disabled and
 *        CONFIG_ESB_NEVER_DISABLE_TX Kconfig option enabled.
 */
void esb_ppi_for_wait_for_rx_clear(void);

/** @brief Get radio disable event address or DPPI channel.
 *
 * @retval RADIO_DISABLED event address if PPI is used.
 *         DDPI channel number connected to RADIO_DISABLED event if DPPI is used.
 */
uint32_t esb_ppi_radio_disabled_get(void);

/** @brief Disable all PPI/DPPI channels used by ESB.
 */
void esb_ppi_disable_all(void);

/** @brief Initialize PPIs/DPPIs for ESB.
 *
 * This function allocates PPI/DPPI channels for the ESB protocol.
 *
 * @retval If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int esb_ppi_init(void);

/** @brief Deinitialize PPIs/DPPIs.
 *
 * This function frees the PPI/DPPI channels allocated by ESB.
 */
void esb_ppi_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* ESB_FEM_H__ */
