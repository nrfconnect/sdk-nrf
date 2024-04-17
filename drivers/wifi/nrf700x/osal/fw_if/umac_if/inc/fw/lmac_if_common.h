/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Common structures and definitions.
 */

#ifndef __LMAC_IF_COMMON__
#define __LMAC_IF_COMMON__

#include "rpu_if.h"
#include "phy_rf_params.h"
#include "pack_def.h"

#define RPU_MEM_LMAC_BOOT_SIG 0xB7000D50
#define RPU_MEM_LMAC_VER 0xB7000D54

#define RPU_MEM_LMAC_PATCH_BIN 0x80044000
#define RPU_MEM_LMAC_PATCH_BIMG 0x8004B000

#define NRF_WIFI_LMAC_VER(ver) ((ver & 0xFF000000) >> 24)
#define NRF_WIFI_LMAC_VER_MAJ(ver) ((ver & 0x00FF0000) >> 16)
#define NRF_WIFI_LMAC_VER_MIN(ver) ((ver & 0x0000FF00) >> 8)
#define NRF_WIFI_LMAC_VER_EXTRA(ver) (ver & 0x000000FF)

#define NRF_WIFI_LMAC_BOOT_SIG 0x5A5A5A5A
#define NRF_WIFI_LMAC_ROM_PATCH_OFFSET (RPU_MEM_LMAC_PATCH_BIMG - RPU_ADDR_LMAC_CORE_RET_START)
#define NRF_WIFI_LMAC_BOOT_EXCP_VECT_0 0x3c1a8000
#define NRF_WIFI_LMAC_BOOT_EXCP_VECT_1 0x275a0000
#define NRF_WIFI_LMAC_BOOT_EXCP_VECT_2 0x03400008
#define NRF_WIFI_LMAC_BOOT_EXCP_VECT_3 0x00000000

#define NRF_WIFI_LMAC_MAX_RX_BUFS 256

#define HW_SLEEP_ENABLE 2
#define SW_SLEEP_ENABLE 1
#define SLEEP_DISABLE 0
#define HW_DELAY 7350
#define SW_DELAY 5000
#define BCN_TIMEOUT 20000
#define CALIB_SLEEP_CLOCK_ENABLE 1

#define ACTIVE_SCAN_DURATION 50
#define PASSIVE_SCAN_DURATION 130
#define WORKING_CH_SCAN_DURATION 50
#define CHNL_PROBE_CNT 2

#define PKT_TYPE_MPDU 0
#define PKT_TYPE_MSDU_WITH_MAC 1
#define PKT_TYPE_MSDU 2

#define NRF_WIFI_RPU_PWR_STATUS_SUCCESS 0
#define NRF_WIFI_RPU_PWR_STATUS_FAIL -1

/**
 * struct lmac_prod_stats : used to get the production mode stats
 **/

/* Events */
#define MAX_RSSI_SAMPLES 10
struct lmac_prod_stats {
	/*Structure that holds all the debug information in LMAC*/
	unsigned int reset_cmd_cnt;
	unsigned int reset_complete_event_cnt;
	unsigned int unable_gen_event;
	unsigned int ch_prog_cmd_cnt;
	unsigned int channel_prog_done;
	unsigned int tx_pkt_cnt;
	unsigned int tx_pkt_done_cnt;
	unsigned int scan_pkt_cnt;
	unsigned int internal_pkt_cnt;
	unsigned int internal_pkt_done_cnt;
	unsigned int ack_resp_cnt;
	unsigned int tx_timeout;
	unsigned int deagg_isr;
	unsigned int deagg_inptr_desc_empty;
	unsigned int deagg_circular_buffer_full;
	unsigned int lmac_rxisr_cnt;
	unsigned int rx_decryptcnt;
	unsigned int process_decrypt_fail;
	unsigned int prepa_rx_event_fail;
	unsigned int rx_core_pool_full_cnt;
	unsigned int rx_mpdu_crc_success_cnt;
	unsigned int rx_mpdu_crc_fail_cnt;
	unsigned int rx_ofdm_crc_success_cnt;
	unsigned int rx_ofdm_crc_fail_cnt;
	unsigned int rxDSSSCrcSuccessCnt;
	unsigned int rxDSSSCrcFailCnt;
	unsigned int rx_crypto_start_cnt;
	unsigned int rx_crypto_done_cnt;
	unsigned int rx_event_buf_full;
	unsigned int rx_extram_buf_full;
	unsigned int scan_req;
	unsigned int scan_complete;
	unsigned int scan_abort_req;
	unsigned int scan_abort_complete;
	unsigned int internal_buf_pool_null;
};

/**
 * struct phy_prod_stats : used to get the production mode stats
 **/
struct phy_prod_stats {
	unsigned int ofdm_crc32_pass_cnt;
	unsigned int ofdm_crc32_fail_cnt;
	unsigned int dsss_crc32_pass_cnt;
	unsigned int dsss_crc32_fail_cnt;
	char averageRSSI;
};

union rpu_stats {
	struct lmac_prod_stats lmac_stats;
	struct phy_prod_stats phy_stats;
};

struct hpqm_queue {
	unsigned int pop_addr;
	unsigned int push_addr;
	unsigned int id_num;
	unsigned int status_addr;
	unsigned int status_mask;
} __NRF_WIFI_PKD;

struct INT_HPQ {
	unsigned int id;
	/* The head and tail values are relative
	 * to the start of the
	 * HWQM register block.
	 */
	unsigned int head;
	unsigned int tail;
} __NRF_WIFI_PKD;

/**
 * @brief LMAC firmware config params
 *
 */
struct lmac_fw_config_params {
	/** lmac firmware boot status. LMAC will set to 0x5a5a5a5a after completing boot process */
	unsigned int boot_status;
	/** LMAC version */
	unsigned int version;
	/** Address to resubmit Rx buffers */
	unsigned int lmac_rx_buffer_addr;
	/** Maximum Rx descriptors */
	unsigned int lmac_rx_max_desc_cnt;
	/** size of each descriptor size */
	unsigned int lmac_rx_desc_size;
	/** rpu config name. this is a string */
	unsigned char rpu_config_name[16];
	/** rpu config number */
	unsigned char rpu_config_number[8];
	/** numRX */
	unsigned int numRX;
	/** numTX */
	unsigned int numTX;
#define FREQ_2_4_GHZ 1
#define FREQ_5_GHZ 2
	/** supported bands */
	unsigned int bands;
	/** system frequency */
	unsigned int sys_frequency_in_mhz;
	/** queue which contains Free GRAM pointers for commands */
	struct hpqm_queue FreeCmdPtrQ;
	/** Command pointer queue. Host should pick pointer from FreeCmdPtrQ, populate
	 *  command into that address and submit back to this queue for RPU
	 */
	struct hpqm_queue cmdPtrQ;
	/** queue which contains Free GRAM pointers for events */
	struct hpqm_queue eventPtrQ;
	/** Event pointer queue. Host should pick pointer from FreeCmdPtrQ, populate
	 *  command into that address and submit back to this queue for RPU
	 */
	struct hpqm_queue freeEventPtrQ;
	/** Rx buffer queue */
	struct hpqm_queue SKBGramPtrQ_1;
	/** Rx buffer queue */
	struct hpqm_queue SKBGramPtrQ_2;
	/** Rx buffer queue */
	struct hpqm_queue SKBGramPtrQ_3;
	/** lmac register address to enable ISR to Host */
	unsigned int HP_lmac_to_host_isr_en;
	/** Address to Clear host ISR */
	unsigned int HP_lmac_to_host_isr_clear;
	/** Address to set ISR to lmac Clear host ISR */
	unsigned int HP_set_lmac_isr;

#define NUM_32_QUEUES 4
	/** Hardware queues */
	struct INT_HPQ hpq32[NUM_32_QUEUES];

} __NRF_WIFI_PKD;

#define MAX_NUM_OF_RX_QUEUES 3

struct rx_buf_pool_params {
	/** buffer size */
	unsigned short buf_sz;
	/** number of buffers */
	unsigned short num_bufs;
} __NRF_WIFI_PKD;

struct temp_vbat_config {
	unsigned int temp_based_calib_en;
	unsigned int temp_calib_bitmap;
	unsigned int vbat_calibp_bitmap;
	unsigned int temp_vbat_mon_period;
	int vth_very_low;
	int vth_low;
	int vth_hi;
	int temp_threshold;
	int vbat_threshold;
} __NRF_WIFI_PKD;
#endif
