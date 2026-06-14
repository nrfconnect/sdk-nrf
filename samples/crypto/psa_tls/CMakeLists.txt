#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(psa_tls_sample)

zephyr_include_directories(src ${COMMON_DIR})
target_include_directories(app PUBLIC ./include)
target_sources(app PRIVATE src/main.c
                           src/psa_tls_common.c)

if(CONFIG_BUILD_WITH_TFM)
  set(tls_src_path "non-secure")
else()
  set(tls_src_path "secure")
endif()

if(CONFIG_SAMPLE_PSA_TLS_TYPE_SERVER)
  target_sources(app PRIVATE src/${tls_src_path}/psa_tls_credentials_server.c)
  if(CONFIG_NET_SOCKETS_ENABLE_DTLS)
    target_sources(app PRIVATE src/psa_dtls_functions_server.c)
  else()
    target_sources(app PRIVATE src/psa_tls_functions_server.c)
  endif()
else()
  target_sources(app PRIVATE src/${tls_src_path}/psa_tls_credentials_client.c)
  if(CONFIG_NET_SOCKETS_ENABLE_DTLS)
    target_sources(app PRIVATE src/psa_dtls_functions_client.c)
  else()
    target_sources(app PRIVATE src/psa_tls_functions_client.c)
  endif()
endif()

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

if(CONFIG_SAMPLE_PSA_TLS_CERTIFICATE_TYPE_ECDSA)
  set(ctype "ec")
  if(CONFIG_PSA_WANT_ECC_SECP_R1_384)
    set(variant "-secp384r1")
  else()
    set(variant "-secp256r1")
  endif()
else()
  set(ctype "rsa")
  # This sample is configured to use RSA pkcs1v1.5 certificates for TLS 1.2,
  # and RSA-PSS certificates for TLS 1.3.
  if(CONFIG_MBEDTLS_SSL_PROTO_TLS1_3 AND CONFIG_PSA_WANT_ALG_RSA_PSS)
    set(variant "-pss")
  else()
    set(variant "-pkcs1v15")
  endif()
endif()

foreach(inc_file
  root-cert
  server-cert
  server-cert-key
)
  set(modified_inc_file "${ctype}-${inc_file}${variant}.der")
  generate_inc_file_for_target(
    app
    certs/${modified_inc_file}
    ${gen_dir}/${inc_file}.der.inc
  )
endforeach()
