/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *
 * @addtogroup nrf700x_fmac_api FMAC API
 * @{
 *
 * @brief Header containing API declarations for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_API_H__
#define __FMAC_API_H__

#include <stdbool.h>

#include "osal_api.h"
#include "host_rpu_umac_if.h"
#include "host_rpu_data_if.h"
#include "host_rpu_sys_if.h"

#include "fmac_structs.h"
#include "fmac_cmd.h"
#include "fmac_event.h"
#include "fmac_vif.h"
#include "fmac_bb.h"
#include "fmac_api_common.h"



/**
 * @brief Initializes the UMAC IF layer.
 *
 * @param data_config Pointer to configuration of data queues.
 * @param rx_buf_pools Pointer to configuration of Rx queue buffers.
 *		       See rx_buf_pool_params
 * @param callbk_fns Pointer to callback functions.
 *
 * This function initializes the UMAC IF layer.It does the following:
 *	    - Creates and initializes the context for the UMAC IF layer.
 *	    - Initializes the HAL layer.
 *	    - Registers the driver to the underlying OS.
 *
 * @return Pointer to the context of the UMAC IF layer.
 */
struct wifi_nrf_fmac_priv *wifi_nrf_fmac_init(struct nrf_wifi_data_config_params *data_config,
					      struct rx_buf_pool_params *rx_buf_pools,
					      struct wifi_nrf_fmac_callbk_fns *callbk_fns);

/**
 * @brief Issue a scan request to the RPU.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the scan is to be performed.
 * @param scan_info: The parameters needed by the RPU for scan operation.
 *
 * This function is used to send a command to
 *	    instruct the firmware to trigger a scan. The scan can be performed in two
 *	    modes:
 *
 *	    Auto Mode (%AUTO_SCAN):
 *        In this mode the host does not need to specify any scan specific
 *        parameters. The RPU will perform the scan on all the channels permitted
 *        by and abiding by the regulations imposed by the
 *        WORLD (common denominator of all regulatory domains) regulatory domain.
 *        The scan will be performed using the wildcard SSID.
 *
 *	    Channel Map Mode (%CHANNEL_MAPPING_SCAN):
 *        In this mode the host can have fine grained control over the scan
 *        specific parameters to be used (for example, Passive/Active scan selection,
 *        Number of probe requests per active scan, Channel list to scan,
 *        Permanence on each channel, SSIDs to scan etc.). This mode expects
 *        the regulatory restrictions to be taken care by the invoker of the
 *       API.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_scan(void *fmac_dev_ctx,
					unsigned char if_idx,
					struct nrf_wifi_umac_scan_info *scan_info);


/**
 * @brief Issue a scan results request to the RPU.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the scan results are to be fetched.
 * @param scan_type: The scan type (i.e. DISPLAY or CONNECT scan).
 *
 * This function is used to send a command to
 *	    instruct the firmware to return the results of a scan.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_scan_res_get(void *fmac_dev_ctx,
						unsigned char if_idx,
						int scan_type);

/**
 * @brief Issue abort of an ongoing scan to the RPU.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the scan results are to be fetched.
 *
 * This function is used to send a command to
 *	    instruct the firmware to abort an ongoing scan.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_abort_scan(void *fmac_dev_ctx,
						unsigned char if_idx);

#if defined(CONFIG_WPA_SUPP) || defined(__DOXYGEN__)
/**
 * @brief Issue a authentication request to the RPU.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the authentication is to be
 *	         performed.
 * @param auth_info The parameters needed by the RPU to generate the authentication
 *                  request.
 *
 * This function is used to send a command to
 *	    instruct the firmware to initiate a authentication request to an AP on the
 *	    interface identified with \p if_idx.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_auth(void *fmac_dev_ctx,
					unsigned char if_idx,
					struct nrf_wifi_umac_auth_info *auth_info);


/**
 * @brief Issue a de-authentication request to the RPU.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the de-authentication is to be
 *          performed.
 * @param deauth_info De-authentication specific information required by the RPU.
 *
 * This function is used to send a command to
 *	    instruct the firmware to initiate a de-authentication notification to an AP
 *	    on the interface identified with \p if_idx.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_deauth(void *fmac_dev_ctx,
					  unsigned char if_idx,
					  struct nrf_wifi_umac_disconn_info *deauth_info);


/**
 * @brief Issue a association request to the RPU.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the association is to be
 *           performed.
 * @param assoc_info The parameters needed by the RPU to generate the association
 *           request.
 *
 * This function is used to send a command to
 *	    instruct the firmware to initiate a association request to an AP on the
 *	    interface identified with \p if_idx.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_assoc(void *fmac_dev_ctx,
					 unsigned char if_idx,
					 struct nrf_wifi_umac_assoc_info *assoc_info);


/**
 * @brief Issue a disassociation request to the RPU.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the disassociation is to be
 *	         performed.
 * @param disassoc_info: Disassociation specific information required by the RPU.
 *
 * This function is used to send a command to
 *	    instruct the firmware to initiate a disassociation notification to an AP
 *	    on the interface identified with \p if_idx.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_disassoc(void *fmac_dev_ctx,
					    unsigned char if_idx,
					    struct nrf_wifi_umac_disconn_info *disassoc_info);


/**
 * @brief Add a key into the RPU security database.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the key is to be added.
 * @param key_info Key specific information which needs to be passed to the RPU.
 * @param mac_addr MAC address of the peer with which the key is associated.
 *
 * This function is used to send a command to
 *	    instruct the firmware to add a key to its security database.
 *	    The key is for the peer identified by \p mac_addr onthe
 *	    interface identified with \p if_idx.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_add_key(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_key_info *key_info,
					   const char *mac_addr);


/**
 * @brief Delete a key from the RPU security database.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface from which the key is to be deleted.
 * @param key_info Key specific information which needs to be passed to the RPU.
 * @param mac_addr MAC address of the peer with which the key is associated.
 *
 * This function is used to send a command to
 *	    instruct the firmware to delete a key from its security database.
 *	    The key is for the peer identified by \p mac_addr  onthe
 *	    interface identified with \p if_idx.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_del_key(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_key_info *key_info,
					   const char *mac_addr);


/**
 * @brief Set a key as a default for data or management
 *	  traffic in the RPU security database.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the key is to be set.
 * @param key_info Key specific information which needs to be passed to the RPU.
 *
 * This function is used to send a command to
 *	    instruct the firmware to set a key as default in its security database.
 *	    The key is either for data or management traffic and is classified based on
 *	    the flags element set in \p key_infoparameter.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_set_key(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_key_info *key_info);


/**
 * @brief Set BSS parameters for an AP mode interface to the RPU.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the BSS parameters are to be set.
 * @param bss_info BSS specific parameters which need to be passed to the RPU.
 *
 * This function is used to send a command to
 *	    instruct the firmware to set the BSS parameter for an AP mode interface.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_set_bss(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_bss_info *bss_info);


/**
 * @brief Update the Beacon Template.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the Beacon Template is to be updated.
 * @param data Beacon Template which need to be passed to the RPU.
 *
 * This function is used to send a command to
 *	    instruct the firmware to update beacon template for an AP mode interface.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_chg_bcn(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_set_beacon_info *data);

/**
 * @brief Start a SoftAP.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the SoftAP is to be started.
 * @param ap_info AP operation specific parameters which need to be passed to the RPU.
 *
 * This function is used to send a command to
 *	    instruct the firmware to start a SoftAP on an interface identified with
 *	    \p if_idx.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_start_ap(void *fmac_dev_ctx,
					    unsigned char if_idx,
					    struct nrf_wifi_umac_start_ap_info *ap_info);


/**
 * @brief Stop a SoftAP.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the SoftAP is to be stopped.
 *
 * This function is used to send a command to
 *	    instruct the firmware to stop a SoftAP on an interface identified with
 *	    \p if_idx.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_stop_ap(void *fmac_dev_ctx,
					   unsigned char if_idx);


/**
 * @brief Start P2P mode on an interface.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the P2P mode is to be started.
 *
 * This function is used to send a command to
 *	    instruct the firmware to start P2P mode on an interface identified with
 *      \p if_idx.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_p2p_dev_start(void *fmac_dev_ctx,
						 unsigned char if_idx);


/**
 * @brief Start P2P mode on an interface.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the P2P mode is to be started.
 *
 * This function is used to send a command to
 *	   instruct the firmware to start P2P mode on an interface identified with
 *	   \p if_idx.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_p2p_dev_stop(void *fmac_dev_ctx,
						unsigned char if_idx);

/**
 * @brief Start p2p remain on channel.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface to be kept on channel and stay there.
 * @param roc_info Contains channel and time in ms to stay on.
 *
 * This function is used to send a command
 *	    to RPU to put p2p device in listen state for a duration.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_p2p_roc_start(void *fmac_dev_ctx,
						 unsigned char if_idx,
						 struct remain_on_channel_info *roc_info);

/**
 * @brief Stop p2p remain on channel.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface to be kept on channel and stay there.
 * @param cookie cancel p2p listen state of the matching cookie.
 *
 * This function is used to send a command to RPU to put p2p device out
 *	    of listen state.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_p2p_roc_stop(void *fmac_dev_ctx,
						unsigned char if_idx,
						unsigned long long cookie);

/**
 * @brief Transmit a management frame.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the frame is to be
 *               transmitted.
 * @param mgmt_tx_info Information regarding the management frame to be
 *                     transmitted.
 *
 * This function is used to instruct the RPU to transmit a management frame.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_mgmt_tx(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_mgmt_tx_info *mgmt_tx_info);


/**
 * @brief Remove a station.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the STA is connected.
 * @param del_sta_info Information regarding the station to be removed.
 *
 * This function is used to instruct the RPU to remove a station and send a
 *	    de-authentication/disassociation frame to the same.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_del_sta(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_del_sta_info *del_sta_info);


/**
 * @brief Indicate a new STA connection to the RPU.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the STA is connected.
 * @param add_sta_info Information regarding the new station.
 *
 * This function is used to indicate to the RPU that a new STA has
 *	    successfully connected to the AP.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_add_sta(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_add_sta_info *add_sta_info);

/**
 * @brief Indicate changes to STA connection parameters to the RPU.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the STA is connected.
 * @param chg_sta_info Information regarding updates to the station parameters.
 *
 * This function is used to indicate changes in the connected STA parameters
 *	    to the RPU.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_chg_sta(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_chg_sta_info *chg_sta_info);



/**
 * @brief Register the management frame type which needs to be sent up to the host by the RPU.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface from which the received frame is to be
 *               sent up.
 * @param frame_info Information regarding the management frame to be sent up.
 *
 * This function is used to send a command to
 *	    instruct the firmware to pass frames matching that type/subtype to be
 *	    passed upto the host driver.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_mgmt_frame_reg(void *fmac_dev_ctx,
						  unsigned char if_idx,
						  struct nrf_wifi_umac_mgmt_frame_info *frame_info);

#endif /* CONFIG_WPA_SUPP */
/**
 * @brief Get unused MAC address from base mac address.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param addr Memory to copy the mac address to.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_mac_addr(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					    unsigned char *addr);

/**
 * @brief Assign a index for a new VIF.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 *
 * This function searches for an unused VIF index and returns it.
 *
 * @return Index to be used for a new VIF
 *         In case of error MAX_NUM_VIFS will be returned.
 */
unsigned char wifi_nrf_fmac_vif_idx_get(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx);

/**
 * @brief Add a new virtual interface.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param os_vif_ctx Pointer to VIF context that the user of the UMAC IF would like
 *                   to be passed during invocation of callback functions like
 *                   rx_frm_callbk_fn() etc.
 * @param vif_info Information regarding the interface to be added.
 *
 * This function is used to send a command to
 *	    instruct the firmware to add a new interface with parameters specified by
 *          \p vif_info.
 *
 * @return Index (maintained by the UMAC IF layer) of the VIF that was added.
 *         In case of error MAX_NUM_VIFS will be returned.
 */
unsigned char wifi_nrf_fmac_add_vif(void *fmac_dev_ctx,
				    void *os_vif_ctx,
				    struct nrf_wifi_umac_add_vif_info *vif_info);


/**
 * @brief Deletes a virtual interface.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface to be deleted.
 *
 * This function is used to send a command to
 *     instruct the firmware to delete an interface which was added using
 *    \p wifi_nrf_fmac_add_vif.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_del_vif(void *fmac_dev_ctx,
					   unsigned char if_idx);

/**
 * @brief Change the attributes of an interface.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the functionality is to be
 *               bound.
 * @param vif_info Interface specific information which needs to be passed to the
 *                 RPU.
 *
 * This function is used to change the attributes of an interface identified
 *     with \p if_idx.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_chg_vif(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_chg_vif_attr_info *vif_info);


/**
 * @brief Change the state of a virtual interface.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface whose state needs to be changed.
 * @param vif_info State information to be changed for the interface.
 *
 * This function is used to send a command to
 *     instruct the firmware to change the state of an interface with an index of
 *     \p if_idx and parameters specified by \p vif_info.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_chg_vif_state(void *fmac_dev_ctx,
						 unsigned char if_idx,
						 struct nrf_wifi_umac_chg_vif_state_info *vif_info);


/**
 * @brief Set MAC address on interface.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface whose MAC address is to be changed.
 * @param mac_addr MAC address to set.
 *
 * This function is used to change the MAC address of an interface identified
 *	    with \p if_idx.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_set_vif_macaddr(void *fmac_dev_ctx,
						   unsigned char if_idx,
						   unsigned char *mac_addr);

/**
 * @brief Transmit a frame to the RPU.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the frame is to be
 *               transmitted.
 * @param netbuf Pointer to the OS specific network buffer.
 *
 * This function takes care of transmitting a frame to the RPU. It does the
 *	    following:
 *
 *		- Queues the frames to a transmit queue.
 *		- Based on token availability sends one or more frames to the RPU using
 *		  the command for transmission.
 *		- The firmware sends an event once the command has
 *		  been processed send to indicate whether the frame(s) have been
 *		  transmitted/aborted.
 *		- The diver can cleanup the frame buffers after receiving this event.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_start_xmit(void *fmac_dev_ctx,
					      unsigned char if_idx,
					      void *netbuf);

/**
 * @brief Inform the RPU that host is going to suspend state.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 *
 * This function is used to send a command to
 *	    inform the RPU that host is going to suspend state.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_suspend(void *fmac_dev_ctx);


/**
 * @brief Notify RPU that host has resumed from a suspended state.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 *
 * This function is used to send a command
 *	    to inform the RPU that host has resumed from a suspended state.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_resume(void *fmac_dev_ctx);


/**
 * @brief Get tx power
 *
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx VIF index.
 *
 * This function is used to send a command
 *	    to get the transmit power on a particular interface.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_get_tx_power(void *fmac_dev_ctx,
						unsigned int if_idx);

/**
 * @brief Get channel definition.
 *
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx VIF index.
 *
 * This function is used to send a command
 *	    to get the channel configured on a particular interface.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_get_channel(void *fmac_dev_ctx,
					       unsigned int if_idx);

/**
 * @brief Get station statistics
 *
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx: VIF index.
 * @param mac MAC address of the station
 *
 * This function is used to send a command
 *	    to get station statistics using a mac address.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_get_station(void *fmac_dev_ctx,
					       unsigned int if_idx,
					       unsigned char *mac);


/* @brief Get interface statistics
 *
 * @param dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx VIF index.
 *
 * This function is used to send a command
 *	    to get interface statistics using interface index.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_get_interface(void *dev_ctx,
					       unsigned int if_idx);


/**
 * @brief Configure WLAN power management.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which power management is to be set.
 * @param state Enable/Disable of WLAN power management.
 *
 * This function is used to send a command
 *	    to RPU to Enable/Disable WLAN Power management.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_set_power_save(void *fmac_dev_ctx,
						  unsigned char if_idx,
						  bool state);

/**
 * @brief Configure WLAN U-APSD queue.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which power management is to be set.
 * @param uapsd_queue Queue to be set for U-APSD.
 *
 * This function is used to send a command (%NRF_WIFI_UMAC_CMD_CONFIG_UAPSD)
 *	    to RPU to set U-APSD queue value.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_set_uapsd_queue(void *fmac_dev_ctx,
						   unsigned char if_idx,
						   unsigned int uapsd_queue);

/**
 * @brief Configure Power save timeout.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which power management is to be set.
 * @param ps_timeout Power save inactivity time.
 *
 * This function is used to send a command
 *	    (%NRF_WIFI_UMAC_CMD_SET_POWER_SAVE_TIMEOUT) to RPU to set power save
 *	    inactivity time.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_set_power_save_timeout(void *fmac_dev_ctx,
							  unsigned char if_idx,
							  int ps_timeout);

/**
 * @brief Configure qos_map of for data
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the qos map be set.
 * @param qos_info qos_map information
 *
 * This function is used to send a command
 *	    to RPU to set qos map information.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_set_qos_map(void *fmac_dev_ctx,
					       unsigned char if_idx,
					       struct nrf_wifi_umac_qos_map_info *qos_info);

/**
 * @brief Configure WoWLAN.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param var Wakeup trigger condition.
 *
 * This function is used to send a command
 *	    to RPU to configure Wakeup trigger condition in RPU.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_set_wowlan(void *fmac_dev_ctx,
					      unsigned int var);

/**
 * @brief Get PHY configuration.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the CMD needs to be sent.
 *
 * This function is used to get PHY configuration from RPU.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_get_wiphy(void *fmac_dev_ctx, unsigned char if_idx);

/**
 * @brief Register to get MGMT frames.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the CMD needs to be sent.
 * @param frame_info Information regarding the management frame.
 *
 * Register with RPU to receive MGMT frames.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_register_frame(void *fmac_dev_ctx, unsigned char if_idx,
						  struct nrf_wifi_umac_mgmt_frame_info *frame_info);

/**
 * @brief Set wiphy parameters
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the CMD needs to be sent.
 * @param wiphy_info wiphy parameters
 *
 * This function is used to send a command
 *	    to RPU to configure parameters related to interface.
 *
 * @return Command execution status
 */

enum wifi_nrf_status wifi_nrf_fmac_set_wiphy_params(void *fmac_dev_ctx,
						 unsigned char if_idx,
						 struct nrf_wifi_umac_set_wiphy_info *wiphy_info);

/**
 * @brief TWT setup command
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the TWT parameters be set.
 * @param twt_info TWT parameters
 *
 * This function is used to send a command
 *	    to RPU to configure parameters related to TWT setup.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_twt_setup(void *fmac_dev_ctx,
					     unsigned char if_idx,
					     struct nrf_wifi_umac_config_twt_info *twt_info);

/**
 * @brief TWT teardown command
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which the TWT parameters be set.
 * @param twt_info TWT parameters
 *
 * This function is used to send a command
 *	    to RPU to tear down an existing TWT session.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_twt_teardown(void *fmac_dev_ctx,
						unsigned char if_idx,
						struct nrf_wifi_umac_config_twt_info *twt_info);

/**
 * @brief Get connection info from RPU
 * @param fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx: Index of the interface.
 *
 * This function is used to send a command
 * to RPU to fetch the connection information.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_get_conn_info(void *fmac_dev_ctx,
						unsigned char if_idx);

/**
 * @brief De-initializes the UMAC IF layer.
 * @param fpriv Pointer to the context of the UMAC IF layer.
 *
 * This function de-initializes the UMAC IF layer of the RPU WLAN FullMAC
 *	    driver. It does the following:
 *
 *		- De-initializes the HAL layer.
 *		- Frees the context for the UMAC IF layer.
 *
 * @return None
 */
void wifi_nrf_fmac_deinit(struct wifi_nrf_fmac_priv *fpriv);

/**
 * @brief Removes a RPU instance.
 * @param fmac_dev_ctx Pointer to the context of the RPU instance to be removed.
 *
 * This function handles the removal of an RPU instance at the UMAC IF layer.
 *
 * @return None.
 */
void wifi_nrf_fmac_dev_rem(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx);


/**
 * @brief Initializes a RPU instance.
 * @param fmac_dev_ctx Pointer to the context of the RPU instance to be removed.
 * @param rf_params_usr RF parameters (if any) to be passed to the RPU.
 * @param sleep_type Type of RPU sleep.
 * @param phy_calib PHY calibration flags to be passed to the RPU.
 * @param op_band Operating band of the RPU.
 * @param tx_pwr_ctrl_params TX power control parameters to be passed to the RPU.
 *
 * This function initializes the firmware of an RPU instance.
 *
 * @return Command execution status
 */
enum wifi_nrf_status wifi_nrf_fmac_dev_init(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
			unsigned char *rf_params_usr,
#if defined(CONFIG_NRF_WIFI_LOW_POWER) || defined(__DOXYGEN__)
			int sleep_type,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
			unsigned int phy_calib,
			enum op_band op_band,
			struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl_params);


/**
 * @brief De-initializes a RPU instance.
 * @param fmac_dev_ctx Pointer to the context of the RPU instance to be removed.
 *
 * This function de-initializes the firmware of an RPU instance.
 *
 * @return None.
 */
void wifi_nrf_fmac_dev_deinit(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx);

/**
 * @brief Configure WLAN listen interval.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which power management is to be set.
 * @param listen_interval listen interval to be configured.
 *
 * This function is used to send a command to RPU to configure listen interval.
 *
 * @return wifi_nrf_status
 */
enum wifi_nrf_status wifi_nrf_fmac_set_listen_interval(void *fmac_dev_ctx,
							unsigned char if_idx,
							unsigned short listen_interval);

/**
 * @brief Configure WLAN PS wakeup mode to DTIM interval or listen interval.
 * @param fmac_dev_ctx Pointer to the UMAC IF context for a RPU WLAN device.
 * @param if_idx Index of the interface on which power management is to be set.
 * @param ps_wakeup_mode Enable listen interval based ps(default is DTIM based)
 *
 * This function is used to configure PS wakeup mode, either DTIM Interval
 *         or Listen interval based. Default is DTIM interval based mode.
 *
 * @return wifi_nrf_status
 */
enum wifi_nrf_status wifi_nrf_fmac_set_ps_wakeup_mode(void *fmac_dev_ctx,
							unsigned char if_idx,
							bool ps_wakeup_mode);
/**
 * @}
 */
#endif /* __FMAC_API_H__ */
