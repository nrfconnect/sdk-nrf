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
#ifndef FLASH_ENCRYPTION_H__
#define FLASH_ENCRYPTION_H__

#include <autoconf.h>

#ifdef CONFIG_TFM_ITS_ENCRYPTION

#include <stdint.h>

#ifdef TFM_HAL_FLASH_ENC_XTS
#include <mbedtls/aes.h>
typedef mbedtls_aes_xts_context fenc_ctx_t;

#else
typedef uint32_t fenc_ctx_t;

#endif

int32_t fenc_prepare(fenc_ctx_t *ctx);

int32_t fenc_encrypt(fenc_ctx_t *ctx, uint8_t *outdata, const uint8_t *indata, uint32_t dest_addr);

int32_t fenc_decrypt(fenc_ctx_t *ctx, uint8_t *outdata, const uint8_t *indata, uint32_t read_addr);

int32_t fenc_finalize(fenc_ctx_t *ctx);

#endif
#endif
