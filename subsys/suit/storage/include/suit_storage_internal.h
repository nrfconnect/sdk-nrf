/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Internal SUIT storage submodules, types and constants.
 *
 * @details The SUIT storage module has been divided into several submodules, depending on the type
 *          of data they modify:
 *           - Manifest Provisioning Information submodule (exposed through dedicated header file)
 *           - Envelope submodule, responsible for parsing and installing envelopes.
 *           - Update candidate submodule, responsible for parsing and storing memory regions,
 *             containing update candidate envelope and caches.
 *           - Non-volatile variables, responsible to parsing and modifying persistent variables,
 *             accessible by the OEM manifests.
 */

#ifndef SUIT_STORAGE_INTERNAL_H__
#define SUIT_STORAGE_INTERNAL_H__

#include <suit_storage.h>
#include <zcbor_common.h>
#include <suit_plat_mem_util.h>
#include <suit_storage_mpi.h>
#include <suit_storage_report_internal.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SUIT_STORAGE_NVM_NODE                                                                      \
	COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(secdom_nvs)), (DT_NODELABEL(secdom_nvs)),          \
		    (DT_CHOSEN(zephyr_flash)))
#define SUIT_STORAGE_WRITE_SIZE DT_PROP(SUIT_STORAGE_NVM_NODE, write_block_size)
#define SUIT_STORAGE_EB_SIZE	DT_PROP(SUIT_STORAGE_NVM_NODE, erase_block_size)
#define SUIT_STORAGE_ACCESS_BLOCK_SIZE                                                             \
	(COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(mpc110)),                                         \
		     (COND_CODE_1(DT_NODE_HAS_PROP(DT_NODELABEL(mpc110), override_granularity),    \
				  (DT_PROP(DT_NODELABEL(mpc110), override_granularity)),           \
				  (SUIT_STORAGE_EB_SIZE))),                                        \
		     (SUIT_STORAGE_EB_SIZE)))

#define CEIL_DIV(a, b)	  ((((a)-1) / (b)) + 1)
#define EB_ALIGN(size)	  (CEIL_DIV(size, SUIT_STORAGE_EB_SIZE) * SUIT_STORAGE_EB_SIZE)
#define WRITE_ALIGN(size) (CEIL_DIV(size, SUIT_STORAGE_WRITE_SIZE) * SUIT_STORAGE_WRITE_SIZE)
#define EB_SIZE(type)	  (EB_ALIGN(sizeof(type)))
#define ACCESS_SIZE(type)                                                                          \
	(CEIL_DIV(sizeof(type), SUIT_STORAGE_ACCESS_BLOCK_SIZE) * SUIT_STORAGE_ACCESS_BLOCK_SIZE)

/* Values defined in the SUIT manifest specification. */
#define SUIT_AUTHENTICATION_WRAPPER_TAG 2
#define SUIT_MANIFEST_TAG		3
#define SUIT_MANIFEST_COMPONENT_ID_TAG	5

/* The maximum length of the encoded CBOR key-value pair header with integer key and bstr value. */
#define SUIT_STORAGE_ENCODED_BSTR_HDR_LEN_MAX 6

/* The maximum length of the encoded SUIT storage header with envelope header. */
#define SUIT_STORAGE_ENCODED_ENVELOPE_HDR_LEN_MAX                                                  \
	(1 /* CBOR MAP */ + 4 /* CBOR keys (3 bytes) and version (1 byte) */ +                     \
	 3 /* class ID offset */ + 3 /* SUIT envelope length */ + 3 /* SUIT envelope tag */)

/* The length of CBOR tag and SUIT envelope header. */
#define SUIT_STORAGE_ENCODED_ENVELOPE_TAG_LEN 3

/* Decoded SUIT storage envelope slot. */
typedef struct {
	suit_plat_mreg_t envelope;
	size_t class_id_offset;
} suit_envelope_hdr_t;

/* Decoded severed SUIT envelope with the manifest component ID field.
 *
 * This representation is used to construct the severed SUIT envelope as well as
 * additional fields, stored inside SUIT storage envelope slot map.
 */
struct SUIT_Envelope_severed {
	struct zcbor_string suit_authentication_wrapper;
	struct zcbor_string suit_manifest;
	struct zcbor_string suit_manifest_component_id;
};

/* Update candidate metadata. */
struct update_candidate_info {
	uint32_t update_magic_value;
	size_t update_regions_len;
	suit_plat_mreg_t update_regions[CONFIG_SUIT_STORAGE_N_UPDATE_REGIONS];
};

/** @defgroup suit_storage_encode Internal CBOR structure encoding and decoding
 *  @{
 */

/** @brief Calculate the value of encoded key-value CBOR map header.
 *
 * @details All values (key, length) are limited to 0xFFFF (16-bit).
 *
 * @param[in]     key       CBOR map key value.
 * @param[in]     bstr_len  CBOR bstr length to be encoded inside the header.
 * @param[out]    p_len     Header length
 *
 * @retval SUIT_PLAT_SUCCESS    if the length was successfully calculated.
 * @retval SUIT_PLAT_ERR_INVAL  if invalid input argument was provided (i.e. NULL).
 */
suit_plat_err_t suit_storage_bstr_kv_header_len(uint32_t key, size_t bstr_len, size_t *p_len);

/** @brief Encode key-value CBOR map header.
 *
 * @details All values (key, length) are limited to 0xFFFF (16-bit).
 *
 * @param[in]     key       CBOR map key value.
 * @param[in]     bstr_len  CBOR bstr length to be encoded inside the header.
 * @param[out]    buf       Buffer to fill.
 * @param[inout]  len       As input - length of the buffer,
 *                          as output - number of bytes consumed.
 *
 * @retval SUIT_PLAT_SUCCESS    if the header was successfully encoded.
 * @retval SUIT_PLAT_ERR_INVAL  if invalid input argument was provided (i.e. NULL, buffer length).
 */
suit_plat_err_t suit_storage_encode_bstr_kv_header(uint32_t key, size_t bstr_len, uint8_t *buf,
						   size_t *len);

/** @brief Decode SUIT storage entry.
 *
 * @param[in]   buf       Buffer to prase.
 * @param[in]   len       Length of the buffer.
 * @param[out]  envelope  Decoded entry.
 *
 * @retval SUIT_PLAT_SUCCESS            if the entry was successfully decoded.
 * @retval SUIT_PLAT_ERR_CBOR_DECODING  if failed to decode entry.
 */
suit_plat_err_t suit_storage_decode_envelope_header(const uint8_t *buf, size_t len,
						    suit_envelope_hdr_t *envelope);

/** @brief Encode SUIT storage entry with SUIT envelope header.
 *
 * @param[in]     envelope  Envelope header to encode.
 * @param[out]    buf       Buffer to fill.
 * @param[inout]  len       As input - length of the buffer,
 *                          as output - number of bytes consumed.
 *
 * @retval SUIT_PLAT_SUCCESS            if the entry was successfully encoded.
 * @retval SUIT_PLAT_ERR_CBOR_DECODING  if failed to encode entry.
 */
suit_plat_err_t suit_storage_encode_envelope_header(suit_envelope_hdr_t *envelope, uint8_t *buf,
						    size_t *len);

/** @brief Decode SUIT envelope.
 *
 * @param[in]   buf           Buffer to prase.
 * @param[in]   len           Length of the buffer.
 * @param[out]  envelope      Decoded envelope.
 * @param[out]  envelope_len  Decoded envelope length.
 *
 * @retval SUIT_PLAT_SUCCESS            if the envelope was successfully decoded.
 * @retval SUIT_PLAT_ERR_CBOR_DECODING  if failed to decode envelope.
 */
suit_plat_err_t suit_storage_decode_suit_envelope_severed(const uint8_t *buf, size_t len,
							  struct SUIT_Envelope_severed *envelope,
							  size_t *envelope_len);

/** @} */

/** @defgroup suit_storage_update_candidate Update candidate submodule
 *  @{
 */

/**
 * @brief Initialize the SUIT storage submodule, responsible for storing/returning
 *        update candidate info.
 *
 * @retval SUIT_PLAT_SUCCESS           if module is successfully initialized.
 * @retval SUIT_PLAT_ERR_HW_NOT_READY  if NVM controller is unavailable.
 */
suit_plat_err_t suit_storage_update_init(void);

/**
 * @brief Get the memory regions, containing update candidate.
 *
 * @param[in]   addr     Address of erase-block aligned memory containing update candidate info.
 * @param[in]   size     Size of erase-block aligned memory, containing update candidate info.
 * @param[out]  regions  List of update candidate memory regions (envelope, caches).
 *                       By convention, the first region holds the SUIT envelope.
 * @param[out]  len      Length of the memory regions list.
 *
 * @retval SUIT_PLAT_SUCCESS        if pointer to the update candidate info successfully returned.
 * @retval SUIT_PLAT_ERR_INVAL      if one of the input arguments is invalid (i.e. NULL).
 * @retval SUIT_PLAT_ERR_SIZE       if update candidate area has incorrect size.
 * @retval SUIT_PLAT_ERR_NOT_FOUND  if update candidate is not set.
 */
suit_plat_err_t suit_storage_update_get(const uint8_t *addr, size_t size,
					const suit_plat_mreg_t **regions, size_t *len);

/**
 * @brief Save the information about update candidate.
 *
 * @param[in]  addr     Address of erase-block aligned memory containing update candidate info.
 * @param[in]  size     Size of erase-block aligned memory, containing update candidate info.
 * @param[in]  regions  List of update candidate memory regions (envelope, caches).
 *                      By convention, the first region holds the SUIT envelope.
 * @param[in]  len      Length of the memory regions list.
 *
 * @retval SUIT_PLAT_SUCCESS           if update candidate info successfully saved.
 * @retval SUIT_PLAT_ERR_INVAL         if one of the input arguments is invalid (i.e. NULL).
 * @retval SUIT_PLAT_ERR_SIZE          if update candidate area has incorrect size or the number
 *                                     of update regions is too big.
 * @retval SUIT_PLAT_ERR_HW_NOT_READY  if NVM controller is unavailable.
 * @retval SUIT_PLAT_ERR_IO            if unable to change NVM contents.
 */
suit_plat_err_t suit_storage_update_set(uint8_t *addr, size_t size, const suit_plat_mreg_t *regions,
					size_t len);

/** @} */

/** @defgroup suit_storage_envelope Envelope submodule
 *  @{
 */

/**
 * @brief Get the address and size of the envelope, stored inside the SUIT partition.
 *
 * @param[in]   area_addr  Address of erase-block aligned memory containing envelope.
 * @param[in]   area_size  Size of erase-block aligned memory, containing envelope.
 * @param[in]   id         Class ID of the manifest inside the envelope.
 * @param[out]  addr       SUIT envelope address.
 * @param[out]  size       SUIT envelope size.
 *
 * @retval SUIT_PLAT_SUCCESS            if the envelope was successfully returned.
 * @retval SUIT_PLAT_ERR_CBOR_DECODING  if failed to decode envelope.
 */
suit_plat_err_t suit_storage_envelope_get(const uint8_t *area_addr, size_t area_size,
					  const suit_manifest_class_id_t *id, const uint8_t **addr,
					  size_t *size);

/**
 * @brief Install the authentication block and manifest of the envelope inside the SUIT storage.
 *
 * @note This API removes all severable elements of the SUIT envelope, such as integrated
 *       payloads, text fields, etc.
 *
 * @param[in]  area_addr  Address of erase-block aligned memory containing envelope.
 * @param[in]  area_size  Size of erase-block aligned memory, containing envelope.
 * @param[in]  id         Class ID of the manifest inside the envelope.
 * @param[in]  addr       SUIT envelope address.
 * @param[in]  size       SUIT envelope size.
 *
 * @retval SUIT_PLAT_SUCCESS              if the envelope was successfully insatlled.
 * @retval SUIT_PLAT_ERR_CBOR_DECODING    if failed to decode input or encode severed envelope.
 * @retval SUIT_PLAT_ERR_INVAL            if one of the input arguments is invalid
 *                                        (i.e. NULL, buffer length, incorrect class ID).
 * @retval SUIT_PLAT_ERR_IO               if unable to change NVM contents.
 * @retval SUIT_PLAT_ERR_HW_NOT_READY     if NVM controller is unavailable.
 * @retval SUIT_PLAT_ERR_INCORRECT_STATE  if the previous installation was unexpectedly aborted.
 */
suit_plat_err_t suit_storage_envelope_install(uint8_t *area_addr, size_t area_size,
					      const suit_manifest_class_id_t *id,
					      const uint8_t *addr, size_t size);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* SUIT_STORAGE_INTERNAL_H__ */
