/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ESL_CLIENT_TAG_STORAGE_H_
#define ESL_CLIENT_TAG_STORAGE_H_

#include <zephyr/bluetooth/bluetooth.h>

#include "esl_common.h"

int save_tag_in_storage(const struct bt_esl_chrc_data *tag);
int load_tag_in_storage(const bt_addr_le_t *ble_addr, struct bt_esl_chrc_data *tag);
int find_tag_in_storage_with_esl_addr(uint16_t esl_addr, bt_addr_le_t *ble_addr);
/* type 0 list esl, type 1 list ble*/
int list_tags_in_storage(uint8_t type);
int remove_tag_in_storage(uint16_t esl_addr, const bt_addr_le_t *peer_addr);

#endif /* ESL_CLIENT_TAG_STORAGE_H_ */
