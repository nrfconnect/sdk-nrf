#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This file contains boards in fw-nrfconnect-nrf which has been replaced with
# a new board name.
# This allows the system to automatically change the board while at the same
# time prints a warning to the user, that the board name is deprecated.
#
# To add a board rename, add a line in following format:
# set(<old_board_name>_DEPRECATED <new_board_name>)

set(nrf52840_pca20035_DEPRECATED thingy91_nrf52840)
set(nrf9160_pca20035_DEPRECATED thingy91_nrf9160)
set(nrf9160_pca20035ns_DEPRECATED thingy91_nrf9160_ns)
set(thingy91_nrf9160ns_DEPRECATED thingy91_nrf9160_ns)

set(nrf52810dmouse_nrf52810_DEPRECATED nrf52810dmouse/nrf52810)
set(nrf52820dongle_nrf52820_DEPRECATED nrf52820dongle/nrf52820)
