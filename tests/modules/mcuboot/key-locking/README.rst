.. _mcuboot_key_locking_test:

MCUboot KMU key lock test
##################

.. contents::
   :local:
   :depth: 2

The test verifies whether KMU key lock, performed by MCUboot, prevents application from key destruction and/or usage.

Requirements
************

Test is expected to be run as an application that MCUboot boots.
The board needs to have, at least, the first MCUboot dedicated ED25519 slot of KMU provisioned with a valid key.
The tests support the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf54l15dk_nrf54l15_cpuapp

Building and running
********************

The test requires no additional options and by default builds with MCUboot as bootloader, generating ready to be flashed binary in merged.hex.
Once KMU has been populated, the test can be programmed to a device.
