/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_ESL_DUMMY_CMD_H_
#define BT_ESL_DUMMY_CMD_H_

/** @brief Create predefined ESL command advertising data into specified ESL group.
 *
 * This function is used to generate predefined advertising data for an AP.
 * The application contains a set of predefined ESL synchronization packets, and this
 * function uses these to create ESL payload. This can be useful in scenarios like
 * testing, debugging.
 *
 * The table in the application documentation shows the content of these predefined packets.
 *
 * @param[in] sync_pkt_type The type of the synchronization packet.
 * @param[in] group_id The ID of the group for which the ESL payload is generated.
 */
void esl_dummy_ap_ad_data(uint8_t sync_pkt_type, uint8_t group_id);


#endif /* BT_ESL_DUMMY_CMD_H_ */
