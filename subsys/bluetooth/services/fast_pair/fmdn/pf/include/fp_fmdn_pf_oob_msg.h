/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_FMDN_PF_OOB_MSG_H_
#define _FP_FMDN_PF_OOB_MSG_H_

#include <zephyr/net_buf.h>
#include <zephyr/types.h>

#include <bluetooth/services/fast_pair/fhn/pf/pf.h>

/**
 * @defgroup fp_fmdn_pf_oob_msg OOB message encoders and decoders for Android ranging
 * @brief Internal API for encoding and decoding out-of-band (OOB) messages defined in
 *        the Android Ranging specification (used in the Fast Pair FMDN Precision Finding
 *        module): https://source.android.com/docs/core/connect/ranging-oob-spec
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum number of ranging technology payloads in a single message. */
#define FP_FMDN_PF_OOB_MSG_RANGING_TECH_PAYLOAD_MAX_NUM (4U)

/** ID of the out-of-band message (OOB) that is described by the @ref fp_fmdn_pf_oob_msg
 *  structure.
 */
enum fp_fmdn_pf_oob_msg_id {
	/* ID of the Ranging Capability Request message. */
	FP_FMDN_PF_OOB_MSG_ID_RANGING_CAPABILITY_REQ = 0x00,

	/* ID of the Ranging Capability Response message. */
	FP_FMDN_PF_OOB_MSG_ID_RANGING_CAPABILITY_RSP = 0x01,

	/* ID of the Ranging Configuration message. */
	FP_FMDN_PF_OOB_MSG_ID_RANGING_CONFIG_REQ = 0x02,

	/* ID of the Ranging Configuration Response message. */
	FP_FMDN_PF_OOB_MSG_ID_RANGING_CONFIG_RSP = 0x03,

	/* ID of the Stop Ranging message. */
	FP_FMDN_PF_OOB_MSG_ID_STOP_RANGING_REQ = 0x06,

	/* ID of the Stop Ranging Response message. */
	FP_FMDN_PF_OOB_MSG_ID_STOP_RANGING_RSP = 0x07,
};

/** Structure representing the out-of-band (OOB) message that respects the format
 *  from the Android Ranging specification.
 */
struct fp_fmdn_pf_oob_msg {
	/* The version from the message header. */
	uint8_t version;

	/* The message ID from the message header. */
	enum fp_fmdn_pf_oob_msg_id id;

	/* Message payload. */
	union {
		/* Payload of the Ranging Capability Request message. */
		struct {
			/* Bitmask of requested ranging technologies. */
			uint16_t ranging_tech_bm;
		} ranging_capability_req;

		/* Payload of the Ranging Capability Response message. */
		struct {
			/* Bitmask of supported ranging technologies. */
			uint16_t ranging_tech_bm;

			/* Array of ranging technology capability payloads. */
			struct bt_fast_pair_fhn_pf_ranging_tech_payload *capability_payloads;

			/* Number of ranging technology capability payloads in the array. */
			uint8_t capability_payload_num;
		} ranging_capability_rsp;

		/* Payload of the Ranging Configuration message. */
		struct {
			/* Bitmask of ranging technologies to set up. */
			uint16_t ranging_tech_bm;

			/* Array of ranging technology configuration payloads. */
			struct bt_fast_pair_fhn_pf_ranging_tech_payload *config_payloads;

			/* Number of ranging technology configuration payloads in the array. */
			uint8_t config_payload_num;
		} ranging_config_req;

		/* Payload of the Ranging Configuration Response message. */
		struct {
			/* Bitmask of ranging technologies that are successfully set up. */
			uint16_t ranging_tech_bm;
		} ranging_config_rsp;

		/* Payload of the Stop Ranging message. */
		struct {
			/* Bitmask of ranging technologies that must stop ranging. */
			uint16_t ranging_tech_bm;
		} stop_ranging_req;

		/* Payload of the Stop Ranging Response message. */
		struct {
			/* Bitmask of ranging technologies that stopped ranging successfully. */
			uint16_t ranging_tech_bm;
		} stop_ranging_rsp;
	};
};

/** Decode the provided out-of-band (OOB) message that respects the format from the
 *  Android Ranging specification. This function can only decode request message types
 *  message types that originate from the Initiator device.
 *
 * @param[in]  msg_buf  Network buffer representing the OOB message as byte array.
 * @param[out] msg_desc The OOB message descriptor
 *
 * @retval 0       If the operation was successful.
 * @retval -EINVAL If the message payload has incorrect format or uses invalid values
 *                 in the specified fields.
 */
int fp_fmdn_pf_oob_msg_decode(struct net_buf_simple *msg_buf,
			      struct fp_fmdn_pf_oob_msg *msg_desc);


/** Decode the provided out-of-band (OOB) message that respects the format from the
 *  Android Ranging specification. This function can only encode response message
 *  types that originate from the Responder device.
 *
 * @param[in]  msg_desc The OOB message descriptor
 * @param[out] msg_buf  Network buffer representing the OOB message as byte array.
 *
 * @retval 0       If the operation was successful.
 * @retval -ENOMEM If the provided network buffer is too small.
 */
int fp_fmdn_pf_oob_msg_encode(const struct fp_fmdn_pf_oob_msg *msg_desc,
			      struct net_buf_simple *msg_buf);

/** Get the message byte length based on the provided message identifier that is
 *  calculated for the message variant with the maximum payload length. The value
 *  can be used to define the output buffer for the @ref fp_fmdn_pf_oob_msg_encode
 *  function. This function can only calculate length for response message types that
 *  originate from the Responder device.
 *
 * @param[in] msg_id The OOB message ID
 *
 * @return The minimum message byte length required for encoding the message variant
 *         with the maximum payload length.
 */
uint8_t fp_fmdn_pf_oob_msg_max_len_get(enum fp_fmdn_pf_oob_msg_id msg_id);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_FMDN_PF_OOB_MSG_H_ */
