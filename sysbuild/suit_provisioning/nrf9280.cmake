#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# List all manifest roles.
# The function internally checks if the enabling symbol is defined.
# If role is enabled, the function verifies if all required options are defined.

generate_mpi_hex(ROOT)
generate_mpi_hex(APP_RECOVERY)
generate_mpi_hex(APP_LOCAL_1)
generate_mpi_hex(APP_LOCAL_2)
generate_mpi_hex(APP_LOCAL_3)
generate_mpi_area(
  APP_AREA
  ROOT
  APP_RECOVERY
  APP_LOCAL_1
  APP_LOCAL_2
  APP_LOCAL_3
)

generate_mpi_hex(RAD_RECOVERY)
generate_mpi_hex(RAD_LOCAL_1)
generate_mpi_hex(RAD_LOCAL_2)
generate_mpi_area(
  RAD_AREA
  RAD_RECOVERY
  RAD_LOCAL_1
  RAD_LOCAL_2
)
