/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_NDEF_URI_REC_H_
#define NFC_NDEF_URI_REC_H_

/**@file
 *
 * @defgroup nfc_uri_rec URI records
 * @{
 * @ingroup  nfc_uri_msg
 *
 * @brief    Generation of NFC NDEF URI record descriptions.
 *
 */
#include <stddef.h>
#include <zephyr/types.h>
#include <nfc/ndef/record.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum nfc_ndef_uri_rec_id
 * @brief URI identifier codes according to "URI Record Type Definition"
 * (denotation "NFCForum-TS-RTD_URI_1.0" published on 2006-07-24) chapter 3.2.2.
 */
enum nfc_ndef_uri_rec_id {
	NFC_URI_NONE          = 0x00,	/**< No prepending is done. */
	NFC_URI_HTTP_WWW      = 0x01,	/**< "http://www." */
	NFC_URI_HTTPS_WWW     = 0x02,	/**< "https://www." */
	NFC_URI_HTTP          = 0x03,	/**< "http:" */
	NFC_URI_HTTPS         = 0x04,	/**< "https:" */
	NFC_URI_TEL           = 0x05,	/**< "tel:" */
	NFC_URI_MAILTO        = 0x06,	/**< "mailto:" */
	NFC_URI_FTP_ANONYMOUS = 0x07,	/**< "ftp://anonymous:anonymous@" */
	NFC_URI_FTP_FTP       = 0x08,	/**< "ftp://ftp." */
	NFC_URI_FTPS          = 0x09,	/**< "ftps://" */
	NFC_URI_SFTP          = 0x0A,	/**< "sftp://" */
	NFC_URI_SMB           = 0x0B,	/**< "smb://" */
	NFC_URI_NFS           = 0x0C,	/**< "nfs://" */
	NFC_URI_FTP           = 0x0D,	/**< "ftp://" */
	NFC_URI_DAV           = 0x0E,	/**< "dav://" */
	NFC_URI_NEWS          = 0x0F,	/**< "news:" */
	NFC_URI_TELNET        = 0x10,	/**< "telnet://" */
	NFC_URI_IMAP          = 0x11,	/**< "imap:" */
	NFC_URI_RTSP          = 0x12,	/**< "rtsp://" */
	NFC_URI_URN           = 0x13,	/**< "urn:" */
	NFC_URI_POP           = 0x14,	/**< "pop:" */
	NFC_URI_SIP           = 0x15,	/**< "sip:" */
	NFC_URI_SIPS          = 0x16,	/**< "sips:" */
	NFC_URI_TFTP          = 0x17,	/**< "tftp:" */
	NFC_URI_BTSPP         = 0x18,	/**< "btspp://" */
	NFC_URI_BTL2CAP       = 0x19,	/**< "btl2cap://" */
	NFC_URI_BTGOEP        = 0x1A,	/**< "btgoep://" */
	NFC_URI_TCPOBEX       = 0x1B,	/**< "tcpobex://" */
	NFC_URI_IRDAOBEX      = 0x1C,	/**< "irdaobex://" */
	NFC_URI_FILE          = 0x1D,	/**< "file://" */
	NFC_URI_URN_EPC_ID    = 0x1E,	/**< "urn:epc:id:" */
	NFC_URI_URN_EPC_TAG   = 0x1F,	/**< "urn:epc:tag:" */
	NFC_URI_URN_EPC_PAT   = 0x20,	/**< "urn:epc:pat:" */
	NFC_URI_URN_EPC_RAW   = 0x21,	/**< "urn:epc:raw:" */
	NFC_URI_URN_EPC       = 0x22,	/**< "urn:epc:" */
	NFC_URI_URN_NFC       = 0x23,	/**< "urn:nfc:" */
	NFC_URI_RFU           = 0xFF	/**< No prepending is done. Reserved for
					  *   future use.
					  */
};

/**
 * @brief Type of description of the payload of a URI record.
 */
struct nfc_ndef_uri_rec_payload {
	/** URI identifier code. */
	enum nfc_ndef_uri_rec_id uri_id_code;
	/** Pointer to a URI string. */
	uint8_t const *uri_data;
	/** Length of the URI string. */
	uint16_t uri_data_len;
};

/**
 * @brief External reference to the type field of the URI record, defined in the
 * file @c uri_rec.c. It is used in the @ref NFC_NDEF_URI_RECORD_DESC_DEF
 * macro.
 */
extern const uint8_t nfc_ndef_uri_rec_type;

/**
 * @brief Construct the payload for a URI record.
 *
 * This function encodes the payload according to the URI record definition.
 * It implements an API compatible with @ref payload_constructor_t.
 *
 * @param input Pointer to the description of the payload.
 * @param buff Pointer to payload destination. If NULL, function will calculate
 * the expected size of the URI record payload.
 * @param len Size of available memory to write as input. Size of generated
 * payload as output.
 *
 * @retval 0 If the payload was encoded successfully.
 * @retval -ENOMEM If the predicted payload size is bigger than the provided
 * buffer space.
 */
int nfc_ndef_uri_rec_payload_encode(struct nfc_ndef_uri_rec_payload *input,
				    uint8_t *buff,
				    uint32_t *len);

/** @brief Macro for generating a description of a URI record.
 *
 * This macro initializes an instance of an NFC NDEF record description of a
 * URI record.
 *
 * @note The record descriptor is declared as automatic variable, which implies
 * that the NDEF message encoding (see @ref nfc_ndef_uri_msg_encode) must be
 * done in the same variable scope.
 *
 * @param name Name for accessing record descriptor.
 * @param uri_id_code_arg URI identifier code that defines the protocol field
 * of the URI.
 * @param uri_data_arg Pointer to the URI string. The string should not contain
 * the protocol field if the protocol was specified in @p uri_id_code.
 * @param uri_data_len_arg  Length of the URI string.
 */
#define NFC_NDEF_URI_RECORD_DESC_DEF(name,				       \
				     uri_id_code_arg,			       \
				     uri_data_arg,			       \
				     uri_data_len_arg)			       \
	struct nfc_ndef_uri_rec_payload name##_ndef_uri_record_payload_desc =  \
	{								       \
		.uri_id_code = (uri_id_code_arg),			       \
		.uri_data = (uri_data_arg),				       \
		.uri_data_len = (uri_data_len_arg)			       \
	};								       \
									       \
	NFC_NDEF_GENERIC_RECORD_DESC_DEF(name,				       \
					 TNF_WELL_KNOWN,		       \
					 NULL,				       \
					 0,				       \
					 &nfc_ndef_uri_rec_type,	       \
					 sizeof(nfc_ndef_uri_rec_type),	       \
					 nfc_ndef_uri_rec_payload_encode,      \
					 &name##_ndef_uri_record_payload_desc) \

/**
 * @brief Macro for accessing the NFC NDEF URI record descriptor instance that
 * was created with @ref NFC_NDEF_URI_RECORD_DESC_DEF.
 */
#define NFC_NDEF_URI_RECORD_DESC(name) NFC_NDEF_GENERIC_RECORD_DESC(name)

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* NFC_NDEF_URI_REC_H_ */
