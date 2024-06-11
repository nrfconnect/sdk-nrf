.. _suit_recovery:

SUIT recovery application
#########################

.. contents::
   :local:
   :depth: 2

The SUIT recovery application is a minimal application that allows recovering the device firmware if the original firmware is damaged.
It is to be used as a companion firmware to the main application that is using :ref:`Software Update for Internet of Things (SUIT) <ug_nrf54h20_suit_intro>` procedure, rather than a stand-alone application.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf54h20dk_nrf54h20_cpuapp

Overview
********

The application uses the SMP protocol through Bluetooth® Low Energy and SUIT to perform the recovery.
It is optimized for memory usage and only contains the basic necessary functionalities.

.. caution::

    This firmware is only able to recover from a situation where the application or radio core are damaged. It does not recover from Nordic Semiconductor-controlled firmware failures.

Configuration
**************

As the recovery firmware is a companion image, it  must be compatible with the main application in terms of hardware configuration (especially the memory partitions).
To achieve this, the appropriate devicetree overlay files from the main application must be passed to the recovery application.

To do this, add the :file:`recovery.overlay` and :file:`recovery_hci_ipc.ovelay` files in the main application's Sysbuild directory.
The former file will be passed automatically to the recovery application image and the latter to the recovery radio image.
These devicetree files must define the ``cpuapp_recovery_partition`` and ``cpurad_recovery_partition`` nodes respectively.`
For an example, see the files in the ``samples/suit/smp_transfer`` sample.

Building and running
********************

In standard applications, the recovery firmware is built as part of the main application build.

Apart from creating the mentioned :file:`recovery.overlay` and :file:`recovery_hci_ipc.ovelay` files,
the ``SB_CONFIG_SUIT_BUILD_RECOVERY`` Sysbuild configuration option must be set in the main application.
This will cause the recovery firmware to be built automatically as part of the main application build.

For example, to build the ``smp_transfer`` sample with the recovery firmware, run the following command:

``west build -b nrf54h20dk/nrf54h20/cpuapp -- -DFILE_SUFFIX=bt -DSB_CONFIG_SUIT_BUILD_RECOVERY=y``

Flashing
========

No additional steps are needed to flash the recovery firmware together with the main application firmware, simply run the following:

.. code-block::

   ``west flash``


The recovery firmware will be flashed automatically from the main application directory.

Testing
=======

|test_sample|

#. |connect_kit|
#. Corrupt the currently running main application (for example by flashing a modified version of the application)
#. Open the Device Manager or the nRF Connect application, and observe the device advertising as "SUIT Recovery"
#. Recover the application using Device Manager in the same way as described in the ``smp_transfer`` sample documentation.

Device firmware update for recovery firmware
============================================

To update the recovery firmware, perform a SUIT firmware update using the SUIT envelope found in :file:`<main_application_build_directory>/recovery/src/recovery-build/DFU/application.suit`.

See the ``smp_transfer`` sample documentation to see how to perform the update using the Device Manager application.
