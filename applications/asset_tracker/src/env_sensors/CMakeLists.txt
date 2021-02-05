#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(.)

if (CONFIG_USE_BME680_BSEC)
	if(CONFIG_FP_HARDABI)
		target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bsec.c)
		set(bsec_dir ${CONFIG_BOSCH_BSEC_LIBRARY_PATH})
		set(BSEC_LIB_DIR     ${bsec_dir}/algo/normal_version/bin/gcc/Cortex_M33F)
		set(BSEC_INCLUDE_DIR ${bsec_dir}/algo/normal_version/bin/gcc/Cortex_M33F)
		if(NOT EXISTS "${BSEC_LIB_DIR}/libalgobsec.a")
			assert(0 "Could not find BSEC library. Minimum version is 1.4.8.0_Generic_Release")
		endif()
		target_include_directories(app PRIVATE ${bsec_dir}/examples/bsec_iot_example)
		target_include_directories(app PRIVATE ${BSEC_INCLUDE_DIR})
		target_sources(app PRIVATE ${bsec_dir}/examples/bsec_iot_example/bsec_integration.c)
		target_sources(app PRIVATE ${bsec_dir}/examples/bsec_iot_example/bme680.c)

		add_library(bsec_lib STATIC IMPORTED GLOBAL)
		add_dependencies(bsec_lib math_lib bsec_target)
		set_target_properties(bsec_lib PROPERTIES IMPORTED_LOCATION "${BSEC_LIB_DIR}/libalgobsec.a")
		set_target_properties(bsec_lib PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${BSEC_INCLUDE_DIR})
		target_link_libraries(bsec_lib INTERFACE -L${LIBC_LIBRARY_DIR})
	else()
		assert(0 "CONFIG_FP_HARDABI must be set")
	endif()

else()
	target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/env_sensors.c)
endif()
