/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file rpu_if.h
 * @brief Common structures and definitions for RPU interface.
 */

#ifndef __RPU_IF_H__
#define __RPU_IF_H__
#include "pack_def.h"

/* Beginning address of the global RAM */
#define RPU_ADDR_GRAM_START 0xB7000000
/* Ending address of the global RAM */
#define RPU_ADDR_GRAM_END 0xB70101FF
/* Beginning address of the system bus register space */
#define RPU_ADDR_SBUS_START 0xA4000000
/* Ending address of the system bus register space */
#define RPU_ADDR_SBUS_END 0xA4007FFF
/* Beginning address of the peripheral bus register space */
#define RPU_ADDR_PBUS_START 0xA5000000
/* Ending address of the peripheral bus register space */
#define RPU_ADDR_PBUS_END 0xA503FFFF
/* Beginning address of the MIPS boot exception vector registers */
#define RPU_ADDR_BEV_START 0xBFC00000
/* Ending address of the MIPS boot exception vector registers */
#define RPU_ADDR_BEV_END 0xBFCFFFFF
/* Beginning address of the packet RAM */
#define RPU_ADDR_PKTRAM_START 0xB0000000
/* Ending address of the packet RAM */
#define RPU_ADDR_PKTRAM_END 0xB0030FFF

/* Starting address of the LMAC MCU (MCU) retention RAM */
#define RPU_ADDR_LMAC_CORE_RET_START 0x80040000
/* Starting address of the UMAC MCU (MCU2) retention RAM */
#define RPU_ADDR_UMAC_CORE_RET_START 0x80080000

/**
 * @brief Regions in the MCU local memory.
 *
 * The MCU local memory in the nRF70 is divided into three regions:
 * - ROM : Read-only memory region.
 * - RETENTION : Retention memory region.
 * - SCRATCH : Scratch memory region.
 */
enum RPU_MCU_ADDR_REGIONS {
	RPU_MCU_ADDR_REGION_ROM = 0,
	RPU_MCU_ADDR_REGION_RETENTION,
	RPU_MCU_ADDR_REGION_SCRATCH,
	RPU_MCU_ADDR_REGION_MAX,
};

/**
 * @brief Address limits of each MCU local memory region.
 *
 * A MCU local memory region is defined by its:
 * - Start address, and
 * - End address.
 */
struct rpu_addr_region {
	unsigned int start;
	unsigned int end;
};

/**
 * @brief Address map of the MCU memory.
 *
 * The MCU memory map consists of three regions:
 * - ROM : Read-only memory region.
 * - RETENTION : Retention memory region.
 * - SCRATCH : Scratch memory region.
 */
struct rpu_addr_map {
	struct rpu_addr_region regions[RPU_MCU_ADDR_REGION_MAX];
};

/**
 * @brief Memory map of the MCUs in the RPU.
 *
 * The RPU consists of two MCUs:
 * - MCU: For the LMAC.
 * - MCU2 : For the UMAC.
 *
 * Each MCU memory consists of three regions:
 * - ROM : Read-only memory region.
 * - RETENTION : Retention memory region.
 * - SCRATCH : Scratch memory region.
 */
static const struct rpu_addr_map RPU_ADDR_MAP_MCU[] = {
	/* MCU - LMAC */
	{
		{
			{0x80000000, 0x80033FFF},
			{0x80040000, 0x8004BFFF},
			{0x80080000, 0x8008FFFF}
		},
	},
	/* MCU2 - UMAC */
	{
		{
			{0x80000000, 0x800617FF},
			{0x80080000, 0x800A3FFF},
			{0x80100000, 0x80137FFF},
		}
	},
};

/* Number of boot exception vectors for each MCU */
#define RPU_MCU_MAX_BOOT_VECTORS 4

/**
 * @brief Boot vector definition for a MCU in nRF70.
 *
 * The boot vectors for the MCUs in the nRF70 are defined by their:
 * - Address, and
 * - Value.
 */
struct rpu_mcu_boot_vector {
	unsigned int addr;
	unsigned int val;
};

/**
 * @brief Boot vectors for the MCUs in nRF70.
 *
 * The boot vectors for the MCUs in nRF70.
 */
struct rpu_mcu_boot_vectors {
	struct rpu_mcu_boot_vector vectors[RPU_MCU_MAX_BOOT_VECTORS];
};

/* Base mask for the nRF70 memory map */
#define RPU_ADDR_MASK_BASE 0xFF000000
/* Offset mask for the nRF70 memory map */
#define RPU_ADDR_MASK_OFFSET 0x00FFFFFF
/* Offset mask for the boot exception vector */
#define RPU_ADDR_MASK_BEV_OFFSET 0x000FFFFF

/* Address of the nRF70 interrupt register */
#define RPU_REG_INT_FROM_RPU_CTRL 0xA4000400
/* Control bit for enabling/disabling of nRF70 interrupts */
#define RPU_REG_BIT_INT_FROM_RPU_CTRL 17

/* Address of the nRF70 IRQ register */
#define RPU_REG_INT_TO_MCU_CTRL 0xA4000480

/* Address of the nRF70 interrupt ack register */
#define RPU_REG_INT_FROM_MCU_ACK 0xA4000488
/* Bit to set to ack nRF70 interrupt */
#define RPU_REG_BIT_INT_FROM_MCU_ACK 31

/* Address of the nRF70 UMAC MCU interrupt enable register */
#define RPU_REG_INT_FROM_MCU_CTRL 0xA4000494
/* Bit to set to enable UMAC MCU interrupts */
#define RPU_REG_BIT_INT_FROM_MCU_CTRL 31

/* Address of the nRF70 register which points to LMAC patch memory address */
#define RPU_REG_UCC_SLEEP_CTRL_DATA_0 0xA4002C2C
/* Address of the nRF70 register which points to UMAC patch memory address */
#define RPU_REG_UCC_SLEEP_CTRL_DATA_1 0xA4002C30
/* Address of the register to soft reset the LMAC MCU */
#define RPU_REG_MIPS_MCU_CONTROL 0xA4000000
/* Address of the register to soft reset the UMAC MCU */
#define RPU_REG_MIPS_MCU2_CONTROL 0xA4000100

/* Address of the nRF70 interrupt status register */
#define RPU_REG_MIPS_MCU_UCCP_INT_STATUS 0xA4000004
/* Bit to check for watchdog interrupt */
#define RPU_REG_BIT_MIPS_WATCHDOG_INT_STATUS 1

/* Address of the nRF70 watchdog timer register */
#define RPU_REG_MIPS_MCU_TIMER 0xA400004C /* 24 bit timer@core clock ticks*/
/* Default watchdog timer value */
#define RPU_REG_MIPS_MCU_TIMER_RESET_VAL 0xFFFFFF

/* Address of the nRF70 watchdog interrupt clear register */
#define RPU_REG_MIPS_MCU_UCCP_INT_CLEAR 0xA400000C
/* Bit to clear the watchdog interrupt */
#define RPU_REG_BIT_MIPS_WATCHDOG_INT_CLEAR 1

/* Registers to control indirect access to LMAC MCU local memory */
/* The MCU local memory address needs to be programmed to the control register */
#define RPU_REG_MIPS_MCU_SYS_CORE_MEM_CTRL 0xA4000030
/* The data to be written to the MCU local memory needs to be programmed to the data register */
#define RPU_REG_MIPS_MCU_SYS_CORE_MEM_WDATA 0xA4000034

/* Boot exception vector registers for the LMAC MCU */
#define RPU_REG_MIPS_MCU_BOOT_EXCP_INSTR_0 0xA4000050
#define RPU_REG_MIPS_MCU_BOOT_EXCP_INSTR_1 0xA4000054
#define RPU_REG_MIPS_MCU_BOOT_EXCP_INSTR_2 0xA4000058
#define RPU_REG_MIPS_MCU_BOOT_EXCP_INSTR_3 0xA400005C

/* Registers to control indirect access to LMAC MCU local memory */
/* The MCU local memory address needs to be programmed to the control register */
#define RPU_REG_MIPS_MCU2_SYS_CORE_MEM_CTRL 0xA4000130
/* The data to be written to the MCU local memory needs to be programmed to the data register */
#define RPU_REG_MIPS_MCU2_SYS_CORE_MEM_WDATA 0xA4000134
/* Boot exception vector registers for the LMAC MCU */
#define RPU_REG_MIPS_MCU2_BOOT_EXCP_INSTR_0 0xA4000150
#define RPU_REG_MIPS_MCU2_BOOT_EXCP_INSTR_1 0xA4000154
#define RPU_REG_MIPS_MCU2_BOOT_EXCP_INSTR_2 0xA4000158
#define RPU_REG_MIPS_MCU2_BOOT_EXCP_INSTR_3 0xA400015C

/* Bit which controls the power state of the nRF70 */
#define RPU_REG_BIT_PS_CTRL 0
/* Bit which indicates hardware bus ready state of the nRF70 */
#define RPU_REG_BIT_PS_STATE 1
/* Bit which indicates the firmware readiness of the nRF70 */
#define RPU_REG_BIT_READY_STATE 2
/* Address which has information about the RX command base */
#define RPU_MEM_RX_CMD_BASE 0xB7000D58

/* Address which has information about the host port queue manager (HPQM) */
#define RPU_MEM_HPQ_INFO 0xB0000024
/* Address which has information about the TX command base */
#define RPU_MEM_TX_CMD_BASE 0xB00000B8

/* Address which has OTP location containing the factory test program version */
#define RPU_MEM_OTP_FT_PROG_VERSION 0xB0004FD8
/* Address which has the OTP flags */
#define RPU_MEM_OTP_INFO_FLAGS 0xB0004FDC
/* Address which has the OTP package type */
#define RPU_MEM_OTP_PACKAGE_TYPE 0xB0004FD4

/* Base address of the area where TX/RX packet buffers can be
 * programmed for transmission/reception
 */
#define RPU_MEM_PKT_BASE 0xB0005000
/* Magic value to indicate start of the command counter synchronization between host and nRF70 */
#define RPU_CMD_START_MAGIC 0xDEAD
/* Maximum size of the RX data command */
#define RPU_DATA_CMD_SIZE_MAX_RX 8
/* Maximum size of the TX data command */
#define RPU_DATA_CMD_SIZE_MAX_TX 148
/* Maximum size of the most common events */
#define RPU_EVENT_COMMON_SIZE_MAX 128

/* Maximum event size */
#define MAX_EVENT_POOL_LEN 1000
/* Maximum number of RX queues */
#define MAX_NUM_OF_RX_QUEUES 3

/* Packet RAM size in the nRF70 */
#define RPU_PKTRAM_SIZE (RPU_ADDR_PKTRAM_END - RPU_MEM_PKT_BASE + 1)

/* Base address of the area where ADC output IQ samples are stored */
#define RPU_MEM_RF_TEST_CAP_BASE 0xB0006000

/* OTP address offsets (word offsets) */
#define REGION_PROTECT 64
#define PRODTEST_FT_PROGVERSION 29
#define PRODTEST_TRIM0 32
#define PRODTEST_TRIM1 33
#define PRODTEST_TRIM2 34
#define PRODTEST_TRIM3 35
#define PRODTEST_TRIM4 36
#define PRODTEST_TRIM5 37
#define PRODTEST_TRIM6 38
#define PRODTEST_TRIM7 39
#define PRODTEST_TRIM8 40
#define PRODTEST_TRIM9 41
#define PRODTEST_TRIM10 42
#define PRODTEST_TRIM11 43
#define PRODTEST_TRIM12 44
#define PRODTEST_TRIM13 45
#define PRODTEST_TRIM14 46
#define INFO_PART 48
#define INFO_VARIANT 49
#define INFO_UUID 52
#define QSPI_KEY 68
#define MAC0_ADDR 72
#define MAC1_ADDR 74
#define CALIB_XO 76
#define REGION_DEFAULTS 85
#define PRODRETEST_PROGVERSION 86
#define PRODRETEST_TRIM0 87
#define PRODRETEST_TRIM1 88
#define PRODRETEST_TRIM2 89
#define PRODRETEST_TRIM3 90
#define PRODRETEST_TRIM4 91
#define PRODRETEST_TRIM5 92
#define PRODRETEST_TRIM6 93
#define PRODRETEST_TRIM7 94
#define PRODRETEST_TRIM8 95
#define PRODRETEST_TRIM9 96
#define PRODRETEST_TRIM10 97
#define PRODRETEST_TRIM11 98
#define PRODRETEST_TRIM12 99
#define PRODRETEST_TRIM13 100
#define PRODRETEST_TRIM14 101
#define OTP_MAX_WORD_LEN 128
#define QSPI_KEY_LENGTH_BYTES 16
#define RETRIM_LEN 15

/* Size of XO calibration value stored in the OTP field CALIB_XO */
#define OTP_SZ_CALIB_XO 1

/* Byte offsets of XO calib value in CALIB_XO field in the OTP */
#define OTP_OFF_CALIB_XO 0

/* Masks to program bit fields in REGION_DEFAULTS field in the OTP */
#define QSPI_KEY_FLAG_MASK ~(1U<<0)
#define MAC0_ADDR_FLAG_MASK ~(1U<<1)
#define MAC1_ADDR_FLAG_MASK ~(1U<<2)
#define CALIB_XO_FLAG_MASK ~(1U<<3)

/* RF register address to facilitate OTP access */
#define OTP_VOLTCTRL_ADDR 0x19004
/* Voltage value to be written into the above RF register for OTP write access */
#define OTP_VOLTCTRL_2V5 0x3b
/* Voltage value to be written into the above RF register for OTP read access */
#define OTP_VOLTCTRL_1V8 0xb

#define OTP_POLL_ADDR 0x01B804
#define OTP_WR_DONE 0x1
#define OTP_READ_VALID 0x2
#define OTP_READY 0x4


#define OTP_RWSBMODE_ADDR 0x01B800
#define OTP_READ_MODE 0x1
#define OTP_BYTE_WRITE_MODE 0x42


#define OTP_RDENABLE_ADDR 0x01B810
#define OTP_READREG_ADDR 0x01B814

#define OTP_WRENABLE_ADDR 0x01B808
#define OTP_WRITEREG_ADDR 0x01B80C

#define OTP_TIMING_REG1_ADDR 0x01B820
#define OTP_TIMING_REG1_VAL 0x0
#define OTP_TIMING_REG2_ADDR 0x01B824
#define OTP_TIMING_REG2_VAL 0x030D8B

#define OTP_FRESH_FROM_FAB 0xFFFFFFFF
#define OTP_PROGRAMMED 0x00000000
#define OTP_ENABLE_PATTERN 0x50FA50FA
#define OTP_INVALID 0xDEADBEEF

#define FT_PROG_VER_MASK 0xF0000

/**
 * @brief RX buffer related information to be passed to nRF70.
 *
 * This structure encapsulates the information to be passed to nRF70 for
 * buffers which the nRf70 will use to pass the received frames.
 */
struct host_rpu_rx_buf_info {
	/** Address in the host memory where the RX buffer is located. */
	unsigned int addr;
} __NRF_WIFI_PKD;

/**
 * @brief Hostport Queue (HPQ) information.
 *
 * This structure encapsulates the information which represents a HPQ.
 */
struct host_rpu_hpq {
	/** HPQ address where the host can post the address of a message intended for the RPU. */
	unsigned int enqueue_addr;
	/** HPQ address where the host can get the address of a message intended for the host. */
	unsigned int dequeue_addr;
} __NRF_WIFI_PKD;

/**
 * @brief Information about Hostport Queues (HPQ) to be used
 *	for exchanging information between the Host and RPU.
 *
 * Hostport queue information passed by the RPU to the host, which the host can
 * use, to communicate with the RPU.
 */
struct host_rpu_hpqm_info {
	/** Queue which the RPU uses to inform the host about events. */
	struct host_rpu_hpq event_busy_queue;
	/** Queue on which the consumed events are pushed so that RPU can reuse them. */
	struct host_rpu_hpq event_avl_queue;
	/** Queue used by the host to push commands to the RPU. */
	struct host_rpu_hpq cmd_busy_queue;
	/** Queue which RPU uses to inform host about command buffers which can be used to
	 * push commands to the RPU.
	 */
	struct host_rpu_hpq cmd_avl_queue;
	/** Queue used by the host to push RX buffers to the RPU. */
	struct host_rpu_hpq rx_buf_busy_queue[MAX_NUM_OF_RX_QUEUES];
} __NRF_WIFI_PKD;

/**
 * @brief Common header included in each command/event.
 * This structure encapsulates the common information included at the start of
 * each command/event exchanged with the RPU.
 */
struct host_rpu_msg_hdr {
	/** Length of the message. */
	unsigned int len;
	/** Flag to indicate whether the recipient is expected to resubmit the
	 * cmd/event address back to the trasmitting entity.
	 */
	unsigned int resubmit;
} __NRF_WIFI_PKD;

#endif /* __RPU_IF_H__ */
