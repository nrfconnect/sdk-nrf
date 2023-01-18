#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE .)
target_sources_ifdef(CONFIG_EXTERNAL_SENSORS app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ext_sensors.c)

if (CONFIG_EXTERNAL_SENSORS_BME680_BSEC)
        target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ext_sensors_bsec.c)
        if (CONFIG_FP_SOFTABI)
                set(BSEC_LIB_DIR ${ZEPHYR_BASE}/../modules/lib/bsec2/src/cortex-m4)
		zephyr_compile_definitions(BME68X_DO_NOT_USE_FPU)
        endif()
        if (CONFIG_FP_HARDABI)
                set(BSEC_LIB_DIR ${ZEPHYR_BASE}/../modules/lib/bsec2/src/cortex-m33/fpv5-sp-d16-hard)
        endif()

        if(NOT EXISTS "${BSEC_LIB_DIR}/libalgobsec.a")
                assert(0 "Could not find BSEC library.")
        endif()

        target_include_directories(app PRIVATE ${ZEPHYR_BASE}/../modules/lib/bme68x/src/bme68x)
        target_include_directories(app PRIVATE ${ZEPHYR_BASE}/../modules/lib/bsec2/src/inc/)

        target_sources(app PRIVATE ${ZEPHYR_BASE}/../modules/lib/bme68x/src/bme68x/bme68x.c)

        add_library(bsec_lib STATIC IMPORTED GLOBAL)
        add_dependencies(bsec_lib math_lib bsec_target)

        set_target_properties(bsec_lib PROPERTIES IMPORTED_LOCATION "${BSEC_LIB_DIR}/libalgobsec.a")
        set_target_properties(bsec_lib PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${BSEC_LIB_DIR})

        target_link_libraries(bsec_lib INTERFACE -L${LIBC_LIBRARY_DIR})
        target_link_libraries(app PUBLIC bsec_lib)
endif()
