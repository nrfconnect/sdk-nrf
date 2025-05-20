/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ISO_COMBINED_BIS_AND_CIS_H_
#define ISO_COMBINED_BIS_AND_CIS_H_

/** Type definition for a callback to set SDU data to be transmitted
 *
 */
typedef void (*iso_tx_set_sdu_data_cb_t)(struct net_buf *);

/** Type definition for a callback to process received SDU data
 *
 */
typedef void (*iso_rx_process_sdu_data_cb_t)(struct net_buf *);

/** Start combined CIS Central + BIS source
 *
 */
void combined_cis_bis_start(void);

/** Initialize RX path channel
 *
 * @param retransmission_number Requested retransmission number (if central)
 * @param process_sdu_data_cb   Callback to process received SDU
 */
void iso_rx_init(uint8_t retransmission_number, iso_rx_process_sdu_data_cb_t process_sdu_data_cb);

/** Obtain pointer to RX channel
 *
 * @retval Pointer to the RX channel
 */
struct bt_iso_chan **iso_rx_channels_get(void);

/** Initialize TX path channels
 *
 * @param retransmission_number Requested retransmission number (if central or broadcaster)
 * @param set_sdu_data_cb       Callback to set SDU data to be transmitted
 */
void iso_tx_init(uint8_t retransmission_number, iso_tx_set_sdu_data_cb_t set_sdu_data_cb);

/** Obtain pointer to TX channel
 *
 * @retval Pointer to the TX channel
 */
struct bt_iso_chan **iso_tx_channels_get(void);
#endif /* ISO_COMBINED_BIS_AND_CIS_H_ */
