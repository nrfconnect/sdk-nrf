/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DTM_HW_CONFIG_H_
#define DTM_HW_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(NRF5340_XXAA_NETWORK) || defined(NRF52833_XXAA) || \
	defined(NRF52811_XXAA) || defined(NRF52820_XXAA)
#define DIRECTION_FINDING_SUPPORTED 1
#else
#define DIRECTION_FINDING_SUPPORTED 0
#endif /* defined(NRF52840_XXAA) || defined(NRF52833_XXAA) ||
	* defined(NRF52811_XXAA) || defined(NRF52820_XXAA)
	*/

/* Maximum transmit or receive time, in microseconds, that the local
 * Controller supports for transmission of a single
 * Link Layer Data Physical Channel PDU, divided by 2.
 */
#if defined(NRF52840_XXAA) || defined(NRF52833_XXAA) || \
	defined(NRF52811_XXAA) || defined(NRF52820_XXAA)
#define NRF_MAX_RX_TX_TIME 0x2148
#else
#define NRF_MAX_RX_TX_TIME 0x424
#endif /* defined(NRF52840_XXAA) || defined(NRF52833_XXAA) ||
	* defined(NRF52811_XXAA) || defined(NRF52820_XXAA)
	*/

#ifdef __cplusplus
}
#endif

#endif /* DTM_HW_CONFIG_H_ */
