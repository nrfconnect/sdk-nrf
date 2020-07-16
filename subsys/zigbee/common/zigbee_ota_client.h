/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef ZIGBEE_OTA_CLIENT_H__
#define ZIGBEE_OTA_CLIENT_H__

#include <zb_zcl_ota_upgrade.h>
#include <zigbee_ota.h>
#include <zb_types.h>


/* Initialize DFU client. */
void zb_ota_client_init(void);

/* Initialize OTA DFU process. */
void zb_ota_dfu_init(void);

/* Function to abort the firmware upgrade process. */
void zb_ota_dfu_abort(void);

/* Marks the currently running image as confirmed. */
void zb_ota_confirm_image(void);

/* Get current app image version from MCUBoot. */
uint32_t zb_ota_get_image_ver(void);

/** @brief Function for rebooting the chip.
 *
 *  @param param   Not used. Required by callback type definition.
 */
void zb_ota_reboot_application(zb_uint8_t param);

/** @brief Code to process the incoming Zigbee OTA frame
 *
 *  @param ota   Pointer to the zb_zcl_ota_upgrade_value_param_t structure,
 *               passed from the handler
 *  @param bufid ZBOSS buffer id
 *
 *  @return ZB_ZCL_OTA_UPGRADE_STATUS_BUSY if OTA has to be suspended,
 *          ZB_ZCL_OTA_UPGRADE_STATUS_OK otherwise
 */
zb_uint8_t zb_ota_process_chunk(const zb_zcl_ota_upgrade_value_param_t *ota,
				zb_bufid_t bufid);

/* Function for initializing periodical OTA server discovery. */
void zb_ota_client_periodical_discovery_srv_init(void);

/* Function for starting periodical OTA server discovery. */
void zb_ota_client_periodical_discovery_srv_start(void);

/* Function for stopping the periodical OTA server discovery. */
void zb_ota_client_periodical_discovery_srv_stop(void);

#endif /* ZIGBEE_OTA_CLIENT_H__ */
