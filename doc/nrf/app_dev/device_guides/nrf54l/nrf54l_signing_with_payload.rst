.. _nRF54l_signing_app_with_flpr_payload:

Signing applications with integrated FLPR payload
#################################################

.. contents::
   :local:
   :depth: 2

The nRF54L SoCs include a FLPR CPU designed to handle fast communication with external devices independently, enhancing flexibility in protocol implementation.
For details, see :ref:`vpr_flpr_nrf54l`.

You can dynamically load and modify the FLPR code during runtime.
This guide describes how to build the application and FLPR cores, creating a single file, signed and managed by MCUboot, with FLPR integrated and executable from the application slot.

Requirements and limitations
****************************

MCUboot requires the application and the FLPR application to be concatenated and signed together to treat them as a single image.
To do this, you have to build the main application (CPUAPP application) and the FLPR application separately, manually merge them, and sign them for MCUboot.

In addition to these requirements, there are also limitations caused by sysbuild.
The FLPR application cannot be built as part of the main sysbuild process.
Instead, the CPUAPP application can be built with MCUboot integration, while the FLPR must be built separately and then merged.
For details, see :ref:`building_nrf54l`.
Note that the DTS file for the CPUAPP application must reserve a required area for the FLPR binary.

Using Partition Manager
=======================

For :ref:`partition_manager`, applications can only use static definitions.
In the current version of the |NCS|, the CPUAPP application and the FLPR application use different partitioning definitions.
The CPUAPP application follows the Partition Manager's scheme, while FLPR relies on DTS partitioning and disregards the Partition Manager scheme.
For this reason, partitions defined for FLPR in DTS must also be reserved in the Partition Manager to ensure consistency.
In addition, FLPR partition parameters such as size and offset must be aligned in the DTS definition, the FLPR application project, and the Partition Manager scheme of the CPUAPP project.

Placement of the FLPR binary on storage devices is flexible, however, the loading mechanism must be aware of its starting point, which is specified in the DTS file.
You must ensure that the FLPR partition in both the DTS and Partition Manager leaves sufficient space at the end for MCUboot swap information, which is necessary to update the application image.
The build system will not check if the compiled application satisfies the size requirements (see the NCSDK-20567 issue on the :ref:`known_issues` page).

Creating a project with FLPR core
*********************************

Each project (CPUAPP application and the FLPR application) can independently enable sysbuild.
The CPUAPP application, operating on the application core, requires the integration of MCUboot as the bootloader to ensure proper boot management and security.
The FLPR application operates independently from the main project structure, allowing it to be built separately.

Configuring memory partitions
*****************************

When the FLPR application is built, partitioning must be done through DTS.
This is necessary because the Partition Manager does not currently support or recognize the FLPR application.

When building the main application that incorporates FLPR, ensure that the Partition Manager is enabled.
Your setup must reserve a partition for the FLPR that reflects the layout defined by the FLPR application.
For details on configuring partitions, see the following sections.

DTS memory definitions for FLPR application
===========================================

Define FLPR partitions for the FLPR application as follows, using example addresses:

.. code-block:: dts

      &cpuflpr_rram {
            reg = <0x98000 0x20000>;
   };

This configuration sets up a partition starting at address ``0x98000`` with a size of ``0x20000``.

DTS memory definitions for CPUAPP application
=============================================

To ensure the CPUAPP application functions correctly with the FLPR payload, you must apply specific configurations.

Memory partition configuration
------------------------------

See an example of memory partition configuration:

.. code-block:: dts

      /{
         soc {
                  reserved-memory {
                           #address-cells = <1>;
                           #size-cells = <1>;
                           cpuflpr_code_partition: image@98000 {
                                 /* FLPR core code partition */
                           reg = <0x98000 0x20000>;
                           };
                  };
                  cpuflpr_sram_code_data: memory@20028000 {
                           compatible = "mmio-sram";
                           reg = <0x20028000 DT_SIZE_K(96)>;
                           #address-cells = <1>;
                           #size-cells = <1>;
                           ranges = <0x0 0x20028000 0x18000>;
                  };
         };
   };
   &cpuapp_sram {
         reg = <0x20000000 DT_SIZE_K(160)>;
         ranges = <0x0 0x20000000 0x28000>;
   };

1. Adjust the following partitions:

   * ``cpuflpr_code_partition`` - This partition defines where the FLPR code is placed on the storage device.
     The code is later loaded by the CPUAPP application.

      * Location: 0x98000
      * Size: 0x20000

   Ensure the ``cpuflpr_code_partition`` does not extend to the end of the image slot.
   You must leave space for the MCUboot swap information.

   .. note::

      When using the Partition Manager, ensure this partition is reserved for FLPR.
      The Partition Manager ignores this setting while allocating space for the application running on CPUAPP, but the :ref:`FLPR minimal sample <vpr_flpr_nrf54l_initiating>` will still use it.

   * ``cpuflpr_sram_code_data`` - This configuration defines the RAM space reserved only for the FLPR.
     It is not available for the application core and should be excluded from its memory allocation.

      * Compatible with ``mmio-sram``.
      * Location: 0x20028000
      * Size: 96KB

   * ``cpuapp_sram`` - This configuration defines the RAM limit for the CPUAPP application.

      * Location: 0x20000000
      * Size: 160KB

#. Configure the CPUAPP application as follows for it to recognize the placement of the FLPR code:

   .. code-block:: dts

      &cpuflpr_vpr {
            execution-memory = <&cpuflpr_sram_code_data>;
            source-memory = <&cpuflpr_code_partition>;
      };

   where

   * The ``execution-memory`` links SRAM definitions to FLPR (``cpuflpr_sram_code_data``).
   * The ``source-memory`` links RRAM definitions to FLPR (``cpuflpr_code_partition``).

   These links inform the main application about the memory usage by the FLPR core.

Partition Manager configuration
-------------------------------

Adjust the static definitions in the Partition Manager as follows:

* For the FLPR partition (``flpr0``):

   .. code-block:: dts

      flpr0:
        address: 0x98000
        end_address: 0xb8000
        region: flash_primary
        size: 0x20000

  The ``address`` and ``size`` must match the DTS definitions.
  The ``end_address`` is the sum of the address and size.

* For the ``mcuboot_primary_app`` configuration:

   .. code-block:: dts

      mcuboot_primary_app:
        address: 0xc800
        end_address: 0xb8000
        orig_span: &id002
        - app
        - flpr0
        region: flash_primary
        size: 0xab800
        span: *id002

  This configuration indicates that ``flpr0`` is now set within ``mcuboot_primary_app``, meaning it is part of the image.

Building project
****************

For detailed instructions on how to build your project, see :ref:`building_nrf54l`.

Creating a single image
***********************

After the build is complete, you must manually collect the artifacts into a single image for MCUboot to work correctly.

Merging binaries
================

Once you have successfully built the CPUAPP and FLPR applications separately and using sysbuild, you can merge the files.

1. Locate the :file:`zephyr.hex` files for both applications:

   * For CPUAPP, find the :file:`build/<cpuapp_application_dir_name>/zephyr/zephyr.hex` file.
   * For FLPR, find the :file:`<flpr_build>/zephyr/zephyr.hex` file.

#. If you do not have your environment set up, first source the :file:`zephyr_env.sh` script or ensure you have set the ``ZEPHYR_BASE`` environmental variable to the Zephyr directory used for building.

#. Execute the merge command:

   .. code-block:: console

      python3 ${ZEPHYR_BASE}/scripts/build/mergehex.py <flpr_build>/zephyr/zephyr.hex build/<cpuapp_application>/zephyr/zephyr.hex -o app_and_flpr_merged.hex

   It results in creating the :file:`app_and_flpr_merged.hex` file that contains both the FLPR and CPUAPP application.

   .. note::

      Merging errors, where memory locations overlap in both HEX files, indicate misalignment between DTS and Partition Manager definitions for FLPR and CPUAPP. This suggests that FLPR is built in an area already allocated to the application.
      To resolve this issue, you must check and adjust the settings in either the Partition Manager, the DTS, or both, to ensure that the FLPR-designated area does not overlap with the application-designated area.

Signing binaries
================

For MCUboot, the merged HEX file (:file:`app_and_flpr_merged.hex`) is a single application that must be signed.

1. Calculate the slot size by using the ``mcuboot_primary_app`` configuration.
   The ``0xac000`` comes from the following calculation: ``0xab800 + 0x800 = 0xac000``, where ``0xab800`` represents the total size of the primary application area, and ``0x800`` is added to accommodate the metadata required by MCUboot, resulting in a total ``slot_size`` of ``0xac000``.

#. Using the :doc:`imgtool<mcuboot:imgtool>`, sign the merged application.

   .. code-block:: console

      python3 ${ZEPHYR_BASE}/bootloader/mcuboot/scripts/imgtool.py sign --version <version> --align 16 --slot-size 0xac000 --pad-header --header-size 0x800 -k <key> app_and_flpr_merged.hex app_and_flpr_merged.signed.hex
      python3 ${ZEPHYR_BASE}/bootloader/mcuboot/scripts/imgtool.py sign --version <version> --align 16 --slot-size 0xac000 --pad-header --header-size 0x800 -k <key> app_and_flpr_merged.hex app_and_flpr_merged.signed.bin

   Adjust the following values in the script:

   * Replace ``0xac000`` in the command line with the calculated value.
   * Replace ``<version>`` with the application version and ``<key>`` with the :ref:`signature key <ug_bootloader_adding_sysbuild_immutable_mcuboot_keys>`.

   At the end of this process, you will have two files:

   * The :file:`app_and_flpr_merged.signed.hex` file, that is an application you can :ref:`program directly to the device<gs_programming>`.
   * The :file:`app_and_flpr_merged.signed.bin` file, that is an application that can be :ref:`uploaded as an update<ug_nrf54l_developing_ble_fota>`.
