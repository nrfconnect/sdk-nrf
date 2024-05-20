:orphan:

.. _suit_recovery:

SUIT recovery application
#########################

.. contents::
   :local:
   :depth: 2

The SUIT recovery application is a minimal application that allows recovering the device firmware if the original firmware is damaged.
It is to be used as a additional companion firmware to the main application using SUIT rather than a standalone application.

The application uses the SMP protocol via BLE and SUIT to perform the recovery.

Building
********

The recovery application needs to be compatible with the main application in terms of hardware configuration.
To achieve this, appropriate devicetree overlay files from the main application must be passed when building the recovery application as ``EXTRA_DTC_OVERLAY_FILE``
.
These devicetree also need to specify a partition for the recovery application.
See the overlay files for the smp_transfer sample as an example.

For example, to build the recovery application as a recovery image for the smp transfer sample, run:

``west build -p --sysbuild -b nrf54h20dk/nrf54h20/cpuapp -- -DEXTRA_DTC_OVERLAY_FILE='../smp_transfer/sysbuild/recovery.overlay' -Dhci_ipc_EXTRA_DTC_OVERLAY_FILE='<absolute_path_to_smp_transfer_sample>/sysbuild/recovery_hci_ipc.overlay'``

Flashing
********

Currently it is not possible to flash the application using ``west flash`` .
Instead, ``nrfutil`` should be used to flash the hex files for the application core image, radio core image and SUIT storage:

``nrfutil device program --options chip_erase_mode=ERASE_NONE --firmware build/recovery/zephyr/suit_installed_envelopes_application_merged.hex``

``nrfutil device program --options chip_erase_mode=ERASE_NONE --firmware build/recovery/zephyr/suit_installed_envelopes_radio_merged.hex``

``nrfutil device program --options chip_erase_mode=ERASE_NONE --firmware build/recovery/zephyr/zephyr.hex``

``nrfutil device program --options chip_erase_mode=ERASE_NONE --firmware build/hci_ipc/zephyr/zephyr.hex``

It is also possible to install recovery application by performing an update using the recovery application envelope found in ``DFU/application.suit``

Updating
********

Note that in order to update the recovery application the ``DFU/application.suit`` file found in the recovery application build directory should be used.
This is different than in case of the main application, where usually ``DFU/root.suit`` should be used.
