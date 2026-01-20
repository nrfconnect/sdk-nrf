/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_FAST_PAIR_FHN_PF_BLE_CS_H_
#define BT_FAST_PAIR_FHN_PF_BLE_CS_H_

#include <bluetooth/services/fast_pair/fhn/pf/pf.h>
#include <zephyr/bluetooth/addr.h>

/**
 * @defgroup bt_fast_pair_fhn_pf_ble_cs The Precision Finding API for Bluetooth Channel Sounding
 * @brief Collection of the Precision Finding APIs that are relevant for the Bluetooth Channel
 *        Sounding as the ranging technology.
 *
 *  The API is only available when the @kconfig{CONFIG_BT_FAST_PAIR_FMDN_PF} Kconfig option
 *  is enabled.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Bluetooth Low Energy (BLE) Channel Sounding (CS) security levels.
 *
 *  The BLE CS security levels are defined in the Bluetooth Core Specification v6.2, Volume 3,
 *  Part C (Generic Access Profile), section 10.11.1 (Channel Sounding security).
 */
enum bt_fast_pair_fhn_pf_ble_cs_security_level {
	/** Unknown BLE CS security level. */
	BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_UNKNOWN = 0,

	/** BLE CS security level 1. */
	BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_ONE = 1,

	/** BLE CS security level 2. */
	BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_TWO = 2,

	/** BLE CS security level 3. */
	BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_THREE = 3,

	/** BLE CS security level 4. */
	BT_FAST_PAIR_FHN_PF_BLE_CS_SECURITY_LEVEL_FOUR = 4,
};

/** @brief Check if a security level is set in a bitmask.
 *
 *  This function can be called from any context.
 *
 *  @param bm    Security level bitmask.
 *  @param level Security level to check.
 *
 *  @return true if the security level is set in the bitmask, false otherwise.
 */
bool bt_fast_pair_fhn_pf_ble_cs_security_level_bm_check(
	uint8_t bm, enum bt_fast_pair_fhn_pf_ble_cs_security_level level);

/** @brief Set or clear a security level bit in a bitmask.
 *
 *  This function can be called from any context.
 *
 *  @param bm    Pointer to the security level bitmask.
 *  @param level Security level to modify.
 *  @param value true to set the bit, false to clear it.
 */
void bt_fast_pair_fhn_pf_ble_cs_security_level_bm_write(
	uint8_t *bm, enum bt_fast_pair_fhn_pf_ble_cs_security_level level, bool value);

/** @brief Descriptor of the BLE CS technology capability payload.
 *
 *  The descriptor is used to encode the BLE CS technology capability payload for the Ranging
 *  Capability Response message with the @ref bt_fast_pair_fhn_pf_ble_cs_ranging_capability_encode
 *  function.
 */
struct bt_fast_pair_fhn_pf_ble_cs_ranging_capability {
	/** Bitmask of the supported BLE CS security levels, composed of
	 *  @ref bt_fast_pair_fhn_pf_ble_cs_security_level bit positions.
	 */
	uint8_t supported_security_level_bm;

	/** Bluetooth device address to use for the BLE CS ranging session. This should be the
	 *  local address that was used during the Bluetooth LE pairing procedure with the peer.
	 */
	bt_addr_t addr;
};

/** @brief Encode a descriptor of the BLE CS ranging capability into a technology-specific payload.
 *
 *  This function encodes the @ref bt_fast_pair_fhn_pf_ble_cs_ranging_capability descriptor into
 *  the technology-specific payload descriptor (@ref bt_fast_pair_fhn_pf_ranging_tech_payload)
 *  that is used in the Ranging Capability Response message. The encoded payload descriptor
 *  can then be passed to the @ref bt_fast_pair_fhn_pf_ranging_capability_response_send API.
 *
 *  The @p capability_payload parameter must have a pre-allocated
 *  @ref bt_fast_pair_fhn_pf_ranging_tech_payload.data buffer with the
 *  @ref bt_fast_pair_fhn_pf_ranging_tech_payload.data_len field set to the size of this buffer.
 *  The buffer size should be at least equal to the size of the
 *  @ref bt_fast_pair_fhn_pf_ble_cs_ranging_capability descriptor.
 *
 *  On success, the function overwrites the @ref bt_fast_pair_fhn_pf_ranging_tech_payload.data_len
 *  field with the actual encoded length of the capability bytes that were written to the
 *  @ref bt_fast_pair_fhn_pf_ranging_tech_payload.data buffer. The function also sets the
 *  @ref bt_fast_pair_fhn_pf_ranging_tech_payload.id field to
 *  @ref BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS value.
 *
 *  This function can be called from any context.
 *
 *  @param capability_desc    BLE CS ranging capability descriptor to encode.
 *  @param capability_payload Technology-specific payload to write the encoded data into.
 *                            The @ref bt_fast_pair_fhn_pf_ranging_tech_payload.data buffer and
 *                            @ref bt_fast_pair_fhn_pf_ranging_tech_payload.data_len fields must
 *                            describe the memory region that will be used to store the encoded
 *                            capability bytes.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_fhn_pf_ble_cs_ranging_capability_encode(
	const struct bt_fast_pair_fhn_pf_ble_cs_ranging_capability *capability_desc,
	struct bt_fast_pair_fhn_pf_ranging_tech_payload *capability_payload);

/** @brief Descriptor of the BLE CS technology configuration payload.
 *
 *  The descriptor is used together with the @ref bt_fast_pair_fhn_pf_ble_cs_ranging_config_decode
 *  function to decode the BLE CS technology configuration data from the technology-specific
 *  payload that is received in the Ranging Configuration message.
 */
struct bt_fast_pair_fhn_pf_ble_cs_ranging_config {
	/** BLE CS security level selected by the connected peer. */
	enum bt_fast_pair_fhn_pf_ble_cs_security_level selected_security_level;

	/** Bluetooth device address of the connected peer to use for the BLE CS ranging
	 *  session.
	 */
	bt_addr_t addr;
};

/** @brief Decode a technology-specific payload into a descriptor of the BLE CS technology ranging
 *         configuration.
 *
 *  This function decodes the technology-specific payload, described by the
 *  @ref bt_fast_pair_fhn_pf_ranging_tech_payload structure, into the
 *  @ref bt_fast_pair_fhn_pf_ble_cs_ranging_config descriptor.
 *
 *  The decoding process is successful if the @p config_payload contains valid BLE CS configuration
 *  data. For instance, the @ref bt_fast_pair_fhn_pf_ranging_tech_payload.id field must be set to
 *  @ref BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_CS.
 *
 *  This function can be called from any context.
 *
 *  @param config_payload Technology-specific payload that describes the BLE CS technology ranging
 *                        configuration.
 *  @param config_desc    BLE CS ranging configuration descriptor to write the decoded data into.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_fhn_pf_ble_cs_ranging_config_decode(
	const struct bt_fast_pair_fhn_pf_ranging_tech_payload *config_payload,
	struct bt_fast_pair_fhn_pf_ble_cs_ranging_config *config_desc);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_FAST_PAIR_FHN_PF_BLE_CS_H_ */
