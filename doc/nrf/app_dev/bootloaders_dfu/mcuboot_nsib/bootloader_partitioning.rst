.. _bootloader_partitioning:

Partitioning device memory
##########################

.. contents::
   :local:
   :depth: 2

Partitioning device memory is a crucial aspect of managing how a device's storage is utilized, especially when dealing with firmware updates and bootloader configurations.
|NCS| follows Zephyr's devicetree-based (DTS) flash partitioning: partition layout is defined in devicetree and resolved at build time using Flash Map API macros and a run-time accessible flash map of partitions that bootloaders and applications use for flash device access.
For scenarios involving DFU, read the following sections.

.. include:: ../../../includes/pm_deprecation.txt

.. _bootloader_partitioning_partitions_file:

Partition map
*************

The partition map describes how non-volatile memory is split into regions such as bootloaders, image slots, storage, and provisioning data.

Partitions are declared under a flash (or other non-volatile) device node in devicetree, typically as child nodes of a ``partitions`` node.
Each partition uses the ``compatible = "fixed-partitions"`` scheme or, on many Nordic boards, ``compatible = "zephyr,mapped-partition"``.
Use ``compatible = "zephyr,mapped-partition"`` for partitions on the main internal non-volatile memory that share the SoC address map (typical Nordic board layouts with ``ranges`` on the parent ``partitions`` node).
Use ``compatible = "fixed-partitions"`` on a separate flash device node when that memory is independent—for example, partitions on external QSPI NOR under ``&mx25r64`` or similar.
Use the node labels expected by the bootloader chain (see :ref:`ug_bootloader_flash`).

Where to define partitions
==========================

There are several methods by which partition node definitions may be provided for the build, for example:

* The board devicetree (``*.dts``) for your target
* Board-specific devicetree overlays in the application (``boards/<board>.overlay``)
* Project-level overlays (``<app>.overlay`` or files included from there)
* Shared include files (``*.dtsi``) included from overlays

On a single-core project, one overlay is usually enough.
In a :ref:`sysbuild <configuration_system_overview_sysbuild>` project, each image in the build (application, MCUboot, |NSIB|, network-core firmware, and other sysbuild images) is built separately.
Each image has its own devicetree and its own effective partition map after overlays are applied.
You must ensure that every image that participates in DFU or boot sees **exactly the same** layout: matching addresses, sizes, and node labels for the regions that image uses.
Apply partition overlays to the application and to each sysbuild image that must know about those regions (for example, ``sysbuild/mcuboot/boards/<board>.overlay`` for MCUboot).

When you are migrating from Partition Manager, consider reviewing information provided in :ref:`migration_partitions`.

.. _bootloader_partitioning_partition_inspection:
.. _bootloader_partitioning_partitions_file_report:

Partition inspection
====================

Devicetree-based partitioning does not provide a dedicated partition report analogous to the Partition Manager ``partition_manager_report`` target.
After building, inspect the resolved layout for each image by reading the generated devicetree in :file:`build/<image>/zephyr/zephyr.dts` (for sysbuild, replace ``<image>`` with the image name, such as ``smp_svr``, ``mcuboot``, or ``b0n``).

Inspect the flash (or RRAM) device nodes that contain a ``partitions`` subnode.
On typical Nordic application targets this is ``&flash0`` or ``&cpuapp_rram``; on nRF5340 network-core builds it is often ``&flash1``.
On nRF5340, the application core and network core each have their own internal flash; devicetree exposes them as separate device nodes (for example ``&flash0`` on the application core build and ``&flash1`` on the network-core build), so partition maps for the two cores are defined on different nodes.
If the secondary MCUboot slot or other data lives on external memory, also check the external device node (for example ``&mx25r64``) and its ``partitions`` child nodes.

Each partition entry shows ``reg`` (offset and size) and the devicetree node label (for example ``slot0_partition``).
Comments in :file:`zephyr.dts` point to an overlay or a source file that defined each property; this may help with identifying what has contributed to the final layout of a partition map.

Depending on your development environment, you can also use one of the following options:

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      Use the extension's `Memory report`_ feature, which shows the size and percentage of memory that each symbol uses on your device for RAM, ROM, and partitions.
      Click the :guilabel:`Memory report` button in the `Actions View`_ to generate the report.
      The partition map is available in the :guilabel:`Partitions` tab.

      Alternatively, you can also use the `Memory Explorer <How to work with the Memory Explorer_>`_ feature of the extension's nRF Debug to check memory sections for the partitions.
      This feature requires `enabling debugging in the build configuration <How to debug_>`_ and providing the partition addresses manually.

   .. group-tab:: Command line

      Open :file:`build/<image>/zephyr/zephyr.dts` for each sysbuild image and review the ``partitions`` nodes under the relevant memory devices, as described above.

      If your build still uses the deprecated Partition Manager, you can run ``west build -t partition_manager_report`` to print its legacy ASCII report.
      New projects should rely on devicetree and :file:`zephyr.dts` instead.

.. _ug_bootloader_flash_partition_requirement:

Partition requirement for DFU
*****************************

DFU requires a **stable** partition map: bootloaders and update tools assume fixed slot addresses and sizes across builds and firmware versions.
Define all regions that bootloaders and DFU depend on explicitly in devicetree (board files, overlays, or included ``*.dtsi`` files).
Do not rely on layout that changes implicitly between builds.

The definition of partition layout is dependent on the selected bootloader chain.
For details, see :ref:`ug_bootloader_flash`.

.. _ug_bootloader_flash:

Flash memory partitions
***********************

Each bootloader handles flash memory partitioning differently.

After building, verify the layout for each relevant sysbuild image as described in :ref:`bootloader_partitioning_partition_inspection`.

.. _ug_bootloader_flash_b0:

|NSIB| partitions
=================

See :ref:`bootloader_flash_layout` for implementation-specific information about this bootloader.

Devicetree node labels
----------------------

On the application core, |NSIB| expects partition nodes with the following labels (in addition to application image slots when MCUboot is present):

* ``b0_partition`` — |NSIB| image
* ``s0_partition`` and ``s1_partition`` — the two alternate slots for the **next stage** in the boot chain after |NSIB|
* ``bl_storage`` and ``provision_partition`` — provisioning data for secure boot (often a single node with both labels, or ``bl_storage`` in UICR on some targets)

The next stage is often :doc:`MCUboot <mcuboot:index-ncs>` used as an **upgradable second-stage bootloader**.
When serving as second-stage bootloader, MCUboot resides in ``s0_partition`` and/or ``s1_partition``; two MCUboot instances may have different versions.
|NSIB| picks the slot with the valid and newest image, and chain-loads MCUboot from there.
Application firmware uses ``slot0_partition`` and ``slot1_partition`` for MCUboot image slots, downstream of MCUboot in the chain.

Devicetree frequently **aliases** ``boot_partition`` to ``s0_partition`` so the default MCUboot sysbuild image links into slot 0, for example:

.. code-block:: DTS

   s0_partition: boot_partition: partition@8000 {
       label = "mcuboot";
       ...
   };

The node label is not the right way to point at the firmware image executable region; use the devicetree ``chosen`` ``zephyr,code-partition`` property with the proper node label instead.

The MCUboot sysbuild image overlay sets devicetree ``chosen`` ``zephyr,code-partition = &boot_partition`` (equivalently ``&s0_partition``).
``s1_partition`` is the alternate MCUboot bank used when upgrading the second-stage bootloader.

When :kconfig:option:`SB_CONFIG_SECURE_BOOT_BUILD_S1_VARIANT_IMAGE` is enabled (default with :kconfig:option:`SB_CONFIG_SECURE_BOOT_APPCORE`), sysbuild builds an additional image linked for ``s1_partition``, so the second bank can be programmed with a correctly placed binary.
With MCUboot in the chain, that image is ``mcuboot_s1_variant``; without MCUboot, it is ``<application>_s1_variant``.
Define ``s1_partition`` in devicetree so that bank exists in the flash map when the S1 variant is built.
See :ref:`bootloader_pre_signed_variants` and :ref:`app_build_output_files` for the generated artifacts.

On the nRF5340 network core, |NSIB| uses ``b0n_partition`` instead of ``b0_partition``, which is used on the application core.
The network-core layout only uses ``s0_partition`` for the networking firmware; there is no ``s1_partition`` on the network core layout.

Provide matching partition definitions in the overlays for each sysbuild image that runs |NSIB| (for example ``sysbuild/b0/`` or ``sysbuild/b0n/`` overlays on nRF5340).

.. _ug_bootloader_flash_mcuboot:

MCUboot partitions
==================

For most applications, MCUboot requires two image slots:

* The *primary slot*, containing the application that will be booted.
* The *secondary slot*, where a new application can be stored before it is activated.

It is possible to use only the *primary slot* for MCUboot by using the ``CONFIG_SINGLE_APPLICATION_SLOT`` option.
This is particularly useful in memory-constrained devices to avoid providing space for two images.

See the *Image Slots* section in the :doc:`MCUboot documentation <mcuboot:design>` for more information.

In devicetree, the MCUboot bootloader image is linked to ``boot_partition``.
In the default single-application-core layout, the primary application slot is ``slot0_partition`` (label ``image-0``) and the secondary slot is ``slot1_partition`` (label ``image-1``).
These names match Zephyr's :ref:`flash map <flash_map_api>` conventions and MCUboot's devicetree integration.

Example excerpt (application core, internal flash only):

.. code-block:: DTS

   &flash0 {
       partitions {
           boot_partition: partition@0 { ... };
           slot0_partition: partition@10000 { label = "image-0"; ... };
           slot1_partition: partition@86000 { label = "image-1"; ... };
       };
   };

More complex maps are used on some SoCs:

* **nRF54H20** — per-core slots, merged-slot updates, and optional external flash.
  See :ref:`ug_nrf54h20_partitioning_merged` and the memory-map examples under :file:`nrf/samples/dfu/smp_svr/dts/`.
* **nRF5340** — simultaneous multi-image DFU (application and network core), external secondary slot, and QSPI execute-in-place (XIP) split images.
  See :ref:`ug_nrf5340_multi_image_dfu`, :ref:`qspi_xip_split_image`, and :ref:`smp_svr_ext_xip`.

You can store the secondary slot in external flash when using MCUboot.
See :ref:`ug_bootloader_external_flash` for more information.

.. _ug_bootloader_external_flash:

Using external flash memory partitions
**************************************

When using MCUboot, you can place the secondary slot (``slot1_partition``) on external flash by defining that partition on the external memory device in devicetree.

Complete the following steps:

1. Enable the external flash device and declare ``slot1_partition`` (and any other needed regions) under that device's ``partitions`` node in devicetree.
   Share the same layout between the application and the MCUboot sysbuild image (for example, include a common ``*.dtsi`` from both ``boards/`` and ``sysbuild/mcuboot/boards/`` overlays).
   See :file:`nrf/samples/dfu/smp_svr/dts/nrf54l15dk_nrf54l15_memory_map_ext_flash.dtsi` for a reference layout.

#. Enable and configure the external flash driver for every image that must read or write those partitions (typically the application and MCUboot).

#. Update the :kconfig:option:`CONFIG_BOOT_MAX_IMG_SECTORS` `MCUboot Kconfig option`_ accordingly.
   This option defines the maximum number of image sectors MCUboot can handle, as MCUboot typically increases slot sizes when external flash is enabled.
   Otherwise the :kconfig:option:`CONFIG_BOOT_MAX_IMG_SECTORS` Kconfig option defaults to the value used for internal flash, and the application may not boot if the value is set too low.

   (The image sector size is the same as the flash erase-block-size across all |NCS| integrated memory.)

The external flash driver must support read and write operations with a **write block size that is the same as or smaller than** that of the internal non-volatile memory used for the primary slot (typically 4-byte alignment on Nordic development kits with QSPI NOR, for example ``mx25r64`` on nRF DKs).
You do not need a driver with single-byte program support specifically for external flash.

The Quad Serial Peripheral Interface (QSPI) NOR flash memory driver is able to satisfy these requirements on a board with QSPI-connected external flash.

See the test in :file:`tests/modules/mcuboot/external_flash` for reference.
This test passes both devicetree overlay files and Kconfig fragments to the MCUboot image through its :file:`sysbuild` folder.

Troubleshooting
***************

This section describes some of the issues you might come across when partitioning device memory.

MCUboot failure
===============

MCUboot could fail, reporting the following error:

.. code-block:: console

   *** Booting Zephyr OS build v3.1.99-ncs1-... ***
   I: Starting bootloader
   W: Failed reading sectors; BOOT_MAX_IMG_SECTORS=512 - too small?
   W: Cannot upgrade: not a compatible amount of sectors
   I: Bootloader chainload address offset: 0x10000
   I: Jumping to the first image slot

This error could be caused by the following issues:

  * Partition layout mismatch between the MCUboot sysbuild image and the application (missing or different ``slot0_partition`` / ``slot1_partition`` definitions).
    Ensure both images use the same devicetree partition map for slots MCUboot manages.

  * The external flash driver is not enabled in the MCUboot image, or the external device is ``status = "disabled"`` in the MCUboot devicetree, so MCUboot cannot access the secondary slot.

  * Insufficient value set for the ``CONFIG_BOOT_MAX_IMG_SECTORS`` Kconfig option, as MCUboot typically increases slot sizes when external flash is enabled.
    See `MCUboot's Kconfig options used in Zephyr <https://github.com/nrfconnect/sdk-mcuboot/blob/main/boot/zephyr/Kconfig#L370>`_ for details.

  * A flash driver bug or board hardware issue causing read or erase failures on internal or external non-volatile memory.
    Verify connectivity and try a minimal flash read/write test on the affected device.

Compilation failure
===================

The compilation could fail, reporting a linker error similar to following:

.. code-block:: console

   undefined reference to '__device_dts_ord_<digits>

This error often means there is a devicetree reference to a flash device (for example for a partition on external memory) but no driver for that device is enabled for compilation; enable the correct flash driver Kconfig options for the image being built and ensure the device node has ``status = "okay"`` in that image's devicetree.
