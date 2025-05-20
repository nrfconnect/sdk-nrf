/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_COMMON_PDU_H
#define DECT_COMMON_PDU_H

uint16_t dect_common_pdu_tx_next_seq_nbr_get(void);
const char *dect_common_pdu_message_type_to_string(int type, char *out_str_buff);

#endif /* DECT_COMMON_PDU_H */
