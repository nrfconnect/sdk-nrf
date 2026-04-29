#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(dfu_multi_image_sample_dir ${ZEPHYR_NRF_MODULE_DIR}/samples/dfu/dfu_multi_image)

set(empty_net_core_EXTRA_ZEPHYR_MODULES
  ${dfu_multi_image_sample_dir}/modules/net_build_time_log
  CACHE INTERNAL ""
)
