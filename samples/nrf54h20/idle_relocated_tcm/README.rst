.. _idle_relocated_tcm_sample:

Multicore idle test with firmware relocated to radio core TCM
#############################################################

.. contents::
   :local:
   :depth: 2

The test benchmarks the idle behavior of an application that runs on multiple cores.
It demonstrates a radio loader pattern where the radio core firmware is loaded from MRAM into Tightly Coupled Memory (TCM) at runtime.

Requirements
************

The test supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf54h20dk_nrf54h20_cpuapp

Overview
********

This test demonstrates how to build a multicore idle application with :ref:`configuration_system_overview_sysbuild` using a two-stage boot process for the radio core:

* Radio Loader - A small bootloader that runs on the radio core, copies firmware from MRAM to TCM, and jumps to it.
* Remote Firmware - The actual application that runs from TCM after being loaded.

The test automatically relocates the remote firmware binary to the correct MRAM address during build time, ensuring it can be loaded by the radio loader.

Architecture
============

The system uses the memory layout compatible with the MCUboot bootloader.
By default, the system uses the MCUboot in the direct-xip mode in the merged slot configuration (no file suffix used).
To configure the system to use the MCUboot in the direct-xip mode in the split slot configuration, build the test with the ``split_slot`` file suffix.

Memory map configuration
========================

The memory maps are defined in the :file:`common` directory and are shared between all images to ensure consistency.

The following changes are included in the project overlay files:

.. tabs::

   .. group-tab:: Merged slot configuration

      1. Entire Radio TCM RAM region is used for the remote firmware (:file:`common/memory_map_ram_cpurad.dtsi`).
      #. A small part of Global RAM is used for the radio loader for the firmware relocation time (:file:`common/memory_map.dtsi`).
      #. The MRAM is partitioned for the MCUboot, application, and radio core images (:file:`common/memory_map.dtsi`).

   .. group-tab:: Split slot configuration

      1. Entire Radio TCM RAM region is used for the remote firmware (:file:`common/memory_map_ram_cpurad.dtsi`).
      #. A small part of Global RAM is used for the radio loader for the firmware relocation time (:file:`common/memory_map_split_slot.dtsi`).
      #. The MRAM is partitioned for the MCUboot, application, and radio core images (:file:`common/memory_map_split_slot.dtsi`).
      #. The ``pm_ramfunc`` DTS node is relocated to the end of the RAM block but before the MCUboot trailer (:file:`common/memory_map_ram_pm_cpurad.dtsi`) to make the image link properly.

Enabling the Radio Loader
*************************

The radio loader is automatically added to the build when you enable it in sysbuild configuration.

Set the :kconfig:option:`SB_CONFIG_NRF_RADIO_LOADER` Kconfig option to ``y`` in the :file:`sysbuild.conf` file.
This single configuration option automatically adds the ``radio_loader`` application located in the :file:`nrf/samples/nrf54h20/radio_loader` folder and builds it for the CPURAD core.
No manual ``ExternalZephyrProject_Add()`` is needed in sysbuild.

You must provide the DTS overlay file for the radio loader that includes the following DTS node labels:

* ``cpurad_slot0_partition`` - The primary slot of the radio loader code.
* ``cpurad_slot1_partition`` - The secondary slot of the radio loader code.
* ``cpurad_slot2_partition`` - The primary slot of the remote firmware code to be relocated to TCM.
* ``cpurad_slot3_partition`` - The secondary slot of the remote firmware code to be relocated to TCM.
* ``cpurad_ram0`` - The radio core TCM RAM region to be used for the remote firmware.

Automatic firmware relocation
*****************************

The remote firmware is linked against the target memory partition (TCM) and must be relocated to match the MRAM partition address where it will be initially stored.
This is automatically done by the :kconfig:option:`CONFIG_BUILD_OUTPUT_ADJUST_LMA` Kconfig option when the devicetree chosen nodes are configured correctly.

How it works
============

Firmware relocation is handled automatically by Zephyr's build system using the :kconfig:option:`CONFIG_BUILD_OUTPUT_ADJUST_LMA` Kconfig option, which is configured in the :file:`nrf/soc/nordic/nrf54h/Kconfig.defconfig.nrf54h20_cpurad` file for all nRF54H20 CPURAD projects.

The configuration automatically detects if the :kconfig:option:`CONFIG_XIP` is enabled in the remote firmware project.
When this option is enabled, the remote firmware runs directly from the ``zephyr,code-partition`` location and no relocation is needed.
When this option is disabled, the remote firmware will run from the ``zephyr,sram`` location but the image is first stored in the MRAM in the ``zephyr,code-partition`` location.
The build system calculates the LMA adjustment to relocate the firmware from TCM to MRAM.

Zephyr automatically calculates the Load Memory Address (LMA) adjustment based on the chosen nodes:

.. code-block:: text

   LMA_adjustment = zephyr,code-partition address - zephyr,sram address

The build system adjusts the hex file so that the code is loaded from MRAM (``0xe000000`` address space), but runs from TCM (``0x23000000`` address space).

Configuration of remote radio firmware
======================================

To use the radio loader pattern, you must configure the remote radio firmware in the following way:

* Disable the :kconfig:option:`CONFIG_XIP` option in the remote firmware project.
* Assign the TCM RAM region to the remote firmware code using the ``zephyr,sram`` chosen node.
* Assign the MRAM partition to the remote firmware code using the ``zephyr,code-partition`` chosen node.

Building and running
********************

.. |test path| replace:: :file:`samples/nrf54h20/idle_relocated_tcm`

.. include:: /includes/build_and_run_test.txt

Testing
=======

After programming the test to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Reset the kit.
#. Observe the console output for both cores:

   .. tabs::

      .. group-tab:: Merged slot configuration

         * For the application core, the output should be as follows:

         .. code-block:: console

            *** Booting MCUboot v2.3.0-dev-0d9411f5dda3 ***
            *** Using nRF Connect SDK v3.1.99-329079d0237c ***
            *** Using Zephyr OS v4.2.99-56fbb4f3c7bb ***
            I: Starting Direct-XIP bootloader
            I: Primary slot: version=0.0.0+0
            I: Image 0 Secondary slot: Image not found
            I: Image 0 loaded from the primary slot
            I: Bootloader chainload address offset: 0x40000
            I: Image version: v0.0.0
            I: Jumping to the image slot
            *** Booting nRF Connect SDK v3.1.99-329079d0237c ***
            *** Using Zephyr OS v4.2.99-56fbb4f3c7bb ***
            [00:00:00.336,735] <inf> idle: Multicore idle test on nrf54h20dk@0.9.0/nrf54h20/cpuapp
            [00:00:00.336,742] <inf> idle: build time: Dec  2 2025 08:52:18
            [00:00:00.336,745] <inf> idle: Multicore idle test iteration 0
            [00:00:02.336,834] <inf> idle: Multicore idle test iteration 1
            ...

         * For the radio core, the output should be as follows:

         .. code-block:: console

            *** Booting nRF Connect SDK v3.1.99-329079d0237c ***
            *** Using Zephyr OS v4.2.99-56fbb4f3c7bb ***
            [00:00:00.512,002] <inf> idle: Multicore idle test on nrf54h20dk@0.9.0/nrf54h20/cpurad
            [00:00:00.512,004] <inf> idle: Original code partition start address: 0xe094000
            [00:00:00.512,006] <inf> idle: Running from the SRAM, start address: 0x23000000, size: 0x30000
            [00:00:00.512,007] <inf> idle: Current PC (program counter) address: 0x23000a90
            [00:00:00.512,009] <inf> idle: build time: Dec  2 2025 08:52:24
            [00:00:00.512,012] <inf> idle: Multicore idle test iteration 0
            [00:00:02.512,072] <inf> idle: Multicore idle test iteration 1
            ...

      .. group-tab:: Split slot configuration

         * For the application core, the output should be as follows:

         .. code-block:: console

            *** Booting MCUboot v2.3.0-dev-0d9411f5dda3 ***
            *** Using nRF Connect SDK v3.1.99-b9ff3eff5eb7 ***
            *** Using Zephyr OS v4.2.99-56fbb4f3c7bb ***
            I: Starting Direct-XIP bootloader
            I: Primary slot: version=0.0.0+0
            I: Image 0 Secondary slot: Image not found
            I: Primary slot: version=0.0.0+0
            I: Image 1 Secondary slot: Image not found
            I: Primary slot: version=0.0.0+0
            I: Image 2 Secondary slot: Image not found
            I: Loading image 0 from slot 0
            I: bootutil_img_validate: slot 0 manifest verified
            I: Try to validate images using manifest in slot 0
            I: Loading image 1 from slot 0
            I: Loading image 2 from slot 0
            I: Image 0 loaded from the primary slot
            I: Image 1 loaded from the primary slot
            I: Image 2 loaded from the primary slot
            I: Bootloader chainload address offset: 0x40000
            I: Image version: v0.0.0
            I: Jumping to the image slot
            *** Booting nRF Connect SDK v3.1.99-b9ff3eff5eb7 ***
            *** Using Zephyr OS v4.2.99-56fbb4f3c7bb ***
            [00:00:00.116,189] <inf> idle: Multicore idle test on nrf54h20dk@0.9.0/nrf54h20/cpuapp
            [00:00:00.116,196] <inf> idle: build time: Dec  2 2025 09:01:59
            [00:00:00.116,198] <inf> idle: Multicore idle test iteration 0
            [00:00:02.116,271] <inf> idle: Multicore idle test iteration 1
            ...

         * For the radio core, the output should be as follows:

         .. code-block:: console

            *** Booting nRF Connect SDK v3.1.99-b9ff3eff5eb7 ***
            *** Using Zephyr OS v4.2.99-56fbb4f3c7bb ***
            [00:00:00.291,381] <inf> idle: Multicore idle test on nrf54h20dk@0.9.0/nrf54h20/cpurad
            [00:00:00.291,383] <inf> idle: Original code partition start address: 0xe096000
            [00:00:00.291,385] <inf> idle: Running from the SRAM, start address: 0x23000000, size: 0x30000
            [00:00:00.291,386] <inf> idle: Current PC (program counter) address: 0x23001290
            [00:00:00.291,388] <inf> idle: build time: Dec  2 2025 09:01:44
            [00:00:00.291,391] <inf> idle: Multicore idle test iteration 0
            [00:00:02.291,443] <inf> idle: Multicore idle test iteration 1
            ...

   The radio loader first loads the firmware from MRAM to TCM and then jumps to the loaded firmware.
   This process is transparent and happens during the early boot stage.

#. Verify the DFU process:

   a. Build the firmware for the secondary app slot, increase the version number in the :file:`prj.conf` file (uncomment the line):

      .. code-block:: kconfig

         CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION="0.0.1+0"

   #. Build the firmware:

      .. tabs::

         .. group-tab:: Merged slot configuration

            .. code-block:: console

               west build -p -b nrf54h20dk/nrf54h20/cpuapp

         .. group-tab:: Split slot configuration

            .. code-block:: console

               west build -p -b nrf54h20dk/nrf54h20/cpuapp -- -DFILE_SUFFIX=split_slot

   #. Program the firmware to the secondary application slot:

      .. tabs::

         .. group-tab:: Merged slot configuration

            .. code-block:: console

               nrfutil device program --firmware build/zephyr_secondary_app.merged.hex  --options chip_erase_mode=ERASE_NONE

         .. group-tab:: Split slot configuration

            .. code-block:: console

               nrfutil device program --firmware build/mcuboot_secondary_app/zephyr/zephyr.signed.hex  --options chip_erase_mode=ERASE_NONE
               nrfutil device program --firmware build/radio_loader_secondary_app/zephyr/zephyr.signed.hex  --options chip_erase_mode=ERASE_NONE
               nrfutil device program --firmware build/remote_rad_secondary_app/zephyr/zephyr.signed.hex  --options chip_erase_mode=ERASE_NONE

   #. Reset the development kit.

      .. code-block:: console

         nrfutil device reset

   #. Observe the following changes in the console output:

      * The application core boots from the secondary application slot.
      * The radio core boots from the secondary radio slot.
      * The build time changes.
