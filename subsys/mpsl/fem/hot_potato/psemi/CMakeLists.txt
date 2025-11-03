set(LIB_SRCS)

if (NOT ((${NRF_PLATFORM} STREQUAL "NRF_PLATFORM_HALTIUM")))
  list(APPEND LIB_SRCS
    mpsl_fem_psemi_nrf54l.c
    mpsl_fem_psemi_common.c)
else()
  list(APPEND LIB_SRCS
    mpsl_fem_psemi_unsupported.c
  )
endif()

create_fem_library (
    NAME psemi
    SOURCES ${LIB_SRCS}
  )
