#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE .)
target_sources_ifdef(CONFIG_EXTERNAL_SENSORS app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ext_sensors.c)

if (CONFIG_EXTERNAL_SENSORS_BME680_BSEC)
        target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ext_sensors_bsec.c)
        set(bsec_dir ${CONFIG_EXTERNAL_SENSORS_BME680_BSEC_PATH})
        set(BSEC_LIB_DIR ${bsec_dir}/algo/normal_version/bin/gcc/Cortex_M33F)

        if(NOT EXISTS "${BSEC_LIB_DIR}/libalgobsec.a")
                assert(0 "Could not find BSEC library. Minimum version is 1.4.8.0_Generic_Release")
        endif()

        target_include_directories(app PRIVATE ${bsec_dir}/examples/bsec_iot_example)
        target_include_directories(app PRIVATE ${BSEC_LIB_DIR})

        target_sources(app PRIVATE ${bsec_dir}/examples/bsec_iot_example/bsec_integration.c)
        target_sources(app PRIVATE ${bsec_dir}/examples/bsec_iot_example/bme680.c)

        add_library(bsec_lib STATIC IMPORTED GLOBAL)
        add_dependencies(bsec_lib math_lib bsec_target)

        set_target_properties(bsec_lib PROPERTIES IMPORTED_LOCATION "${BSEC_LIB_DIR}/libalgobsec.a")
        set_target_properties(bsec_lib PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${BSEC_LIB_DIR})

        target_link_libraries(bsec_lib INTERFACE -L${LIBC_LIBRARY_DIR})
        target_link_libraries(app PUBLIC bsec_lib)
endif()
