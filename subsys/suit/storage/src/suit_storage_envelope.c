/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_storage_internal.h>
#include <zephyr/drivers/flash.h>
#include <suit_plat_decode_util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_storage_envelope, CONFIG_SUIT_LOG_LEVEL);

/** @brief Save the next part of the envelope.
 *
 * @details Since the SUIT envelope must be modified, to remove parts like integrated payloads,
 *          the save functionality cannot be done in a single transaction.
 *          This function allows to pass unaligned buffers and trigger only aligned transactions
 *          on the flash driver.
 *
 * @param[in]  area_addr  Address of erase-block aligned memory containing envelope.
 * @param[in]  area_size  Size of erase-block aligned memory, containing envelope.
 * @param[in]  addr       Address of a buffer (part of SUIT envelope) to append.
 * @param[in]  size       Size of a buffer (part of SUIT envelope) to append.
 * @param[in]  reset      Erase envelope storage before writing.
 * @param[in]  flush      Write all remaining, unaligned data into the envelope storage.
 *
 * @retval SUIT_PLAT_SUCCESS              if the next part of envelope successfully saved.
 * @retval SUIT_PLAT_ERR_IO               if unable to change NVM contents.
 * @retval SUIT_PLAT_ERR_HW_NOT_READY     if NVM controller is unavailable.
 * @retval SUIT_PLAT_ERR_INVAL            if the area is too small to save the provided data.
 * @retval SUIT_PLAT_ERR_INCORRECT_STATE  if the destination envelope address was changed without
 *                                        reset.
 */
static suit_plat_err_t save_envelope_partial(uint8_t *area_addr, size_t area_size, const void *addr,
					     size_t size, bool reset, bool flush)
{
	int err = 0;
	static uint8_t write_buf[SUIT_STORAGE_WRITE_SIZE];
	static uint8_t buf_fill_level;
	static size_t offset;
	static uint8_t *current_area_addr;
	static size_t envelope_offset = -1;
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	if (!device_is_ready(fdev)) {
		fdev = NULL;
		return SUIT_PLAT_ERR_HW_NOT_READY;
	}

	if (size >= (area_size - offset)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (reset) {
		buf_fill_level = 0;
		offset = 0;
		current_area_addr = area_addr;
		envelope_offset = suit_plat_mem_nvm_offset_get(area_addr);

		err = flash_erase(fdev, suit_plat_mem_nvm_offset_get(area_addr), area_size);
	}

	/* Require to set reset flag before changing envelope index. */
	if ((current_area_addr == NULL) || (area_addr != current_area_addr)) {
		LOG_ERR("It is required to set reset flag to change the write address.");
		return SUIT_PLAT_ERR_INCORRECT_STATE;
	}

	LOG_DBG("Writing %d bytes (cache fill: %d)", size, buf_fill_level);

	/* Fill the write buffer to flush non-aligned bytes from the previous call. */
	if ((err == 0) && buf_fill_level) {
		size_t len = sizeof(write_buf) - buf_fill_level;

		len = MIN(len, size);
		memcpy(&write_buf[buf_fill_level], addr, len);

		buf_fill_level += len;
		addr = ((uint8_t *)addr) + len;
		size -= len;

		/* If write buffer is full - write it into the memory. */
		if (buf_fill_level == sizeof(write_buf)) {
			LOG_DBG("Write continuous %d cache bytes (address: %p)", sizeof(write_buf),
				(void *)(envelope_offset + offset));
			err = flash_write(fdev, envelope_offset + offset, write_buf,
					  sizeof(write_buf));

			buf_fill_level = 0;
			offset += sizeof(write_buf);
		}
	}

	/* Write aligned data directly from input buffer. */
	if ((err == 0) && (size >= sizeof(write_buf))) {
		size_t write_size = ((size / sizeof(write_buf)) * sizeof(write_buf));

		LOG_DBG("Write continuous %d image bytes (address: %p)", write_size,
			(void *)(envelope_offset + offset));
		err = flash_write(fdev, envelope_offset + offset, addr, write_size);

		size -= write_size;
		offset += write_size;
		addr = ((uint8_t *)addr + write_size);
	}

	/* Store remaining bytes into the write buffer. */
	if ((err == 0) && (size > 0)) {
		LOG_DBG("Cache %d bytes (address: %p)", size, (void *)(envelope_offset + offset));

		memcpy(&write_buf[buf_fill_level], addr, size);
		buf_fill_level += size;
	}

	/* Flush buffer if requested. */
	if ((err == 0) && (flush) && (buf_fill_level > 0) && (sizeof(write_buf) > 1)) {
		/* Do not leak information about the previous requests. */
		memset(&write_buf[buf_fill_level], 0xFF, sizeof(write_buf) - buf_fill_level);

		LOG_DBG("Flush %d bytes (address: %p)", sizeof(write_buf),
			(void *)(envelope_offset + offset));
		err = flash_write(fdev, envelope_offset + offset, write_buf, sizeof(write_buf));

		buf_fill_level = 0;
		offset += sizeof(write_buf);
	}

	if (err != 0) {
		return SUIT_PLAT_ERR_IO;
	}

	return SUIT_PLAT_SUCCESS;
}

/** @brief Append the next CBOR map element with payload encoded as byte string.
 *
 * @param[in]  area_addr  Address of erase-block aligned memory containing envelope.
 * @param[in]  area_size  Size of erase-block aligned memory, containing envelope.
 * @param[in]  key        The CBOR map key value.
 * @param[in]  bstr       The byte string to append.
 * @param[in]  flush      Write all remaining, unaligned data into the envelope storage.
 *
 * @retval SUIT_PLAT_SUCCESS              if the CBOR map element successfully encoded.
 * @retval SUIT_PLAT_ERR_INVAL            if invalid input argument was provided
 *                                        (i.e. NULL, buffer length).
 * @retval SUIT_PLAT_ERR_IO               if unable to change NVM contents.
 * @retval SUIT_PLAT_ERR_HW_NOT_READY     if NVM controller is unavailable.
 * @retval SUIT_PLAT_ERR_INCORRECT_STATE  if the destination envelope address was changed without
 *                                        reset.
 */
static suit_plat_err_t save_envelope_partial_bstr(uint8_t *area_addr, size_t area_size,
						  uint16_t key, struct zcbor_string *bstr,
						  bool flush)
{
	uint8_t kv_hdr_buf[SUIT_STORAGE_ENCODED_BSTR_HDR_LEN_MAX];
	size_t hdr_len = sizeof(kv_hdr_buf);

	suit_plat_err_t err =
		suit_storage_encode_bstr_kv_header(key, bstr->len, kv_hdr_buf, &hdr_len);
	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}

	err = save_envelope_partial(area_addr, area_size, kv_hdr_buf, hdr_len, false, false);
	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}

	return save_envelope_partial(area_addr, area_size, bstr->value, bstr->len, false, flush);
}

suit_plat_err_t suit_storage_envelope_get(const uint8_t *area_addr, size_t area_size,
					  const suit_manifest_class_id_t *id, const uint8_t **addr,
					  size_t *size)
{
	struct SUIT_Envelope_severed envelope;
	suit_envelope_hdr_t envelope_hdr;

	suit_plat_err_t err =
		suit_storage_decode_envelope_header(area_addr, area_size, &envelope_hdr);
	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}

	*addr = (void *)(envelope_hdr.envelope.mem);

	/* Validate stored data by parsing buffer as SUIT envelope.
	 * Set the size variable value to the decoded envelope size,
	 * that does not contain severable elements.
	 */
	err = suit_storage_decode_suit_envelope_severed(
		envelope_hdr.envelope.mem, envelope_hdr.envelope.size, &envelope, size);

	return err;
}

suit_plat_err_t suit_storage_envelope_install(uint8_t *area_addr, size_t area_size,
					      const suit_manifest_class_id_t *id,
					      const uint8_t *addr, size_t size)
{
	uint8_t envelope_hdr_buf[SUIT_STORAGE_ENCODED_ENVELOPE_HDR_LEN_MAX];
	size_t envelope_hdr_buf_len = sizeof(envelope_hdr_buf);
	suit_envelope_hdr_t envelope_hdr;
	suit_manifest_class_id_t *class_id;
	struct SUIT_Envelope_severed envelope;
	suit_plat_err_t err;

	if ((addr == NULL) || (size == 0)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	/* Validate input data by parsing buffer as SUIT envelope.
	 * Additionally change the size variable value from envelope size to
	 * the decoded envelope size, that does not contain severable elements.
	 */
	err = suit_storage_decode_suit_envelope_severed(addr, size, &envelope, &size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to install envelope: decode failed (%d)", err);
		return err;
	}

	if (suit_plat_decode_manifest_class_id(&envelope.suit_manifest_component_id, &class_id) !=
	    SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to install envelope: class ID not decoded");
		return SUIT_PLAT_ERR_INVAL;
	}

	if (SUIT_PLAT_SUCCESS != suit_metadata_uuid_compare(id, class_id)) {
		LOG_ERR("Unable to install envelope: class ID does not match");
		return SUIT_PLAT_ERR_INVAL;
	}

	size_t class_id_offset = (size_t)class_id - (size_t)envelope.suit_manifest.value +
				 envelope.suit_authentication_wrapper.len;
	/* Move offset and size by:
	 * - SUIT envelope tag
	 * - Authentication wrapper bstr header
	 * - Manifest bstr header
	 */
	size_t encoding_overhead = SUIT_STORAGE_ENCODED_ENVELOPE_TAG_LEN;
	size_t header_len = 0;

	err = suit_storage_bstr_kv_header_len(SUIT_AUTHENTICATION_WRAPPER_TAG,
					      envelope.suit_authentication_wrapper.len,
					      &header_len);
	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}
	encoding_overhead += header_len;

	err = suit_storage_bstr_kv_header_len(SUIT_MANIFEST_TAG, envelope.suit_manifest.len,
					      &header_len);

	if (err != SUIT_PLAT_SUCCESS) {
		return err;
	}
	encoding_overhead += header_len;

	envelope_hdr.class_id_offset = encoding_overhead + class_id_offset;
	envelope_hdr.envelope.mem = addr;
	envelope_hdr.envelope.size = encoding_overhead + envelope.suit_manifest.len +
				     envelope.suit_authentication_wrapper.len;

	if (envelope_hdr_buf_len + envelope_hdr.envelope.size > area_size) {
		LOG_ERR("Unable to install envelope: envelope too big (%d)",
			envelope_hdr.envelope.size);
		return SUIT_PLAT_ERR_INVAL;
	}

	err = suit_storage_encode_envelope_header(&envelope_hdr, envelope_hdr_buf,
						  &envelope_hdr_buf_len);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to encode envelope header (%d)", err);
		return err;
	}

	err = save_envelope_partial(area_addr, area_size, envelope_hdr_buf, envelope_hdr_buf_len,
				    true, false);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to save envelope header (%d)", err);
		return err;
	}

	err = save_envelope_partial_bstr(area_addr, area_size, SUIT_AUTHENTICATION_WRAPPER_TAG,
					 &envelope.suit_authentication_wrapper, false);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to save authentication wrapper (%d)", err);
		return err;
	}

	err = save_envelope_partial_bstr(area_addr, area_size, SUIT_MANIFEST_TAG,
					 &envelope.suit_manifest, true);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to save manifest (%d)", err);
		return err;
	}

	LOG_INF("Envelope saved.");

	return err;
}
