/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef __NRF_ESB_H
#define __NRF_ESB_H

#include <errno.h>
#include <sys/util.h>
#include <nrf.h>
#include <stdbool.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup nrf_esb Enhanced ShockBurst
 * @{
 * @ingroup proprietary_api
 *
 * @brief Enhanced ShockBurst (ESB) is a basic protocol that supports two-way
 *        data packet communication including packet buffering, packet
 *        acknowledgment, and automatic retransmission of lost packets.
 */


/** The ESB event IRQ number when running on an nRF5 device. */
#define ESB_EVT_IRQ SWI0_IRQn
/** The handler for @ref ESB_EVT_IRQ when running on an nRF5 device. */
#define ESB_EVT_IRQHandler SWI0_IRQHandler

/** @brief Default radio parameters.
 *
 *  Roughly equal to the nRF24Lxx default parameters except for CRC,
 *  which is set to 16 bit, and protocol, which is set to DPL.
 */
#define NRF_ESB_DEFAULT_CONFIG                                                 \
	{                                                                      \
		.protocol = NRF_ESB_PROTOCOL_ESB_DPL,                          \
		.mode = NRF_ESB_MODE_PTX,				       \
		.event_handler = 0,					       \
		.bitrate = NRF_ESB_BITRATE_2MBPS,			       \
		.crc = NRF_ESB_CRC_16BIT,				       \
		.tx_output_power = NRF_ESB_TX_POWER_0DBM,                      \
		.retransmit_delay = 600,				       \
		.retransmit_count = 3,					       \
		.tx_mode = NRF_ESB_TXMODE_AUTO,				       \
		.radio_irq_priority = 1,				       \
		.event_irq_priority = 2,				       \
		.payload_length = 32,					       \
		.selective_auto_ack = false                                    \
	}

/** @brief Default legacy radio parameters.
 *
 *  Identical to the nRF24Lxx defaults.
 */
#define NRF_ESB_LEGACY_CONFIG                                                  \
	{                                                                      \
		.protocol = NRF_ESB_PROTOCOL_ESB,			       \
		.mode = NRF_ESB_MODE_PTX,				       \
		.event_handler = 0,					       \
		.bitrate = NRF_ESB_BITRATE_2MBPS,			       \
		.crc = NRF_ESB_CRC_8BIT,                                       \
		.tx_output_power = NRF_ESB_TX_POWER_0DBM,                      \
		.retransmit_delay = 600,				       \
		.retransmit_count = 3,					       \
		.tx_mode = NRF_ESB_TXMODE_AUTO,				       \
		.radio_irq_priority = 1,				       \
		.event_irq_priority = 2,				       \
		.payload_length = 32,					       \
		.selective_auto_ack = false                                    \
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
#define NRF_ESB_CREATE_PAYLOAD(_pipe, ...)                                     \
	{                                                                      \
		.pipe = _pipe,                                                 \
		.length = NUM_VA_ARGS_LESS_1(_pipe, __VA_ARGS__),	       \
		.data = {						       \
			__VA_ARGS__                                            \
		}                                                              \
	}

/** @brief Enhanced ShockBurst protocols. */
enum nrf_esb_protocol {
	NRF_ESB_PROTOCOL_ESB,    /**< Enhanced ShockBurst with fixed payload
				   *  length.
				   */
	NRF_ESB_PROTOCOL_ESB_DPL /**< Enhanced ShockBurst with dynamic payload
				   *  length.
				   */
};

/** @brief Enhanced ShockBurst modes. */
enum nrf_esb_mode {
	NRF_ESB_MODE_PTX, /**< Primary transmitter mode. */
	NRF_ESB_MODE_PRX  /**< Primary receiver mode.    */
};

/** @brief Enhanced ShockBurst bitrate modes. */
enum nrf_esb_bitrate {
	/** 1 Mb radio mode. */
	NRF_ESB_BITRATE_1MBPS = RADIO_MODE_MODE_Nrf_1Mbit,
	 /** 2 Mb radio mode. */
	NRF_ESB_BITRATE_2MBPS = RADIO_MODE_MODE_Nrf_2Mbit,
#if !(defined(CONFIG_SOC_NRF52840) || defined(CONFIG_SOC_NRF52810)\
		|| defined(CONFIG_SOC_NRF52811))
	/** 250 Kb radio mode. */
	NRF_ESB_BITRATE_250KBPS = RADIO_MODE_MODE_Nrf_250Kbit,
#endif
	/** 1 Mb radio mode using @e Bluetooth low energy radio parameters. */
	NRF_ESB_BITRATE_1MBPS_BLE = RADIO_MODE_MODE_Ble_1Mbit,
#if defined(CONFIG_SOC_SERIES_NRF52X)
	/** 2 Mb radio mode using @e Bluetooth low energy radio parameters. */
	NRF_ESB_BITRATE_2MBPS_BLE = 4,
#endif
};

/** @brief Enhanced ShockBurst CRC modes. */
enum nrf_esb_crc {
	NRF_ESB_CRC_16BIT = RADIO_CRCCNF_LEN_Two,   /**< Use two-byte CRC. */
	NRF_ESB_CRC_8BIT = RADIO_CRCCNF_LEN_One,    /**< Use one-byte CRC. */
	NRF_ESB_CRC_OFF = RADIO_CRCCNF_LEN_Disabled /**< Disable CRC. */
};

/** @brief Enhanced ShockBurst radio transmission power modes. */
enum nrf_esb_tx_power {
	/** 4 dBm radio transmit power. */
	NRF_ESB_TX_POWER_4DBM = RADIO_TXPOWER_TXPOWER_Pos4dBm,
#if defined(CONFIG_SOC_SERIES_NRF52X)
	/** 3 dBm radio transmit power. */
	NRF_ESB_TX_POWER_3DBM = RADIO_TXPOWER_TXPOWER_Pos3dBm,
#endif
	/** 0 dBm radio transmit power. */
	NRF_ESB_TX_POWER_0DBM = RADIO_TXPOWER_TXPOWER_0dBm,
	/** -4 dBm radio transmit power. */
	NRF_ESB_TX_POWER_NEG4DBM = RADIO_TXPOWER_TXPOWER_Neg4dBm,
	/** -8 dBm radio transmit power. */
	NRF_ESB_TX_POWER_NEG8DBM = RADIO_TXPOWER_TXPOWER_Neg8dBm,
	/** -12 dBm radio transmit power. */
	NRF_ESB_TX_POWER_NEG12DBM = RADIO_TXPOWER_TXPOWER_Neg12dBm,
	/** -16 dBm radio transmit power. */
	NRF_ESB_TX_POWER_NEG16DBM = RADIO_TXPOWER_TXPOWER_Neg16dBm,
	/** -20 dBm radio transmit power. */
	NRF_ESB_TX_POWER_NEG20DBM = RADIO_TXPOWER_TXPOWER_Neg20dBm,
	/** -30 dBm radio transmit power. */
	NRF_ESB_TX_POWER_NEG30DBM = RADIO_TXPOWER_TXPOWER_Neg30dBm,
	/** -40 dBm radio transmit power. */
	NRF_ESB_TX_POWER_NEG40DBM = RADIO_TXPOWER_TXPOWER_Neg40dBm
};

/** @brief Enhanced ShockBurst transmission modes. */
enum nrf_esb_tx_mode {
	/** Automatic TX mode: When the TX FIFO contains packets and the
	 * radio is idle, packets are sent automatically.
	 */
	NRF_ESB_TXMODE_AUTO,
	/** Manual TX mode: Packets are not sent until @ref nrf_esb_start_tx
	 * is called. This mode can be used to ensure consistent packet timing.
	 */
	NRF_ESB_TXMODE_MANUAL,
	/** Manual start TX mode: Packets are not sent until
	 * @ref nrf_esb_start_tx is called. Then, transmission continues
	 * automatically until the TX FIFO is empty.
	 */
	NRF_ESB_TXMODE_MANUAL_START
};

/** @brief Enhanced ShockBurst event IDs. */
enum nrf_esb_evt_id {
	NRF_ESB_EVENT_TX_SUCCESS, /**< Event triggered on TX success. */
	NRF_ESB_EVENT_TX_FAILED,  /**< Event triggered on TX failure. */
	NRF_ESB_EVENT_RX_RECEIVED /**< Event triggered on RX received. */
};

/** @brief Enhanced ShockBurst payload.
 *
 *  The payload is used both for transmissions and for acknowledging a
 *  received packet with a payload.
 */
struct nrf_esb_payload {
	u8_t length; /**< Length of the packet when not in DPL mode. */
	u8_t pipe;   /**< Pipe used for this payload. */
	s8_t rssi;   /**< RSSI for the received packet. */
	u8_t noack;  /**< Flag indicating that this packet will not be
		       *  acknowledged. Flag is ignored when selective auto
		       *  ack is enabled.
		       */
	u8_t pid;    /**< PID assigned during communication. */
	u8_t data[CONFIG_NRF_ESB_MAX_PAYLOAD_LENGTH]; /**< The payload data. */
};

/** @brief Enhanced ShockBurst event. */
struct nrf_esb_evt {
	enum nrf_esb_evt_id evt_id;	/**< Enhanced ShockBurst event ID. */
	u32_t tx_attempts;	/**< Number of TX retransmission attempts. */
};

/** @brief Definition of the event handler for the module. */
typedef void (*nrf_esb_event_handler)(const struct nrf_esb_evt *event);

/** @brief Main configuration structure for the module. */
struct nrf_esb_config {
	enum nrf_esb_protocol protocol;		/**< ESB protocol. */
	enum nrf_esb_mode mode;			/**< ESB mode. */
	nrf_esb_event_handler event_handler;	/**< ESB event handler. */
	/* General RF parameters */
	enum nrf_esb_bitrate bitrate;		/**< Bitrate mode. */
	enum nrf_esb_crc crc;			/**< CRC mode. */
	enum nrf_esb_tx_power tx_output_power;	/**< Radio TX power. */

	u16_t retransmit_delay; /**< The delay between each retransmission of
				  *  unacknowledged packets.
				  */
	u16_t retransmit_count; /**< The number of retransmission attempts
				  *  before transmission fail.
				  */

	/* Control settings */
	enum nrf_esb_tx_mode tx_mode;	/**< Transmission mode. */

	u8_t radio_irq_priority;	/**< nRF radio interrupt priority. */
	u8_t event_irq_priority;	/**< ESB event interrupt priority. */

	u8_t payload_length; /**< Length of the payload (maximum length depends
			       *  on the platforms that are used on each side).
			       */
	bool selective_auto_ack; /**< Selective auto acknowledgement.
				   *  When this feature is disabled, all packets
				   *  will be acknowledged ignoring the noack
				   *  field.
				   */
};

/** @brief Initialize the Enhanced ShockBurst module.
 *
 *  @param  config	Parameters for initializing the module.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int nrf_esb_init(const struct nrf_esb_config *config);

/** @brief Suspend the Enhanced ShockBurst module.
 *
 *  Calling this function stops ongoing communications without changing the
 *  queues.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_esb_suspend(void);

/** @brief Disable the Enhanced ShockBurst module.
 *
 *  Calling this function disables the Enhanced ShockBurst module immediately.
 *  Doing so might stop ongoing communications.
 *
 *  @note All queues are flushed by this function.
 *
 */
void nrf_esb_disable(void);

/** @brief Check if the Enhanced ShockBurst module is idle.
 *
 *  @return True if the module is idle, false otherwise.
 */
bool nrf_esb_is_idle(void);

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
int nrf_esb_write_payload(const struct nrf_esb_payload *payload);

/** @brief Read a payload.
 *
 *  @param[in,out] payload	The payload to be received.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_esb_read_rx_payload(struct nrf_esb_payload *payload);

/** @brief Start transmitting data.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_esb_start_tx(void);

/** @brief Start receiving data.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_esb_start_rx(void);

/** @brief Stop data reception.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_esb_stop_rx(void);

/** @brief Flush the TX buffer.
 *
 * This function clears the TX FIFO buffer.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_esb_flush_tx(void);

/** @brief Pop the first item from the TX buffer.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_esb_pop_tx(void);

/** @brief Flush the RX buffer.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_esb_flush_rx(void);

/** @brief Set the length of the address.
 *
 *  @param[in] length	Length of the ESB address (in bytes).
 *
* @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
  */
int nrf_esb_set_address_length(u8_t length);

/** @brief Set the base address for pipe 0.
 *
 * @param[in] addr	Address.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_esb_set_base_address_0(const u8_t *addr);

/** @brief Set the base address for pipe 1 to pipe 7.
 *
 *  @param[in] addr	Address.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_esb_set_base_address_1(const u8_t *addr);

/** @brief Set the number of pipes and the pipe prefix addresses.
 *
 *  This function configures the number of available pipes, enables the pipes,
 *  and sets their prefix addresses.
 *
 *  @param[in] prefixes		Prefixes for each pipe.
 *  @param[in] num_pipes	Number of pipes. Must be less than or equal to
 *				@ref CONFIG_NRF_ESB_PIPE_COUNT.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_esb_set_prefixes(const u8_t *prefixes, u8_t num_pipes);

/** @brief Enable select pipes.
 *
 *  The @p enable_mask parameter must contain the same number of pipes as has
 *  been configured with @ref nrf_esb_set_prefixes. This number may not be
 *  greater than the number defined by @ref CONFIG_NRF_ESB_PIPE_COUNT
 *
 *  @param enable_mask	Bitfield mask to enable or disable pipes.
 *			Setting a bit to 0 disables the pipe.
 *			Setting a bit to 1 enables the pipe.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_esb_enable_pipes(u8_t enable_mask);

/** @brief Update pipe prefix.
 *
 *  @param pipe		Pipe for which to set the prefix.
 *  @param prefix	Prefix to set for the pipe.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_esb_update_prefix(u8_t pipe, u8_t prefix);

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
int nrf_esb_set_rf_channel(u32_t channel);

/** @brief Get the current radio channel.
 *
 *  @param[in, out] channel	Channel number.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_esb_get_rf_channel(u32_t *channel);

/** @brief Set the radio output power.
 *
 *  @param[in] tx_output_power	Output power.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_esb_set_tx_power(enum nrf_esb_tx_power tx_output_power);

/** @brief Set the packet retransmit delay.
 *
 *  @param[in] delay	Delay between retransmissions.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_esb_set_retransmit_delay(u16_t delay);

/** @brief Set the number of retransmission attempts.
 *
 *  @param[in] count	Number of retransmissions.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_esb_set_retransmit_count(u16_t count);

/** @brief Set the radio bitrate.
 *
 * @param[in] bitrate	Radio bitrate.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_esb_set_bitrate(enum nrf_esb_bitrate bitrate);

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
int nrf_esb_reuse_pid(u8_t pipe);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_ESB */
