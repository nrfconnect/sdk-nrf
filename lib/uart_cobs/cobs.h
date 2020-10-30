/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef COBS_H__
#define COBS_H__

#include <stddef.h>
#include <errno.h>

#include <zephyr/types.h>

/**
 * @file cobs.h
 * @defgroup cobs COBS encoder/decoder.
 * @{
 * @brief API for COBS encoding and decoding.
 */

/** COBS frame delimiter value. */
#define COBS_DELIMITER          0x00
/** Number of overhead bytes in a COBS frame. */
#define COBS_OVERHEAD_BYTES     2
/** Maximum encoded length of a COBS frame. */
#define COBS_MAX_BYTES          255
/** Maximum decoded length of a COBS frame. */
#define COBS_MAX_DATA_BYTES     (COBS_MAX_BYTES - COBS_OVERHEAD_BYTES)

/**
 * @brief Get encoded length of a COBS frame.
 * @param[in]	_sz	Decoded length of frame.
 */
#define COBS_ENCODED_SIZE(_sz)  ((_sz) + COBS_OVERHEAD_BYTES)


/** COBS in-place encoding buffer. */
struct cobs_enc_buf {
	/** Header of COBS frame */
	uint8_t hdr;
	/** COBS encoded data and delimiter */
	uint8_t buf[COBS_MAX_BYTES - 1];
} __packed;

BUILD_ASSERT(sizeof(struct cobs_enc_buf) == COBS_MAX_BYTES,
	     "COBS buffer should be packed.");

/** COBS decoder structure. */
struct cobs_dec {
	/** Output data buffer. */
	uint8_t *buf;
	/** Current index into output data buffer. */
	size_t idx;
	/** Counter of bytes until next delimiter. */
	size_t counter;
	/** In a partially decoded frame. */
	bool in_frame;
};


/**
 * @brief COBS-encode a buffer in-place.
 * @param[in,out] buf Buffer to encode.
 * @param[in] len Length of data buffer in @ref buf
 * @retval 0 If successful.
 * @retval -ENOMEM If len is greater than maximum allowed length.
 */
static inline int cobs_encode(struct cobs_enc_buf *buf, size_t len)
{
	if (len > COBS_MAX_BYTES) {
		/* Total length of encoded frame is greater than
		 * maximum allowed size.
		 */
		return -ENOMEM;
	}

	uint8_t *ptr = &buf->hdr;

	for (size_t i = 0; i < len; i++) {
		if (buf->buf[i] == COBS_DELIMITER) {
			*ptr = &buf->buf[i] - ptr;
			ptr += *ptr;
		}
	}
	*ptr = &buf->buf[len] - ptr;
	buf->buf[len] = COBS_DELIMITER;

	return 0;
}

/**
 * @brief Reset COBS decoder to initial state.
 * @param[in,out] dec Decoder to reset.
 */
static inline void cobs_dec_reset(struct cobs_dec *dec)
{
	dec->idx = 0;
	dec->counter = 0;
	dec->in_frame = false;
}

/**
 * @brief Initialize COBS decoder.
 * @param[in,out] dec Decoder to initialize.
 * @param[in] buf Output buffer to initialize decoder with.
 */
static inline void cobs_dec_init(struct cobs_dec *dec, uint8_t *buf)
{
	dec->buf = buf;
	cobs_dec_reset(dec);
}

/**
 * @brief Perform a single step of COBS-decoding.
 * @param[in,out] dec Decoder to decode with.
 * @param[in] byte Byte to feed into decoder.
 *
 * @return Length of the decoded data if complete, negative value otherwise.
 * @retval -EINPROGRESS	If decoding is not yet finished.
 * @retval -EMSGSIZE    If decoded data length exceeds output buffer size.
 * @retval -ENOTSUP     If an invalid frame was encountered while decoding.
 */
static inline int cobs_decode_step(struct cobs_dec *dec, uint8_t byte)
{
	if (!dec->in_frame) {
		if (byte == COBS_DELIMITER) {
			return -ENOTSUP;
		}
		dec->idx = 0;
		dec->counter = byte;
		dec->in_frame = true;
	} else {
		dec->counter--;
		if (byte == COBS_DELIMITER) {
			dec->in_frame = false;
			return !dec->counter ? dec->idx : -ENOTSUP;
		} else if (dec->idx >= COBS_MAX_DATA_BYTES) {
			dec->in_frame = false;
			return -EMSGSIZE;
		} else if (!dec->counter) {
			dec->counter = byte;
			byte = COBS_DELIMITER;
		}
		dec->buf[dec->idx++] = byte;
	}
	return -EINPROGRESS;
}

/**
 * @brief Check if COBS decoder is in a partially complete frame.
 * @param[in] dec Decoder to check.
 * @returns true if @p dec is in a frame, false otherwise.
 */
static inline bool cobs_dec_in_frame(struct cobs_dec *dec)
{
	return dec->in_frame;
}

/**
 * Get the length of the COBS decoder's decoded data.
 * @param[in] dec Decoder to check.
 * @returns Length of the decoded data in @p dec.
 */
static inline size_t cobs_dec_current_len(struct cobs_dec *dec)
{
	return dec->idx;
}

/** @} */

#endif  /* COBS_H__ */
