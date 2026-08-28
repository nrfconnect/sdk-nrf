/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file nwrt_shadow.h
 * @brief SWD host control: common, TX, and RX blocks at fixed DT addresses.
 *
 * Three shadow blocks plus an RX capture buffer are placed in devicetree-reserved
 * SRAM. The host writes 32-bit words over SWD, then writes @c submit (packed
 * @c pending + @c seq). Firmware polls from @ref nwrt_shadow_poll(),
 * executes the request, and writes @c result and @c ack.
 *
 * Submit layout (v8): @c pending in bits [15:0], @c seq in bits [31:16].
 *
 * Runtime host modes (same firmware image):
 * - ATE: write @c pending only (seq=0), poll @c submit until 0 or @c result until
 *   not @ref NWRT_SHADOW_RESULT_RUNNING.
 * - Lab: write @c seq << 16 | pending, poll @c ack == seq, read @c result.
 *
 * Absolute field addresses: README-SWD.md.
 */

#ifndef NWRT_SHADOW_H__
#define NWRT_SHADOW_H__

#include <radio_test/fmac_structs.h>
#include <stdint.h>

#if defined(__ZEPHYR__)
#include <zephyr/devicetree.h>

///< Common block base (DT).
#define NWRT_SHADOW_COMMON_BASE DT_REG_ADDR(DT_NODELABEL(nwrt_shadow_common_sram))
#define NWRT_SHADOW_TX_BASE     DT_REG_ADDR(DT_NODELABEL(nwrt_shadow_tx_sram)) ///< TX block base (DT).
#define NWRT_SHADOW_RX_BASE     DT_REG_ADDR(DT_NODELABEL(nwrt_shadow_rx_sram)) ///< RX block base (DT).
///< RX capture buffer base (DT).
#define NWRT_RX_CAP_BASE DT_REG_ADDR(DT_NODELABEL(nwrt_rx_cap_sram))
///< RX capture buffer size in bytes (DT).
#define NWRT_RX_CAP_SIZE DT_REG_SIZE(DT_NODELABEL(nwrt_rx_cap_sram))
#else
#define NWRT_SHADOW_COMMON_BASE 0x2007f000U   ///< Common block base (nrf7120dk default).
#define NWRT_SHADOW_TX_BASE     0x2007f100U   ///< TX block base (nrf7120dk default).
#define NWRT_SHADOW_RX_BASE     0x2007f200U   ///< RX block base (nrf7120dk default).
#define NWRT_RX_CAP_BASE        0x2007b000U   ///< RX capture buffer base (nrf7120dk default).
#define NWRT_RX_CAP_SIZE        (16U * 1024U) ///< RX capture buffer size in bytes (default).
#endif

///< Size of each shadow block slot in SRAM.
#define NWRT_SHADOW_BLOCK_SIZE 0x100U
///< Max capture samples (3 bytes per sample).
#define NWRT_RX_CAP_MAX_SAMPLES (NWRT_RX_CAP_SIZE / 3U)

#define NWRT_SHADOW_MAGIC   0x4e575254U ///< Shadow layout magic word ("NWRT").
#define NWRT_SHADOW_VERSION 8U          ///< Shadow protocol version; must match common.version.

#define NWRT_SHADOW_ERR_INVAL (-22) ///< result: invalid request, bad magic/version, or bad params.
#define NWRT_SHADOW_ERR_BUSY  (-16) ///< result: RF test path busy (TX/RX/tone/capture active).
#define NWRT_SHADOW_ERR_NODEV (-19) ///< result: FMAC/RPU device context not ready.
#define NWRT_SHADOW_ERR_IO    (-5)  ///< result: FMAC or driver call failed.

/** result: command accepted and still running (ATE poll until not this value). */
#define NWRT_SHADOW_RESULT_RUNNING (-256)

///< Low 16 bits of @c submit: @c NWRT_*_PENDING_* flags.
#define NWRT_SHADOW_SUBMIT_PENDING_MASK 0xFFFFU
///< Bit shift of @c seq in @c submit.
#define NWRT_SHADOW_SUBMIT_SEQ_SHIFT 16U

/** Pack pending flags and seq into one @c submit word (host writes once). */
#define NWRT_SHADOW_SUBMIT_MAKE(pending, seq)                                 \
    (((uint32_t)(pending) & NWRT_SHADOW_SUBMIT_PENDING_MASK) |                \
     (((uint32_t)(seq) & 0xFFFFU) << NWRT_SHADOW_SUBMIT_SEQ_SHIFT))

/** Pending flags from @c submit (low 16 bits). */
#define NWRT_SHADOW_SUBMIT_PENDING(submit) \
    ((uint32_t)(submit) & NWRT_SHADOW_SUBMIT_PENDING_MASK)

/** Seq from @c submit (high 16 bits). Zero selects ATE-style auto-seq. */
#define NWRT_SHADOW_SUBMIT_SEQ(submit) \
    (((uint32_t)(submit) >> NWRT_SHADOW_SUBMIT_SEQ_SHIFT) & 0xFFFFU)

/** @name Common block pending flags (low 16 bits of submit) @{ */
///< Load default rpu_conf_params (no radio test init).
#define NWRT_COMMON_PENDING_CONF_INIT  (1U << 0)
#define NWRT_COMMON_PENDING_RADIO_INIT (1U << 1) ///< Enter RF test mode on band_idx / channel.
/** @} */

/** @name TX block pending flags (low 16 bits of submit) @{ */
#define NWRT_TX_PENDING_APPLY (1U << 0) ///< Copy shadow TX params into live rpu_conf_params.
#define NWRT_TX_PENDING_PROG  (1U << 1) ///< Start or stop packet TX (enable_tx).
#define NWRT_TX_PENDING_TONE  (1U << 2) ///< Start or stop CW tone TX (tx_tone, tx_tone_freq).
/** @} */

/** @name RX block pending flags (low 16 bits of submit) @{ */
#define NWRT_RX_PENDING_APPLY  (1U << 0) ///< Copy shadow RX params into live rpu_conf_params.
#define NWRT_RX_PENDING_PROG   (1U << 1) ///< Start or stop RX (enable_rx).
#define NWRT_RX_PENDING_STATS  (1U << 2) ///< Fetch RX statistics into nwrt_shadow_stats.
#define NWRT_RX_PENDING_RX_CAP (1U << 3) ///< Run RX capture into nwrt_rx_cap_buf.
/** @} */

/** @name RX capture type (nwrt_shadow_rx.rx_cap_type; enum nrf_wifi_rf_test) @{ */
#define NWRT_RX_CAP_TYPE_ADC      0U ///< ADC capture.
#define NWRT_RX_CAP_TYPE_STAT_PKT 1U ///< Static packet capture.
#define NWRT_RX_CAP_TYPE_DYN_PKT  2U ///< Dynamic packet capture.

/** @} */

/**
 * @brief Common shadow block: layout header, control words, and radio-init params.
 *
 * Mapped at @ref NWRT_SHADOW_COMMON_BASE.
 */
struct nwrt_shadow_common
{
    uint32_t magic;    ///< Must be @ref NWRT_SHADOW_MAGIC.
    uint32_t version;  ///< Must be @ref NWRT_SHADOW_VERSION.
    uint32_t submit;   ///< Packed pending (low 16) + seq (high 16); cleared to 0 when done.
    int32_t  result;   ///< 0 on success, @ref NWRT_SHADOW_RESULT_RUNNING while busy, else error.
    uint32_t ack;      ///< Last processed seq (lab debug: poll until ack == seq from submit).
    uint32_t band_idx; ///< For RADIO_INIT: 0 = 2.4 GHz, 1 = 5 GHz, 2 = 6 GHz.
    uint32_t channel;  ///< Primary channel number for RADIO_INIT.
};

/**
 * @brief TX shadow block: control words and packet/tone TX parameters.
 *
 * Mapped at @ref NWRT_SHADOW_TX_BASE.
 */
struct nwrt_shadow_tx
{
    uint32_t submit;           ///< Packed pending (low 16) + seq (high 16).
    int32_t  result;           ///< Completion status (see nwrt_shadow_common.result).
    uint32_t ack;              ///< Last processed seq.
    uint32_t tx_power;         ///< TX power (driver units; applied on APPLY).
    int32_t  tx_tone_freq;     ///< CW tone frequency offset.
    uint32_t tx_pkt_len;       ///< Packet length in bytes.
    int32_t  tx_pkt_num;       ///< Number of packets; -1 for continuous.
    int32_t  tx_pkt_mcs;       ///< MCS index for packet TX.
    int32_t  tx_pkt_rate;      ///< Legacy rate index for packet TX.
    uint32_t tx_pkt_tput_mode; ///< Throughput test mode selector.
    uint32_t enable_tx;        ///< 1 to start packet TX, 0 to stop (PROG pending).
    uint32_t tx_tone;          ///< 1 to enable CW tone, 0 to disable (TONE pending).
};

/**
 * @brief RX shadow block: control words, capture params, and readback fields.
 *
 * Mapped at @ref NWRT_SHADOW_RX_BASE. Capture samples land in @ref nwrt_rx_cap_buf.
 */
struct nwrt_shadow_rx
{
    uint32_t submit;             ///< Packed pending (low 16) + seq (high 16).
    int32_t  result;             ///< Completion status (see nwrt_shadow_common.result).
    uint32_t ack;                ///< Last processed seq.
    uint32_t cap_timeout_status; ///< Set after RX_CAP (driver timeout status).
    uint32_t lna_gain;           ///< LNA gain for RX capture.
    uint32_t bb_gain;            ///< Baseband gain for RX capture.
    uint32_t capture_length;     ///< Sample count to capture (@ref NWRT_RX_CAP_MAX_SAMPLES max).
    uint32_t capture_timeout;    ///< Capture timeout in driver units.
    uint32_t enable_rx;          ///< 1 to start RX, 0 to stop (PROG pending).
    uint32_t rx_cap_type;        ///< @ref NWRT_RX_CAP_TYPE_ADC, STAT_PKT, or DYN_PKT.
};

/** Common shadow instance in DT-reserved SRAM (@ref NWRT_SHADOW_COMMON_BASE). */
extern struct nwrt_shadow_common nwrt_shadow_common;
/** TX shadow instance in DT-reserved SRAM (@ref NWRT_SHADOW_TX_BASE). */
extern struct nwrt_shadow_tx nwrt_shadow_tx;
/** RX shadow instance in DT-reserved SRAM (@ref NWRT_SHADOW_RX_BASE). */
extern struct nwrt_shadow_rx nwrt_shadow_rx;
/** RX statistics buffer filled by @ref NWRT_RX_PENDING_STATS. */
extern struct rpu_rt_op_stats nwrt_shadow_stats;
/** RX capture sample buffer (@ref NWRT_RX_CAP_BASE, @ref NWRT_RX_CAP_SIZE bytes). */
extern uint8_t nwrt_rx_cap_buf[NWRT_RX_CAP_SIZE];

/**
 * @brief Initialize all shadow blocks and capture buffer defaults.
 *
 * Clears structures, fills param defaults, sets @c magic and @c version on common.
 * Called at @c PRE_KERNEL_2 and again from @c main(). Does not touch the Wi-Fi driver.
 */
void nwrt_shadow_init(void);

/**
 * @brief Process pending host requests on common, TX, and RX blocks.
 *
 * Call from the main loop after RPU/FMAC is ready. For each block with a new
 * @c submit, runs the pending handlers, clears @c submit to 0, and updates
 * @c result and @c ack.
 */
void nwrt_shadow_poll(void);

#endif /* NWRT_SHADOW_H__ */
