.. _nc_bootloader:

nRF5340: Network core bootloader
################################

.. contents::
   :local:
   :depth: 2

The network core bootloader sample, also called B0n, is an immutable first-stage bootloader that can update the application firmware on the network core of the nRF5340 System on Chip (SoC).

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The network core bootloader sample protects the flash memory areas allocated to both itself and the application running on the network core.

You must use this sample as a child image of a :ref:`multi-image <ug_multi_image>` build, where MCUboot is enabled and there is a network core application.
MCUboot verifies and shares with the network core bootloader any new network core application image received through a device firmware update (DFU) transport layer, like a serial or a Bluetooth® LE connection.
For this reason, without MCUboot, this sample does nothing else but directly launches the application.

During the boot process, the network core bootloader sample and MCUboot interact as follows:

1. When MCUboot receives a new network core application image, it writes the configuration for the network core bootloader to the shared SRAM.

   The configuration includes an update instruction and both the SHA and the data of the image.

#. The network core bootloader locks the flash memory areas containing itself and its configuration.

   The ACL peripheral is used for locking.
   For details on locking, see the :ref:`fprotect_readme` driver.

#. The network core bootloader performs any pending network core firmware update.

   It calls the :ref:`subsys_pcd` library to inspect the SRAM region shared with the application core:

   a. If MCUboot has written an update instruction, the network core bootloader copies the specified data range to the application partition on the network core.
   #. Once the copy is done, the network core bootloader compares the SHA of the data in the application partition against the SHA specified in the shared SRAM.
   #. It then communicates the result of the comparison to MCUboot using the shared SRAM.

#. The network core bootloader then locks the flash memory areas containing the network core application.

   The ACL peripheral is used for locking.
   For details on locking, see the :ref:`fprotect_readme` driver.

#. The network core bootloader boots the application on the network core.

   After performing a potential firmware update and enabling flash memory protection, the network core bootloader uninitializes all peripherals that it used and boots the application.

.. _net_bootloader_build_and_run:

Configuration
*************

|config|

Configuration options
=====================

To set the minimum partitioning size, use the Kconfig option :kconfig:option:`CONFIG_NETBOOT_MIN_PARTITION_SIZE`.

Building and running
********************

This sample can be found under :file:`samples/nrf5340/netboot/` in the |NCS| folder structure.

Follow the steps below to include the sample as a child image in a :ref:`multi-image <ug_multi_image>` build that contains a network core application:

#. To add MCUboot to the build, enable the :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` option in the application that runs on the application core.
   The build system includes the sample in the build by automatically enabling the :kconfig:option:`CONFIG_SECURE_BOOT` option for the application that runs on the network core.
#. To enable the :ref:`subsys_pcd` library for MCUboot, set the :kconfig:option:`CONFIG_PCD_APP` option when building its image.

The build system generates a new set of firmware update files.
These files match the ones described in :ref:`mcuboot:mcuboot_ncs`, except that they contain the network core application firmware and are prefixed with ``net_core_``.

See :ref:`configure_application` for information on how to enable the required configuration options.
Then follow the instructions in :ref:`ug_nrf5340_building` to build and program the images for the network and application core.

.. note::
   To try out the network core bootloader sample, use the :ref:`peripheral_uart` sample as the basis for the multi-image build.
   This sample automatically includes the network core sample :zephyr:code-sample:`bluetooth_hci_ipc` when built for the nRF5340 DK.
   Then apply the options mentioned to include the network core bootloader sample with MCUboot.

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal_specific|

   .. note::
      The nRF5340 DK has multiple UART instances, so the correct port must be identified.
      See :ref:`logging_cpunet` for additional details.

#. Reset the kit.
#. Observe the following lines in the console output:

   .. code-block:: console

      I: Starting bootloader
      I: Primary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
      I: Secondary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
      I: Boot source: none
      I: Swap type: none
      I: Bootloader chainload address offset: 0xc000
      I: Jumping to the first image slot
      *** Booting Zephyr OS build v2.7.99-ncs1-2195-g186cf4539e5a  ***

#. Program the network core update image using nrfjprog:

   .. code-block:: console

      nrfjprog --program zephyr/net_core_app_moved_test_update.hex --sectorerase

   .. note::
      Typically, the update image is received through serial interface or Bluetooth.
      For testing purposes, use nrfjprog to program the update image directly into the update slot.


#. Reset the kit.
#. Observe that the output includes the following lines indicating that the MCUBoot in the application core has read the update image and performed a firmware update of the network core:

   .. code-block:: console

      I: Starting network core update
      I: Turned on network core
      I: Turned off network core
      I: Done updating network core
      I: Bootloader chainload address offset: 0xc000
      I: Jumping to the first image slot
      *** Booting Zephyr OS build v2.7.99-ncs1-2195-g186cf4539e5a  ***

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`doc_fw_info`
* :ref:`fprotect_readme`
* ``include/bl_validation.h``
* ``include/bl_crypto.h``
* ``subsys/bootloader/include/provision.h``

The sample also uses drivers from nrfx.
