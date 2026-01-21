#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_MCUBOOT_EXTRA_IMAGES)
  include(${ZEPHYR_NRF_MODULE_DIR}/cmake/dfu_extra.cmake)

  set(ext_img1_path "${CMAKE_BINARY_DIR}/ext_img1.bin")
  set(ext_img2_path "${CMAKE_BINARY_DIR}/ext_img2.bin")

  file(WRITE ${ext_img1_path} "EXTRA_IMAGE_1_CONTENT_FOR_TESTING")
  file(WRITE ${ext_img2_path} "EXTRA_IMAGE_2_CONTENT_FOR_TESTING")

  dfu_extra_add_binary(
    BINARY_PATH ${ext_img1_path}
    NAME "ext_img1"
  )

  dfu_extra_add_binary(
    BINARY_PATH ${ext_img2_path}
    NAME "ext_img2"
    VERSION "1.0.0+1"
    DEPENDS ${ext_img2_path}
  )

  # Verification of dfu_extra module

  set(dfu_multi_image "${CMAKE_BINARY_DIR}/dfu_multi_image.bin")
  set(dfu_zip "${CMAKE_BINARY_DIR}/dfu_application.zip")

  add_custom_target(verify_dfu_packages ALL
    COMMAND sh -c "\
      echo '=== DFU Package Verification ===' && \
      test -f ${CMAKE_BINARY_DIR}/dfu_application.zip || (echo 'ERROR: Missing ZIP' && exit 1) && \
      test -f ${CMAKE_BINARY_DIR}/dfu_multi_image.bin || (echo 'ERROR: Missing BIN' && exit 1) && \
      grep -q '\"image_index\": \"0\"' ${CMAKE_BINARY_DIR}/dfu_application.zip_manifest.json || (echo 'ERROR: Missing image 0' && exit 1) && \
      grep -q '\"image_index\": \"1\"' ${CMAKE_BINARY_DIR}/dfu_application.zip_manifest.json || (echo 'ERROR: Missing image 1' && exit 1) && \
      grep -q '\"image_index\": \"2\"' ${CMAKE_BINARY_DIR}/dfu_application.zip_manifest.json || (echo 'ERROR: Missing image 2' && exit 1) && \
      grep -q '\"file\": \"ext_img1.signed.bin\"' ${CMAKE_BINARY_DIR}/dfu_application.zip_manifest.json || (echo 'ERROR: Missing extra_img1.signed.bin in manifest' && exit 1) && \
      grep -q '\"file\": \"ext_img2.signed.bin\"' ${CMAKE_BINARY_DIR}/dfu_application.zip_manifest.json || (echo 'ERROR: Missing extra_img2.signed.bin in manifest' && exit 1) && \
      test $(grep -c -- '--image' ${CMAKE_BINARY_DIR}/dfu_multi_image.bin.args) -eq 3 || (echo 'ERROR: Expected 3 images in args' && exit 1) && \
      grep -q 'ext_img1.signed.bin' ${CMAKE_BINARY_DIR}/dfu_multi_image.bin.args || (echo 'ERROR: Missing extra_img1.signed.bin path in args' && exit 1) && \
      grep -q 'ext_img2.signed.bin' ${CMAKE_BINARY_DIR}/dfu_multi_image.bin.args || (echo 'ERROR: Missing extra_img2.signed.bin path in args' && exit 1) && \
      echo 'Result: PASS'"
    DEPENDS ${dfu_multi_image} ${dfu_zip}
    COMMENT "Verifying DFU packages"
    VERBATIM
  )
endif()
