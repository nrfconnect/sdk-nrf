/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZIGBEE_COMMON_ZIGBEE_LOGGER_EPRXZCL_H_
#define ZIGBEE_COMMON_ZIGBEE_LOGGER_EPRXZCL_H_

#include <zboss_api.h>

/** @file zigbee_logger_eprxzcl.h
 *
 * @defgroup zigbee_logger_ep Zigbee endpoint logger library.
 * @{
 * @brief  Library for logging incoming ZCL packets.
 *
 * @details Provides Zigbee endpoint handler for parsing incoming ZCL packets
 * and logging packet's fields and payload.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Handler function which may be called to log an incoming frame
 *        onto Zigbee endpoint.
 *
 * @details When this function is called as a callback bound to endpoint
 *          via ZB_AF_SET_ENDPOINT_HANDLER(),
 *          (directly or indirectly) it produces a log line similar
 *          to the following:
 * @code
 * Received ZCL command (17): src_addr=0x0000(short) src_ep=64 dst_ep=64
 * cluster_id=0x0000 profile_id=0x0104 rssi=0 cmd_dir=0 common_cmd=1 cmd_id=0x00
 * cmd_seq=128 disable_def_resp=0 manuf_code=void payload=[0700] (17)
 * @endcode
 *
 * @param  bufid     Reference to Zigbee buffer holding received
 *                   zcl command to be logged
 *
 * @retval ZB_FALSE  in all conditions. This enables possibility
 *                   to use this function directly
 *                   as Zigbee stack endpoint handler.
 *
 */
zb_uint8_t zigbee_logger_eprxzcl_ep_handler(zb_bufid_t bufid);

#ifdef __cplusplus
}
#endif

/**@} */

#endif /* ZIGBEE_COMMON_ZIGBEE_LOGGER_EPRXZCL_H_ */
