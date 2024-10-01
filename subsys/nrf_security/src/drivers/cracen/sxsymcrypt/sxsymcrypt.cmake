# TODO: NCSDK-19483: Only compile sources that are enabled in Kconfig
list(APPEND cracen_driver_sources
  ${CMAKE_CURRENT_LIST_DIR}/src/aead.c
  ${CMAKE_CURRENT_LIST_DIR}/src/blkcipher.c
  ${CMAKE_CURRENT_LIST_DIR}/src/chachapoly.c
  ${CMAKE_CURRENT_LIST_DIR}/src/cmac.c
  ${CMAKE_CURRENT_LIST_DIR}/src/cmdma.c
  ${CMAKE_CURRENT_LIST_DIR}/src/cmmask.c
  ${CMAKE_CURRENT_LIST_DIR}/src/hash.c
  ${CMAKE_CURRENT_LIST_DIR}/src/hmac.c
  ${CMAKE_CURRENT_LIST_DIR}/src/keyref.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mac.c
  ${CMAKE_CURRENT_LIST_DIR}/src/sha3.c
  ${CMAKE_CURRENT_LIST_DIR}/src/trng.c
  ${CMAKE_CURRENT_LIST_DIR}/src/platform/baremetal/cmdma_hw.c
  ${CMAKE_CURRENT_LIST_DIR}/src/platform/baremetal/interrupts.c
)

list(APPEND cracen_driver_include_dirs
    ${CMAKE_CURRENT_LIST_DIR}/include
)
