/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ESB_COMMON_H_
#define ESB_COMMON_H_

#include <esb.h>
#include "esb_hw.h"
#include "esb_queues.h"


/** Switch time on PRX from end of packet to beginning of the ACK packet.
 *  It is minimum switching time on specific hardware plus additional
 *  margin to ensure the PTX switched on time.
 */
#define SW_TIME_FAST_RX_TO_ACK 21
#define SW_TIME_NORMAL_RX_TO_ACK 42
#define SW_TIME_LEGACY_RX_TO_ACK 132

/** Maximum in-RAM buffer size for radio packets:
 *  S0 bytes + LEN bytes + S1 bytes + max payload length
 */
#define ESB_MAX_RADIO_BUFFER_SIZE (0 + 1 + 1 + ESB_MAX_PAYLOAD_LENGTH)


/* Dynamic length radio PDU header definition. */
struct esb_radio_dynamic_pdu {
	/* Payload length. */
#if ESB_MAX_PAYLOAD_LENGTH > 32
	uint8_t length;
#else
	uint8_t length:6;
	uint8_t rfu0:2;
#endif /* ESB_MAX_PAYLOAD_LENGTH > 32 */

	/* Disable acknowledge. */
	uint8_t ack:1;

	/* Packet ID of the last received packet. Used to detect retransmits. */
	uint8_t pid:2;
	uint8_t rfu1:5;
} __packed;


/* Address information structure. */
struct esb_address_info {
	uint32_t base[2];
	uint8_t prefixes[8];
	uint16_t frequency;
	uint8_t base_length;
	uint8_t pipes_enabled;
};


/* Current ESB configuration. */
extern struct esb_config esb_config;

/* Current ESB address information. */
extern struct esb_address_info esb_address;

/* State of the ESB. */
extern bool esb_running;

/* Active RADIO IRQ handler. It may change depending on context. */
extern void (*volatile esb_active_radio_irq)(void);

/* Active TIMER IRQ handler. It may change depending on context. */
extern void (*volatile esb_active_timer_irq)(void);

/* Redefine payload structures separately for data and ack packets.
 * For compatibility, those two structures must be compatible with esb_payload structure.
 */
struct esb_data_payload {
	uint8_t length;
	uint8_t pipe;
	int8_t rssi;
	uint8_t noack;
	uint8_t hold_tx;
	uint8_t pid;
	uint8_t data[MAX(1, CONFIG_ESB_MAX_DATA_PAYLOAD_LENGTH)];
};

BUILD_ASSERT(sizeof(struct esb_data_payload) <= sizeof(struct esb_payload));
BUILD_ASSERT(offsetof(struct esb_data_payload, length) == offsetof(struct esb_payload, length));
BUILD_ASSERT(offsetof(struct esb_data_payload, pipe) == offsetof(struct esb_payload, pipe));
BUILD_ASSERT(offsetof(struct esb_data_payload, rssi) == offsetof(struct esb_payload, rssi));
BUILD_ASSERT(offsetof(struct esb_data_payload, noack) == offsetof(struct esb_payload, noack));
BUILD_ASSERT(offsetof(struct esb_data_payload, hold_tx) == offsetof(struct esb_payload, hold_tx));
BUILD_ASSERT(offsetof(struct esb_data_payload, pid) == offsetof(struct esb_payload, pid));
BUILD_ASSERT(offsetof(struct esb_data_payload, data) == offsetof(struct esb_payload, data));


struct esb_ack_payload {
	uint8_t length;
	uint8_t pipe;
	int8_t rssi;
	uint8_t noack;
	uint8_t hold_tx;
	uint8_t pid;
	uint8_t data[MAX(1, CONFIG_ESB_MAX_ACK_PAYLOAD_LENGTH)];
};

BUILD_ASSERT(sizeof(struct esb_ack_payload) <= sizeof(struct esb_payload));
BUILD_ASSERT(offsetof(struct esb_ack_payload, length) == offsetof(struct esb_payload, length));
BUILD_ASSERT(offsetof(struct esb_ack_payload, pipe) == offsetof(struct esb_payload, pipe));
BUILD_ASSERT(offsetof(struct esb_ack_payload, rssi) == offsetof(struct esb_payload, rssi));
BUILD_ASSERT(offsetof(struct esb_ack_payload, noack) == offsetof(struct esb_payload, noack));
BUILD_ASSERT(offsetof(struct esb_ack_payload, hold_tx) == offsetof(struct esb_payload, hold_tx));
BUILD_ASSERT(offsetof(struct esb_ack_payload, pid) == offsetof(struct esb_payload, pid));
BUILD_ASSERT(offsetof(struct esb_ack_payload, data) == offsetof(struct esb_payload, data));


/* Structure for holding ACK payload in a queue. */
struct ack_payload_item {
	struct esb_list_item queue_item;
	/* Pointer to the ACK payload. */
	struct esb_ack_payload payload;
};


/** Do setup RADIO and TIMER peripherals according to current configuration and address information.
 * @param force_reconfig If false, reconfigure only if configuration was changed.
 */
void esb_configure_peripherals(bool force_reconfig);


/** Forward declaration of PTX and PRX specific functions. */
int esb_prx_init(const struct esb_config *config);
int esb_ptx_init(const struct esb_config *config);
void esb_prx_disable(void);
void esb_ptx_disable(void);
void esb_ptx_egu_irq(void);
void esb_prx_egu_irq(void);


#endif /* ESB_COMMON_H_ */
