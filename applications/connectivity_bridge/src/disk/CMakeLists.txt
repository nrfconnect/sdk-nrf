#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

# Write the generated files into the include/generated directory, which
# is already in the system path
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
generate_inc_file_for_target(app thingy91_cdc_acm.cat ${gen_dir}/thingy91_cdc_acm.cat.inc)
generate_inc_file_for_target(app thingy91_cdc_acm.inf ${gen_dir}/thingy91_cdc_acm.inf.inc)

target_sources_ifdef(
		     CONFIG_BRIDGE_MSC_ENABLE
		     app PRIVATE
		     ${CMAKE_CURRENT_SOURCE_DIR}/readme.c
		     ${CMAKE_CURRENT_SOURCE_DIR}/config.c
		     ${CMAKE_CURRENT_SOURCE_DIR}/thingy91_cdc_acm.cat.c
		     ${CMAKE_CURRENT_SOURCE_DIR}/thingy91_cdc_acm.inf.c)
