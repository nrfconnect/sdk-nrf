/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __ESB_H
#define __ESB_H

#include <stdbool.h>
#include <errno.h>

#include <nrf.h>
#include <hal/nrf_radio.h>

#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup esb Enhanced ShockBurst
 * @{
 * @ingroup proprietary_api
 *
 * @brief Enhanced ShockBurst (ESB) is a basic protocol that supports two-way
 *        data packet communication including packet buffering, packet
 *        acknowledgment, and automatic retransmission of lost packets.
 */

/** @brief Default radio parameters.
 *
 *  Roughly equal to the nRF24Lxx default parameters except for CRC,
 *  which is set to 16 bit, and protocol, which is set to DPL.
 */
#define ESB_DEFAULT_CONFIG                                                     \
	{                                                                      \
		.protocol = ESB_PROTOCOL_ESB_DPL,                              \
		.mode = ESB_MODE_PTX,					       \
		.event_handler = 0,					       \
		.bitrate = ESB_BITRATE_2MBPS,				       \
		.crc = ESB_CRC_16BIT,					       \
		.tx_output_power = 0,			                       \
		.retransmit_delay = 600,				       \
		.retransmit_count = 3,					       \
		.tx_mode = ESB_TXMODE_AUTO,				       \
		.payload_length = 32,					       \
		.selective_auto_ack = false,                                   \
		.use_fast_ramp_up = false                                      \
	}

/** @brief Default legacy radio parameters.
 *
 *  Identical to the nRF24Lxx defaults.
 */
#define ESB_LEGACY_CONFIG                                                      \
	{                                                                      \
		.protocol = ESB_PROTOCOL_ESB,				       \
		.mode = ESB_MODE_PTX,					       \
		.event_handler = 0,					       \
		.bitrate = ESB_BITRATE_2MBPS,				       \
		.crc = ESB_CRC_8BIT,					       \
		.tx_output_power = 0,			                       \
		.retransmit_delay = 600,				       \
		.retransmit_count = 3,					       \
		.tx_mode = ESB_TXMODE_AUTO,				       \
		.payload_length = 32,					       \
		.selective_auto_ack = false,                                   \
		.use_fast_ramp_up = false                                      \
	}

/** @brief Macro to create an initializer for a TX data packet.
 *
 *  This macro generates an initializer.
 *
 *  @param[in]   _pipe   The pipe to use for the data packet.
 *  @param[in]   ...     Comma separated list of character data to put in the
 *                       TX buffer. Supported values consist of 1 to 63
 *			 characters.
 *
 *  @return  Initializer that sets up the pipe, length, and byte array for
 *           content of the TX data.
 */
#define ESB_CREATE_PAYLOAD(_pipe, ...)                                         \
	{                                                                      \
		.pipe = _pipe,                                                 \
		.length = NUM_VA_ARGS_LESS_1(_pipe, __VA_ARGS__),	       \
		.data = {						       \
			__VA_ARGS__                                            \
		}                                                              \
	}

/** @brief Enhanced ShockBurst protocols. */
enum esb_protocol {
	ESB_PROTOCOL_ESB,	/**< Fixed payload length. */
	ESB_PROTOCOL_ESB_DPL	/**< Dynamic payload length. */
};

/** @brief Enhanced ShockBurst modes. */
enum esb_mode {
	ESB_MODE_PTX,	/**< Primary transmitter mode. */
	ESB_MODE_PRX	/**< Primary receiver mode.    */
};

/** @brief Enhanced ShockBurst bitrate modes. */
enum esb_bitrate {
	/** 1 Mb radio mode. */
	ESB_BITRATE_1MBPS = NRF_RADIO_MODE_NRF_1MBIT,
	/** 2 Mb radio mode. */
	ESB_BITRATE_2MBPS = NRF_RADIO_MODE_NRF_2MBIT,

#if defined(RADIO_MODE_MODE_Nrf_250Kbit)
	/** 250 Kb radio mode. */
	ESB_BITRATE_250KBPS = NRF_RADIO_MODE_NRF_250KBIT,
#endif /* defined(RADIO_MODE_MODE_Nrf_250Kbit) */

	/** 1 Mb radio mode using @e Bluetooth low energy radio parameters. */
	ESB_BITRATE_1MBPS_BLE = NRF_RADIO_MODE_BLE_1MBIT,

#if defined(RADIO_MODE_MODE_Ble_2Mbit)
	/** 2 Mb radio mode using @e Bluetooth low energy radio parameters. */
	ESB_BITRATE_2MBPS_BLE = NRF_RADIO_MODE_BLE_2MBIT,
#endif /* defined(RADIO_MODE_MODE_Ble_2Mbit) */
};

/** @brief Enhanced ShockBurst CRC modes. */
enum esb_crc {
	ESB_CRC_16BIT = RADIO_CRCCNF_LEN_Two,	/**< Use two-byte CRC. */
	ESB_CRC_8BIT = RADIO_CRCCNF_LEN_One,	/**< Use one-byte CRC. */
	ESB_CRC_OFF = RADIO_CRCCNF_LEN_Disabled /**< Disable CRC. */
};

/** @brief Enhanced ShockBurst radio transmission power modes. */
enum esb_tx_power {
#if defined(RADIO_TXPOWER_TXPOWER_Pos4dBm)
	/** 4 dBm radio transmit power. */
	ESB_TX_POWER_4DBM = NRF_RADIO_TXPOWER_POS4DBM,
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos4dBm) */

#if defined(RADIO_TXPOWER_TXPOWER_Pos3dBm)
	/** 3 dBm radio transmit power. */
	ESB_TX_POWER_3DBM = NRF_RADIO_TXPOWER_POS3DBM,
#endif /* defined(RADIO_TXPOWER_TXPOWER_Pos3dBm) */

	/** 0 dBm radio transmit power. */
	ESB_TX_POWER_0DBM = NRF_RADIO_TXPOWER_0DBM,
	/** -4 dBm radio transmit power. */
	ESB_TX_POWER_NEG4DBM = NRF_RADIO_TXPOWER_NEG4DBM,
	/** -8 dBm radio transmit power. */
	ESB_TX_POWER_NEG8DBM = NRF_RADIO_TXPOWER_NEG8DBM,
	/** -12 dBm radio transmit power. */
	ESB_TX_POWER_NEG12DBM = NRF_RADIO_TXPOWER_NEG12DBM,
	/** -16 dBm radio transmit power. */
	ESB_TX_POWER_NEG16DBM = NRF_RADIO_TXPOWER_NEG16DBM,
	/** -20 dBm radio transmit power. */
	ESB_TX_POWER_NEG20DBM = NRF_RADIO_TXPOWER_NEG20DBM,
	/** -30 dBm radio transmit power. */
	ESB_TX_POWER_NEG30DBM = NRF_RADIO_TXPOWER_NEG30DBM,
	/** -40 dBm radio transmit power. */
#if defined(RADIO_TXPOWER_TXPOWER_Neg40dBm)
	ESB_TX_POWER_NEG40DBM = NRF_RADIO_TXPOWER_NEG40DBM
#endif /* defined(RADIO_TXPOWER_TXPOWER_Neg40dBm) */
};

/** @brief Enhanced ShockBurst transmission modes. */
enum esb_tx_mode {
	/** Automatic TX mode: When the TX FIFO contains packets and the
	 * radio is idle, packets are sent automatically.
	 */
	ESB_TXMODE_AUTO,
	/** Manual TX mode: Packets are not sent until @ref esb_start_tx
	 * is called. This mode can be used to ensure consistent packet timing.
	 */
	ESB_TXMODE_MANUAL,
	/** Manual start TX mode: Packets are not sent until
	 * @ref esb_start_tx is called. Then, transmission continues
	 * automatically until the TX FIFO is empty.
	 */
	ESB_TXMODE_MANUAL_START
};

/** @brief Enhanced ShockBurst event IDs. */
enum esb_evt_id {
	ESB_EVENT_TX_SUCCESS, /**< Event triggered on TX success. */
	ESB_EVENT_TX_FAILED,  /**< Event triggered on TX failure. */
	ESB_EVENT_RX_RECEIVED /**< Event triggered on RX received. */
};

/** @brief Enhanced ShockBurst payload.
 *
 *  The payload is used both for transmissions and for acknowledging a
 *  received packet with a payload.
 */
struct esb_payload {
	uint8_t length; /**< Length of the packet when not in DPL mode. */
	uint8_t pipe;   /**< Pipe used for this payload. */
	int8_t rssi;   /**< RSSI for the received packet. */
	uint8_t noack;  /**< Flag indicating that this packet will not be
		       *  acknowledged. Flag is ignored when selective auto
		       *  ack is enabled.
		       */
	uint8_t pid;    /**< PID assigned during communication. */
	uint8_t data[CONFIG_ESB_MAX_PAYLOAD_LENGTH]; /**< The payload data. */
};

/** @brief Enhanced ShockBurst event. */
struct esb_evt {
	enum esb_evt_id evt_id;	/**< Enhanced ShockBurst event ID. */
	uint32_t tx_attempts;	/**< Number of TX retransmission attempts. */
};

/** @brief Event handler prototype. */
typedef void (*esb_event_handler)(const struct esb_evt *event);

/** @brief Main configuration structure for the module. */
struct esb_config {
	enum esb_protocol protocol;		/**< Protocol. */
	enum esb_mode mode;			/**< Mode. */
	esb_event_handler event_handler;	/**< Event handler. */
	/* General RF parameters */
	enum esb_bitrate bitrate;		/**< Bitrate mode. */
	enum esb_crc crc;			/**< CRC mode. */
	int8_t tx_output_power;	                /**< Radio TX power. */

	uint16_t retransmit_delay; /**< The delay between each retransmission of
				  *  unacknowledged packets.
				  *  If the CONFIG_ESB_NEVER_DISABLE_TX Kconfig option is enabled,
				  *  this is the delay between two consecutive transmissions.
				  *  Depending on the reception processing time, a minimal
				  *  value might be required (for example, a typical value
				  *  for 32-bit payload is 20 µs).
				  */
	uint16_t retransmit_count; /**< The number of retransmission attempts
				  *  before transmission fail.
				  */

	/* Control settings */
	enum esb_tx_mode tx_mode;	/**< Transmission mode. */

	uint8_t payload_length; /**< Length of the payload (maximum length depends
			       *  on the platforms that are used on each side).
			       */
	bool selective_auto_ack; /**< Selective auto acknowledgement.
				   *  When this feature is disabled, all packets
				   *  will be acknowledged ignoring the noack
				   *  field.
				   */
	bool use_fast_ramp_up; /**<  When this feature is enabled, radio TXEN and
				 *  RXEN delays are reduced from 130 µs to 40 µs.
				 *  The radio peripheral needs some time to start up
				 *  analog components of the radio. On the nRF51 and
				 *  nRF24L Series devices, a hard-coded 130 µs delay is
				 *  implemented. If ESB connection is achieved only
				 *  between nRF52 and/or nRF53 Series devices, this delay can
				 *  be reduced to 40 µs.
				 */
};

/** @brief Initialize the Enhanced ShockBurst module.
 *
 *  @param  config	Parameters for initializing the module.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int esb_init(const struct esb_config *config);

/** @brief Suspend the Enhanced ShockBurst module.
 *
 *  Calling this function stops ongoing communications without changing the
 *  queues.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_suspend(void);

/** @brief Disable the Enhanced ShockBurst module.
 *
 *  Calling this function disables the Enhanced ShockBurst module immediately.
 *  Doing so might stop ongoing communications.
 *
 *  @note All queues are flushed by this function.
 *
 */
void esb_disable(void);

/** @brief Check if the Enhanced ShockBurst module is idle.
 *
 *  @return True if the module is idle, false otherwise.
 */
bool esb_is_idle(void);

/** @brief Write a payload for transmission or acknowledgement.
 *
 *  This function writes a payload that is added to the queue. When the module
 *  is in PTX mode, the payload is queued for a regular transmission. When the
 *  module is in PRX mode, the payload is queued for when a packet is received
 *  that requires an acknowledgement with payload.
 *
 *  @param[in]   payload     The payload.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_write_payload(const struct esb_payload *payload);

/** @brief Read a payload.
 *
 *  @param[in,out] payload	The payload to be received.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_read_rx_payload(struct esb_payload *payload);

/** @brief Start transmitting data.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_start_tx(void);

/** @brief Start receiving data.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_start_rx(void);

/** @brief Stop data reception.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_stop_rx(void);

/** @brief Flush the TX buffer.
 *
 * This function clears the TX FIFO buffer.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_flush_tx(void);

/** @brief Pop the first item from the TX buffer.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_pop_tx(void);

/** @brief Check if there is some free space left in TX FIFO.
 *
 * @retval true when the TX FIFO is full, otherwise false.
 */
bool esb_tx_full(void);

/** @brief Flush the RX buffer.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_flush_rx(void);

/** @brief Set the length of the address.
 *
 *  @param[in] length	Length of the ESB address (in bytes).
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_set_address_length(uint8_t length);

/** @brief Set the base address for pipe 0.
 *
 * @param[in] addr	Address.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_set_base_address_0(const uint8_t *addr);

/** @brief Set the base address for pipe 1 to pipe 7.
 *
 *  @param[in] addr	Address.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_set_base_address_1(const uint8_t *addr);

/** @brief Set the number of pipes and the pipe prefix addresses.
 *
 *  This function configures the number of available pipes, enables the pipes,
 *  and sets their prefix addresses.
 *
 *  @param[in] prefixes		Prefixes for each pipe.
 *  @param[in] num_pipes	Number of pipes. Must be less than or equal to
 *				@kconfig{CONFIG_ESB_PIPE_COUNT}.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_set_prefixes(const uint8_t *prefixes, uint8_t num_pipes);

/** @brief Enable select pipes.
 *
 *  The @p enable_mask parameter must contain the same number of pipes as has
 *  been configured with @ref esb_set_prefixes. This number may not be
 *  greater than the number defined by @kconfig{CONFIG_ESB_PIPE_COUNT}
 *
 *  @param enable_mask	Bitfield mask to enable or disable pipes.
 *			Setting a bit to 0 disables the pipe.
 *			Setting a bit to 1 enables the pipe.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_enable_pipes(uint8_t enable_mask);

/** @brief Update pipe prefix.
 *
 *  @param pipe		Pipe for which to set the prefix.
 *  @param prefix	Prefix to set for the pipe.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_update_prefix(uint8_t pipe, uint8_t prefix);

/** @brief Set the channel to use for the radio.
 *
 *  The module must be in an idle state to call this function. As a PTX, the
 *  application must wait for an idle state and as a PRX, the application must
 *  stop RX before changing the channel. After changing the channel, operation
 *  can be resumed.
 *
 *  @param[in] channel	Channel to use for radio.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_set_rf_channel(uint32_t channel);

/** @brief Get the current radio channel.
 *
 *  @param[in, out] channel	Channel number.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_get_rf_channel(uint32_t *channel);

/** @brief Set the radio output power.
 *
 *  @param[in] tx_output_power	Output power in dBm. The @ref esb_tx_power values can be used
 *                              to provide backward compatibility.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_set_tx_power(int8_t tx_output_power);

/** @brief Set the packet retransmit delay.
 *
 *  @param[in] delay	Delay between retransmissions.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_set_retransmit_delay(uint16_t delay);

/** @brief Set the number of retransmission attempts.
 *  @details If the CONFIG_ESB_NEVER_DISABLE_TX Kconfig option is enabled,
 *           this is the delay between two consecutive transmissions.
 *           Depending on the reception processing time, a minimal
 *           value might be required (for example, a typical value
 *           for 32-bit payload is 20 µs).
 *  @param[in] count	Number of retransmissions.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_set_retransmit_count(uint16_t count);

/** @brief Set the radio bitrate.
 *
 * @param[in] bitrate	Radio bitrate.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_set_bitrate(enum esb_bitrate bitrate);

/** @brief Reuse a packet ID for a specific pipe.
 *
 *  The ESB protocol uses a 2-bit sequence number (packet ID) to identify
 *  retransmitted packets. By default, the packet ID is incremented for every
 *  uploaded packet. Use this function to prevent this and send two different
 *  packets with the same packet ID.
 *
 *  @param[in] pipe	Pipe.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int esb_reuse_pid(uint8_t pipe);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ESB */
