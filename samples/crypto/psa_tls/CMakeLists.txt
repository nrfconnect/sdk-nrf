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
  set(TLS_SRC_PATH "non-secure")
else()
  set(TLS_SRC_PATH "secure")
endif()

if(CONFIG_PSA_TLS_SAMPLE_TYPE_SERVER)
    target_sources(app PRIVATE src/${TLS_SRC_PATH}/psa_tls_credentials_server.c)
    if(CONFIG_NET_SOCKETS_ENABLE_DTLS)
        target_sources(app PRIVATE src/psa_dtls_functions_server.c)
    else()
        target_sources(app PRIVATE src/psa_tls_functions_server.c)
    endif()
else()
    target_sources(app PRIVATE src/${TLS_SRC_PATH}/psa_tls_credentials_client.c)
    if(CONFIG_NET_SOCKETS_ENABLE_DTLS)
        target_sources(app PRIVATE src/psa_dtls_functions_client.c)
    else()
        target_sources(app PRIVATE src/psa_tls_functions_client.c)
    endif()
endif()

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

foreach(inc_file
  ec-root-cert.der
	ec-echo-apps-cert.der
	ec-echo-apps-key.der
    )
  generate_inc_file_for_target(
    app
    certs/ecdsa/${inc_file}
    ${gen_dir}/${inc_file}.inc
    )
endforeach()

foreach(inc_file
  root-cert.der
	echo-apps-cert.der
	echo-apps-key.der
    )
  generate_inc_file_for_target(
    app
    certs/rsa/${inc_file}
    ${gen_dir}/${inc_file}.inc
    )
endforeach()
