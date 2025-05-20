#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(suit_service.c)
zephyr_library_sources(suit_update.c)
zephyr_library_sources(suit_mci.c)
zephyr_library_sources(suit_invoke.c)
zephyr_library_sources_ifdef(CONFIG_SUIT_PROCESSOR suit_auth.c)
zephyr_library_sources_ifdef(CONFIG_SUIT_MANIFEST_VARIABLES suit_mfst_var.c)
zephyr_library_sources_ifdef(CONFIG_SUIT_IPUC suit_ipuc.c)

zephyr_library_link_libraries(suit_utils)
zephyr_library_link_libraries_ifdef(CONFIG_SUIT_MANIFEST_VARIABLES suit_manifest_variables)
zephyr_library_link_libraries_ifdef(CONFIG_SUIT_IPUC suit_ipuc)

if(CONFIG_SUIT_STREAM_SOURCE_IPC)
  zephyr_library_link_libraries(suit_stream_sources_interface)
  zephyr_library_link_libraries(suit_stream_sinks_interface)
endif()

generate_and_add_cbor_files(suit_service.cddl zcbor_generated
  suit_req
  suit_rsp
  suit_nfy
)
