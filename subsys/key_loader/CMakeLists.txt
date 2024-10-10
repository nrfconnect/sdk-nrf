# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library_named(key_loader)

zephyr_library_sources(key_loader.c)

set(base_dir ${CONFIG_NRF_KEY_LOADER_BASE_PATH})
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

set(required_keys
  MANIFEST_PUBKEY_APPLICATION_GEN1
  MANIFEST_PUBKEY_APPLICATION_GEN2
  MANIFEST_PUBKEY_APPLICATION_GEN3
  FWENC_APPLICATION_GEN1
  FWENC_APPLICATION_GEN2
  MANIFEST_PUBKEY_RADIO_GEN1
  MANIFEST_PUBKEY_RADIO_GEN2
  MANIFEST_PUBKEY_RADIO_GEN3
  FWENC_RADIO_GEN1
  FWENC_RADIO_GEN2
  MANIFEST_PUBKEY_OEM_ROOT_GEN1
  MANIFEST_PUBKEY_OEM_ROOT_GEN2
  MANIFEST_PUBKEY_OEM_ROOT_GEN3
  )

foreach (key_name ${required_keys} )
  generate_inc_file_for_target(
    key_loader
    ${CONFIG_NRF_KEY_LOADER_BASE_PATH}/${key_name}
    ${gen_dir}/${key_name})
endforeach()
