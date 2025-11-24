.. _idle_relocated_tcm_sample:

Multicore idle test with firmware relocated to radio core TCM
#############################################################

.. contents::
   :local:
   :depth: 2

The test benchmarks the idle behavior of an application that runs on multiple cores.
It demonstrates a radio loader pattern where the radio core firmware is loaded from MRAM into TCM (Tightly Coupled Memory) at runtime.

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

The system uses the following memory layout:

* **MRAM (Non-volatile):**

  * ``cpurad_loader_partition`` @ 0x92000 - Contains the radio loader (8 KB)
  * ``cpurad_loaded_fw`` @ 0x94000 - Contains the remote firmware binary (128 KB)

* **TCM (Volatile, fast execution):**

  * ``cpurad_ram0`` @ 0x23000000 - Code execution region (128 KB)
  * ``cpurad_data_ram`` @ 0x23020000 - Data region (64 KB)

Additional files
================

The test comes with the following additional files:

* :file:`sysbuild.conf` - Enables the radio loader by setting ``CONFIG_NRF_RADIO_LOADER=y``.
* :file:`boards/memory_map.overlay` - Shared memory map configuration for both loader and remote firmware.
* :file:`sysbuild/radio_loader/` - Radio loader configuration overrides (:file:`prj.conf`, overlay).
* :file:`sysbuild/remote_rad/` - Radio core firmware configuration overrides (:file:`prj.conf`, overlay).

Enabling the Radio Loader
*************************

The radio loader is automatically added to the build when you enable it in sysbuild configuration.

In :file:`sysbuild.conf`:

.. code-block:: kconfig

   SB_CONFIG_NRF_RADIO_LOADER=y

This single configuration option:

#. Automatically adds the ``radio_loader`` application located in the :file:`nrf/samples/nrf54h20/radio_loader` folder.
#. Builds it for the CPURAD core.
#. No manual ``ExternalZephyrProject_Add()`` needed in sysbuild.

Memory map configuration
========================

The memory map is defined in :file:`boards/memory_map.overlay` and is shared between the radio loader and remote firmware to ensure consistency.

The overlay defines:

#. TCM regions:

   .. code-block:: devicetree

      cpurad_ram0: sram@23000000 {
          compatible = "mmio-sram";
          reg = <0x23000000 0x20000>;  /* 128 KB for code */
      };
      cpurad_data_ram: sram@23020000 {
          compatible = "mmio-sram";
          reg = <0x23020000 0x10000>;  /* 64 KB for data */
      };

#. MRAM partitions:

   .. code-block:: devicetree

      &mram1x {
          /delete-node/ partitions;

          partitions {
              compatible = "fixed-partitions";
              #address-cells = <1>;
              #size-cells = <1>;

              cpurad_loader_partition: partition@92000 {
                  label = "cpurad_loader_partition";
                  reg = <0x92000 DT_SIZE_K(8)>;  /* 8 KB allocated (~4 KB actual) */
              };

              cpurad_loaded_fw: partition@94000 {
                  label = "cpurad_loaded_fw";
                  reg = <0x94000 DT_SIZE_K(128)>;  /* 128 KB fixed */
              };
          };
      };

Automatic firmware relocation
*****************************

The remote firmware must be relocated to match the MRAM partition address where it will be stored.
This is automatically done by Zephyr's ``CONFIG_BUILD_OUTPUT_ADJUST_LMA`` feature when the devicetree chosen nodes are configured correctly.

How it works
============

Firmware relocation is handled automatically by Zephyr's build system using the ``CONFIG_BUILD_OUTPUT_ADJUST_LMA`` configuration option, which is configured in ``zephyr/soc/nordic/nrf54h/Kconfig.defconfig.nrf54h20_cpurad`` for all nRF54H20 CPURAD projects.

The configuration automatically detects the ``fw-to-relocate`` chosen node in your devicetree.
When present, it calculates the LMA adjustment to relocate firmware from MRAM to TCM.
Without this chosen node, firmware runs directly from the ``zephyr,code-partition`` location (standard XIP behavior).

Simply configure the devicetree chosen nodes correctly in your firmware's overlay:

.. code-block:: devicetree

   /{
       chosen {
           /* VMA: where code runs (TCM) */
           zephyr,code-partition = &cpurad_ram0;
           zephyr,sram = &cpurad_data_ram;

           /* LMA: where to load from (MRAM partition) - enables relocation */
           fw-to-relocate = &cpurad_loaded_fw;
       };
   };

Zephyr automatically calculates the Load Memory Address (LMA) adjustment based on your chosen nodes:

**With fw-to-relocate chosen node** (for radio loader pattern):

.. code-block:: text

   LMA_adjustment = fw-to-relocate address - zephyr,code-partition address
                  = cpurad_loaded_fw - cpurad_ram0
                  = 0x94000 - 0x23000000

**Without fw-to-relocate** (standard behavior):

.. code-block:: text

   LMA_adjustment = zephyr,code-partition address - zephyr,sram address

The build system then adjusts the hex file so that the firmware is loaded from MRAM (``0x94000``), but runs from TCM (``0x23000000``).

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

   * For the application core, the output should be as follows:

     .. code-block:: console

       *** Booting nRF Connect SDK zephyr-v3.5.0-3517-g9458a1aaf744 ***
       build time: Nov 22 2025 17:00:59
       Multicore idle test on nrf54h20dk@0.9.0/nrf54h20/cpuapp
       Multicore idle test iteration 0
       Multicore idle test iteration 1
       ...

   * For the radio core, the output should be as follows:

     .. code-block:: console

       *** Booting nRF Connect SDK zephyr-v3.5.0-3517-g9458a1aaf744 ***
       build time: Nov 22 2025 17:00:29
       Multicore idle test on nrf54h20dk@0.9.0/nrf54h20/cpurad
       Current PC (program counter) address: 0x23000ae0
       Multicore idle test iteration 0
       Multicore idle test iteration 1
       ...

   The radio loader first loads the firmware from MRAM (``0x0e094000``) to TCM (``0x23000000``) and then jumps to the loaded firmware.
   This process is transparent and happens during the early boot stage.

#. Verify the DFU process:

   #. Build the firmware for the secondary app slot, increase the version number in the :file:`prj.conf` file (uncomment the line):

      .. code-block:: kconfig

         CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION="0.0.1+0"

   #. Build the firmware:

      .. code-block:: console

         west build -p -b nrf54h20dk/nrf54h20/cpuapp

   #. Program the firmware to the secondary application slot:

      .. code-block:: console

         nrfutil device program --firmware build/zephyr_secondary_app.merged.hex  --options chip_erase_mode=ERASE_NONE

   Reset the development kit.
   The firmware must boot from the secondary application slot.
   Observe the change in build time in the console output.
