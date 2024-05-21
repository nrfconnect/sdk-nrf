#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# High-level Crypto API library
list(APPEND cracen_driver_sources
  ${CMAKE_CURRENT_LIST_DIR}/src/coprime_check.c
  ${CMAKE_CURRENT_LIST_DIR}/src/ecc.c
  ${CMAKE_CURRENT_LIST_DIR}/src/ecdsa.c
  ${CMAKE_CURRENT_LIST_DIR}/src/ed25519.c
  ${CMAKE_CURRENT_LIST_DIR}/src/ed448.c
  ${CMAKE_CURRENT_LIST_DIR}/src/hash.c
  ${CMAKE_CURRENT_LIST_DIR}/src/hkdf.c
  ${CMAKE_CURRENT_LIST_DIR}/src/hmac.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mem.c
  ${CMAKE_CURRENT_LIST_DIR}/src/montgomery.c
  ${CMAKE_CURRENT_LIST_DIR}/src/pbkdf2.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rndinrange.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rsa_keygen.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rsaes_oaep.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rsaes_pkcs1v15.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rsamgf1xor.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rsapss.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rsassa_pkcs1v15.c
  ${CMAKE_CURRENT_LIST_DIR}/src/sicrypto.c
  ${CMAKE_CURRENT_LIST_DIR}/src/sig.c
  ${CMAKE_CURRENT_LIST_DIR}/src/util.c
  # IKG sources
  ${CMAKE_CURRENT_LIST_DIR}/src/iksig.c
)

list(APPEND cracen_driver_include_dirs
    ${CMAKE_CURRENT_LIST_DIR}/include
)
