/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Common structures and definations.
 */

#ifndef __RPU_IF_H__
#define __RPU_IF_H__
#include "pack_def.h"

#define RPU_ADDR_SPI_START 0x00000000
#define RPU_ADDR_GRAM_START 0xB7000000
#define RPU_ADDR_GRAM_END 0xB70101FF
#define RPU_ADDR_SBUS_START 0xA4000000
#define RPU_ADDR_SBUS_END 0xA4007FFF
#define RPU_ADDR_PBUS_START 0xA5000000
#define RPU_ADDR_PBUS_END 0xA503FFFF
#define RPU_ADDR_BEV_START 0xBFC00000
#define RPU_ADDR_BEV_END 0xBFCFFFFF
#define RPU_ADDR_PKTRAM_START 0xB0000000
#define RPU_ADDR_PKTRAM_END 0xB0030FFF
#define RPU_ADDR_MCU1_CORE_ROM_START 0x80000000
#define RPU_ADDR_MCU1_CORE_ROM_END 0x8002E7FF
#define RPU_ADDR_MCU1_CORE_RET_START 0x80040000
#define RPU_ADDR_MCU1_CORE_RET_END 0x8004C000
#define RPU_ADDR_MCU1_CORE_SCRATCH_START 0x80080000
#define RPU_ADDR_MCU1_CORE_SCRATCH_END 0x8008FFFF
/*UMAC*/
#define RPU_ADDR_MCU2_CORE_ROM_START 0x80000000
#define RPU_ADDR_MCU2_CORE_ROM_END 0x800617FF
#define RPU_ADDR_MCU2_CORE_RET_START 0x80080000
#define RPU_ADDR_MCU2_CORE_RET_END 0x800A3FFF
#define RPU_ADDR_MCU2_CORE_SCRATCH_START 0x80100000
#define RPU_ADDR_MCU2_CORE_SCRATCH_END 0x80137FFF

#define RPU_ADDR_MASK_BASE 0xFF000000
#define RPU_ADDR_MASK_OFFSET 0x00FFFFFF
#define RPU_ADDR_MASK_BEV_OFFSET 0x000FFFFF

/* Register locations from the Echo520 TRM */
#define RPU_REG_INT_FROM_RPU_CTRL 0xA4000400 /* 8.1.1 */
#define RPU_REG_BIT_INT_FROM_RPU_CTRL 17

#define RPU_REG_INT_TO_MCU_CTRL 0xA4000480 /* 8.3.7 */

#define RPU_REG_INT_FROM_MCU_ACK 0xA4000488 /* 8.3.9 */
#define RPU_REG_BIT_INT_FROM_MCU_ACK 31

#define RPU_REG_INT_FROM_MCU_CTRL 0xA4000494 /* 8.3.12 */
#define RPU_REG_BIT_INT_FROM_MCU_CTRL 31

#define RPU_REG_UCC_SLEEP_CTRL_DATA_0 0xA4002C2C /* 9.1.11 */
#define RPU_REG_UCC_SLEEP_CTRL_DATA_1 0xA4002C30 /* 9.1.12 */

#define RPU_REG_MIPS_MCU_CONTROL 0xA4000000 /* 13.1.1 */
#define RPU_REG_BIT_MIPS_MCU_LATCH_SOFT_RESET 1
#define RPU_REG_MIPS_MCU2_CONTROL 0xA4000100 /* 13.1.1 */

#define RPU_REG_MIPS_MCU_UCCP_INT_STATUS 0xA4000004
#define RPU_REG_BIT_MIPS_UCCP_INT_STATUS 0
#define RPU_REG_BIT_MIPS_WATCHDOG_INT_STATUS 1

#define RPU_REG_MIPS_MCU_TIMER_CONTROL 0xA4000048

#define RPU_REG_MIPS_MCU_SYS_CORE_MEM_CTRL 0xA4000030 /* 13.1.10 */
#define RPU_REG_MIPS_MCU_SYS_CORE_MEM_WDATA 0xA4000034 /* 13.1.11 */
#define RPU_REG_MIPS_MCU_BOOT_EXCP_INSTR_0 0xA4000050 /* 13.1.15 */
#define RPU_REG_MIPS_MCU_BOOT_EXCP_INSTR_1 0xA4000054 /* 13.1.16 */
#define RPU_REG_MIPS_MCU_BOOT_EXCP_INSTR_2 0xA4000058 /* 13.1.17 */
#define RPU_REG_MIPS_MCU_BOOT_EXCP_INSTR_3 0xA400005C /* 13.1.18 */

#define RPU_REG_MIPS_MCU2_SYS_CORE_MEM_CTRL 0xA4000130 /* 13.1.10 */
#define RPU_REG_MIPS_MCU2_SYS_CORE_MEM_WDATA 0xA4000134 /* 13.1.11 */
#define RPU_REG_MIPS_MCU2_BOOT_EXCP_INSTR_0 0xA4000150 /* 13.1.15 */
#define RPU_REG_MIPS_MCU2_BOOT_EXCP_INSTR_1 0xA4000154 /* 13.1.16 */
#define RPU_REG_MIPS_MCU2_BOOT_EXCP_INSTR_2 0xA4000158 /* 13.1.17 */
#define RPU_REG_MIPS_MCU2_BOOT_EXCP_INSTR_3 0xA400015C /* 13.1.18 */

#define RPU_REG_MCP_SYS_CSTRCTRL 0xA4001200
#define RPU_REG_MCP_SYS_CSTRDAT32 0xA4001218

#define RPU_REG_MCP2_SYS_CSTRCTRL 0xA4003200
#define RPU_REG_MCP2_SYS_CSTRDAT32 0xA4003218

#define RPU_REG_MCP3_SYS_CSTRCTRL 0xA4004200
#define RPU_REG_MCP3_SYS_CSTRDAT32 0xA4004218

#ifdef RPU_RF_C0_SUPPORT
#define PWR_CTRL1_SYSDEF 0xA4019000
#define PWR_COUNTERSTART_SYSDEF 0xA40190A0
#define PWR_COUNTERCYCLES_SYSDEF 0xA40190A4
#define PWR_COUNTERSTATUS0_SYSDEF 0xA40190B0
#define PWR_COUNTERSTATUS1_SYSDEF 0xA40190B4
#define PWR_COUNTERSTATUS2_SYSDEF 0xA40190B8
#define PWR_COUNTERSTATUS3_SYSDEF 0xA40190BC
#define WL_PWR_MON_SYSDEF 0xA40010
#define WL_PWR_AUX_SYSDEF 0xA40014
#define WL_PWR_VMON_CTRL_SYSDEF 0xA40030
#define WL_PWR_VMON_DATA_SYSDEF 0xA40034
#define WLAFE_WL_BBPLLEN_SYSDEF 0xA400B004
#define WLAFE_RG_BBPLL_CLK_01_SYSDEF 0xA400B050
#define WLAFE_RG_AFE_LDOCTRL_SYSDEF 0xA400B0F0

#define PWR_BREAKTIMER90_SYSDEF 0xA4019190
#define PWR_BREAKCOND2_SYSDEF 0xA4019094
#define PWR_BREAK3_SYSDEF 0xA4019080
#define PWR_BREAKCOND3_SYSDEF 0xA4019098
#define PWR_BREAK5_SYSDEF 0xA4019088

#else /* RPU_RF_C0_SUPPORT */

#define RPU_REG_RFCTL_UCC_RF_CTRL_CONFIG_00 0xA401C200
#define RPU_REG_RFCTL_UCC_RF_CTRL_CONFIG_01 0xA401C204
#define RPU_REG_RFCTL_UCC_RF_CTRL_CONFIG_02 0xA401C208
#define RPU_REG_RFCTL_UCC_RF_CTRL_CONFIG_04 0xA401C210
#define RPU_REG_RFCTL_UCC_RF_CTRL_CONFIG_16 0xA401C260
#define RPU_REG_RFCTL_SPI_CMD_DATA_TABLE_0 0xA401C300
#define RPU_REG_RFCTL_SPI_CMD_DATA_TABLE_1 0xA401C304
#define RPU_REG_RFCTL_SPI_CMD_DATA_TABLE_2 0xA401C308
#define RPU_REG_RFCTL_SPI_READ_DATA_TABLE_0 0xA401C380

#define PWR_CTRL1_SYSDEF 0x1040
#define PWR_COUNTERSTART_SYSDEF 0x1158
#define PWR_COUNTERCYCLES_SYSDEF 0x1159
#define PWR_COUNTERSTATUS0_SYSDEF 0x115C
#define PWR_COUNTERSTATUS1_SYSDEF 0x115D
#define PWR_COUNTERSTATUS2_SYSDEF 0x115E
#define PWR_COUNTERSTATUS3_SYSDEF 0x115F
#define WL_PWR_MON_SYSDEF 0x0144
#define WL_PWR_AUX_SYSDEF 0x0145

#define PWR_BREAKTIMER90_SYSDEF 0x1264
#define PWR_BREAKCOND2_SYSDEF 0x1155
#define PWR_BREAK3_SYSDEF 0x1150
#define PWR_BREAKCOND3_SYSDEF 0x1156
#define PWR_BREAK5_SYSDEF 0x1152

#define SPI_PAGESELECT 0x007C
#define SPI_DIGREFCLOCKCTRL 0x007D
#endif /* !RPU_RF_C0_SUPPORT */

#define RPU_REG_BIT_HARDRST_CTRL 8
#define RPU_REG_BIT_PS_CTRL 0
#define RPU_REG_BIT_PS_STATE 1
#define RPU_REG_BIT_READY_STATE 2

#define RPU_MEM_RX_CMD_BASE 0xB7000D58

#define RPU_MEM_HPQ_INFO 0xB0000024
#define RPU_MEM_TX_CMD_BASE 0xB00000B8
#define RPU_MEM_OTP_INFO 0xB000005C
#define RPU_MEM_LMAC_IF_INFO 0xB0004FE0

#define RPU_MEM_PKT_BASE 0xB0005000
#define RPU_CMD_START_MAGIC 0xDEAD
#define RPU_DATA_CMD_SIZE_MAX_RX 8
#define RPU_DATA_CMD_SIZE_MAX_TX 148
#define RPU_EVENT_COMMON_SIZE_MAX 128

/*Event pool*/
#define EVENT_POOL_NUM_ELEMS (7)
#define MAX_EVENT_POOL_LEN 1000

#define MAX_NUM_OF_RX_QUEUES 3

#define IMG_RPU_PWR_DATA_TYPE_LFC_ERR 0
#define IMG_RPU_PWR_DATA_TYPE_VBAT_MON 1
#define IMG_RPU_PWR_DATA_TYPE_TEMP 2
#define IMG_RPU_PWR_DATA_TYPE_ALL 3
#define IMG_RPU_PWR_DATA_TYPE_MAX 4

#ifndef RPU_RF_C0_SUPPORT
#define IMG_RPU_RF_CLK_TYPE_20 0
#define IMG_RPU_RF_CLK_TYPE_40 1
#define IMG_RPU_RF_CLK_TYPE_MAX 2
#endif /* RPU_RF_C0_SUPPORT */

/**
 * struct img_rpu_pwr_data - Data that host may want to read from the Power IP.
 * @lfc_err: Estimated Lo Frequency Clock error in ppm.
 * @vbat_mon: Vbat monitor readout. The actual Vbat in volt equals 2.5 + 0.07*vbat_mon.
 * @temp: Estimated die temperature (degC).
 *
 * This structure represents the Power IP monitoring data.
 */
struct img_rpu_pwr_data {
	int lfc_err;
	int vbat_mon;
	int temp;
};

/**
 * struct host_rpu_rx_buf_info - RX buffer related information to be passed to
 *                               the RPU.
 * @addr: Address in the host memory where the RX buffer is located.
 *
 * This structure encapsulates the information to be passed to the RPU for
 * buffers which the RPU will use to pass the received frames.
 */

struct host_rpu_rx_buf_info {
	unsigned int addr;
} __IMG_PKD;

/**
 * struct host_rpu_hpq - Hostport Queue (HPQ) information.
 * @enqueue_addr: HPQ address where the host can post the address of a
 *                message intended for the RPU.
 * @dequeue_addr: HPQ address where the host can get the address of a
 *                message intended for the host.
 *
 * This structure encapsulates the information which represents a HPQ.
 */

struct host_rpu_hpq {
	unsigned int enqueue_addr;
	unsigned int dequeue_addr;
} __IMG_PKD;

/**
 * struct host_rpu_hpqm_info - Information about Hostport Queues (HPQ) to be used
 *            for exchanging information between the Host and RPU.
 * @event_busy_queue: Queue which the RPU uses to inform the host about events.
 * @event_avl_queue: Queue on which the consumed events are pushed so that RPU
 *                    can reuse them.
 * @cmd_busy_queue: Queue used by the host to push commands to the RPU.
 * @cmd_avl_queue: Queue which RPU uses to inform host about command
 *                  buffers which can be used to push commands to the RPU.
 *
 * Hostport queue information passed by the RPU to the host, which the host can
 * use, to communicate with the RPU.
 */

struct host_rpu_hpqm_info {
	struct host_rpu_hpq event_busy_queue;
	struct host_rpu_hpq event_avl_queue;
	struct host_rpu_hpq cmd_busy_queue;
	struct host_rpu_hpq cmd_avl_queue;
	struct host_rpu_hpq rx_buf_busy_queue[MAX_NUM_OF_RX_QUEUES];
} __IMG_PKD;

/**
 * struct host_rpu_msg_hdr - Common header included in each command/event.
 * @len: Length of the message.
 * @resubmit: Flag to indicate whether the recipient is expected to resubmit
 *            the cmd/event address back to the trasmitting entity.
 *
 * This structure encapsulates the common information included at the start of
 * each command/event exchanged with the RPU.
 */

struct host_rpu_msg_hdr {
	unsigned int len;
	unsigned int resubmit;
} __IMG_PKD;

#define BT_INIT 0x1
#define BT_MODE 0x2
#define BT_CTRL 0x4

/*! BT coexistence module enable or disable */

#define BT_COEX_DISABLE 0
#define BT_COEX_ENABLE 1

/* External BT mode  master or slave */

#define SLAVE 0
#define MASTER 1

struct pta_ext_params {
	/*! Set polarity to 1 if  BT_TX_RX active high indicates Tx. Set polarity to 0 if BT_TX_RX
	 * active high indicates Rx.
	 */
	unsigned char tx_rx_pol;

	/*! BT_ACTIVE signal lead time period. This is with reference to time instance at which
	 *BT slot boundary starts if BT supports classic only mode and BT activity starts if BT
	 *supports BLE or dual mode
	 */
	unsigned int lead_time;

	/*! Time instance at which BT_STATUS is sampled by PTA to get the BT_PTI information. This
	 *is done anywhere between BT_ACTIVE_ASSERT time and BT_STATUS priority signalling time
	 *period ends.This is with reference to BT_ACTIVE assert time.
	 */
	unsigned int pti_samp_time;

	/*! Time instance at which BT_STATUS is sampled by PTA to get BT_TX_RX information.
	 *This is done by PTA after the end of time period T2.  This is with reference to BT_ACTIVE
	 *assert time.
	 */
	unsigned int tx_rx_samp_time;

	/*! Time instance at which PTA takes arbitration decision and posts WLAN_DENY to BT. This
	 * is with reference to BT_ACTIVE assert time.
	 */
	unsigned int dec_time;
} __IMG_PKD;

#endif /* __RPU_IF_H__ */
