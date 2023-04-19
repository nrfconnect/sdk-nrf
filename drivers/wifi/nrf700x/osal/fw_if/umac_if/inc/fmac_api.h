/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
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


#ifdef CONFIG_NRF700X_RADIO_TEST
/**
 * wifi_nrf_fmac_init() - Initializes the UMAC IF layer of the RPU WLAN FullMAC
 *                        driver.
 *
 * This function initializes the UMAC IF layer of the RPU WLAN FullMAC driver.
 * It does the following:
 *
 *     - Creates and initializes the context for the UMAC IF layer.
 *     - Initializes the HAL layer.
 *     - Registers the driver to the underlying OS.
 *
 * Returns: Pointer to the context of the UMAC IF layer.
 */
struct wifi_nrf_fmac_priv *wifi_nrf_fmac_init(void);


/**
 * wifi_nrf_fmac_stats_get() - Issue a request to get stats from the RPU.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @op_mode: Production/FCM mode.
 * @stats: Pointer to memory where the stats are to be copied.
 *
 * This function is used to send a command to
 * instruct the firmware to return the current RPU statistics. The RPU will
 * send the event with the current statistics.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_stats_get(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					     int op_mode,
					     struct rpu_op_stats *stats);


/**
 * wifi_nrf_fmac_radio_test_init() - Initialize the RPU for radio tests.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @params: Parameters necessary for the initialization.
 *
 * This function is used to send a command to RPU to initialize it
 * for the radio test mode.
 *
 * Returns: Status
 *              Pass : %WIFI_NRF_STATUS_SUCCESS
 *              Error: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_radio_test_init(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						   struct rpu_conf_params *params);

/**
 * wifi_nrf_fmac_radio_test_prog_tx() - Start TX tests in radio test mode.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @params: Parameters necessary for the TX tests.
 *
 * This function is used to send a command to RPU to start
 * the TX tests in radio test mode.
 *
 * Returns: Status
 *              Pass : %WIFI_NRF_STATUS_SUCCESS
 *              Error: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_radio_test_prog_tx(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						      struct rpu_conf_params *params);

/**
 * wifi_nrf_fmac_radio_test_prog_rx() - Start RX tests in radio test mode.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @params: Parameters necessary for the RX tests.
 *
 * This function is used to send a command to RPU to start
 * the RX tests in radio test mode.
 *
 * Returns: Status
 *              Pass : %WIFI_NRF_STATUS_SUCCESS
 *              Error: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_radio_test_prog_rx(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						      struct rpu_conf_params *params);


/**
 * nrf_wifi_fmac_rf_test_rx_cap() - Start RF test capture in radio test mode.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @cap_data: Pointer to the memory where the RF test capture is to be stored.
 * @num_samples: Number of RF test samples to capture.
 * @lna_gain: LNA gain value.
 * @bb_gain: Baseband gain value.
 *
 * This function is used to send a command to RPU to start
 * the RF test capture in radio test mode.
 *
 * Returns: Status
 *              Pass : %WIFI_NRF_STATUS_SUCCESS
 *              Error: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status nrf_wifi_fmac_rf_test_rx_cap(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						  enum nrf_wifi_rf_test rf_test_type,
						  void *cap_data,
						  unsigned short int num_samples,
						  unsigned char lna_gain,
						  unsigned char bb_gain);


/**
 * nrf_wifi_fmac_rf_test_tx_tone() - Start/Stop RF TX tone test in radio test mode.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @enable: Enable/Disable TX tone test.
 * @tone_freq: Desired tone frequency in MHz in steps of 1 MHz from -10 MHz to +10 MHz.
 * @tx_power: Desired TX power in the range -16dBm to +24dBm.
 *
 * This function is used to send a command to RPU to start the RF TX tone test
 * in radio test mode.
 *
 * Returns: Status
 *              Pass : %WIFI_NRF_STATUS_SUCCESS
 *              Error: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status nrf_wifi_fmac_rf_test_tx_tone(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						  unsigned char enable,
						  signed char tone_freq,
						  signed char tx_power);



/**
 * nrf_wifi_fmac_rf_test_dpd() - Start/Stop RF DPD test in radio test mode.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @enable: Enable/Disable DPD test.
 *
 * This function is used to send a command to RPU to start
 * the RF DPD test in radio test mode.
 *
 * Returns: Status
 *              Pass : %WIFI_NRF_STATUS_SUCCESS
 *              Error: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status nrf_wifi_fmac_rf_test_dpd(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					       unsigned char enable);



/**
 * nrf_wifi_fmac_rf_get_temp() - Get temperature in fahrenheit using temperature sensor.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 *
 * This function is used to send a command to RPU to get the current temperature.
 * using the radio test mode.
 *
 * Returns: Status
 *              Pass : %WIFI_NRF_STATUS_SUCCESS
 *              Error: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status nrf_wifi_fmac_rf_get_temp(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx);



/**
 * nrf_wifi_fmac_rf_get_rf_rssi() - Get RF RSSI status.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 *
 * This function is used to send a command to RPU to get
 * RF RSSI status in radio test mode.
 *
 * Returns: Status
 *              Pass : %WIFI_NRF_STATUS_SUCCESS
 *              Error: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status nrf_wifi_fmac_rf_get_rf_rssi(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx);


/**
 * nrf_wifi_fmac_set_xo_val() - set XO adjustment value.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @value: XO adjustment value.
 *
 * This function is used to send a command to RPU to set XO adjustment
 * value in radio test mode.
 *
 * Returns: Status
 *              Pass : %WIFI_NRF_STATUS_SUCCESS
 *              Error: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status nrf_wifi_fmac_set_xo_val(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					      unsigned char value);

/**
 * nrf_wifi_fmac_rf_test_compute_optimal_xo_val() - Get XO calibrated value.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 *
 * This function is used to send a command to RPU. RPU estimates and
 * returns optimal XO value.
 *
 * Returns: Status
 *              Pass : %WIFI_NRF_STATUS_SUCCESS
 *              Error: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status nrf_wifi_fmac_rf_test_compute_xo(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx);
#else

/**
 * wifi_nrf_fmac_init() - Initializes the UMAC IF layer of the RPU WLAN FullMAC
 *                        driver.
 *
 * @rx_buf_pools : Pointer to configuration of Rx queue buffers.
 *		   See &struct rx_buf_pool_params
 *
 * This function initializes the UMAC IF layer of the RPU WLAN FullMAC driver.
 * It does the following:
 *
 *     - Creates and initializes the context for the UMAC IF layer.
 *     - Initializes the HAL layer.
 *     - Registers the driver to the underlying OS.
 *
 * Returns: Pointer to the context of the UMAC IF layer.
 */
struct wifi_nrf_fmac_priv *wifi_nrf_fmac_init(struct nrf_wifi_data_config_params *data_config,
					      struct rx_buf_pool_params *rx_buf_pools,
					      struct wifi_nrf_fmac_callbk_fns *callbk_fns);

/**
 * wifi_nrf_fmac_stats_get() - Issue a request to get stats from the RPU.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @stats_type: The type of RPU statistics to get.
 * @stats: Pointer to memory where the stats are to be copied.
 *
 * This function is used to send a command to
 * instruct the firmware to return the current RPU statistics. The RPU will
 * send the event with the current statistics.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_stats_get(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					     struct rpu_op_stats *stats);


/**
 * wifi_nrf_fmac_scan() - Issue a scan request to the RPU.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the scan is to be performed.
 * @scan_info: The parameters needed by the RPU for scan operation.
 *
 * This function is used to send a command to
 * instruct the firmware to trigger a scan. The scan can be performed in two
 * modes:
 *
 * Auto Mode (%AUTO_SCAN):
 *     In this mode the host does not need to specify any scan specific
 *     parameters. The RPU will perform the scan on all the channels permitted
 *     by and abiding by the regulations imposed by the
 *     WORLD (common denominator of all regulatory domains) regulatory domain.
 *     The scan will be performed using the wildcard SSID.
 *
 * Channel Map Mode (%CHANNEL_MAPPING_SCAN):
 *      In this mode the host can have fine grained control over the scan
 *      specific parameters to be used (e.g. Passive/Active scan selection,
 *      Number of probe requests per active scan, Channel list to scan,
 *      Permanence on each channel, SSIDs to scan etc.). This mode expects
 *      the regulatory restrictions to be taken care by the invoker of the
 *      API.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_scan(void *fmac_dev_ctx,
					unsigned char if_idx,
					struct nrf_wifi_umac_scan_info *scan_info);


/**
 * wifi_nrf_fmac_scan_res_get() - Issue a scan results request to the RPU.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the scan results are to be fetched.
 * @scan_type: The scan type (i.e. DISPLAY or CONNECT scan).
 *
 * This function is used to send a command to
 * instruct the firmware to return the results of a scan.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_scan_res_get(void *fmac_dev_ctx,
						unsigned char if_idx,
						int scan_type);

/**
 * wifi_nrf_fmac_scan() - Issue abort of an ongoing scan to the RPU.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the scan results are to be fetched.
 *
 * This function is used to send a command to
 * instruct the firmware to abort an ongoing scan.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_abort_scan(void *dev_ctx,
						unsigned char if_idx);

#ifdef CONFIG_WPA_SUPP
/**
 * wifi_nrf_fmac_auth() - Issue a authentication request to the RPU.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the authentication is to be
 *	    performed.
 * @auth_info: The parameters needed by the RPU to generate the authentication
 *             request.
 *
 * This function is used to send a command to
 * instruct the firmware to initiate a authentication request to an AP on the
 * interface identified with @if_idx.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_auth(void *fmac_dev_ctx,
					unsigned char if_idx,
					struct nrf_wifi_umac_auth_info *auth_info);


/**
 * wifi_nrf_fmac_deauth() - Issue a deauthentication request to the RPU.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the deauthentication is to be
 *          performed.
 * @deauth_info: Deauthentication specific information required by the RPU.
 *
 * This function is used to send a command to
 * instruct the firmware to initiate a deauthentication notification to an AP
 * on the interface identified with @if_idx.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_deauth(void *fmac_dev_ctx,
					  unsigned char if_idx,
					  struct nrf_wifi_umac_disconn_info *deauth_info);


/**
 * wifi_nrf_fmac_assoc() - Issue a association request to the RPU.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the association is to be
 *          performed.
 * @assoc_info: The parameters needed by the RPU to generate the association
 *              request.
 *
 * This function is used to send a command to
 * instruct the firmware to initiate a association request to an AP on the
 * interface identified with @if_idx.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_assoc(void *fmac_dev_ctx,
					 unsigned char if_idx,
					 struct nrf_wifi_umac_assoc_info *assoc_info);


/**
 * wifi_nrf_fmac_disassoc() - Issue a disassociation request to the RPU.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the disassociation is to be
 *	    performed.
 * @disassoc_info: Disassociation specific information required by the RPU.
 *
 * This function is used to send a command to
 * instruct the firmware to initiate a disassociation notification to an AP
 * on the interface identified with @if_idx.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_disassoc(void *fmac_dev_ctx,
					    unsigned char if_idx,
					    struct nrf_wifi_umac_disconn_info *disassoc_info);


/**
 * wifi_nrf_fmac_add_key() - Add a key into the RPU security database.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the key is to be added.
 * @key_info: Key specific information which needs to be passed to the RPU.
 * @mac_addr: MAC address of the peer with which the key is associated.
 *
 * This function is used to send a command to
 * instruct the firmware to add a key to its security database.
 * The key is for the peer identified by @mac_addr on the
 * interface identified with @if_idx.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_add_key(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_key_info *key_info,
					   const char *mac_addr);


/**
 * wifi_nrf_fmac_del_key() - Delete a key from the RPU security database.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface from which the key is to be deleted.
 * @key_info: Key specific information which needs to be passed to the RPU.
 * @mac_addr: MAC address of the peer with which the key is associated.
 *
 * This function is used to send a command to
 * instruct the firmware to delete a key from its security database.
 * The key is for the peer identified by @mac_addr  on the
 * interface identified with @if_idx.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_del_key(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_key_info *key_info,
					   const char *mac_addr);


/**
 * wifi_nrf_fmac_set_key() - Set a key as a default for data or management
 *                           traffic in the RPU security database.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the key is to be set.
 * @key_info: Key specific information which needs to be passed to the RPU.
 *
 * This function is used to send a command to
 * instruct the firmware to set a key as default in its security database.
 * The key is either for data or management traffic and is classified based on
 * the flags element set in @key_info parameter.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_set_key(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_key_info *key_info);


/**
 * wifi_nrf_fmac_set_bss() - Set BSS parameters for an AP mode interface to the
 *                           RPU.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the BSS parameters are to be set.
 * @bss_info: BSS specific parameters which need to be passed to the RPU.
 *
 * This function is used to send a command to
 * instruct the firmware to set the BSS parameter for an AP mode interface.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_set_bss(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_bss_info *bss_info);


/**
 * wifi_nrf_fmac_chg_bcn() - Update the Beacon Template.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the Beacon Template is to be updated.
 * @data: Beacon Template which need to be passed to the RPU.
 *
 * This function is used to send a command to
 * instruct the firmware to update beacon template for an AP mode interface.
 *
 * Returns: Status
 *              Pass: %WIFI_NRF_STATUS_SUCCESS
 *              Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_chg_bcn(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_set_beacon_info *data);

/**
 * wifi_nrf_fmac_start_ap() - Start a SoftAP.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the SoftAP is to be started.
 * @ap_info: AP operation specific parameters which need to be passed to the
 *           RPU.
 *
 * This function is used to send a command to
 * instruct the firmware to start a SoftAP on an interface identified with
 * @if_idx.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_start_ap(void *fmac_dev_ctx,
					    unsigned char if_idx,
					    struct nrf_wifi_umac_start_ap_info *ap_info);


/**
 * wifi_nrf_fmac_stop_ap() - Stop a SoftAP.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the SoftAP is to be stopped.
 *
 * This function is used to send a command to
 * instruct the firmware to stop a SoftAP on an interface identified with
 * @if_idx.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_stop_ap(void *fmac_dev_ctx,
					   unsigned char if_idx);


/**
 * wifi_nrf_fmac_start_p2p_dev() - Start P2P mode on an interface.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the P2P mode is to be started.
 *
 * This function is used to send a command to
 * instruct the firmware to start P2P mode on an interface identified with
 * @if_idx.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_p2p_dev_start(void *fmac_dev_ctx,
						 unsigned char if_idx);


/**
 * wifi_nrf_fmac_stop_p2p_dev() - Start P2P mode on an interface.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the P2P mode is to be started.
 *
 * This function is used to send a command to
 * instruct the firmware to start P2P mode on an interface identified with
 * @if_idx.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_p2p_dev_stop(void *fmac_dev_ctx,
						unsigned char if_idx);

/**
 * wifi_nrf_fmac_p2p_roc_start()
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface to be kept on channel and stay there.
 * @roc_info: Contans channel and time in ms to stay on.
 *
 * This function is used to send a command
 * to RPU to put p2p device in listen state for a duration.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_FMAC_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_FMAC_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_p2p_roc_start(void *fmac_dev_ctx,
						 unsigned char if_idx,
						 struct remain_on_channel_info *roc_info);

/**
 * wifi_nrf_fmac_p2p_roc_stop()
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx Index of the interface to be kept on channel and stay there.
 * @cookie: cancel p2p listen state of the matching cookie.
 *
 * This function is used to send a command
 * to RPU to put p2p device out
 * of listen state.
 *
 * Returns: Status
 *		 Pass: %WIFI_NRF_FMAC_STATUS_SUCCESS
 *		 Fail: %WIFI_NRF_FMAC_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_p2p_roc_stop(void *fmac_dev_ctx,
						unsigned char if_idx,
						unsigned long long cookie);

/**
 * wifi_nrf_fmac_mgmt_tx() - Transmit a management frame.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the frame is to be
 *              transmitted.
 * @mgmt_tx_info: Information regarding the management frame to be
 *                transmitted.
 *
 * This function is used to instruct the RPU to transmit a management frame.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_mgmt_tx(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_mgmt_tx_info *mgmt_tx_info);


/**
 * wifi_nrf_fmac_del_sta() - Remove a station.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the STA is connected.
 * @del_sta_info: Information regarding the station to be removed.
 *
 * This function is used to instruct the RPU to remove a station and send a
 * deauthentication/disassociation frame to the same.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_del_sta(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_del_sta_info *del_sta_info);


/**
 * wifi_nrf_fmac_add_sta() - Indicate a new STA connection to the RPU.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the STA is connected.
 * @add_sta_info: Information regarding the new station.
 *
 * This function is used to indicate to the RPU that a new STA has
 * successfully connected to the AP.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_add_sta(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_add_sta_info *add_sta_info);

/**
 * wifi_nrf_fmac_chg_sta() - Indicate changes to STA connection parameters to
 *                           the RPU.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the STA is connected.
 * @chg_sta_info: Information regarding updates to the station parameters.
 *
 * This function is used to indicate changes in the connected STA parameters
 * to the RPU.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_chg_sta(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_chg_sta_info *chg_sta_info);



/**
 * wifi_nrf_fmac_mgmt_frame_reg() - Register the management frame type which
 *                                  needs to be sent up to the host by the RPU.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface from which the received frame is to be
 *              sent up.
 * @frame_info: Information regarding the management frame to be sent up.
 *
 * This function is used to send a command to
 * instruct the firmware to pass frames matching that type/subtype to be
 * passed upto the host driver.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_mgmt_frame_reg(void *fmac_dev_ctx,
						  unsigned char if_idx,
						  struct nrf_wifi_umac_mgmt_frame_info *frame_info);

#endif /* CONFIG_WPA_SUPP */
/**
 * wifi_nrf_fmac_mac_addr() - Get unused MAC address from base mac address.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @addr: Memory to copy the mac address to.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_mac_addr(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					    unsigned char *addr);

/**
 * wifi_nrf_fmac_vif_idx_get() - Assign a index for a new VIF.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 *
 * This function searches for an unused VIF index and returns it.
 *
 * Returns: Index to be used for a new VIF
 *          In case of error @MAX_VIFS will be returned.
 */
unsigned char wifi_nrf_fmac_vif_idx_get(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx);

/**
 * wifi_nrf_fmac_add_vif() - Add a new virtual interface.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @os_vif_ctx: Pointer to VIF context that the user of the UMAC IF would like
 *              to be passed during invocation of callback functions like
 *              @rx_frm_callbk_fn etc.
 * @vif_info: Information regarding the interface to be added.
 *
 * This function is used to send a command to
 * instruct the firmware to add a new interface with parameters specified by
 * @vif_info.
 *
 * Returns: Index (maintained by the UMAC IF layer) of the VIF that was added.
 *          In case of error @MAX_VIFS will be returned.
 */
unsigned char wifi_nrf_fmac_add_vif(void *fmac_dev_ctx,
				    void *os_vif_ctx,
				    struct nrf_wifi_umac_add_vif_info *vif_info);


/**
 * wifi_nrf_fmac_del_vif() - Deletes a virtual interface.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface to be deleted.
 *
 * This function is used to send a command to
 * instruct the firmware to delete an interface which was added using
 * @wifi_nrf_fmac_add_vif.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_del_vif(void *fmac_dev_ctx,
					   unsigned char if_idx);

/**
 * wifi_nrf_fmac_chg_vif() - Change the attributes of an interface.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the functionality is to be
 *              bound.
 * @vif_info: Interface specific information which needs to be passed to the
 *            RPU.
 *
 * This function is used to change the attributes of an interface identified
 * with @if_idx.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_chg_vif(void *fmac_dev_ctx,
					   unsigned char if_idx,
					   struct nrf_wifi_umac_chg_vif_attr_info *vif_info);


/**
 * wifi_nrf_fmac_chg_vif_state() - Change the state of a virtual interface.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface whose state needs to be changed.
 * @vif_info: State information to be changed for the interface.
 *
 * This function is used to send a command to
 * instruct the firmware to change the state of an interface with an index of
 * @if_idx and parameters specified by @vif_info.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_chg_vif_state(void *fmac_dev_ctx,
						 unsigned char if_idx,
						 struct nrf_wifi_umac_chg_vif_state_info *vif_info);


/**
 * wifi_nrf_fmac_set_vif_macaddr() - Set MAC address on interface.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface whose MAC address is to be changed.
 * @mac_addr: MAC address to set.
 *
 * This function is used to change the MAC address of an interface identified
 * with @if_idx.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_set_vif_macaddr(void *fmac_dev_ctx,
						   unsigned char if_idx,
						   unsigned char *mac_addr);

/**
 * wifi_nrf_fmac_start_xmit() - Trasmit a frame to the RPU.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the frame is to be
 *              transmitted.
 * @netbuf: Pointer to the OS specific network buffer.
 *
 * This function takes care of transmitting a frame to the RPU. It does the
 * following:
 *
 *     - Queues the frames to a transmit queue.
 *     - Based on token availability sends one or more frames to the RPU using
 *       the command for transmission.
 *     - The firmware sends an event once the command has
 *       been processed send to indicate whether the frame(s) have been
 *       transmited/aborted.
 *     - The diver can cleanup the frame buffers after receiving this event.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_start_xmit(void *fmac_dev_ctx,
					      unsigned char if_idx,
					      void *netbuf);

/**
 * wifi_nrf_fmac_suspend() - Inform the RPU that host is going to suspend
 *                           state.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 *
 * This function is used to send a command to
 * inform the RPU that host is going to suspend state.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_suspend(void *fmac_dev_ctx);


/**
 * wifi_nrf_fmac_resume() - Notify RPU that host has resumed from a suspended
 *                          state.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 *
 * This function is used to send a command
 * to inform the RPU that host has resumed from a suspended state.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_resume(void *fmac_dev_ctx);


/**
 * wifi_nrf_fmac_get_tx_power() - Get tx power
 *
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: VIF index.
 *
 * This function is used to send a command
 * to get the transmit power on a particular interface.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_get_tx_power(void *fmac_dev_ctx,
						unsigned int if_idx);

/**
 * wifi_nrf_fmac_get_channel() - Get channel definition.
 *
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: VIF index.
 *
 * This function is used to send a command
 * to get the channel configured on a particular interface.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_get_channel(void *fmac_dev_ctx,
					       unsigned int if_idx);

/**
 * wifi_nrf_fmac_get_station() - Get station statistics
 *
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: VIF index.
 * @mac : MAC address of the station
 *
 * This function is used to send a command
 * to get station statistics using a mac address.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_get_station(void *fmac_dev_ctx,
					       unsigned int if_idx,
					       unsigned char *mac);


/* wifi_nrf_fmac_get_interface() - Get interface statistics
 *
 * @dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: VIF index.
 *
 * This function is used to send a command
 * to get interface statistics using interface index.
 *
 * Returns: Status
 *              Pass: %WIFI_NRF_STATUS_SUCCESS
 *              Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_get_interface(void *dev_ctx,
					       unsigned int if_idx);


/**
 * wifi_nrf_fmac_set_power_save() - Configure WLAN power management.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which power management is to be set.
 * @state: State (Enable/Disable) of WLAN power management.
 *
 * This function is used to send a command
 * to RPU to Enable/Disable WLAN Power management.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_set_power_save(void *fmac_dev_ctx,
						  unsigned char if_idx,
						  bool state);

/**
 * nrf_wifi_fmac_set_uapsd_queue() - Configure WLAN uapsd queue.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which power management is to be set.
 * @uapsd_queue: Uapsd_queue to be used.
 *
 * This function is used to send a command (%NRF_WIFI_UMAC_CMD_CONFIG_UAPSD)
 * to RPU to set uapsd queue value.
 *
 * Returns: Status
 *              Pass : %WIFI_NRF_STATUS_SUCCESS
 *              Fail : %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_set_uapsd_queue(void *fmac_dev_ctx,
						   unsigned char if_idx,
						   unsigned int uapsd_queue);

/**
 * nrf_wifi_fmac_set_power_save_timeout() - Configure Power save timeout.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which power management is to be set.
 * @ps_timeout: Power save inactivity time.
 *
 * This function is used to send a command
 * (%NRF_WIFI_UMAC_CMD_SET_POWER_SAVE_TIMEOUT) to RPU to set power save
 * inactivity time.
 *
 * Returns: Status
 *              Pass : %WIFI_NRF_STATUS_SUCCESS
 *              Fail : %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_set_power_save_timeout(void *dev_ctx,
							  unsigned char if_idx,
							  int ps_timeout);

/**
 * wifi_nrf_fmac_set_qos_map() - Configure qos_map of for data
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the qos map be set.
 * @qos_info: qos_map information
 *
 * This function is used to send a command
 * to RPU to set qos map information.
 *
 * Returns: Status
 *              Pass: %WIFI_NRF_STATUS_SUCCESS
 *              Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_set_qos_map(void *fmac_dev_ctx,
					       unsigned char if_idx,
					       struct nrf_wifi_umac_qos_map_info *qos_info);

/**
 * wifi_nrf_fmac_set_wowlan() - Configure WOWLAN.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @var: Wakeup trigger condition.
 *
 * This function is used to send a command
 * to RPU to configure Wakeup trigger condition in RPU.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_set_wowlan(void *fmac_dev_ctx,
					      unsigned int var);

/**
 * wifi_nrf_fmac_get_wiphy() - Get PHY configuration.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the CMD needs to be sent.
 *
 * This function is used to get PHY configuration from RPU.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_get_wiphy(void *fmac_dev_ctx, unsigned char if_idx);

/**
 * wifi_nrf_fmac_register_frame() - Register to get MGMT frames.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the CMD needs to be sent.
 * @frame_info: Information regarding the management frame.
 *
 * Register with RPU to receive MGMT frames.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_register_frame(void *dev_ctx, unsigned char if_idx,
						  struct nrf_wifi_umac_mgmt_frame_info *frame_info);

/**
 * wifi_nrf_fmac_set_wiphy_params() - set wiphy parameters
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @wiphy_info: wiphy parameters
 *
 * This function is used to send a command
 * to RPU to configure parameters relted to interface.
 *
 * Returns: Status
 *              Pass: %WIFI_NRF_STATUS_SUCCESS
 *              Fail: %WIFI_NRF_STATUS_FAIL
 */

enum wifi_nrf_status wifi_nrf_fmac_set_wiphy_params(void *fmac_dev_ctx,
						 unsigned char if_idx,
						 struct nrf_wifi_umac_set_wiphy_info *wiphy_info);

/**
 * wifi_nrf_fmac_twt_setup() - TWT setup command
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the TWT parameters be set.
 * @twt_info: TWT parameters
 *
 * This function is used to send a command
 * to RPU to configure parameters relted to TWT setup.
 *
 * Returns: Status
 *              Pass: %WIFI_NRF_STATUS_SUCCESS
 *              Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_twt_setup(void *fmac_dev_ctx,
					     unsigned char if_idx,
					     struct nrf_wifi_umac_config_twt_info *twt_info);

/**
 * wifi_nrf_fmac_twt_teardown() - TWT teardown command
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which the TWT parameters be set.
 * @twt_info: TWT parameters
 *
 * This function is used to send a command
 * to RPU to tear down an existing TWT session.
 *
 * Returns: Status
 *              Pass: %WIFI_NRF_STATUS_SUCCESS
 *              Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_twt_teardown(void *fmac_dev_ctx,
						unsigned char if_idx,
						struct nrf_wifi_umac_config_twt_info *twt_info);

/**
 * wifi_nrf_fmac_get_conn_info() - Get connection info from RPU
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface.
 *
 * This function is used to send a command
 * to RPU to fetch the connection information.
 *
 * Returns: Status
 *              Pass: %WIFI_NRF_STATUS_SUCCESS
 *              Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_get_conn_info(void *fmac_dev_ctx,
						unsigned char if_idx);
#endif /* !CONFIG_NRF700X_RADIO_TEST */


/**
 * wifi_nrf_fmac_deinit() - De-initializes the UMAC IF layer of the RPU WLAN
 *                          FullMAC driver.
 * @fpriv: Pointer to the context of the UMAC IF layer.
 *
 * This function de-initializes the UMAC IF layer of the RPU WLAN FullMAC
 * driver. It does the following:
 *
 *     - De-initializes the HAL layer.
 *     - Frees the context for the UMAC IF layer.
 *
 * Returns: None
 */
void wifi_nrf_fmac_deinit(struct wifi_nrf_fmac_priv *fpriv);


/**
 * wifi_nrf_fmac_dev_add() - Adds a RPU instance.
 * @fpriv: Pointer to the context of the UMAC IF layer.
 *
 * This function adds an RPU instance. This function will return the
 * pointer to the context of the RPU instance. This pointer will need to be
 * supplied while invoking further device specific API's,
 * e.g. @wifi_nrf_fmac_scan etc.
 *
 * Returns: Pointer to the context of the RPU instance.
 */
struct wifi_nrf_fmac_dev_ctx *wifi_nrf_fmac_dev_add(struct wifi_nrf_fmac_priv *fpriv,
						    void *os_dev_ctx);


/**
 * wifi_nrf_fmac_dev_rem() - Removes a RPU instance.
 * @fmac_dev_ctx: Pointer to the context of the RPU instance to be removed.
 *
 * This function handles the removal of an RPU instance at the UMAC IF layer.
 *
 * Returns: None.
 */
void wifi_nrf_fmac_dev_rem(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx);


/**
 * wifi_nrf_fmac_dev_init() - Initializes a RPU instance.
 * @fmac_dev_ctx: Pointer to the context of the RPU instance to be removed.
 * @rf_params_usr: RF parameters (if any) to be passed to the RPU.
 * @sleep_type: Type of RPU sleep.
 * @phy_calib: PHY calibration flags to be passed to the RPU.
 * @ant_gain_2g: Antenna gain value for 2.4 GHz band.
 * @ant_gain_5g_band1: Antenna gain value for 5 GHz band (5150 MHz - 5350 MHz).
 * @ant_gain_5g_band2: Antenna gain value for 5 GHz band (5470 MHz - 5730 MHz).
 * @ant_gain_5g_band3: Antenna gain value for 5 GHz band (5730 MHz - 5895 MHz).
 *
 * This function initializes the firmware of an RPU instance.
 *
 * Returns:
 *		Pass: WIFI_NRF_STATUS_SUCCESS.
 *		Fail: WIFI_NRF_STATUS_FAIL.
 */
enum wifi_nrf_status wifi_nrf_fmac_dev_init(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
#ifndef CONFIG_NRF700X_RADIO_TEST
					    unsigned char *rf_params_usr,
#endif /* !CONFIG_NRF700X_RADIO_TEST */
#ifdef CONFIG_NRF_WIFI_LOW_POWER
					    int sleep_type,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
					    unsigned int phy_calib,
					    unsigned char ant_gain_2g,
					    unsigned char ant_gain_5g_band1,
					    unsigned char ant_gain_5g_band2,
					    unsigned char ant_gain_5g_band3);


/**
 * wifi_nrf_fmac_dev_deinit() - De-initializes a RPU instance.
 * @fmac_dev_ctx: Pointer to the context of the RPU instance to be removed.
 *
 * This function de-initializes the firmware of an RPU instance.
 *
 * Returns: None.
 */
void wifi_nrf_fmac_dev_deinit(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx);


/**
 * wifi_nrf_fmac_fw_load() - Loads the Firmware(s) to the RPU WLAN device.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device, which was
 *            passed as @fmac_dev_ctx parameter via the @add_dev_callbk_fn
 *            callback function.
 * @fmac_fw: Information about the FullMAC firmware(s) to be loaded.
 *
 * This function loads the FullMAC firmware(s) to the RPU WLAN device.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_fw_load(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					   struct wifi_nrf_fmac_fw_info *fmac_fw);


/**
 * wifi_nrf_fmac_ver_get() - Get FW versions from the RPU.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @umac_ver: Pointer to the address where the UMAC version needs to be copied.
 * @lmac_ver: Pointer to the address where the LMAC version needs to be copied.
 *
 * This function is used to get Firmware versions from the RPU.
 *
 * Returns: Status
 *		Pass: %WIFI_NRF_STATUS_SUCCESS
 *		Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_ver_get(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					  unsigned int *umac_ver,
					  unsigned int *lmac_ver);


/**
 * wifi_nrf_fmac_conf_btcoex() - Configure BT-Coex parameters in RPU.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @params: BT-coex parameters which will be configured in RPU.
 *
 * This function is used to send a command to RPU to configure
 * BT-Coex parameters in RPU.
 *
 * Returns: Status
 *              Pass: %WIFI_NRF_STATUS_SUCCESS
 *              Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_conf_btcoex(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					       void *cmd, unsigned int cmd_len);

/**
 * wifi_nrf_fmac_conf_ltf_gi() - Configure HE LTF and GI parameters.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @he_ltf: HE LTF parameter which will be configured in RPU.
 * @he_gi: HE GI parameter which will be configured in RPU
 * @enabled: enable/disable HE LTF and GI parameter configured
 *
 * This function is used to send a command to RPU
 * to configure HE ltf and gi parameters in RPU.
 *
 * Returns: Status
 *              Pass : %WIFI_NRF_STATUS_SUCCESS
 *              Error: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_conf_ltf_gi(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					       unsigned char he_ltf,
					       unsigned char he_gi,
					       unsigned char enabled);


/**
 * wifi_nrf_fmac_set_mcast_addr - set the Multicast filter address.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface whose state needs to be changed.
 * @mcast_info: Multicast information to be set
 *
 * This function is used to send a command (%NRF_WIFI_UMAC_CMD_MCAST_FILTER) to
 * instruct the firmware to set the multicast filter address to an interface
 * with index @if_idx and parameters specified by @mcast_info.
 *
 * Returns: Status
 *              Pass : %WIFI_NRF_STATUS_SUCCESS
 *              Error: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_set_mcast_addr(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						  unsigned char if_idx,
						  struct nrf_wifi_umac_mcast_cfg *mcast_info);


/**
 * wifi_nrf_fmac_otp_mac_addr_get() - Fetch MAC address from OTP.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @vif_idx: Interface index for which the MAC address is to be fetched.
 * @mac_addr: Pointer to the address where the MAC address needs to be copied.
 *
 * This function is used to fetch MAC address from the OTP.
 *
 * Returns: Status
 *              Pass : %WIFI_NRF_STATUS_SUCCESS
 *              Error: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_otp_mac_addr_get(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						    unsigned char vif_idx,
						    unsigned char *mac_addr);


/**
 * wifi_nrf_fmac_rf_params_get() - Get the RF parameters to be programmed to the RPU.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @rf_params: Pointer to the address where the RF params information needs to be copied.
 *
 * This function is used to fetch RF parameters information from the RPU and
 * update the default RF parameter with the OTP values. The updated RF
 * parameters are then returned in the @rf_params.
 *
 * Returns: Status
 *              Pass : %WIFI_NRF_STATUS_SUCCESS
 *              Error: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_rf_params_get(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						 unsigned char *rf_params);

struct wifi_nrf_fmac_reg_info {
	unsigned char alpha2[NRF_WIFI_COUNTRY_CODE_LEN];
	bool force;
};

/**
 * wifi_nrf_fmac_set_reg() - Set regulatory domain in RPU.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @reg_info: Pointer to the address where the regulatory domain information
 *		  needs to be copied.
 * This function is used to set regulatory domain in the RPU.
 * Returns: Status
 *			Pass : %WIFI_NRF_STATUS_SUCCESS
 *			Error: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_set_reg(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					   struct wifi_nrf_fmac_reg_info *reg_info);

/**
 * wifi_nrf_fmac_get_reg() - Get regulatory domain from RPU.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @reg_info: Pointer to the address where the regulatory domain information
 *		  needs to be copied.
 * This function is used to get regulatory domain from the RPU.
 * Returns: Status
 *			Pass : %WIFI_NRF_STATUS_SUCCESS
 *			Error: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_get_reg(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					   struct wifi_nrf_fmac_reg_info *reg_info);

/**
 * wifi_nrf_fmac_get_power_save_info() - Get the current power save info.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @if_idx: Index of the interface on which power management is to be set.
 *
 * This function is used to send a command
 * to RPU to Enable/Disable WLAN Power management.
 *
 * Returns: Status
 *              Pass: %WIFI_NRF_STATUS_SUCCESS
 *              Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_get_power_save_info(void *fmac_dev_ctx,
						       unsigned char if_idx);

#ifdef CONFIG_NRF700X_UTIL
enum wifi_nrf_status wifi_nrf_fmac_set_tx_rate(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					       unsigned char rate_flag,
					       int data_rate);
#ifdef CONFIG_NRF_WIFI_LOW_POWER
/**
 * wifi_nrf_fmac_get_host_ref_rpu_ps_ctrl_state() - Get the RPU power save
 *			status from host perspective.
 * @fmac_dev_ctx: Pointer to the UMAC IF context for a RPU WLAN device.
 * @rpu_ps_ctrl_state: Pointer to the address where the current rpu ps state
 *			from host perspective needs to be copied.
 *
 * This function is used to fetch the RPU power save status
 * from host perspective.
 * Returns: Status
 *              Pass: %WIFI_NRF_STATUS_SUCCESS
 *              Fail: %WIFI_NRF_STATUS_FAIL
 */
enum wifi_nrf_status wifi_nrf_fmac_get_host_rpu_ps_ctrl_state(void *fmac_dev_ctx,
							      int *rpu_ps_ctrl_state);
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
#endif
#endif /* __FMAC_API_H__ */
