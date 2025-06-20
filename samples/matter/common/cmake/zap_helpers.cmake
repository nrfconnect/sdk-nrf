#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(ncs_get_zap_parent_dir zap_parent_dir_out)
  string(CONFIGURE "${CONFIG_NCS_SAMPLE_MATTER_ZAP_FILE_PATH}" zap_file_path)
  cmake_path(GET zap_file_path PARENT_PATH zap_parent_dir)

  set(${zap_parent_dir_out} ${zap_parent_dir} PARENT_SCOPE)
endfunction()
