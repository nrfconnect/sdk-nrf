/*
 *
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @addtogroup nrf71_wifi_fw_if Wi-Fi driver and firmware interface
 * @{
 * @brief Coexistence interface between host and RPU
 */

#ifndef __NRF71_COEX_IF_H__
#define __NRF71_COEX_IF_H__

#include "common/pack_def.h"

/*
 * The shared SR priority-range structure (struct coex_sr_priority_range_t) and
 * the CCCONF range constant are defined by the CD to Short-Range interface
 * header. The Coexistence Manager uses only the regular Rx/Tx ranges from it;
 * the critical ranges are consumed by the Short-Range driver.
 */
#include "nrf71_cd_sr_if.h"

/** Number of elements in SW priority range buffers (min, max, step). */
#define NUM_ELEMENTS_IN_SW_PTI_RANGE 3U

#define NRF_COEX_PARAMS                                                                            \
	"0100000001000000140000000100000080000000030000000300000032000000CE0000001000000010000000" \
	"01000000010000000100010100010100010100000100"

/**
 * COEXC mode for Wi-Fi band.
 *
 * Indicates the COEXC mode based on Wi-Fi operating frequency band.
 */
enum coexc_mode_wifi_t {
	/** COEXC mode - Wi-Fi operating in 2.4G. */
	COEXC_MODE_WIFI_2PT4G = 0,
	/** COEXC mode - Wi-Fi operating in 5G. */
	COEXC_MODE_WIFI_5G,
	/** COEXC mode - Wi-Fi operating in 6G. Currently unused as band sel register is of 1-bit.
	 */
	COEXC_MODE_WIFI_6G,
};

/**
 * Wi-Fi SW client request status.
 *
 * Indicates whether the Wi-Fi SW client request was granted or not.
 */
enum coex_wifi_sw_client_req_status_t {
	/** Indicates the SW client request is granted. */
	WIFI_SW_CLIENT_REQ_SUCCESS = 0,
	/** Indicates the SW client request is NOT granted. */
	WIFI_SW_CLIENT_REQ_FAIL
};

/**
 * Different CM event types.
 *
 * Indicates event (response to latest received command) of the CM.
 */
enum cm_event_to_host_t {
	/** Response to CD2CM_GET_STATS command */
	STATISTICS_EVENT = 0,
	/** Response to CD2CM_WIFI_SW_CLIENT_REQUEST command */
	SW_CLIENT_STATUS_EVENT
};

/**
 * Coexistence related statistics.
 *
 * Contains coexistence related statistics for monitoring and debugging.
 */
struct cm_stats_t {
	/** Coex initialization count */
	unsigned int init_count;

	/** PPW start request count. */
	unsigned int ppw_start_msgs_cnt;
	/** PPW stop request count. */
	unsigned int ppw_stop_msgs_cnt;
	/** PPW stop timeout count */
	unsigned int ppw_stop_timeout_cnt;
	/** Number of Wi-Fi priority windows in PPWs generation. */
	unsigned int num_wifi_pti_windows;
	/** Number of SR priority windows in PPWs generation. */
	unsigned int num_sr_pti_windows;
	/** Priority Value Statistics */
	/** Wi-Fi CCCONF for Wi-Fi window. */
	unsigned int wifi_ccconf_wifi_window;
	/** Wi-Fi CCCONF for SR window. */
	unsigned int wifi_ccconf_sr_window;
	/** Invalid Wi-Fi/SR window durations. */
	unsigned int ppw_invalid_params_cnt;
	/** Invalid request (other than start/stop). */
	unsigned int ppw_invalid_request_cnt;

	/** Wi-Fi operating in 5G count. */
	unsigned int cfg_band_sel_5g_cnt;
	/** Wi-Fi operating in 2.4G count. */
	unsigned int cfg_band_sel_2pt4g_cnt;

	/** Number of Wi-Fi SW client requests. */
	unsigned int wifi_sw_client_req_cnt;
	/** Number of Wi-Fi SW client releases. */
	unsigned int wifi_sw_client_rel_cnt;
	/** Number of Wi-Fi SW client grants. */
	unsigned int wifi_sw_client_gnt_cnt;
	/** Number of Wi-Fi SW client release timeout. */
	unsigned int wifi_sw_client_rel_timeout_cnt;
	/** Indicates Wi-Fi SW client requests in non 2.4G band */
	unsigned int wifi_sw_client_req_non2pt4g_band_cnt;
	/** Indicates Wi-Fi SW client requests in 2.4G band */
	unsigned int wifi_sw_client_req_2pt4g_band_cnt;
	/** Number of Wi-Fi SW client decision in favour in 2.4G. */
	unsigned int wifi_sw_client_decison_in_favour_cnt_2pt4g;
	/** Number of Wi-Fi SW client decision not in favour in 2.4G. */
	unsigned int wifi_sw_client_decison_not_in_favour_cnt_2pt4g;
	/** Number of Wi-Fi SW client no-grants in 2.4G. */
	unsigned int wifi_sw_client_no_gnt_cnt_2pt4g;
	/** Number of Wi-Fi SW client no-grants in 2.4G. */
	unsigned int wifi_sw_client_no_gnt_cnt_non_2pt4g;
	/** Number of Wi-Fi SW client releases in 2.4G. */
	unsigned int wifi_sw_client_rel_cnt_2pt4g;
	/** Number of Wi-Fi SW client releases in non 2.4G. */
	unsigned int wifi_sw_client_rel_cnt_non_2pt4g;
	/** Number of Wi-Fi SW client grants in 2.4G. */
	unsigned int wifi_sw_client_gnt_cnt_2pt4g;
	/** Number of Wi-Fi SW client grants in non 2.4G. */
	unsigned int wifi_sw_client_gnt_cnt_non_2pt4g;

	/** Number of diff Wi-Fi SW client requests. */
	unsigned int wifi_beacon_rx_req_cnt;
	unsigned int wifi_conn_req_cnt;
	unsigned int wifi_calib_req_cnt;
	unsigned int wifi_scan_req_cnt;

	/** Number of diff Wi-Fi SW client releases. */
	unsigned int wifi_beacon_rx_rel_cnt;
	unsigned int wifi_conn_rel_cnt;
	unsigned int wifi_calib_rel_cnt;
	unsigned int wifi_scan_rel_cnt;

	/** Number of diff Wi-Fi SW client grants. */
	unsigned int wifi_beacon_rx_gnt_cnt;
	unsigned int wifi_conn_gnt_cnt;
	unsigned int wifi_calib_gnt_cnt;
	unsigned int wifi_scan_gnt_cnt;

	/** Number of diff Wi-Fi SW client grants. */
	unsigned int wifi_beacon_rx_no_gnt_cnt;
	unsigned int wifi_conn_no_gnt_cnt;
	unsigned int wifi_calib_no_gnt_cnt;
	unsigned int wifi_scan_no_gnt_cnt;

	/** Number of diff Wi-Fi SW client release timeouts. */
	unsigned int wifi_beacon_rx_rel_to_cnt;
	unsigned int wifi_conn_rel_to_cnt;
	unsigned int wifi_calib_rel_to_cnt;
	unsigned int wifi_scan_rel_to_cnt;

	/*
	 * Additional Debug Statistics for System-Level Debugging
	 */

	/** SR Rx Protection Statistics */
	/** SR Rx protection success count, 2.4G. */
	unsigned int sr_rx_prot_success_cnt_2pt4g;
	/** SR Rx protection fail count, 2.4G. */
	unsigned int sr_rx_prot_fail_cnt_2pt4g;
	/** SR Rx protection success count, non 2.4G. */
	unsigned int sr_rx_prot_success_cnt_non_2pt4g;
	/** SR Rx protection fail count, non 2.4G. */
	unsigned int sr_rx_prot_fail_cnt_non_2pt4g;
	/** SR Rx protection - inactive2listen PS scenario count, 2.4G. */
	unsigned int sr_rx_prot_inact2listen_ps_cnt_2pt4g;
	/** SR Rx protection - listen2inactive PS scenario count, 2.4G. */
	unsigned int sr_rx_prot_listen2inact_ps_cnt_2pt4g;
	/** SR Rx protection - inactive2listen calib scenario count, 2.4G. */
	unsigned int sr_rx_prot_inact2listen_calib_cnt_2pt4g;
	/** SR Rx protection - listen2inactive calib scenario count, 2.4G. */
	unsigned int sr_rx_prot_listen2inact_calib_cnt_2pt4g;
	/** SR Rx protection - inactive2listen PS scenario count, non 2.4G. */
	unsigned int sr_rx_prot_inact2listen_ps_cnt_non_2pt4g;
	/** SR Rx protection - listen2inactive PS scenario count, non 2.4G. */
	unsigned int sr_rx_prot_listen2inact_ps_cnt_non_2pt4g;
	/** SR Rx protection - inactive2listen calib scenario count, non 2.4G. */
	unsigned int sr_rx_prot_inact2listen_calib_cnt_non_2pt4g;
	/** SR Rx protection - listen2inactive calib scenario count, non 2.4G. */
	unsigned int sr_rx_prot_listen2inact_calib_cnt_non_2pt4g;

	/** Wi-Fi 2.4G band  */
	unsigned int sr_rx_prot_2pt4g_band_cnt;
	/** Wi-Fi non 2.4G band - no SR conflict). */
	unsigned int sr_rx_prot_non2pt4g_band_cnt;
	/** Force Wi-Fi protection count during SR Rx protection. */
	unsigned int sr_rx_prot_force_wifi_cnt_2pt4g;

	/** COEX Enable/Disable Statistics */
	/** Number of times COEX was enabled. */
	unsigned int coex_enable_cnt;
	/** Number of times COEX was disabled. */
	unsigned int coex_disable_cnt;

	/** Number of times ANTSWC inhibit was configured. */
	unsigned int antswc_inhibit_cfg_cnt;
	/** Number of times ANTSWC antenna override was configured. */
	unsigned int antswc_anten_override_cfg_cnt;
	/** Number of times ANTSWC PA override was configured. */
	unsigned int antswc_pa_override_cfg_cnt;
	/** Number of times ANTSWC LNA switch override was configured. */
	unsigned int antswc_lnasw_override_cfg_cnt;

	/** CCCONF Save/Restore Statistics */
	/** Number of times Wi-Fi CCCONF was saved. */
	unsigned int wifi_ccconf_save_cnt;
	/** Number of times Wi-Fi CCCONF was restored. */
	unsigned int wifi_ccconf_restore_cnt;

	/** Command Processing Statistics */
	/** Number of CD2CM_UPDATE_COEX_PARAMS commands processed. */
	unsigned int cmd_update_coex_params_cnt;
	/** Number of CD2CM_UPDATE_COEX_USER_PARAMS commands processed. */
	unsigned int cmd_update_user_params_cnt;
	/** Number of CD2CM_ENABLE_COEXISTENCE commands processed. */
	unsigned int cmd_enable_coex_cnt;
	/** Number of CD2CM_ALLOCATE_PPW commands processed. */
	unsigned int cmd_allocate_ppw_cnt;
	/** Number of CD2CM_SET_PRIORITY_RANGES commands processed. */
	unsigned int cmd_set_pti_ranges_cnt;
	/** Number of CD2CM_GET_STATS commands processed. */
	unsigned int cmd_get_stats_cnt;
	/** Number of CD2CM_WIFI_SW_CLIENT_REQUEST commands processed. */
	unsigned int cmd_sw_client_req_cnt;

	/** Event Statistics */
	/** Number of statistics events sent to host. */
	unsigned int stats_event_to_host_cnt;
	/** Number of SW client status events sent to host. */
	unsigned int sw_client_event_to_host_cnt;

	/** Error counts */
	/** invalid CD2CM message ID count */
	unsigned int cd2cm_null_cmd_buf_cnt;
	/** invalid CD2CM message ID count */
	unsigned int cd2cm_invalid_msg_id_cnt;
	/** Coex parameters command buffer address is null */
	unsigned int coex_params_buf_error;
	/** User parameters command buffer address is null */
	unsigned int user_params_buf_error;
	/** Enable coex command buffer address is null */
	unsigned int en_coex_buf_error;
	/** PPW generation command buffer address is null */
	unsigned int ppw_buf_error;
	/** Set priority ranges command buffer address is null */
	unsigned int set_pti_ranges_buf_error;
	/** SW cclient request command buffer address is null */
	unsigned int sw_client_req_buf_error;
	/** Number of times event buffer was unavailable. */
	unsigned int event_buf_unavailable_cnt;
	/** Number of unknown events to host */
	unsigned int unknown_event_to_host;
	/** Indicates a wrong Wi-Fi SW client name. */
	unsigned int wifi_wrong_sw_client_id_cnt;
	/** Indicates a wrong Wi-Fi SW client request type. */
	unsigned int wifi_wrong_sw_client_request_type_cnt;
	/** Wrong ANTSWC control type. */
	unsigned int wrong_antswc_ctrl_type_cnt;
	/** Wrong Wi-Fi HW client PTI level. */
	unsigned int wrong_wifi_hw_client_pti_level_cnt;
	/** Wrong input to populate priority from range */
	unsigned int wrong_input_populate_pti_cnt;
	/** Wrong input to populate priority from range */
	unsigned int wrong_sr_rx_prot_scenario_cnt;
	/** Wrong SR Rx protection percentage probability */
	unsigned int wrong_sr_rx_prot_perc_prob_cnt;
	/** Wrong Wi-Fi software client ID */
	unsigned int wrong_wifi_sw_client_id_cnt;
	/** Wrong Wi-Fi SW client percentage probability */
	unsigned int wrong_wifi_sw_client_perc_prob_cnt;
	/** Wrong PPW Wi-Fi or SR window duration (zero) */
	unsigned int zero_ppw_win_duration_cnt;
	/** Wrong PPW timeout value (zero) */
	unsigned int zero_ppw_timeout_cnt;
};

/**
 * Message IDs from Coexistence Driver to Coexistence Manager.
 *
 * IDs of different messages posted from Coexistence Driver (CD) to
 * Coexistence Manager (CM) for command routing.
 */
enum cd2cm_msg_id_t {
	/** To enable coexistence. */
	CD2CM_ENABLE_COEXISTENCE = 0,
	/** To allocate periodic priority windows to Wi-Fi and SR. */
	CD2CM_ALLOCATE_PPW,
	/** To set Wi-Fi (SW and HW) and SR SW priority ranges. */
	CD2CM_SET_PRIORITY_RANGES,
	/** To initialize coexistence user parameters. */
	CD2CM_UPDATE_COEX_USER_PARAMS,
	/** To initialize coexistence parameters. */
	CD2CM_UPDATE_COEX_PARAMS,
	/** To post a SW client request to protect Wi-Fi activity */
	CD2CM_WIFI_SW_CLIENT_REQUEST,
	/** To get all the CM stats. */
	CD2CM_GET_STATS,
	/** Total number of valid message IDs. */
	CD2CM_MSG_ID_COUNT
};

/**
 * PPW allocation control.
 *
 * Indicates if allocation of Periodic Priority Windows (PPWs) is to be
 * started or stopped.
 */
enum start_stop_ppw_t {
	/** To stop allocation of windows. */
	STOP_ALLOC_WINDOWS = 0,
	/** To start allocation of windows. */
	START_ALLOC_WINDOWS
};

/**
 * Radio selection for first priority window.
 *
 * Indicates to which radio the first priority window of PPWs to be allocated.
 */
enum coex_radios_t {
	/** Allocate first window to Wi-Fi radio. */
	WIFI_RADIO = 0,
	/** Allocate first window to SR radio. */
	SR_RADIO
};

/**
 * Antenna allocation mode.
 *
 * Indicates the antenna allocation mode for the shared antenna.
 */
enum antenna_allocation_t {
	/** Antenna allocation dynamic (as per default logic). */
	ANT_ALLOC_DYNAMIC = 0,
	/** Antenna allocation static (overrides default logic). Connect to Wi-Fi. */
	ANT_ALLOC_STATIC_WIFI,
	/** Antenna allocation static (overrides default logic). Connect to Short Range. */
	ANT_ALLOC_STATIC_SR
};

/**
 * COEX enable/disable control.
 *
 * Indicates COEX is to be enabled/disabled. This is used to configure
 * the coexistence HW blocks accordingly.
 */
enum coex_en_or_dis_t {
	/** To disable coexistence. */
	COEX_DISABLE = 0,
	/** To enable coexistence. */
	COEX_ENABLE
};

/**
 * Wi-Fi SW client request type.
 *
 * Indicates the type of SW client operation.
 */
enum coex_wifi_sw_client_req_type_t {
	/** Indicates the SW client release. */
	WIFI_SW_CLIENT_RELEASE = 0,
	/** Indicates the SW client request. */
	WIFI_SW_CLIENT_REQUEST = 1
};

/**
 * Wi-Fi SW client request priority levels.
 *
 * Indicates the priority level of the SW client request.
 */
enum coex_wifi_sw_client_req_pti_level_t {
	/** Low priority level. */
	WIFI_SW_CLIENT_REQ_PTI_LOW = 0,
	/** Medium priority level. */
	WIFI_SW_CLIENT_REQ_PTI_MEDIUM,
	/** High priority level. */
	WIFI_SW_CLIENT_REQ_PTI_HIGH,
	/** Highest priority level. */
	WIFI_SW_CLIENT_REQ_PTI_HIGHEST,
	/** Total number of priority levels. */
	WIFI_SW_CLIENT_REQ_PTI_COUNT
};

/**
 * Wi-Fi SW client types.
 *
 * Indicates different Wi-Fi SW clients that can request COEX resources.
 */
enum wifi_sw_client_t {
	/** To protect beacon reception from SR interference. */
	WIFI_BEACON_RECEPTION = 0,
	/** To protect connection phase from SR interference. */
	WIFI_CONNECTION,
	/** To protect calibrations from SR interference. */
	WIFI_CALIBRATIONS,
	/** To protect scan from SR interference. */
	WIFI_SCAN,
	/** Total number of Wi-Fi SW client types. */
	WIFI_SW_CLIENT_COUNT
};

/**
 * Periodic priority windows generation parameters.
 *
 * This structure holds the parameters required for generating
 * Periodic Priority Windows (PPWs) for Wi-Fi and SR radios.
 * Embedded in cd2cm_genarate_ppw_t message.
 */
struct coex_ppw_parameters_t {
	/** Start or stop priority windows. see &enum start_stop_ppw_t */
	unsigned int start_or_stop_ppw;
	/** Radio to which first priority window to be allocated. see &enum coex_radios_t */
	unsigned int first_window_to_wifi_or_sr;
	/** Wi-Fi priority window duration in milliseconds. */
	unsigned int wifi_pti_window_duration;
	/** SR priority window duration in milliseconds. */
	unsigned int sr_pti_window_duration;
	/** Maximum time (in milliseconds) to wait for a corresponding "stop" command
	 * after a "start" has been issued. If this timeout expires without receiving
	 * the "stop" signal, the Coexistence Manager (CM) will automatically terminate
	 * Priority Window (PPW) generation to prevent indefinite continuation.
	 */
	unsigned int ppws_timeout;
} __NRF_WIFI_PKD;

/**
 * Message to allocate PPWs to Wi-Fi and SR.
 *
 * Message from driver to CM to allocate Periodic Priority Windows
 * to Wi-Fi and SR radios.
 */
struct cd2cm_genarate_ppw_t {
	/** Message ID. Set to CD2CM_ALLOCATE_PPW. see &enum cd2cm_msg_id_t */
	unsigned int message_id;
	/** Parameters related to PPW generation. */
	struct coex_ppw_parameters_t ppw_parameters;
} __NRF_WIFI_PKD;

/**
 * Wi-Fi SW and HW clients priority range values.
 *
 * Contains Wi-Fi SW and HW clients priority range values for
 * coexistence configuration. Each range is defined as [min, max, step].
 */
struct coex_wifi_priority_range_t {
	/** Wi-Fi SW request priority range */
	unsigned char sw_request_priority_range[NUM_ELEMENTS_IN_SW_PTI_RANGE];
	/** Wi-Fi high priority Rx client CCCONF priority range. */
	unsigned char client0_ccconf_pti_range[NUM_ELEMENTS_IN_CCCONF_PTI_RANGE];
	/** Wi-Fi high priority Tx client CCCONF priority range. */
	unsigned char client1_ccconf_pti_range[NUM_ELEMENTS_IN_CCCONF_PTI_RANGE];
	/** Wi-Fi low priority Rx client CCCONF priority range. */
	unsigned char client2_ccconf_pti_range[NUM_ELEMENTS_IN_CCCONF_PTI_RANGE];
	/** Wi-Fi low priority Tx client CCCONF priority range. */
	unsigned char client3_ccconf_pti_range[NUM_ELEMENTS_IN_CCCONF_PTI_RANGE];
	/** Priority level used to choose the priority value. */
	unsigned char hw_client_priority_level;
} __NRF_WIFI_PKD;

/*
 * struct coex_sr_priority_range_t is defined by nrf71_cd_sr_if.h (included
 * above). The Coexistence Manager uses only its regular SR Rx/Tx ranges.
 */

/**
 * Message to enable/disable the coexistence.
 *
 * Message from CD to CM to enable/disable the coexistence.
 */
struct cd2cm_enable_coexistence_t {
	/** Message ID. Set to CD2CM_ENABLE_COEXISTENCE. see &enum cd2cm_msg_id_t */
	unsigned int message_id;
	/** Indicates if COEX is enabled or disabled. see &enum coex_en_or_dis_t */
	unsigned int coex_en_or_dis;
} __NRF_WIFI_PKD;

/**
 * Message to initialize Wi-Fi and SR priority ranges.
 *
 * Message from CD to CM to initialize Wi-Fi and SR priority ranges.
 */
struct cd2cm_set_priority_ranges_t {
	/** Message ID. Set to CD2CM_SET_PRIORITY_RANGES. see &enum cd2cm_msg_id_t */
	unsigned int message_id;
	/** Wi-Fi priority range values. */
	struct coex_wifi_priority_range_t wifi_pti_range;
	/** SR priority range values. */
	struct coex_sr_priority_range_t sr_pti_range;
} __NRF_WIFI_PKD;

/**
 * Message to get COEX statistics.
 *
 * Message from CD to CM to get the COEX statistics.
 */
struct cd2cm_get_coex_stats_t {
	/** Message ID. Set to CD2CM_GET_STATS. see &enum cd2cm_msg_id_t */
	unsigned int message_id;
	/** Reserved field for future use. */
	unsigned int reserved;
} __NRF_WIFI_PKD;

/**
 * Software client request parameters
 *
 * This structure holds the parameters required to post
 * a SW client request ro request COEX resources.
 */
struct coex_sw_client_params_t {
	/** Wi-Fi SW client request/release. see &enum coex_wifi_sw_client_req_type_t */
	unsigned int sw_client_request;
	/** SW client priority level. see &enum coex_wifi_sw_client_req_pti_level_t */
	unsigned int sw_client_pti_level;
	/** SW client type. see &enum wifi_sw_client_t */
	unsigned int sw_client_type;
	/** SW request timeout in milliseconds */
	unsigned int request_timeout_in_ms;
	/** Wi-Fi operating band */
	unsigned int wifi_operating_band;
} __NRF_WIFI_PKD;

/**
 * Message to post a SW client request.
 *
 * Message from CD to CM to request COEX resources.
 */
struct cd2cm_wifi_sw_client_request_t {
	/** Message ID. Set to CD2CM_WIFI_SW_CLIENT_REQUEST. see &enum cd2cm_msg_id_t */
	unsigned int message_id;
	/** SW client request parameters */
	struct coex_sw_client_params_t sw_client_parameters;
} __NRF_WIFI_PKD;

/**
 * Wi-Fi scan puncture information.
 *
 * Contains information related to Wi-Fi scan puncturing based on
 * overall system performance.
 * For now, there is only one parameter. Add additional parameters based on design.
 */
struct wifi_scan_puncture_info_t {
	/** Wi-Fi scan protection probability (0-100%). */
	unsigned int wifi_scan_prot_prob;
} __NRF_WIFI_PKD;

/**
 * Coexistence parameters from user.
 *
 * Contains coexistence parameters configurable by the user.
 */
struct coex_user_params_t {
	/** Message ID. Set to CD2CM_UPDATE_COEX_USER_PARAMS. see &enum cd2cm_msg_id_t */
	unsigned int message_id;
	/**
	 * Percentage probability to protect SR Rx (PS: listen to inactive).
	 *
	 * Wi-Fi enter and exit powersave scenario (active <=> inactive).
	 * Valid range: 0-100.
	 */
	unsigned int listen2inactive_sr_rx_prot_prob_ps;
	/**
	 * Percentage probability to protect SR Rx (PS: inactive to listen).
	 *
	 * Wi-Fi enter and exit powersave scenario.
	 * Valid range: 0-100.
	 */
	unsigned int inactive2listen_sr_rx_prot_prob_ps;
	/**
	 * Percentage probability to protect SR Rx (calib: inactive to listen).
	 *
	 * Wi-Fi calibrations start and stop scenario.
	 * Valid range: 0-100.
	 */
	unsigned int inactive2listen_sr_rx_prot_prob_calib;
	/**
	 * Percentage probability to protect SR Rx (calib: listen to inactive).
	 *
	 * Wi-Fi calibrations start and stop scenario.
	 * Valid range: 0-100.
	 */
	unsigned int listen2inactive_sr_rx_prot_prob_calib;

	/** Wi-Fi scan puncture information. */
	struct wifi_scan_puncture_info_t wifi_scan_puncture_info;

	/** Wi-Fi beacon protection percentage probability, Valid range: 0-100. */
	unsigned int wifi_beacon_prot_prob;
	/** Wi-Fi connection protection percentage probability, Valid range: 0-100. */
	unsigned int wifi_conn_prot_prob;
	/** Wi-Fi calibrations protection percentage probability, Valid range: 0-100. */
	unsigned int wifi_calib_prot_prob;

	/** Force shared antenna allocation mode. see &enum antenna_allocation_t */
	unsigned int shared_ant_control;
} __NRF_WIFI_PKD;

/**
 * Message to update the user parameters.
 *
 * Message from driver to CM to update user parameters.
 */
struct cd2cm_coex_user_params_t {
	/** Message ID. Set to CD2CM_UPDATE_COEX_USER_PARAMS. see &enum cd2cm_msg_id_t */
	unsigned int message_id;
	/** Coexistence parameters configurable by the user */
	struct coex_user_params_t user_params;
} __NRF_WIFI_PKD;

/**
 * Wi-Fi power-state events reported to the Coexistence Driver.
 *
 * Host-side (Wi-Fi driver to CD) notification; not part of the CD to CM wire
 * ABI.
 */
enum coex_wifi_power_event_t {
	/** Wi-Fi is about to power down. */
	COEX_WIFI_PREPARE_POWER_DOWN = 0,
	/** Wi-Fi has powered up and is ready. */
	COEX_WIFI_POWERED_UP_READY
};

/**
 * Notify CD before Wi-Fi power-down and after Wi-Fi is ready following power-up.
 *
 * Returns zero on success or a negative errno value.
 */
int coex_cd_wifi_power_notify(enum coex_wifi_power_event_t event);

/**
 * @}
 */
#endif /* __NRF71_COEX_IF_H__ */
