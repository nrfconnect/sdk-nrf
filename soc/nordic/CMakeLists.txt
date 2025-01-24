# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library()

if(CONFIG_ARM)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "SoC Linker script")
endif()

zephyr_library_sources(
  ${ZEPHYR_BASE}/soc/nordic/validate_base_addresses.c
  ${ZEPHYR_BASE}/soc/nordic/validate_binding_headers.c
  ${ZEPHYR_BASE}/soc/nordic/validate_enabled_instances.c
)

# Include dt-bindings headers into the build. This lets us validate all required
# DT values against the MDK, without having to conditionally include different
# headers for different SoCs.
set(dt_binding_includes ${DTS_INCLUDE_FILES})
list(FILTER dt_binding_includes INCLUDE REGEX "/dt-bindings/.*\.h$")
list(TRANSFORM dt_binding_includes PREPEND "-include;")
set_source_files_properties(
  validate_binding_headers.c
  DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  PROPERTIES COMPILE_OPTIONS "${dt_binding_includes}"
)

if(CONFIG_SOC_HAS_TIMING_FUNCTIONS AND NOT CONFIG_BOARD_HAS_TIMING_FUNCTIONS)
  if(CONFIG_TIMING_FUNCTIONS)
    # Use nRF-specific timing calculations only if DWT is not present
    if(NOT CONFIG_CORTEX_M_DWT)
      zephyr_library_sources(${ZEPHYR_BASE}/soc/nordic/timing.c)
    endif()
  endif()
endif()

if(CONFIG_BUILD_WITH_TFM)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DHAL_NORDIC_PATH=${ZEPHYR_HAL_NORDIC_MODULE_DIR}
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DZEPHYR_BASE=${ZEPHYR_BASE}
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DNRF_NS_STORAGE=${CONFIG_TFM_NRF_NS_STORAGE}
  )
endif()

add_subdirectory(${SOC_SERIES})
include_directories(${ZEPHYR_BASE}/soc/nordic/common)
