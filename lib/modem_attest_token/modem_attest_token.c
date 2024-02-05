/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <nrf_modem_at.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/sys/base64.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <modem/modem_attest_token.h>

#define BASE64_PAD_CHAR '='
#define AT_ATTEST_CMD "AT%ATTESTTOKEN"

/* UUID v4 hypen locations with respect to the original byte array */
#define UUID_HYPHEN_POS_1 (8 / 2)
#define UUID_HYPHEN_POS_2 (12 / 2)
#define UUID_HYPHEN_POS_3 (16 / 2)
#define UUID_HYPHEN_POS_4 (20 / 2)

int modem_attest_token_get(struct nrf_attestation_token *const token)
{
	int ret = 0;
	size_t attest_sz;
	size_t cose_sz;
	bool attest_alloc = false;
	/* Maximum response length is 200 bytes */
	char attest[128] = {0};
	char cose[128] = {0};

	if (!token) {
		return -EINVAL;
	} else if ((token->attest && !token->attest_sz) ||
		   (token->cose && !token->cose_sz)) {
		return -EBADF;
	}

	/* Execute AT command to get attestation token */
	ret = nrf_modem_at_scanf(AT_ATTEST_CMD,
		"%%ATTESTTOKEN: \"%127[^.].%127[^\"]\"", &attest, &cose);
	if (ret != 2) {
		return -EBADMSG;
	}

	ret = 0;

	attest_sz = strlen(attest) + 1;
	cose_sz = strlen(cose) + 1;

	/* Ensure provided buffers are large enough */
	if (((token->attest) && (token->attest_sz < attest_sz)) ||
	    ((token->cose) && (token->cose_sz < cose_sz))) {
		return -EMSGSIZE;
	}

	/* Allocate if not provided */
	if (!token->attest) {
		token->attest = k_calloc(attest_sz, 1);
		if (!token->attest) {
			ret = -ENOMEM;
			goto cleanup;
		}
		attest_alloc = true;
		token->attest_sz = attest_sz;
	}
	if (!token->cose) {
		token->cose = k_calloc(cose_sz, 1);
		if (!token->cose) {
			ret = -ENOMEM;
			goto cleanup;
		}
		token->cose_sz = cose_sz;
	}

	/* Copy token contents */
	memcpy(token->attest, attest, attest_sz);
	memcpy(token->cose, cose, cose_sz);

cleanup:
	if (ret && attest_alloc) {
		k_free(token->attest);
		token->attest = NULL;
		token->attest_sz = 0;
	}

	return ret;
}

void modem_attest_token_free(struct nrf_attestation_token *const token)
{
	if (!token) {
		return;
	}

	if (token->attest) {
		k_free(token->attest);
		token->attest = NULL;
	}

	if (token->cose) {
		k_free(token->cose);
		token->cose = NULL;
	}
}

#if defined(CONFIG_MODEM_ATTEST_TOKEN_PARSING)
static int base64_url_unformat(char *const base64url_string)
{
	if (base64url_string == NULL) {
		return -EINVAL;
	}

	char *found = NULL;

	/* replace '-' with "+" */
	for (found = base64url_string; (found = strchr(found, '-'));) {
		*found = '+';
	}

	/* replace '_' with "/" */
	for (found = base64url_string; (found = strchr(found, '_'));) {
		*found = '/';
	}

	/* return number of padding chars required */
	return (strlen(base64url_string) % 4);
}

static int base64url_to_binary(const char *const base64url_str, char **bin_buf,
			       size_t *bin_buf_sz)
{
	int err = 0;
	char *b64_str = NULL;
	size_t b64_str_sz = 0;
	size_t base64url_str_len = strlen(base64url_str);
	int pad_chars = base64url_str_len % 4;

	if (pad_chars >= 2) {
		pad_chars = 4 - pad_chars;
	} else {
		pad_chars = 0;
	}

	b64_str_sz = pad_chars + base64url_str_len + 1;

	*bin_buf = NULL;

	/* The decoder does not handle b64 url formatted strings.
	 * Allocate a buffer to hold a non-url formatted b64 string
	 * so it can be properly decoded to binary.
	 */
	b64_str = k_calloc(b64_str_sz, 1);
	if (!b64_str) {
		err = -ENOMEM;
		goto cleanup;
	}

	/* Copy and un-format, then add pad characters if necessary */
	memcpy(b64_str, base64url_str, base64url_str_len);
	base64_url_unformat(b64_str);
	if (pad_chars) {
		memset(&b64_str[base64url_str_len], BASE64_PAD_CHAR, pad_chars);
	}

	/* Determine size of binary buffer */
	(void)base64_decode(NULL, 0, bin_buf_sz, b64_str, b64_str_sz - 1);
	if (!(*bin_buf_sz)) {
		err = -EBADMSG;
		goto cleanup;
	}

	/* Allocate and decode */
	*bin_buf = k_calloc(*bin_buf_sz, 1);
	if (!(*bin_buf)) {
		err = -ENOMEM;
		goto cleanup;
	}
	err = base64_decode(*bin_buf, *bin_buf_sz, bin_buf_sz, b64_str,
			    b64_str_sz - 1);

	if (err) {
		err = -EIO;
		goto cleanup;
	}

cleanup:
	if (b64_str) {
		k_free(b64_str);
	}
	if (err && *bin_buf) {
		k_free(*bin_buf);
		*bin_buf = NULL;
		*bin_buf_sz = 0;
	}

	return err;
}

static int get_attest_data(zcbor_state_t *state,
			   struct nrf_attestation_data *const data)
{
	int dev_type;
	struct zcbor_string dev_uuid;
	struct zcbor_string fw_uuid;
	struct zcbor_string nonce;

	/* Attestation message format:
	 * Tag (55799)
	 * Array (len 5)
	 * - INT: msg type (NRF_ID_SRVC_MSG_TYPE_ID_V1)
	 * - BSTR: device UUID
	 * - INT: device type
	 * - BSTR: FW UUID
	 * - BSTR: nonce
	 */
	if (!zcbor_tag_expect(state, ZCBOR_TAG_CBOR)) {
		return -ENOMSG;
	}

	if (zcbor_list_start_decode(state) &&
	    zcbor_int32_expect(state, NRF_ID_SRVC_MSG_TYPE_ID_V1) &&
	    zcbor_bstr_decode(state, &dev_uuid) && (dev_uuid.len == sizeof(data->device_uuid)) &&
	    zcbor_int32_decode(state, &dev_type) &&
	    zcbor_bstr_decode(state, &fw_uuid) && (fw_uuid.len == sizeof(data->fw_uuid)) &&
	    zcbor_bstr_decode(state, &nonce) && (nonce.len == sizeof(data->nonce)) &&
	    zcbor_list_end_decode(state)) {

		data->msg_type = NRF_ID_SRVC_MSG_TYPE_ID_V1;
		data->dev_type = (enum nrf_device_type)dev_type;

		memcpy(data->device_uuid, dev_uuid.value, sizeof(data->device_uuid));
		memcpy(data->fw_uuid,	  fw_uuid.value,  sizeof(data->fw_uuid));
		memcpy(data->nonce,	  nonce.value,	  sizeof(data->nonce));

		return 0;
	}

	return -EBADMSG;
}

int modem_attest_token_parse(struct nrf_attestation_token const *const token_in,
			     struct nrf_attestation_data *const data_out)
{
	if (!token_in || !data_out || !token_in->attest) {
		return -EINVAL;
	}

	size_t bin_buf_sz = 0;
	char *bin_buf = NULL;
	zcbor_state_t states[3];
	int err = 0;

	/* Convert to binary */
	err = base64url_to_binary(token_in->attest, &bin_buf, &bin_buf_sz);
	if (err) {
		goto cleanup;
	}

	zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t), bin_buf, bin_buf_sz, 1,
			NULL, 0);

	/* Get attestation token data */
	err = get_attest_data(states, data_out);
	if (err) {
		memset(data_out, 0, sizeof(*data_out));
		data_out->dev_type = NRF_DEVICE_TYPE_INVALID;
		data_out->msg_type = NRF_ID_SRVC_MSG_TYPE_INVALID;
	}

cleanup:
	if (bin_buf) {
		k_free(bin_buf);
	}

	return err;
}

static int format_uuid(const char * const bytes_in, const size_t bytes_sz,
		       char * const str_out, const size_t str_sz)
{
	__ASSERT_NO_MSG(bytes_in != NULL);
	__ASSERT_NO_MSG(bytes_sz == NRF_UUID_BYTE_SZ);
	__ASSERT_NO_MSG(str_out != NULL);
	__ASSERT_NO_MSG(str_sz >= NRF_UUID_V4_STR_LEN);

	int sz;
	size_t w_pos = 0;

	for (int i = 0; (i < bytes_sz); ++i) {
		if ((i == UUID_HYPHEN_POS_1) || (i == UUID_HYPHEN_POS_2) ||
		    (i == UUID_HYPHEN_POS_3) || (i == UUID_HYPHEN_POS_4)) {
			str_out[w_pos++] = '-';
		}

		sz = sprintf(&str_out[w_pos], "%02x", bytes_in[i]);
		if (sz != 2) {
			return -EIO;
		}
		w_pos += 2;
	}

	return 0;
}

int modem_attest_token_get_uuids(struct nrf_device_uuid *dev,
				 struct nrf_modem_fw_uuid *mfw)
{
	if ((dev == NULL) && (mfw == NULL)) {
		return -EINVAL;
	}

	int err;
	struct nrf_attestation_token a_tok;
	struct nrf_attestation_data a_data;

	memset(&a_tok, 0, sizeof(a_tok));
	err = modem_attest_token_get(&a_tok);
	if (err) {
		return err;
	}

	memset(&a_data, 0, sizeof(a_data));
	err = modem_attest_token_parse(&a_tok, &a_data);
	if (err) {
		goto cleanup;
	}

	if (dev) {
		err = format_uuid(a_data.device_uuid, sizeof(a_data.device_uuid),
				  dev->str, sizeof(dev->str));
		if (err) {
			goto cleanup;
		}
	}

	if (mfw) {
		err = format_uuid(a_data.fw_uuid, sizeof(a_data.fw_uuid),
				  mfw->str, sizeof(mfw->str));
		if (err) {
			goto cleanup;
		}
	}

cleanup:
	modem_attest_token_free(&a_tok);

	if (err) {
		if (dev) {
			memset(dev->str, 0, sizeof(dev->str));
		}
		if (mfw) {
			memset(mfw->str, 0, sizeof(mfw->str));
		}
	}

	return err;
}
#endif
