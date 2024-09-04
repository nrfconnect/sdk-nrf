/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HW_UNIQUE_KEY_H_
#define HW_UNIQUE_KEY_H_

/**
 * @file
 * @defgroup hw_unique_key Hardware Unique Key (HUK) loading
 * @{
 *
 * @brief API for loading the Hardware Unique Key (HUK).
 *
 *        The library supports loading the HUK:
 *            - To CryptoCell KDR registers.
 *            - To Cracen SEED registers.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#if defined(__NRF_TFM__)
#include <zephyr/autoconf.h>
#endif
#include <zephyr/devicetree.h>

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_kmu) || defined(CONFIG_CRACEN_HW_PRESENT)
#define HUK_HAS_KMU
#endif
#if defined(CONFIG_HAS_HW_NRF_CC310)
#define HUK_HAS_CC310
#endif
#if defined(CONFIG_HAS_HW_NRF_CC312)
#define HUK_HAS_CC312
#endif

#if defined(HUK_HAS_CC310)
#define HUK_SIZE_WORDS 4
#elif defined(HUK_HAS_CC312)
#define HUK_SIZE_WORDS 8
#elif defined(CONFIG_CRACEN_HW_PRESENT)
#define HUK_SIZE_WORDS 12
#else
#error "This library requires CryptoCell or Cracen to be available."
#endif

#define HUK_SIZE_BYTES (HUK_SIZE_WORDS * 4)

/** @brief Error value when the hardware unique key is missing */
#define HW_UNIQUE_KEY_ERR_MISSING               (0x16501)

/** @brief Error value when writing the hardware unique key failed */
#define HW_UNIQUE_KEY_ERR_WRITE_FAILED          (0x16502)

/** @brief Error value when the generation of the hardware unique key failed */
#define HW_UNIQUE_KEY_ERR_GENERATION_FAILED     (0x16503)

/** @brief Error value when key derivation using the hardware unique key failed */
#define HW_UNIQUE_KEY_ERR_DERIVE_FAILED         (0x16504)

/** @brief Error value when the hardware unique key had a generic failure */
#define HW_UNIQUE_KEY_ERR_GENERIC_ERROR         (0x16505)

/** @brief Return code for success */
#define HW_UNIQUE_KEY_SUCCESS                   (0x0)

/* The available slots. KDR is always available, while the MKEK and MEXT
 * keys are only stored when there is a KMU, since without a key, the key
 * store must be locked after booting, and the KDR is the only key that can
 * live in the CC HW for the entire boot cycle of the device.
 */
enum hw_unique_key_slot {
#ifndef HUK_HAS_KMU
	HUK_KEYSLOT_KDR  = 0, /* Device Root Key */
#else
	HUK_KEYSLOT_MKEK = 2, /* Master Key Encryption Key */
	HUK_KEYSLOT_MEXT = 4, /* Master External Storage Encryption Key */
#endif
};

#define KMU_SELECT_SLOT(KEYSLOT) (uint32_t)((KEYSLOT) + 1) /* NRF_KMU KEYSLOT are 1-indexed. */

/**
 * @brief Write a Hardware Unique Key to the KMU.
 *
 * @details This can only be done once (until a mass erase happens).
 *          This function waits for the flash operation to finish before returning.
 *
 * @param[in] key_slot  The keyslot to write to, see HUK_KEYSLOT_*.
 * @param[in] key       The key to write. Must be @ref HUK_SIZE_BYTES bytes long.
 *
 * @return HW_UNIQUE_KEY_SUCCESS on success, otherwise a negative hardware unique key error code
 */
int hw_unique_key_write(enum hw_unique_key_slot key_slot, const uint8_t *key);

/**
 * @brief Read random numbers from nrf_cc3xx_platform_ctr_drbg_get
 *        and write them to all slots with @ref hw_unique_key_write.
 *
 * @return HW_UNIQUE_KEY_SUCCESS on success, otherwise a negative hardware unique key error code
 */
int hw_unique_key_write_random(void);

/**
 * @brief Check whether a Hardware Unique Key has been written to the KMU.
 *
 * @param[in] key_slot  The keyslot to check, see HUK_KEYSLOT_*.
 *
 * @retval true   if a HUK has been written to the specified keyslot,
 * @retval false  otherwise.
 */
bool hw_unique_key_is_written(enum hw_unique_key_slot key_slot);

/**
 * @brief Check whether any Hardware Unique Keys are written to the KMU.
 *
 * @retval true   if one or more HUKs are written
 * @retval false  if all HUKs are unwritten.
 */
bool hw_unique_key_are_any_written(void);

/**
 * @brief Load the Hardware Unique Key (HUK) into the KDR registers of the
 *        Cryptocell.
 *
 * @details It also locks the flash page which contains the key material from
 *          reading and writing until the next reboot.
 *
 * @return HW_UNIQUE_KEY_SUCCESS on success, otherwise a negative hardware unique key error code
 */
int hw_unique_key_load_kdr(void);

/**
 * @brief Derive a key from the specified HUK, using the nrf_cc3xx_platform API on CryptoCell.
 *        Cracen PSA driver APIs on Cracen.
 *
 * See nrf_cc3xx_platform_kmu_shadow_key_derive() for more info.
 *
 * @param[in]  key_slot      Keyslot to derive from.
 * @param[in]  context       Context for key derivation.
 * @param[in]  context_size  Size of context.
 * @param[in]  label         Label for key derivation.
 * @param[in]  label_size    Size of label.
 * @param[out] output        The derived key.
 * @param[in]  output_size   Size of output.
 *
 * @return HW_UNIQUE_KEY_SUCCESS on success, otherwise a negative hardware unique key error code
 */
int hw_unique_key_derive_key(enum hw_unique_key_slot key_slot,
	const uint8_t *context, size_t context_size,
	uint8_t const *label, size_t label_size,
	uint8_t *output, uint32_t output_size);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* HW_UNIQUE_KEY_H_ */
