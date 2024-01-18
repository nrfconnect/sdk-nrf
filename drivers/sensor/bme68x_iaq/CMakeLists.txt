#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_include_directories(${ZEPHYR_BASE}/../modules/lib/bsec/src/inc)
zephyr_library_include_directories(${ZEPHYR_BASE}/../modules/lib/bme68x/src/bme68x)
zephyr_library_sources(${ZEPHYR_BASE}/../modules/lib/bme68x/src/bme68x/bme68x.c)

zephyr_library_compile_definitions_ifdef(CONFIG_BME68X_IAQ_SAMPLE_RATE_ULTRA_LOW_POWER
  BSEC_SAMPLE_RATE=BSEC_SAMPLE_RATE_ULP
  BSEC_GAS_SAMPLE_RATE=BSEC_SAMPLE_RATE_ULP
  BSEC_SAMPLE_PERIOD_S=300
)
zephyr_library_compile_definitions_ifdef(CONFIG_BME68X_IAQ_SAMPLE_RATE_LOW_POWER
  BSEC_SAMPLE_RATE=BSEC_SAMPLE_RATE_LP
  BSEC_GAS_SAMPLE_RATE=BSEC_SAMPLE_RATE_LP
  BSEC_SAMPLE_PERIOD_S=3
)
zephyr_library_compile_definitions_ifdef(CONFIG_BME68X_IAQ_SAMPLE_RATE_CONTINUOUS
  BSEC_SAMPLE_RATE=BSEC_SAMPLE_RATE_CONT
  BSEC_GAS_SAMPLE_RATE=BSEC_SAMPLE_RATE_CONT
  BSEC_SAMPLE_PERIOD_S=1
)
zephyr_library_compile_definitions_ifdef(BME68X_IAQ_SAMPLE_RATE_QUICK_ULTRA_LOW_POWER
  BSEC_SAMPLE_RATE=BSEC_SAMPLE_RATE_LP
  BSEC_GAS_SAMPLE_RATE=BSEC_SAMPLE_RATE_ULP
  BSEC_SAMPLE_PERIOD_S=3
)

if (CONFIG_FP_HARDABI)
  if (CONFIG_CPU_CORTEX_M33)
    zephyr_library_import(bsec_lib ${ZEPHYR_BASE}/../modules/lib/bsec/src/cortex-m33/fpv5-sp-d16-hard/libalgobsec.a)
  elseif(CONFIG_CPU_CORTEX_M4)
    zephyr_library_import(bsec_lib ${ZEPHYR_BASE}/../modules/lib/bsec/src/cortex-m4/fpv4-sp-d16-hard/libalgobsec.a)
  else()
    assert(0 "Unsupported configuration.")
  endif()
else()
  zephyr_library_compile_definitions(BME68X_DO_NOT_USE_FPU)
  if (CONFIG_CPU_CORTEX_M33 OR CONFIG_CPU_CORTEX_M4)
    zephyr_library_import(bsec_lib ${ZEPHYR_BASE}/../modules/lib/bsec/src/cortex-m4/libalgobsec.a)
  else()
    assert(0 "Unsupported configuration.")
  endif()
endif()
zephyr_library_sources(bme68x_iaq.c)
