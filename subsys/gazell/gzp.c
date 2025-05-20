/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <nrf_gzll.h>
#include <gzp.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/kernel.h>
#include "gzp_internal.h"

/*
 * Constant holding base address part of the pairing address.
 */
static const uint8_t pairing_base_address[4] = GZP_ADDRESS;

/*
 * Constant holding prefix byte of the pairing address.
 */
static const uint8_t pairing_address_prefix_byte;

#ifdef CONFIG_GAZELL_PAIRING_CRYPT

/*
 * Constant holding pre-defined "validation ID".
 */
static const uint8_t gzp_validation_id[GZP_VALIDATION_ID_LENGTH] = GZP_VALIDATION_ID;

/*
 * Constant holding pre-defined "secret key".
 */
static const uint8_t gzp_secret_key[16] = GZP_SECRET_KEY;

/*
 * Variable used for AES key selection
 */
static enum gzp_key_select gzp_key_select;

#endif

#if defined(CONFIG_GAZELL_PAIRING_CRYPT) || defined(CONFIG_GAZELL_PAIRING_HOST)
static const struct device *entropy_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));
#endif

#ifdef CONFIG_GAZELL_PAIRING_CRYPT
static const struct device *crypto_dev = DEVICE_DT_GET_ONE(nordic_nrf_ecb);
#endif

#ifdef CONFIG_GAZELL_PAIRING_CRYPT
static uint8_t gzp_session_token[GZP_SESSION_TOKEN_LENGTH];
static uint8_t gzp_dyn_key[GZP_DYN_KEY_LENGTH];
#endif


void gzp_init_internal(void)
{
#if defined(CONFIG_GAZELL_PAIRING_CRYPT) || defined(CONFIG_GAZELL_PAIRING_HOST)
	__ASSERT(device_is_ready(entropy_dev), "Entropy device not ready");
#endif

#ifdef CONFIG_GAZELL_PAIRING_CRYPT
	__ASSERT(device_is_ready(crypto_dev), "Crypto device not ready");
#endif
}

bool gzp_update_radio_params(const uint8_t *system_address)
{
	uint8_t i;
	uint8_t channels[NRF_GZLL_CONST_MAX_CHANNEL_TABLE_SIZE];
	uint32_t channel_table_size;
	uint32_t pairing_base_address_32, system_address_32;
	bool update_ok = true;
	bool gzll_enabled_state;

	/* Configure "global" pairing address */
	pairing_base_address_32 = (pairing_base_address[0]) +
				  ((uint32_t)pairing_base_address[1] <<  8) +
				  ((uint32_t)pairing_base_address[2] << 16) +
				  ((uint32_t)pairing_base_address[3] << 24);
	if (system_address != NULL) {
		system_address_32 = (system_address[0]) +
				    ((uint32_t)system_address[1] <<  8) +
				    ((uint32_t)system_address[2] << 16) +
				    ((uint32_t)system_address[3] << 24);
	} else {
		return false;
	}

	gzll_enabled_state = nrf_gzll_is_enabled();

	nrf_gzp_disable_gzll();
	update_ok &= nrf_gzll_set_base_address_0(pairing_base_address_32);
	update_ok &= nrf_gzll_set_address_prefix_byte(GZP_PAIRING_PIPE,
						      pairing_address_prefix_byte);
	update_ok &= nrf_gzll_set_base_address_1(system_address_32);

	/* Configure address for pipe 1 - 7. Address byte set to equal pipe number. */
	for (i = 1; i < NRF_GZLL_CONST_PIPE_COUNT; i++) {
		update_ok &= nrf_gzll_set_address_prefix_byte(i, i);
	}

	channel_table_size = nrf_gzll_get_channel_table_size();
	gzp_generate_channels(&channels[0], system_address, channel_table_size);

	/* Write generated channel subset to Gazell Link Layer */
	update_ok &= nrf_gzll_set_channel_table(&channels[0], channel_table_size);
	if (gzll_enabled_state) {
		update_ok &= nrf_gzll_enable();
	}

	return update_ok;
}

void gzp_generate_channels(uint8_t *ch_dst, const uint8_t *system_address, uint8_t channel_tab_size)
{
	uint8_t binsize, spacing, i;

	binsize = (GZP_CHANNEL_MAX - GZP_CHANNEL_MIN) / channel_tab_size;

	ch_dst[0] = GZP_CHANNEL_LOW;
	ch_dst[channel_tab_size - 1] = GZP_CHANNEL_HIGH;

	if (system_address != NULL) {
		for (i = 1; i < (channel_tab_size - 1); i++) {
			ch_dst[i] = (binsize * i) + (system_address[i % 4] % binsize);
		}
	}

	/* If channels are too close, shift them to better positions */
	for (i = 1; i < channel_tab_size; i++) {
		spacing = (ch_dst[i] - ch_dst[i - 1]);
		if (spacing < GZP_CHANNEL_SPACING_MIN) {
			ch_dst[i] += (GZP_CHANNEL_SPACING_MIN - spacing);
		}
	}
}

void nrf_gzp_disable_gzll(void)
{
	if (nrf_gzll_is_enabled()) {
		unsigned int key = irq_lock();

		nrf_gzll_disable();

		while (nrf_gzll_is_enabled()) {
			k_cpu_atomic_idle(key);
			key = irq_lock();
		}

		irq_unlock(key);
	}
}

#ifdef CONFIG_GAZELL_PAIRING_CRYPT

void gzp_xor_cipher(uint8_t *dst, const uint8_t *src, const uint8_t *pad, uint8_t length)
{
	uint8_t i;

	for (i = 0; i < length; i++) {
		*dst = *src ^ *pad;
		dst++;
		src++;
		pad++;
	}
}

bool gzp_validate_id(const uint8_t *id)
{
	return (memcmp(id, (void *)gzp_validation_id, GZP_VALIDATION_ID_LENGTH) == 0);
}

void gzp_add_validation_id(uint8_t *dst)
{
	memcpy(dst, (void const *)gzp_validation_id, GZP_VALIDATION_ID_LENGTH);
}

void gzp_crypt_set_session_token(const uint8_t *token)
{
	memcpy(gzp_session_token, (void const *)token, GZP_SESSION_TOKEN_LENGTH);
}

void gzp_crypt_set_dyn_key(const uint8_t *key)
{
	memcpy(gzp_dyn_key, (void const *)key, GZP_DYN_KEY_LENGTH);
}

void gzp_crypt_get_session_token(uint8_t *dst_token)
{
	memcpy(dst_token, (void const *)gzp_session_token, GZP_SESSION_TOKEN_LENGTH);
}

void gzp_crypt_get_dyn_key(uint8_t *dst_key)
{
	memcpy(dst_key, (void const *)gzp_dyn_key, GZP_DYN_KEY_LENGTH);
}

void gzp_crypt_select_key(enum gzp_key_select key_select)
{
	gzp_key_select = key_select;
}

void gzp_crypt(uint8_t *dst, const uint8_t *src, uint8_t length)
{
	int err;
	uint8_t i;
	uint8_t key[16];
	uint8_t iv[16];
	uint8_t encrypted[16] = {0};

	/* Build AES key based on "gzp_key_select" */

	switch (gzp_key_select) {
	case GZP_ID_EXCHANGE:
		memcpy(key, (void const *)gzp_secret_key, 16);
		break;
	case GZP_KEY_EXCHANGE:
		memcpy(key, (void const *)gzp_secret_key, 16);
		gzp_get_host_id(key);
		break;
	case GZP_DATA_EXCHANGE:
		memcpy(key, (void const *)gzp_secret_key, 16);
		memcpy(key, (void const *)gzp_dyn_key, GZP_DYN_KEY_LENGTH);
		break;
	default:
		return;
	}

	/* Build init vector from "gzp_session_token" */
	for (i = 0; i < 16; i++) {
		if (i < GZP_SESSION_TOKEN_LENGTH) {
			iv[i] = gzp_session_token[i];
		} else {
			iv[i] = 0;
		}
	}

	uint32_t cap_flags = CAP_RAW_KEY | CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS;
	struct cipher_ctx ini = {
		.keylen = sizeof(key),
		.key.bit_stream = key,
		.flags = cap_flags,
	};
	struct cipher_pkt encrypt = {
		.in_buf = iv,
		.in_len = sizeof(iv),
		.out_buf_max = sizeof(encrypted),
		.out_buf = encrypted,
	};

	err = cipher_begin_session(crypto_dev, &ini, CRYPTO_CIPHER_ALGO_AES,
				   CRYPTO_CIPHER_MODE_ECB,
				   CRYPTO_CIPHER_OP_ENCRYPT);
	__ASSERT(!err, "Cannot set up crypto session");

	/* Encrypt IV using ECB mode */
	err = cipher_block_op(&ini, &encrypt);
	__ASSERT(!err, "Cannot perform crypto operation");

	err = cipher_free_session(crypto_dev, &ini);
	__ASSERT(!err, "Cannot clean up crypto session");

	/* Encrypt data by XOR'ing with AES output */
	gzp_xor_cipher(dst, src, encrypted, length);
}
#endif

#if defined(CONFIG_GAZELL_PAIRING_CRYPT) || defined(CONFIG_GAZELL_PAIRING_HOST)
void gzp_random_numbers_generate(uint8_t *dst, uint8_t n)
{
	int err;

	err = entropy_get_entropy(entropy_dev, dst, n);
	__ASSERT(!err, "Cannot generate random numbers");
}
#endif

/*
 * @brief Function for setting the Primask variable.
 *
 * @param primask The primask value. 1 to disable interrupts, 0 otherwise.
 */
static void nrf_gzp_set_primask(uint32_t primask)
{
	__set_PRIMASK(primask);
}

void nrf_gzp_flush_rx_fifo(uint32_t pipe)
{
	static uint8_t dummy_packet[NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH];
	uint32_t length;

	nrf_gzp_set_primask(1);

	while (nrf_gzll_get_rx_fifo_packet_count(pipe) > 0) {
		length = NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH;
		(void)nrf_gzll_fetch_packet_from_rx_fifo(pipe, dummy_packet, &length);
	}

	nrf_gzp_set_primask(0);
}
