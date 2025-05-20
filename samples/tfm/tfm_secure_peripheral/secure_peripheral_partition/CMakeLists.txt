#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_library(tfm_app_rot_partition_spp STATIC)

target_include_directories(tfm_app_rot_partition_spp
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        ${CMAKE_BINARY_DIR}/secure_peripheral_partition
)
target_include_directories(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/secure_peripheral_partition
)

target_sources(tfm_app_rot_partition_spp
    PRIVATE
        secure_peripheral_partition.c
        util.c
)

target_include_directories(tfm_app_rot_partition_spp
    PUBLIC
        .
        ${HAL_NORDIC_PATH}/nrfx
        ${HAL_NORDIC_PATH}/nrfx/mdk
        ${HAL_NORDIC_PATH}/nrfx/drivers/include
)

# The generated sources
target_sources(tfm_app_rot_partition_spp
    PRIVATE
        ${CMAKE_BINARY_DIR}/secure_peripheral_partition/auto_generated/intermedia_tfm_secure_peripheral_partition.c
)
target_sources(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/secure_peripheral_partition/auto_generated/load_info_tfm_secure_peripheral_partition.c
)

target_link_libraries(tfm_app_rot_partition_spp
    PRIVATE
        platform_s
        tfm_sprt
)

target_link_libraries(tfm_spm
    PRIVATE
        tfm_app_rot_partition_spp
)

############################ Partition Defs ####################################

target_link_libraries(tfm_partitions
    INTERFACE
        tfm_app_rot_partition_spp
)

target_compile_definitions(tfm_config
    INTERFACE
        TFM_PARTITION_SECURE_PERIPHERAL_PARTITION
)
