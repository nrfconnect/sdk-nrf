#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if(CONFIG_FPU)
  if(CONFIG_FP_HARDABI)
    set(float_dir hard-float)
  elseif(CONFIG_FP_SOFTABI)
    set(float_dir softfp-float)
  else()
    assert(0 "Unreachable code")
  endif()
else()
  set(float_dir soft-float)
endif()

assert(GCC_M_CPU "GCC_M_CPU must be set to find correct lib.")

set(lib_path lib/${GCC_M_CPU}/${float_dir})

set(LWM2M_CARRIER_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${lib_path})

if(NOT EXISTS ${LWM2M_CARRIER_LIB_PATH})
  message(WARNING "This combination of SoC and floating point ABI is not supported by liblwm2m_carrier."
                  "(${LWM2M_CARRIER_LIB_PATH} doesn't exist.)")
endif()

set(                    LWM2M_CARRIER_TARGET liblwm2m_carrier)
zephyr_library_import(${LWM2M_CARRIER_TARGET} ${LWM2M_CARRIER_LIB_PATH}/liblwm2m_carrier.a)
target_link_libraries(${LWM2M_CARRIER_TARGET} INTERFACE bsd_nrf9160_xxaa -lc)

zephyr_include_directories(include)
zephyr_library()
zephyr_library_sources(
  os/lwm2m_carrier.c
  os/lwm2m_os.c
)
