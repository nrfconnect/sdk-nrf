/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __GZP_CONFIG_H
#define __GZP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Definition of "secret key" used during "Host ID exchange".
 */

#define GZP_SECRET_KEY {1, 23, 45, 57, 26, 68, 12, 64, 13, 73, 13, 62, 26, 45, 12, 77}

/*---------------------------------------------------------------------------*/

/**
 * Definition of the first static selected pairing channel. Should be located in
 * the lower Nth of the channel range, where N is the size if the channel subset
 * selected by the application.
 */
#define GZP_CHANNEL_LOW 2

/**
 * Definition of the second static selected pairing channel. Should be located in
 * the upper Nth of the channel range, where N is the size if the channel subset
 * selected by the application.
 */
#define GZP_CHANNEL_HIGH 79

/**
 * Definition of the static "global" pairing address.
 */
#define GZP_ADDRESS {4, 6, 8, 10}

/**
 * Reduced TX power for use during close proximity pairing.
 */
#define GZP_POWER NRF_GZLL_TX_POWER_N16_DBM

/**
 * Definition of pairing request timeout.
 */
#define GZP_REQ_TX_TIMEOUT 200

/**
 * Definition of the maximimum number of "backoff" packets (step 0) to be transmitted.
 */
#define GZP_MAX_BACKOFF_PACKETS 100

/**
 * Definition of the period a device shall wait before attempting to send the packet for
 * fetching the pairing response (step 1).
 */
#define GZP_TX_ACK_WAIT_TIMEOUT (GZP_CLOSE_PROXIMITY_BACKOFF_RX_TIMEOUT + 50)

/**
 * Definition of the period a device in close proximity shall back off on the pairing
 * address after a backoff packet has been received.
 */
#define GZP_CLOSE_PROXIMITY_BACKOFF_RX_TIMEOUT ((GZP_REQ_TX_TIMEOUT / 2) + 50)

/**
 * Definition of the period a device NOT in close proximity shall back off on the pairing
 * address after a backoff packet has been received.
 */
#define GZP_NOT_PROXIMITY_BACKOFF_RX_TIMEOUT (GZP_CLOSE_PROXIMITY_BACKOFF_RX_TIMEOUT + \
					      GZP_STEP1_RX_TIMEOUT)

/**
 * Definition of the time the host waits for a device to complete
 * transmission of the pairing request step 1 packet.
 */
#define GZP_STEP1_RX_TIMEOUT (((GZP_REQ_TX_TIMEOUT / 2) + GZP_TX_ACK_WAIT_TIMEOUT) + 50)

/**
 * Definition of the lowest boundary for the channel range to be used.
 */
#define GZP_CHANNEL_MIN 2

/**
 * Definition of the upper boundary for the channel range to be used.
 */
#define GZP_CHANNEL_MAX 80

/**
 * Definition of the minimum channel spacing for the channel subset generated
 * during pairing.
 */
#define GZP_CHANNEL_SPACING_MIN 5

/**
 * Maximum Device TX payload length [bytes].
 */
#define GZP_MAX_FW_PAYLOAD_LENGTH 17

/**
 * Maximum Host ACK payload length [bytes].
 */
#define GZP_MAX_ACK_PAYLOAD_LENGTH 10



#ifdef __cplusplus
}
#endif

#endif /* __GZP_CONFIG_H */
