/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_NDEF_CH_H_
#define NFC_NDEF_CH_H_

/**@file
 *
 * @defgroup nfc_ndef_ch Connection Handover records
 * @{
 *
 * @brief Generation of NFC NDEF Connection Handover records for NDEF messages.
 *
 */

#include <stdint.h>
#include <string.h>
#include <zephyr/types.h>
#include <nfc/ndef/record.h>
#include <nfc/ndef/msg.h>
#include <nfc/ndef/payload_type_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Size of the type field of the Connection Handover records.
 * Record types are defined in the file @c ch_rec.c.
 */
#define NFC_NDEF_CH_REC_TYPE_LENGTH 2

/**
 * @brief Handover Carrier record payload descriptor.
 */
struct nfc_ndef_ch_hc_rec {
	/** Carrier Type Format, it has the same semantics like TNF.
	 *  Supported TNF:
	 *    - NFC Forum Well Known Type
	 *    - Media Type
	 *    - Absolute URI
	 *    - NFC Forum external type
	 */
	enum nfc_ndef_record_tnf ctf;

	/** Carrier description. */
	struct {
		/** Carrier type field length. */
		uint8_t type_len;

		/** A unique identification of the alternative carrier.
		 *  It shall follow the structure, encoding, and format
		 *  implied by the value of the CTF field.
		 */
		const uint8_t *type;

		/** Optional data length. */
		uint8_t data_len;

		/** Optional information about alternative carrier.
		 *  It length can be calculated using following formula:
		 *  (NDEF Record Payload length - carrier type length) - 2
		 */
		uint8_t *data;
	} carrier;
};

/**
 * @brief Carrier Power State.
 *
 * Possible Carrier Power State field values in an Alternative Carrier record.
 */
enum nfc_ndef_ch_ac_rec_cps {
	/** Alternative Carrier inactive. */
	NFC_AC_CPS_INACTIVE     = 0x00,

	/** Alternative Carrier active. */
	NFC_AC_CPS_ACTIVE       = 0x01,

	/** Alternative Carrier activating. */
	NFC_AC_CPS_ACTIVATING   = 0x02,

	/** Alternative Carrier power status unknown. */
	NFC_AC_CPS_UNKNOWN      = 0x03
};

/** Mask of Carrier Power State bits in a first ac record byte. */
#define NFC_NDEF_CH_AC_CPS_MASK (NFC_AC_CPS_UNKNOWN)

/**
 * @brief Carrier Data Reference and Auxiliary Data Reference descriptor.
 */
struct nfc_ndef_ch_ac_rec_ref {
	/** Length of the data field. */
	uint8_t length;

	/** Pointer to the Data Reference characters.
	 *  Not relevant if length is 0.
	 */
	const uint8_t *data;
};

/**
 * @brief Alternative Carrier record payload descriptor.
 */
struct nfc_ndef_ch_ac_rec {
	/** Carrier Power State value. */
	enum nfc_ndef_ch_ac_rec_cps cps;

	/** Carrier Data Reference. */
	struct nfc_ndef_ch_ac_rec_ref carrier_data_ref;

	/** Maximum number of Auxiliary Data Reference fields. */
	const uint8_t max_aux_data_ref_cnt;

	/** Number of Auxiliary Data Reference fields. */
	uint8_t aux_data_ref_cnt;

	/** Pointer to the Auxiliary Data Reference fields. */
	struct nfc_ndef_ch_ac_rec_ref *aux_data_ref;
};

/**
 * @brief Collision Resolution record payload descriptor.
 */
struct nfc_ndef_ch_cr_rec {
	/** Randomly generated number. */
	uint16_t random;
};

/**
 * @brief Handover Select record payload descriptor.
 */
struct nfc_ndef_ch_rec {
	/** Major version number of the supported
	 *  Connection Handover specification.
	 */
	uint8_t major_version;

	/** Minor version number of the supported
	 *  Connection Handover specification.
	 */
	uint8_t minor_version;

	/** Pointer to a message encapsulating local records. */
	struct nfc_ndef_msg_desc *local_records;
};

/**
 * @brief Constructor for an NFC Connection Handover generic record payload.
 *
 * @param[in] ch_rec Pointer to the Connection Handover record
 *                   description.
 * @param[out] buf Pointer to the payload destination. If NULL, function will
 *                 calculate the expected size of the Handover Connection
 *                 record payload.
 * @param[in,out] len Size of the available memory to write as input. Size of
 *                    the generated record payload as output.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_ch_rec_payload_encode(const struct nfc_ndef_ch_rec *ch_rec,
				   uint8_t *buf, uint32_t *len);

/**
 * @brief Macro for creating and initializing an NFC NDEF record descriptor
 * for the Connection Handover.
 *
 * This macro creates and initializes an instance of type
 * @ref nfc_ndef_record_desc and an instance of type
 * @ref nfc_ndef_ch_rec, which together constitute an instance
 * of a Connection Handover record.
 *
 * Use the macro @ref NFC_NDEF_CH_RECORD_DESC to access the
 * NDEF Connection Handover record descriptor instance.
 *
 * @param[in] _name Name of the created record descriptor instance.
 * @param[in] _rec_type_field The NFC Forum Well Known Type
 *                            Connection Handover record type.
 * @param[in] _major_version Major version number of the supported
 *                           Connection Handover specification.
 * @param[in] _minor_version Minor version number of the supported
 *                           Connection Handover specification.
 * @param[in] _max_records Maximum number of local records.
 */
#define NFC_NDEF_CH_RECORD_DESC_DEF(_name,			         \
				    _rec_type_field,		         \
				    _major_version,		         \
				    _minor_version,		         \
				    _max_records)		         \
	NFC_NDEF_MSG_DEF(_name, _max_records);				 \
	struct nfc_ndef_ch_rec _name##_nfc_ch_rec = {			 \
		.major_version = _major_version,			 \
		.minor_version = _minor_version,			 \
		.local_records = &NFC_NDEF_MSG(_name)			 \
	};								 \
	NFC_NDEF_GENERIC_RECORD_DESC_DEF(_name,				 \
					 TNF_WELL_KNOWN,		 \
					 0,				 \
					 0,				 \
					 _rec_type_field,		 \
					 NFC_NDEF_CH_REC_TYPE_LENGTH,	 \
					 nfc_ndef_ch_rec_payload_encode, \
					 &(_name##_nfc_ch_rec))

/**
 * @brief Macro for creating and initializing an NFC NDEF record descriptor
 * for a Handover Select record.
 *
 * This macro creates and initializes an instance of type
 * @ref nfc_ndef_record_desc and an instance of type
 * @ref nfc_ndef_ch_rec, which together constitute an instance
 * of a Handover Select record.
 *
 * Use the macro @ref NFC_NDEF_CH_RECORD_DESC to access the
 * NDEF Handover Select record descriptor instance.
 *
 * @param[in] _name Name of the created record descriptor instance.
 * @param[in] _major_version Major version number of the supported
 *                           Connection Handover specification.
 * @param[in] _minor_version Minor version number of the supported
 *                           Connection Handover specification.
 * @param[in] _max_records Maximum number of local records
 *                      struct  (ac records plus optional err record).
 */
#define NFC_NDEF_CH_HS_RECORD_DESC_DEF(_name,			   \
				       _major_version,		   \
				       _minor_version,		   \
				       _max_records)		   \
	NFC_NDEF_CH_RECORD_DESC_DEF(_name,			   \
				    nfc_ndef_ch_hs_rec_type_field, \
				    _major_version,		   \
				    _minor_version,		   \
				    _max_records)

/**
 * @brief Macro for creating and initializing an NFC NDEF record descriptor
 * for a Handover Request record.
 *
 * This macro creates and initializes an instance of type
 * @ref nfc_ndef_record_desc and an instance of type
 * @ref nfc_ndef_ch_rec, which together constitute an instance
 * of a Handover Request record.
 *
 * Use the macro @ref NFC_NDEF_CH_RECORD_DESC to access the
 * NDEF Handover Request record descriptor instance.
 *
 * @param[in] _name Name of the created record descriptor instance.
 * @param[in] _major_version Major version number of the supported
 *                           Connection Handover specification.
 * @param[in] _minor_version Minor version number of the supported
 *                           Connection Handover specification.
 * @param[in] _max_records Maximum number of local records
 *                        (cr record plus ac records).
 */
#define NFC_NDEF_CH_HR_RECORD_DESC_DEF(_name,			   \
				       _major_version,		   \
				       _minor_version,		   \
				       _max_records)		   \
	NFC_NDEF_CH_RECORD_DESC_DEF(_name,			   \
				    nfc_ndef_ch_hr_rec_type_field, \
				    _major_version,		   \
				    _minor_version,		   \
				    _max_records)

/**
 * @brief Macro for creating and initializing an NFC NDEF record descriptor
 * for a Handover Mediation record.
 *
 * This macro creates and initializes an instance of type
 * @ref nfc_ndef_record_desc and an instance of type
 * @ref nfc_ndef_ch_rec, which together constitute an instance
 * of a Handover Mediation record.
 *
 * Use the macro @ref NFC_NDEF_CH_RECORD_DESC to access the
 * NDEF Handover Mediation record descriptor instance.
 *
 * @param[in] _name Name of the created record descriptor instance.
 * @param[in] _major_version Major version number of the supported
 *                           Connection Handover specification.
 * @param[in] _minor_version Minor version number of the supported
 *                           Connection Handover specification.
 * @param[in] _max_records Maximum number of local records
 *                        (ac records).
 */
#define NFC_NDEF_CH_HM_RECORD_DESC_DEF(_name,                      \
				       _major_version,		   \
				       _minor_version,		   \
				       _max_records)		   \
	NFC_NDEF_CH_RECORD_DESC_DEF(_name,			   \
				    nfc_ndef_ch_hm_rec_type_field, \
				    _major_version,		   \
				    _minor_version,		   \
				    _max_records)

/**
 * @brief Macro for creating and initializing an NFC NDEF record descriptor
 * for a Handover Initiate record.
 *
 * This macro creates and initializes an instance of type
 * @ref nfc_ndef_record_desc.
 *
 * Use the macro @ref NFC_NDEF_CH_RECORD_DESC to access the
 * NDEF Handover Initiate record descriptor instance.
 *
 * @param[in] _name Name of the created record descriptor instance.
 * @param[in] _major_version Major version number of the supported
 *                           Connection Handover specification.
 * @param[in] _minor_version Minor version number of the supported
 *                           Connection Handover specification.
 * @param[in] _max_records Maximum number of local records (ac records).
 */
#define NFC_NDEF_CH_HI_RECORD_DESC_DEF(_name,			   \
				       _major_version,		   \
				       _minor_version,		   \
				       _max_records)		   \
	NFC_NDEF_CH_RECORD_DESC_DEF(_name,			   \
				    nfc_ndef_ch_hi_rec_type_field, \
				    _major_version,		   \
				    _minor_version,		   \
				    _max_records)

/**
 * @brief Macro for accessing the NFC NDEF Handover Select record descriptor
 * instance that was created with @ref NFC_NDEF_CH_HS_RECORD_DESC_DEF.
 */
#define NFC_NDEF_CH_RECORD_DESC(_name) NFC_NDEF_GENERIC_RECORD_DESC(_name)

/**
 * @brief Function for clearing local records in the NFC NDEF Connection
 * Handover records.
 *
 * This function clears local records of the Connection Handover records.
 *
 * @param[in, out] ch_rec Pointer to the Connection Handover record descriptor.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
void nfc_ndef_ch_rec_local_record_clear(struct nfc_ndef_record_desc *ch_rec);

/**
 * @brief Function for adding a local record to an
 * NFC NDEF Handover Select record.
 *
 * @param[in, out] ch_rec Pointer to a Connection Handover record.
 * @param[in] local_rec Pointer to a local record to add.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_ch_rec_local_record_add(struct nfc_ndef_record_desc *ch_rec,
				     const struct nfc_ndef_record_desc *local_rec);

/**
 * @brief Constructor for an NFC Connection Handover Carrier Record payload.
 *
 * @param[in] hc_rec Pointer to the Connection Handover Carrier record
 *                   description.
 * @param[out] buf Pointer to the payload destination. If NULL, function will
 *                 calculate the expected size of the Handover Connection
 *                 Carrier Record payload.
 * @param[in,out] len Size of the available memory to write as input. Size of
 *                    the generated record payload as output.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_ch_hc_rec_payload_encode(const struct nfc_ndef_ch_hc_rec *hc_rec,
				      uint8_t *buf, uint32_t *len);

/**
 * @brief Macro for creating and initializing an NFC NDEF record descriptor
 * for the Handover Carrier record.
 *
 * This macro creates and initializes an instance of type
 * @ref nfc_ndef_record_desc and an instance of type
 * @ref nfc_ndef_ch_rec, which together constitute an instance
 * of a Connection Handover record.
 *
 * Use the macro @ref NFC_NDEF_CH_HC_RECORD_DESC to access the
 * NDEF Connection Handover record descriptor instance.
 *
 * @param[in] _name Name of the created record descriptor instance.
 * @param[in] _payload_id ID of the payload.
 * @param[in] _payload_id_length Length of the payload.
 * @param[in] _payload_desc Description of the payload.
 */
#define NFC_NDEF_CH_HC_RECORD_DESC_DEF(_name,				    \
				       _payload_id,			    \
				       _payload_id_length,		    \
				       _payload_desc)			    \
									    \
	NFC_NDEF_GENERIC_RECORD_DESC_DEF(_name,				    \
					 TNF_WELL_KNOWN,                    \
					 _payload_id,                       \
					 _payload_id_length,                \
					 nfc_ndef_ch_hc_rec_type_field,     \
					 NFC_NDEF_CH_REC_TYPE_LENGTH,	    \
					 nfc_ndef_ch_hc_rec_payload_encode, \
					 (_payload_desc))

/**
 * @brief Macro for accessing the NFC NDEF Handover Carrier record descriptor
 * instance that was created with @em NFC_NDEF_CH_HC_GENERIC_RECORD_DESC_DEF.
 */
#define NFC_NDEF_CH_HC_RECORD_DESC(_name) NFC_NDEF_GENERIC_RECORD_DESC(_name)

/**
 * @brief Constructor for an NFC NDEF Alternative Carrier record payload.
 *
 * @param[in] nfc_rec_ac Pointer to the Alternative Carrier
 *                       record description.
 * @param[in] buf Pointer to the payload destination. If NULL, function will
 *                calculate the expected size of the Alternative Carrier
 *                record payload.
 * @param[in,out] len Size of the available memory to write as input. Size of
 *                    the generated record payload as output.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_ch_ac_rec_payload_encode(const struct nfc_ndef_ch_ac_rec *nfc_rec_ac,
				      uint8_t *buf, uint32_t *len);

/**
 * @brief Macro for creating and initializing an NFC NDEF record descriptor
 *        for an Alternative Carrier record.
 *
 * This macro creates and initializes an instance of type
 * @ref nfc_ndef_record_desc and an instance of type
 * @ref nfc_ndef_ch_ac_rec, which together constitute an instance
 * of an Alternative Carrier record.
 *
 * Use the macro @ref NFC_NDEF_CH_AC_RECORD_DESC to access the
 * NDEF Alternative Carrier record descriptor instance.
 *
 * @param[in] _name Name of the created record descriptor instance.
 * @param[in] _cps Carrier Power State value.
 * @param[in] _carr_data_ref_len Length of the Carrier Data Reference field.
 * @param[in] _carr_data_ref Pointer to the Carrier Data Reference field.
 * @param[in] _max_aux_data_ref_cnt Maximum number of Auxiliary Data Reference
 *                                  fields.
 */
#define NFC_NDEF_CH_AC_RECORD_DESC_DEF(_name,				    \
				       _cps,				    \
				       _carr_data_ref_len,		    \
				       _carr_data_ref,			    \
				       _max_aux_data_ref_cnt)		    \
	struct nfc_ndef_ch_ac_rec_ref					    \
		_name##nfc_ndef_ch_ac_rec_ref[_max_aux_data_ref_cnt];	    \
	struct nfc_ndef_ch_ac_rec				            \
		_name##_nfc_ac_rec = {					    \
		.cps = _cps,						    \
		.carrier_data_ref = { _carr_data_ref_len, _carr_data_ref }, \
		.max_aux_data_ref_cnt = _max_aux_data_ref_cnt,		    \
		.aux_data_ref_cnt = 0,					    \
		.aux_data_ref = _name##nfc_ndef_ch_ac_rec_ref		    \
	};								    \
	NFC_NDEF_GENERIC_RECORD_DESC_DEF(_name,				    \
					 TNF_WELL_KNOWN,		    \
					 0,				    \
					 0,				    \
					 nfc_ndef_ch_ac_rec_type_field,     \
					 NFC_NDEF_CH_REC_TYPE_LENGTH,       \
					 nfc_ndef_ch_ac_rec_payload_encode, \
					 &(_name##_nfc_ac_rec))

/**
 * @brief Macro for accessing the NFC NDEF Alternative Carrier record descriptor
 * instance that was created with @em NFC_NDEF_AC_RECORD_DESC_DEF.
 */
#define NFC_NDEF_CH_AC_RECORD_DESC(_name) NFC_NDEF_GENERIC_RECORD_DESC(_name)

/**
 * @brief Function for clearing an Auxiliary Data Reference in an
 * NFC NDEF Alternative Carrier record.
 *
 * This function clears the Auxiliary Data References from the
 * Alternative Carrier record.
 *
 * @param[in, out] ac_rec Pointer to the Alternative Carrier record descriptor.
 */
void nfc_ndef_ch_ac_rec_auxiliary_data_ref_clear(struct nfc_ndef_record_desc *ac_rec);

/**
 * @brief Function for adding an Auxiliary Data Reference to an NFC NDEF
 * Alternative Carrier record.
 *
 * @param[in,out] ac_rec Pointer to an ac record.
 * @param[in] aux_data Pointer to the Auxiliary Data Reference data buffer.
 * @param[in] aux_length Length of the Auxiliary Data Reference data.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_ch_ac_rec_auxiliary_data_ref_add(struct nfc_ndef_record_desc *ac_rec,
					      const uint8_t *aux_data,
					      uint8_t aux_length);

/**
 * @brief Constructor for an NFC NDEF Collision Resolution record payload.
 *
 * @param[in] nfc_rec_cr Pointer to the Collision Resolution
 *                       record description.
 * @param[out] buf Pointer to the payload destination. If NULL, function will
 *                 calculate the expected size of the Collision Resolution
 *                 record payload.
 * @param[in,out] len Size of the available memory to write as input. Size of
 *                    the generated record payload as output.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_ch_cr_rec_payload_encode(const struct nfc_ndef_ch_cr_rec *nfc_rec_cr,
				      uint8_t *buf, uint32_t *len);

/**
 * @brief Macro for creating and initializing an NFC NDEF record descriptor
 *        for an Collision Resolution record.
 *
 * This macro creates and initializes an instance of type
 * @ref nfc_ndef_record_desc and an instance of type
 * @ref nfc_ndef_ch_cr_rec, which together constitute an instance
 * of an Collision Resolution record.
 *
 * Use the macro @ref NFC_NDEF_CR_RECORD_DESC to access the
 * NDEF Collision Resolution record descriptor instance.
 *
 * @param[in] _name Name of the created record descriptor instance.
 * @param[in] _random_number Generated random number required to
 *                           resolve a collision.
 */
#define NFC_NDEF_CH_CR_RECORD_DESC_DEF(_name,				    \
				       _random_number)			    \
	struct nfc_ndef_ch_cr_rec					    \
		_name##_nfc_cr_rec = {					    \
		.random = _random_number,				    \
	};								    \
	NFC_NDEF_GENERIC_RECORD_DESC_DEF(_name,				    \
					 TNF_WELL_KNOWN,		    \
					 0,				    \
					 0,				    \
					 nfc_ndef_ch_cr_rec_type_field,	    \
					 NFC_NDEF_CH_REC_TYPE_LENGTH,	    \
					 nfc_ndef_ch_cr_rec_payload_encode, \
					 &(_name##_nfc_cr_rec))

/** @brief Macro for accessing the NFC NDEF Alternative Carrier record
 * descriptor instance that was created with
 * @ref NFC_NDEF_CH_CR_RECORD_DESC_DEF.
 */
#define NFC_NDEF_CR_RECORD_DESC(_name) NFC_NDEF_GENERIC_RECORD_DESC(_name)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* NFC_NDEF_CH_H_ */
