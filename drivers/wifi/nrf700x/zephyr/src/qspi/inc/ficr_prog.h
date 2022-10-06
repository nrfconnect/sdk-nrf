/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing address/offets and functions for writing
 * the FICR fields of the OTP memory on nRF7002 device
 */

#ifndef __OTP_PROG_H_
#define __OTP_PROG_H_

#include <stdio.h>
#include <stdlib.h>

/* REGION PROTECT : OTP Address offsets (word offsets) */
#define REGION_PROTECT          64
#define QSPI_KEY                68
#define MAC0_ADDR               72
#define MAC1_ADDR               74
#define CALIB_XO                76
#define CALIB_PDADJMCS7         77
#define CALIB_PDADJMCS0         78
#define CALIB_MAXPOW2G4         79
#define CALIB_MAXPOW5G0MCS7     80
#define CALIB_MAXPOW5G0MCS0     81
#define CALIB_RXGAINOFFSET      82
#define CALIB_TXPOWBACKOFFT     83
#define CALIB_TXPOWBACKOFFV     84
#define REGION_DEFAULTS         85
#define OTP_MAX_WORD_LEN        128

/* MASKS to program bit fields in REGION_DEFAULTS register */
#define QSPI_KEY_FLAG_MASK              ~(1U<<0)
#define MAC0_ADDR_FLAG_MASK             ~(1U<<1)
#define MAC1_ADDR_FLAG_MASK             ~(1U<<2)
#define CALIB_XO_FLAG_MASK              ~(1U<<3)
#define CALIB_PDADJMCS7_FLAG_MASK       ~(1U<<4)
#define CALIB_PDADJMCS0_FLAG_MASK       ~(1U<<5)
#define CALIB_MAXPOW2G4_FLAG_MASK       ~(1U<<6)
#define CALIB_MAXPOW5G0MCS7_FLAG_MASK   ~(1U<<7)
#define CALIB_MAXPOW5G0MCS0_FLAG_MASK   ~(1U<<8)
#define CALIB_RXGAINOFFSET_FLAG_MASK    ~(1U<<9)
#define CALIB_TXPOWBACKOFFT_FLAG_MASK   ~(1U<<10)
#define CALIB_TXPOWBACKOFFV_FLAG_MASK   ~(1U<<11)

/* OTP Device address definitions */
#define OTP_VOLTCTRL_ADDR   0x19004
#define OTP_VOLTCTRL_2V5    0x3b
#define OTP_VOLTCTRL_1V8    0xb

#define OTP_POLL_ADDR       0x01B804
#define OTP_WR_DONE         0x1
#define OTP_READ_VALID      0x2
#define OTP_READY           0x4


#define OTP_RWSBMODE_ADDR   0x01B800
#define OTP_STANDBY_MODE    0x0
#define OTP_READ_MODE       0x1
#define OTP_BYTE_WRITE_MODE 0x42


#define OTP_RDENABLE_ADDR   0x01B810
#define OTP_READREG_ADDR    0x01B814

#define OTP_WRENABLE_ADDR   0x01B808
#define OTP_WRITEREG_ADDR   0x01B80C

#define OTP_TIMING_REG1_ADDR  0x01B820
#define OTP_TIMING_REG1_VAL   0x0
#define OTP_TIMING_REG2_ADDR  0x01B824
#define OTP_TIMING_REG2_VAL   0x030D8B

#define PRODTEST_TRIM_LEN     15

#define OTP_FRESH_FROM_FAB    0xFFFFFFFF
#define OTP_PROGRAMMED        0x00000000
#define OTP_ENABLE_PATTERN    0x50FA50FA
#define OTP_INVALID           0xDEADBEEF

int write_otp_memory(unsigned int otp_addr, unsigned int *write_val);
int read_otp_memory(unsigned int otp_addr, unsigned int *read_val, int len);
unsigned int check_protection(unsigned int *buff, unsigned int off1, unsigned int off2,
					unsigned int off3, unsigned int off4);

#endif /* __OTP_PROG_H_ */
