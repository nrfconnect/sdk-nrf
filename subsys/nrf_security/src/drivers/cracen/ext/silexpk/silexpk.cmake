#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# TODO: NCSDK-19483: Conditionally add sources based on Kconfig
list(APPEND cracen_driver_sources

  ${CMAKE_CURRENT_LIST_DIR}/interface/sxbuf/sxbufops.c
  ${CMAKE_CURRENT_LIST_DIR}/src/blind.c
  ${CMAKE_CURRENT_LIST_DIR}/src/ed448.c
  ${CMAKE_CURRENT_LIST_DIR}/src/ed25519.c
  ${CMAKE_CURRENT_LIST_DIR}/src/iomem.c
  ${CMAKE_CURRENT_LIST_DIR}/src/montgomery.c
  ${CMAKE_CURRENT_LIST_DIR}/src/statuscodes.c

  ${CMAKE_CURRENT_LIST_DIR}/target/baremetal_ba414e_with_ik/pk_baremetal.c

  ${CMAKE_CURRENT_LIST_DIR}/target/hw/ik/ikhardware.c
  ${CMAKE_CURRENT_LIST_DIR}/target/hw/ik/cmddefs_ik.c

  ${CMAKE_CURRENT_LIST_DIR}/target/hw/ba414/pkhardware_ba414e.c
  ${CMAKE_CURRENT_LIST_DIR}/target/hw/ba414/ba414_status.c
  ${CMAKE_CURRENT_LIST_DIR}/target/hw/ba414/ec_curves.c
  ${CMAKE_CURRENT_LIST_DIR}/target/hw/ba414/cmddefs_ecc.c
  ${CMAKE_CURRENT_LIST_DIR}/target/hw/ba414/cmddefs_modmath.c
  ${CMAKE_CURRENT_LIST_DIR}/target/hw/ba414/cmddefs_ecjpake.c
  ${CMAKE_CURRENT_LIST_DIR}/target/hw/ba414/cmddefs_srp.c
)

# We need internal.h to be on path
list(APPEND cracen_driver_include_dirs
  ${CMAKE_CURRENT_LIST_DIR}/target/baremetal_ba414e_with_ik
  ${CMAKE_CURRENT_LIST_DIR}/include
)
