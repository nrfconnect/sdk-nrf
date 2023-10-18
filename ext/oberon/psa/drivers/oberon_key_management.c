/*
 * Copyright (c) 2016 - 2023 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "psa/crypto.h"
#include "oberon_key_management.h"

#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_ECC
#include "oberon_ec_keys.h"
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_ECC */
#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_RSA
#include "oberon_rsa.h"
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_RSA */

psa_status_t oberon_export_public_key(const psa_key_attributes_t *attributes, const uint8_t *key,
				      size_t key_length, uint8_t *data, size_t data_size,
				      size_t *data_length)
{
	psa_key_type_t type = psa_get_key_type(attributes);

#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_ECC
	if (PSA_KEY_TYPE_IS_ECC(type)) {
		return oberon_export_ec_public_key(attributes, key, key_length, data, data_size,
						   data_length);
	} else
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_ECC */

#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_RSA
		if (PSA_KEY_TYPE_IS_RSA(type)) {
		return oberon_export_rsa_public_key(attributes, key, key_length, data, data_size,
						    data_length);
	} else
#endif /* PSA_NEED_OBERON_RSAC_KEY_PAIR */

	{
		(void)key;
		(void)key_length;
		(void)data;
		(void)data_size;
		(void)data_length;
		(void)type;
		return PSA_ERROR_NOT_SUPPORTED;
	}
}

psa_status_t oberon_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
			       size_t data_length, uint8_t *key, size_t key_size,
			       size_t *key_length, size_t *key_bits)
{
	psa_key_type_t type = psa_get_key_type(attributes);

#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_ECC
	if (PSA_KEY_TYPE_IS_ECC(type)) {
		return oberon_import_ec_key(attributes, data, data_length, key, key_size,
					    key_length, key_bits);
	} else
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_ECC */

#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_RSA
		if (PSA_KEY_TYPE_IS_RSA(type)) {
		return oberon_import_rsa_key(attributes, data, data_length, key, key_size,
					     key_length, key_bits);
	} else
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_RSA */

	{
		(void)data;
		(void)data_length;
		(void)key;
		(void)key_size;
		(void)key_length;
		(void)key_bits;
		(void)type;
		return PSA_ERROR_NOT_SUPPORTED;
	}
}

psa_status_t oberon_generate_key(const psa_key_attributes_t *attributes, uint8_t *key,
				 size_t key_size, size_t *key_length)
{
	psa_key_type_t type = psa_get_key_type(attributes);

#ifdef PSA_NEED_OBERON_KEY_MANAGEMENT_ECC
	if (PSA_KEY_TYPE_IS_ECC(type)) {
		return oberon_generate_ec_key(attributes, key, key_size, key_length);
	} else
#endif /* PSA_NEED_OBERON_KEY_MANAGEMENT_ECC */

	{
		(void)key;
		(void)key_size;
		(void)key_length;
		(void)type;
		return PSA_ERROR_NOT_SUPPORTED;
	}
}
