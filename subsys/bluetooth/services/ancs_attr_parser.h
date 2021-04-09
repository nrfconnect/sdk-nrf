/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_ANCS_ATTR_PARSER_H__
#define BT_ANCS_ATTR_PARSER_H__

/** @file
 *
 * @addtogroup bt_ancs_client
 * @{
 */

#include <bluetooth/services/ancs_client.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Function for parsing notification or app attribute response data.
 *
 * @details The data that comes from the Notification Provider can be much longer than what
 *          would fit in a single GATT notification. Therefore, this function relies on a
 *          state-oriented switch case.
 *          UID and command ID will be received only once at the beginning of the first
 *          GATT notification of a new attribute request for a given iOS notification.
 *          After this, we can loop several ATTR_ID > LENGTH > DATA > ATTR_ID > LENGTH > DATA until
 *          we have received all attributes we wanted as a Notification Consumer.
 *          The Notification Provider can also simply stop sending attributes.
 *
 * 1 byte  |  4 bytes    |1 byte |2 bytes |... X bytes ... |1 bytes| 2 bytes| ... X bytes ...
 * --------|-------------|-------|--------|----------------|-------|--------|----------------
 * CMD_ID  |  NOTIF_UID  |ATTR_ID| LENGTH |    DATA        |ATTR_ID| LENGTH |    DATA
 *
 * @param[in] ancs_c     ANCS client instance to which the event belongs.
 * @param[in] data_src   Pointer to data that was received from the Notification Provider.
 * @param[in] data_len   Length of the data that was received from the Notification Provider.
 */
void bt_ancs_parse_get_attrs_response(struct bt_ancs_client *ancs_c,
				      const uint8_t *data_src,
				      const uint16_t data_len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_ANCS_ATTR_PARSER_H__ */
