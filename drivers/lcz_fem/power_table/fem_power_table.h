/*
 * Copyright (c) 2023 Laird Connectivity
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* The output of the soc is reduced so that the power at the antenna
 * is reduced.
 */
#define MPSL_FEM_POWER_REDUCE(p) (CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB - p)

extern const char *const POWER_TABLE_STR;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
extern const mpsl_tx_power_envelope_t ENV_125K;
extern const mpsl_tx_power_envelope_t ENV_500K;
#endif

#if defined(CONFIG_BT)
extern const mpsl_tx_power_envelope_t ENV_1M;
extern const mpsl_tx_power_envelope_t ENV_2M;
#endif

#if defined(CONFIG_NRF_802154_SL)
extern const mpsl_tx_power_envelope_t ENV_802154;
#endif

/* Envelope indices are the BLE channel
 *  .envelope.tx_power_ble[0]  is 2404 MHz
 *   .envelope.tx_power_ble[1]     2406
 *   .envelope.tx_power_ble[2]     2408
 *   .envelope.tx_power_ble[17] is 2440
 *   .envelope.tx_power_ble[35] is 2476
 *   .envelope.tx_power_ble[36] is 2478
 *   .envelope.tx_power_ble[37] is 2402 MHz **
 *   .envelope.tx_power_ble[38] is 2426 **
 *   .envelope.tx_power_ble[39] is 2480
 */
