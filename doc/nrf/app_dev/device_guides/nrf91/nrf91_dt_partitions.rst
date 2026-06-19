.. _nrf91_dt_partitions:

Configuring partitions on an nRF91 Series device
################################################

.. contents::
   :local:
   :depth: 2

The default SRAM and Flash partition layouts in Zephyr allocate space for all Trusted Firmware-M (TF-M) features and require you to enable MCUboot.
As MCUboot is disabled by default in |NCS|, and the modem handles the security requirements, all applications are expected to set the partition tables using a devicetree overlay.
This ensures that the partitions match the actual requirements and maximizes the available memory for your application.

Including default partition layouts
***********************************

The easiest way is to include two of the ``.dtsi`` files from the :file:`nrf/dts/samples/cellular` folder to your board's :file:`.overlay` file.

.. code-block:: devicetree

   #include <samples/cellular/nrf91_no_bootloader_partitions.dtsi>

   #include <samples/cellular/nrf91_sram_partitions.dtsi>

* There are four sample flash layouts:

   * ``nrf91_no_bootloader_partitions.dtsi`` - This layout works with minimal TF-M configurations without a bootloader.
     This layout is used by most samples.
   * ``nrf91_bootloader_partitions.dtsi`` - This layout works with minimal TF-M setups that include MCUboot.
   * ``nrf91_immutable_bootloader_partitions.dtsi`` - This layout works with minimal TF-M and both an :ref:`bootloader` and an updatable MCUboot.
     Both the b0 and MCUboot are built separately and must also know about these partitions.
     You need to add a ``b0`` and ``mcuboot`` folder inside a ``sysbuild`` folder with a :file:`prj.conf` and the required overlay in a ``boards`` folder.
   * ``nrf91_crypto_partitions.dtsi`` - This layout works  for the full TF-M features without a bootloader.
     You can use it to test TF-M functionalities.

* There are two sample SRAM layouts:

   * ``nrf91_sram_partitions.dtsi`` - This layout works with minimal TF-M configurations.
     This layout is used by most samples.
   * ``nrf91_sram_crypto_partitions.dtsi``- This layout works for the full TF-M features.
     You can use it to test TF-M functionalities.

Creating a custom partition layout
**********************************

Instead of including these ``.dtsi`` files, you can copy their contents and modify them as required.

The flash memory uses 4 KB pages, so each partition must start on a 4 KB boundary.
The flash memory has 32 KB blocks that can be marked as secure or non-secure.
Any 32 KB block that contains a ``slot0_ns_partition``, ``slot1_ns_partition``, or ``storage_partition`` will be marked as non-secure.
Secure partitions must not share a 32 KB block with any non-secure partition.
In addition, the ``storage_partition`` must not share a 32 KB block with the ``slot0_ns_partition``.
Any partitions that need to be accessed by the application must be part of (or included within) the storage partition.

The SRAM has 8 KB blocks that can be marked as secure or non-secure.
You need to add a specific block of non-secure memory that is shared with the modem as described in :ref:`devicetree_integration`.

References
**********

See `Migrating from Partition Manager to devicetree (DTS)`_ for instructions on how to migrate your project to devicetree in general.
