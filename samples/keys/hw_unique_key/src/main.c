/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include <stdio.h>
#include <hw_unique_key.h>
#include <psa/crypto.h>
#ifdef CONFIG_BUILD_WITH_TFM
#include <tfm_crypto_defs.h>
#else /* CONFIG_BUILD_WITH_TFM */
#include <nrf_cc3xx_platform.h>
#endif /* CONFIG_BUILD_WITH_TFM */
#if !defined(HUK_HAS_KMU)
#include <zephyr/sys/reboot.h>
#endif /* !defined(HUK_HAS_KMU) */

#define IV_LEN 12
#define MAC_LEN 16


#ifdef HUK_HAS_CC310
#define ENCRYPT_ALG PSA_ALG_CCM
#else
#define ENCRYPT_ALG PSA_ALG_GCM
#endif

psa_key_id_t derive_key(psa_key_attributes_t *attributes, uint8_t *key_label,
			uint32_t label_size);

void hex_dump(uint8_t *buff, uint32_t len)
{
	for (int i = 0; i < len; i++) {
		printk("%s%02x", i && ((i % 32) == 0) ? "\n" : "", buff[i]);
	}
	printk("\n");
}

int main(void)
{
	psa_status_t status;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	uint8_t key_label[] = "HUK derivation sample label";
	psa_key_id_t key_id_out = 0;
	uint8_t plaintext[] = "Lorem ipsum dolor sit amet. This will be encrypted.";
	uint8_t ciphertext[sizeof(plaintext) + MAC_LEN];
	uint32_t ciphertext_out_len;
	uint8_t iv[IV_LEN];
	uint8_t additional_data[] = "This will be authenticated but not encrypted.";

	printk("Derive a key, then use it to encrypt a message.\n\n");

#if !defined(CONFIG_BUILD_WITH_TFM)
	int result = nrf_cc3xx_platform_init();

	if (result != NRF_CC3XX_PLATFORM_SUCCESS) {
		printk("nrf_cc3xx_platform_init returned error: %d\n", result);
		return 0;
	}

	if (!hw_unique_key_are_any_written()) {
		printk("Writing random keys to KMU\n");
		hw_unique_key_write_random();
		printk("Success!\n\n");

#if !defined(HUK_HAS_KMU)
		/* Reboot to allow the bootloader to load the key into CryptoCell. */
		sys_reboot(0);
#endif /* !defined(HUK_HAS_KMU) */
	}
#endif /* !defined(CONFIG_BUILD_WITH_TFM) */

	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		printk("psa_crypto_init returned error: %d\n", status);
		return 0;
	}

	printk("Generating random IV\n");
	status = psa_generate_random(iv, IV_LEN);
	if (status != PSA_SUCCESS) {
		printk("psa_generate_random returned error: %d\n", status);
		return 0;
	}
	printk("IV:\n");
	hex_dump(iv, IV_LEN);
	printk("\n");
	printk("Deriving key\n");

	/* Set the key attributes for the storage key */
	psa_set_key_usage_flags(&attributes,
			(PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT));
	psa_set_key_algorithm(&attributes, ENCRYPT_ALG);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, PSA_BYTES_TO_BITS(HUK_SIZE_BYTES));

	key_id_out = derive_key(&attributes, key_label, sizeof(key_label) - 1);
	if (key_id_out == 0) {
		return 0;
	}

	printk("Key ID: 0x%x\n\n", key_id_out);
	printk("Encrypting\n");
	printk("Plaintext:\n\"%s\"\n", plaintext);
	hex_dump(plaintext, sizeof(plaintext) - 1);

	status = psa_aead_encrypt(key_id_out, ENCRYPT_ALG, iv, IV_LEN,
				additional_data, sizeof(additional_data) - 1,
				plaintext, sizeof(plaintext) - 1,
				ciphertext, sizeof(ciphertext), &ciphertext_out_len);
	if (status != PSA_SUCCESS) {
		printk("psa_aead_encrypt returned error: %d\n", status);
		return 0;
	}
	if (ciphertext_out_len != (sizeof(plaintext) - 1 + MAC_LEN)) {
		printk("ciphertext has wrong length: %d\n", ciphertext_out_len);
		return 0;
	}

	printk("Ciphertext (with authentication tag):\n");
	hex_dump(ciphertext, ciphertext_out_len);

	return 0;
}
