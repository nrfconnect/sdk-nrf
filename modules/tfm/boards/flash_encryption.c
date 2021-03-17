/*
 * Copyright (c) 2021 Nordic Semiconductor ASA. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <autoconf.h>

#ifdef CONFIG_TFM_ITS_ENCRYPTION

#include "flash_encryption.h"
#include <string.h>
#include <stdint.h>
#include <flash_layout.h>

#ifdef TFM_HAL_FLASH_ENC_XTS
#include <mbedtls/aes.h>
#include <nrfx.h>

#define KEY_SIZE_BYTES 16
#define KEY_IDX 1
#define KEY_ADDR (&NRF_UICR->KEYSLOT.KEY[KEY_IDX])

static uint8_t data_unit[KEY_SIZE_BYTES] = {0};

int32_t fenc_prepare(fenc_ctx_t *ctx)
{
	mbedtls_aes_xts_init(ctx);
	return mbedtls_aes_xts_setkey_dec(ctx, KEY_ADDR, KEY_SIZE_BYTES * 2 * 8);
}

int32_t fenc_encrypt(fenc_ctx_t *ctx, uint8_t *outdata, const uint8_t *indata, uint32_t dest_addr)
{
	*(uint32_t *)data_unit = dest_addr;
	return mbedtls_aes_crypt_xts(ctx, MBEDTLS_AES_ENCRYPT, TFM_ENC_PROGRAM_UNIT, data_unit,
					indata, outdata);
}

int32_t fenc_decrypt(fenc_ctx_t *ctx, uint8_t *outdata, const uint8_t *indata, uint32_t read_addr)
{
	*(uint32_t *)data_unit = read_addr;
	return mbedtls_aes_crypt_xts(ctx, MBEDTLS_AES_DECRYPT, TFM_ENC_PROGRAM_UNIT, data_unit,
					indata, outdata);
}

int32_t fenc_finalize(fenc_ctx_t *ctx)
{
	return mbedtls_aes_xts_free(&xts_ctx);
}

#elif defined(TFM_HAL_FLASH_ENC_DUMMY)

int32_t fenc_prepare(fenc_ctx_t *ctx)
{
	return 0;
}

int32_t fenc_encrypt(fenc_ctx_t *ctx, uint8_t *outdata, const uint8_t *indata, uint32_t dest_addr)
{
	for (int i = 0; i < (TFM_ENC_PROGRAM_UNIT/4); i++) {
		((uint32_t*)outdata)[i] = ~(((uint32_t*)indata)[i]);
	}
	return 0;
}

int32_t fenc_decrypt(fenc_ctx_t *ctx, uint8_t *outdata, const uint8_t *indata, uint32_t read_addr)
{
	for (int i = 0; i < (TFM_ENC_PROGRAM_UNIT/4); i++) {
		((uint32_t*)outdata)[i] = ~(((uint32_t*)indata)[i]);
	}
	return 0;
}

int32_t fenc_finalize(fenc_ctx_t *ctx)
{
	return 0;
}

#endif

#endif
