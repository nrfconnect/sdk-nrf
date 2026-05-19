/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DULT_BT_H_
#define _DULT_BT_H_

#include <stdint.h>
#include <stddef.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <dult/dult.h>

/**
 * @defgroup dult_bt Detecting Unwanted Location Trackers - Bluetooth
 * @brief Detecting Unwanted Location Trackers specification implementation Bluetooth API
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum length in bytes of the proprietary data field in the DULT AD when the surrounding AD
 *  payload includes the optional Flags TLV (default, spec-compliant case).
 *
 *  The DULT specification reserves bytes 15-36 of the Location-Enabled
 *  Payload (22 bytes) for the proprietary company payload when the Flags
 *  TLV is present at bytes 6-8.
 */
#define DULT_BT_ADV_PROPRIETARY_DATA_MAX_LEN_WITH_FLAGS	(22)

/** Maximum length in bytes of the proprietary data field in the DULT AD when the surrounding AD
 *  payload omits the optional Flags TLV.
 *
 *  When no Flags TLV is emitted, the 3 bytes at positions 6-8 of the
 *  Location-Enabled Payload can be reused by the proprietary payload,
 *  which raises the practical limit to 25 bytes. Using this extended limit
 *  is only safe when the caller can guarantee that no Flags TLV is also
 *  emitted in the surrounding AD payload. Select it through the
 *  @ref dult_bt_adv_data.flags_present field.
 *
 *  @note This 25-byte limit is an extension, not the DULT spec's 22-byte
 *        proprietary field (payload bytes 15-36).
 */
#define DULT_BT_ADV_PROPRIETARY_DATA_MAX_LEN_NO_FLAGS	(25)

/** Encoding descriptor for the DULT Bluetooth advertising data (AD). */
struct dult_bt_adv_data {
	/** Whether the surrounding AD payload includes the optional Flags TLV.
	 *
	 *  Selects the proprietary-data length limit enforced by
	 *  @ref dult_bt_adv_data_fill: @ref
	 *  DULT_BT_ADV_PROPRIETARY_DATA_MAX_LEN_WITH_FLAGS when true, or
	 *  @ref DULT_BT_ADV_PROPRIETARY_DATA_MAX_LEN_NO_FLAGS when false.
	 */
	bool flags_present;

	/** Network-specific proprietary data. */
	struct {
		/** Network-specific proprietary data buffer. */
		uint8_t *buf;

		/** Length of the network-specific proprietary data buffer.
		 *
		 *  The maximum length depends on @ref flags_present and is either
		 *  @ref DULT_BT_ADV_PROPRIETARY_DATA_MAX_LEN_WITH_FLAGS or
		 *  @ref DULT_BT_ADV_PROPRIETARY_DATA_MAX_LEN_NO_FLAGS.
		 */
		size_t len;
	} proprietary;
};

/** @brief Initialize the DULT Bluetooth advertising data structure.
 *
 *  @param[in] _flags_present	Whether the surrounding AD payload includes the
 *				optional Flags TLV.
 *  @param[in] _proprietary_buf	Network-specific proprietary data buffer.
 *  @param[in] _proprietary_len	Length of the network-specific proprietary data buffer.
 *
 *  @return Initialized DULT Bluetooth advertising data structure.
 */
#define DULT_BT_ADV_DATA_INIT(_flags_present, _proprietary_buf, _proprietary_len)	\
	{										\
		.flags_present = (_flags_present),					\
		.proprietary = {							\
			.buf = (_proprietary_buf),					\
			.len = (_proprietary_len),					\
		},									\
	}

/** @brief Initialize the DULT Bluetooth advertising data structure with proprietary data.
 *
 *  Convenience initializer for advertising flows that appends proprietary data.
 *
 *  @param[in] _flags_present	Whether the surrounding AD payload includes the
 *				optional Flags TLV.
 *  @param[in] _proprietary_buf	Network-specific proprietary data buffer.
 *				Must be an array.
 *
 *  @return Initialized DULT Bluetooth advertising data structure.
 */
#define DULT_BT_ADV_DATA_PROPRIETARY_INIT(_flags_present, _proprietary_buf)	\
	DULT_BT_ADV_DATA_INIT(_flags_present, _proprietary_buf, ARRAY_SIZE(_proprietary_buf))

/** @brief Initialize the DULT Bluetooth advertising data structure
 *         without proprietary data.
 *
 *  Convenience initializer for advertising flows that do not append proprietary data.
 *
 *  @return Initialized DULT Bluetooth advertising data structure.
 */
#define DULT_BT_ADV_DATA_NO_PROPRIETARY_INIT			\
	DULT_BT_ADV_DATA_INIT(false, NULL, 0)

/** @brief Encode the DULT location-enabled Bluetooth advertising payload.
 *
 *  This function can only be called if the DULT user was previously registered with the
 *  @ref dult_user_register API.
 *
 *  Serializes the DULT data (UUID, network ID, near-owner byte, and optional
 *  proprietary data from @p adv_data) into @p buf and populates @p bt_adv_data to
 *  reference it. The caller must ensure that @p buf remains valid for as long as
 *  @p bt_adv_data is in use.
 *
 *  @param[in]  user		Pointer to the DULT user structure.
 *  @param[out] bt_adv_data	Pointer to the Bluetooth advertising data structure to populate.
 *  @param[out] buf		Backing buffer for the serialized payload. It must be large
 *				enough to hold the fixed DULT header followed by
 *				@p adv_data->proprietary.len bytes; otherwise the function
 *				returns ``-EINVAL``.
 *  @param[in]  buf_size	Size of @p buf in bytes.
 *  @param[in]  adv_data	Advertising data to encode.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_bt_adv_data_fill(const struct dult_user *user, struct bt_data *bt_adv_data,
			  uint8_t *buf, size_t buf_size, const struct dult_bt_adv_data *adv_data);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _DULT_BT_H_ */
