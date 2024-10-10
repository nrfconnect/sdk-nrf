/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include "asn1_decode.h"
#include "pkcs15_decode.h"

/* ASN.1 Object Identifier 2.23.43.9.1 in encoded form. */
#define ASN1_LWM2M_BOOTSTRAP_OID "672b0901"
#define ASN1_OMA_TS_LWM2M_BOOTSTRAP_OID "0604672b0901"

static void asn1_dec_Path(asn1_ctx_t *ctx, void *data)
{
	pkcs15_object_t *object = (pkcs15_object_t *)data;
	uint8_t tag;
	size_t len;

	/* Here only path is handled.
	 *
	 * PKCS15Path ::= SEQUENCE {
	 *     path    OCTET STRING,
	 *     index   INTEGER (0..pkcs15-ub-index) OPTIONAL,
	 *     length  [0] INTEGER (0..pkcs15-ub-index) OPTIONAL
	 * } (WITH COMPONENTS {..., index PRESENT, length PRESENT} |
	 *    WITH COMPONENTS {..., index ABSENT, length ABSENT})
	 */

	while (asn1_dec_head(ctx, &tag, &len)) {
		switch (tag) {
		case UP4:
			asn1_dec_octet_string(ctx, len, object->path, sizeof(object->path));
			break;
		default:
			asn1_dec_skip(ctx, len);
			break;
		}
	}
}

static void asn1_dec_PathOrObjects_Data(asn1_ctx_t *ctx, void *data)
{
	pkcs15_object_t *object = (pkcs15_object_t *)data;
	uint8_t tag;
	size_t len;

	/* Here only the PKCS15Path choice is handled.
	 *
	 * PKCS15PathOrObjects {ObjectType} ::= CHOICE {
	 *     path     PKCS15Path,
	 *     objects  [0] SEQUENCE OF ObjectType,
	 *     ...,
	 *     indirect-protected [1] ReferencedValue {EnvelopedData {SEQUENCE OF ObjectType}},
	 *     direct-protected [2] EnvelopedData {SEQUENCE OF ObjectType},
	 * }
	 */

	while (asn1_dec_head(ctx, &tag, &len)) {
		switch (tag) {
		case UC16: /* PKCS15Path */
			asn1_dec_sequence(ctx, len, &object->path, asn1_dec_Path);
			break;
		default:
			asn1_dec_skip(ctx, len);
			break;
		}
	}
}

static void asn1_dec_OidDO(asn1_ctx_t *ctx, void *data)
{
	pkcs15_object_t *object = (pkcs15_object_t *)data;
	uint8_t tag;
	size_t len;

	/* Here only decode value when OID match 2.23.43.9.1 "lwm2m_bootstrap".
	 *
	 * Because of a bug in the OMA-TS-LwM2M_Core spec example this
	 * variant must also be checked.
	 *
	 * PKCS15OidDO ::= SEQUENCE {
	 *     id     OBJECT IDENTIFIER,
	 *     value  ObjectValue {PKCS15-OPAQUE.&Type}
	 * }
	 */

	uint8_t oid[32];
	bool decode_path = false;

	while (asn1_dec_head(ctx, &tag, &len)) {
		switch (tag) {
		case UP6: /* id */
			/* Using octet string decoder to simplify code using string matching */
			asn1_dec_octet_string(ctx, len, oid, sizeof(oid));

			/* Check for Object Identifier 2.23.43.9.1 or the OMA-TS variant */
			if ((strcmp(oid, ASN1_LWM2M_BOOTSTRAP_OID) == 0) ||
			    (strcmp(oid, ASN1_OMA_TS_LWM2M_BOOTSTRAP_OID) == 0)) {
				decode_path = true;
			} else {
				decode_path = false;
			}
			break;
		case UC16: /* value (Path) */
			if (decode_path) {
				asn1_dec_sequence(ctx, len, &object->path, asn1_dec_Path);
			} else {
				asn1_dec_skip(ctx, len);
			}
			break;
		default:
			asn1_dec_skip(ctx, len);
			break;
		}
	}
}

static void asn1_dec_TypeAttributes_OidDO(asn1_ctx_t *ctx, void *data)
{
	uint8_t tag;
	size_t len;

	while (asn1_dec_head(ctx, &tag, &len)) {
		switch (tag) {
		case UC16: /* PKCS15OidDO */
			asn1_dec_sequence(ctx, len, data, asn1_dec_OidDO);
			break;
		default:
			asn1_dec_skip(ctx, len);
			break;
		}
	}
}

static void asn1_dec_DataObject_OidDO(asn1_ctx_t *ctx, void *data)
{
	uint8_t tag;
	size_t len;

	/* Here only TypeAttributes{OidDO} is handled.
	 *
	 * PKCS15Object {ClassAttributes, SubClassAttributes, TypeAttributes} ::= SEQUENCE {
	 *     commonObjectAttributes  PKCS15CommonObjectAttributes,
	 *     classAttributes         ClassAttributes
	 *     subClassAttributes      [0] SubClassAttributes OPTIONAL,
	 *     typeAttributes          [1] TypeAttributes
	 * }
	 *
	 * PKCS15DataObject {DataObjectAttributes} ::= PKCS15Object {
	 *     PKCS15CommonDataObjectAttributes, NULL, DataObjectAttributes
	 * }
	 */

	while (asn1_dec_head(ctx, &tag, &len)) {
		switch (tag) {
		case CC1: /* TypeAttributes {OidDO} */
			asn1_dec_sequence(ctx, len, data, asn1_dec_TypeAttributes_OidDO);
			break;
		default:
			asn1_dec_skip(ctx, len);
			break;
		}
	}
}

bool pkcs15_ef_odf_path_decode(const uint8_t *bytes, size_t bytes_len, pkcs15_object_t *object)
{
	asn1_ctx_t asn1_ctx = {
		.asnbuf = bytes,
		.length = bytes_len
	};
	uint8_t tag;
	size_t len;

	/* The EF(ODF) (Elementary File - Object Directory File) may contain pointers
	 * to multiple Directory Files. Here only the PKCS15DataObjects choice is handled.
	 *
	 * PKCS15Objects ::= CHOICE {
	 *     privateKeys          [0] PKCS15PrivateKeys,
	 *     publicKeys           [1] PKCS15PublicKeys,
	 *     trustedPublicKeys    [2] PKCS15PublicKeys,
	 *     secretKeys           [3] PKCS15SecretKeys,
	 *     certificates         [4] PKCS15Certificates,
	 *     trustedCertificates  [5] PKCS15Certificates,
	 *     usefulCertificates   [6] PKCS15Certificates,
	 *     dataObjects          [7] PKCS15DataObjects,
	 *     authObjects          [8] PKCS15AuthObjects,
	 *     ... -- For future extensions
	 * }
	 *
	 * PKCS15DataObjects ::= PKCS15PathOrObjects {PKCS15Data}
	 */

	while (asn1_dec_head(&asn1_ctx, &tag, &len)) {
		switch (tag) {
		case CC7: /* PKCS15DataObjects (PKCS15PathOrObjects {PKCS15Data}) */
			asn1_dec_sequence(&asn1_ctx, len, object, asn1_dec_PathOrObjects_Data);
			break;
		default: /* Other PKCS15Objects choices */
			asn1_dec_skip(&asn1_ctx, len);
			break;
		}
	}

	/* The UICC record may be padded with 0xFF, this is no error */
	return !(asn1_ctx.error && tag != 0xFF && len != 0xFF);
}

bool pkcs15_ef_dodf_path_decode(const uint8_t *bytes, size_t bytes_len, pkcs15_object_t *object)
{
	asn1_ctx_t asn1_ctx = {
		.asnbuf = bytes,
		.length = bytes_len
	};
	uint8_t tag;
	size_t len;

	/* The EF(DODF) (Elementary File - Data Object Directory File) may contain
	 * multiple Data Objects. Here only the PKCS15OidDO choice is handled.
	 *
	 * PKCS15Data ::= CHOICE {
	 *     opaqueDO     PKCS15DataObject {PKCS15Opaque},
	 *     externalIDO  [0] PKCS15DataObject {PKCS15ExternalIDO},
	 *     oidDO        [1] PKCS15DataObject {PKCS15OidDO},
	 *     ... -- For future extensions
	 * }
	 */

	while (asn1_dec_head(&asn1_ctx, &tag, &len)) {
		switch (tag) {
		case CC1: /* PKCS15DataObject {PKCS15OidDO} */
			asn1_dec_sequence(&asn1_ctx, len, object, asn1_dec_DataObject_OidDO);
			break;
		default: /* Other PKCS15Data choices */
			asn1_dec_skip(&asn1_ctx, len);
			break;
		}
	}

	/* The UICC record may be padded with 0xFF, this is no error */
	return !(asn1_ctx.error && tag != 0xFF && len != 0xFF);
}
