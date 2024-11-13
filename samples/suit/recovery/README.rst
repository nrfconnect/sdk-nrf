.. _suit_recovery:

SUIT: Recovery application
##########################

.. contents::
   :local:
   :depth: 2

The SUIT recovery application is a minimal application that allows recovering the device firmware if the original firmware is damaged.
It is to be used as a companion firmware to the main application that is using :ref:`Software Update for Internet of Things (SUIT) <ug_nrf54h20_suit_intro>` procedure, rather than a stand-alone application.

The following limitations apply to this application:

* The recovery firmware is only able to recover from a situation where the application or radio core are damaged.
  It does not recover from Nordic Semiconductor-controlled firmware failures.
* The recovery firmware is not compatible with a main application that uses an external flash for update.

.. _suit_recovery_reqs:

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf54h20dk_nrf54h20_cpuapp

.. _suit_recovery_overview:

Overview
********

The application uses the SMP protocol through Bluetooth® Low Energy and SUIT to perform the recovery.
It is optimized for memory usage and only contains the basic necessary functionalities.

.. caution::

    This firmware is only able to recover from a situation where the application or radio core are damaged. It does not recover from Nordic Semiconductor-controlled firmware failures.

.. _suit_recovery_config:

Configuration
*************

|config|

As the recovery firmware is a companion image, it must be compatible with the main application in terms of hardware configuration (especially the memory partitions).
To achieve this, the appropriate devicetree overlay files from the main application must be passed to the recovery application.

To do this, add the :file:`recovery.overlay` and :file:`recovery_hci_ipc.ovelay` files in the main application's :ref:`configuration_system_overview_sysbuild` directory.
The former file will be passed automatically to the recovery application image and the latter to the recovery radio image.
These devicetree files must define the ``cpuapp_recovery_partition`` and ``cpurad_recovery_partition`` nodes respectively.`
For an example, see the files in the ``samples/suit/smp_transfer`` sample.

.. _suit_recovery_build_run:

Building and running
********************

.. |sample path| replace:: :file:`samples/suit/recovery`

This sample can be found under |sample path| in the |NCS| folder structure.

Including recovery application image with sysbuild
==================================================

In standard applications, the recovery firmware is built as part of the main application build.

Apart from creating the mentioned :file:`recovery.overlay` and :file:`recovery_hci_ipc.ovelay` files,
you must set the ``SB_CONFIG_SUIT_BUILD_RECOVERY`` sysbuild configuration option in the main application.
This will cause the recovery firmware to be built automatically as part of the main application build.

To build the main application, follow the instructions in :ref:`building` for your preferred building environment.

.. note::
    |sysbuild_autoenabled_ncs|

For example, to build the :ref:`Device firmware update on the nRF54H20 SoC <nrf54h_suit_sample>` sample with the recovery firmware on the command line, you can run the following command:

.. code-block:: console

   west build -b nrf54h20dk/nrf54h20/cpuapp -- -DFILE_SUFFIX=bt -DSB_CONFIG_SUIT_BUILD_RECOVERY=y

The recovery firmware will be flashed automatically from the main application directory.

See also :ref:`programming` for programming steps and :ref:`testing` for general information about testing and debugging in the |NCS|.

Testing
=======

|test_sample|

#. |connect_kit|
#. Corrupt the currently running main application.
   This can be done by overwriting the beginning of the application partition:

.. code-block:: console

   nrfutil device x-write --address 0xE0A5000 --value 0xFFFF --serial-port COM7

#. Reset the device to enter the recovery path:

.. code-block:: console

   nrfutil device reset --serial-port COM7

#. Open the nRF Device Manager app on your mobile phone, and observe the device now advertising itself as *SUIT Recovery*.
#. Recover the application using the nRF Device Manager app to perform an update, following the process outlined in the :ref:`nrf54h_suit_sample` sample documentation.
   Use the :file:`root.suit` file generated from the previous build.
#. After the update completes, the device will reset and launch the application.
#. Open the nRF Device Manager app on your mobile phone and observe that the device is now advertising as *SUIT SMP Sample*.

Device firmware update for recovery firmware
============================================

To update the recovery firmware, perform a SUIT firmware update using the SUIT envelope found in :file:`<main_application_build_directory>/DFU/app_recovery.suit`.
For updating using the Device Manager application, you can also use the zip file found in :file:`<main_application_build_directory>/zephyr/dfu_suit_recovery.zip`.

See :ref:`nrf54h_suit_sample` to see how to perform the update using the Device Manager application.
