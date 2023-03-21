#!/usr/bin/env python3
#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import cryptography.hazmat.primitives.cmac as cmac
import cryptography.hazmat.primitives.ciphers.algorithms as alg
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.hkdf import HKDF

# The label used in the function tfm_plat_get_huk.
common_huk_label = b"TFM_HW_UNIQ_KEY\0"

# The function tfm_builtin_key_loader_get_key_buffer
# uses the owner ID as label when the HUK is being
# used for key derivation.
label_owner_id = b"\xff\xff\xff\xff"

key_nrf53 = [
    0xc5, 0xa8, 0x08, 0xeb, 0xe3, 0x1e, 0xa5, 0xb4,
    0xe9, 0x44, 0x1a, 0x76, 0x45, 0x58, 0xd8, 0x8b,
    0x40, 0x26, 0x33, 0xa8, 0xcd, 0x2d, 0x51, 0x67,
    0x8d, 0xef, 0x00, 0x24, 0x30, 0x52, 0xd7, 0x3d]

label_53 = [
    0xe2, 0x75, 0xab, 0x25, 0x00, 0x3b, 0x15, 0xe1,
    0xa1, 0x98, 0x78, 0x5b, 0x4e, 0x57, 0x43, 0x9a]

key_nrf91 = [
    0x19, 0x9a, 0xe3, 0xc7, 0x9d, 0xd0, 0x16, 0x8c,
    0x3e, 0xee, 0xa8, 0x46, 0xea, 0x4e, 0xdc, 0x6e]

label_91 = [
    0x29, 0xd6, 0xa6, 0x8d, 0x3f, 0xfb, 0x39, 0x1f,
    0x03, 0x6d, 0xd5, 0x3b, 0xe3, 0x54, 0xe4, 0x02,
    0x9a, 0x69, 0x5a]

context_53 = []
context_91 = []

# The fixed numbers that are included in the update process come from
# the KDF function we use (Special Publication 800-108).
# In CryptoCell the implementation of this KDF function with a KMU key is
# in the function mbedtls_shadow_key_derive.
cmac_53 = cmac.CMAC(alg.AES(bytes(key_nrf53)), backend=None)
cmac_53.update(b'\x01' + bytes(common_huk_label) + b'\x00' + bytes(context_53) + b'\x01\x00')
derived_huk_53 = cmac_53.finalize()

cmac_53_2 = cmac.CMAC(alg.AES(bytes(key_nrf53)), backend=None)
cmac_53_2.update(b'\x02' + bytes(common_huk_label) + b'\x00' + bytes(context_53) + b'\x01\x00')
derived_huk_53 += cmac_53_2.finalize()

# This is equivalent HKDF derivation in tfm_builtin_key_loader_get_key_buffer
hkdf = HKDF(
    algorithm=hashes.SHA256(),
    length=32,
    salt=None,
    info=label_owner_id,
)
tfm_key_from_huk_53 = hkdf.derive(derived_huk_53)

# This is the final derivation happening in the test code
hkdf = HKDF(
    algorithm=hashes.SHA256(),
    length=32,
    salt=None,
    info=bytes(label_53),
)
final_key_53 = hkdf.derive(tfm_key_from_huk_53)

print("NRF5340 key to be copied to the testing code:")
print("===================================================")
print("static uint8_t expected_key[" + str(len(final_key_53)) + "] = {")
print(", ".join(hex(c) for c in final_key_53[0:len(final_key_53)]))
print("};")
print("===================================================")


cmac_91 = cmac.CMAC(alg.AES(bytes(key_nrf91)), backend=None)
cmac_91.update(b'\x01' + bytes(common_huk_label) + b'\x00' + bytes(context_91) + b'\x01\x00')
derived_huk_91 = cmac_91.finalize()

hkdf = HKDF(
    algorithm=hashes.SHA256(),
    length=16,
    salt=None,
    info=label_owner_id,
)
tfm_key_from_huk_91 = hkdf.derive(derived_huk_91)

hkdf = HKDF(
    algorithm=hashes.SHA256(),
    length=16,
    salt=None,
    info=bytes(label_91),
)
final_key_91 = hkdf.derive(tfm_key_from_huk_91)

print("NRF9160 key to be copied to the testing code:")
print("===================================================")
print("static uint8_t expected_key[" + str(len(final_key_91)) + "] = {")
print(", ".join(hex(c) for c in final_key_91[0:len(final_key_91)]))
print("};")
print("===================================================")
