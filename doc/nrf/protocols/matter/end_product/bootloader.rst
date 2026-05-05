.. _ug_matter_device_bootloader:

Bootloader configuration in Matter
##################################

.. contents::
   :local:
   :depth: 2

All Matter nodes are required to implement a firmware update mechanism that validates the authenticity of new firmware before executing it.
To meet this requirement, Nordic Semiconductor recommends using :doc:`MCUboot <mcuboot:index-ncs>` bootloader for installing a new firmware image.

This page contains guidelines for configuring the MCUboot bootloader in Matter projects.

Adding MCUboot to application
*****************************

Read :ref:`ug_bootloader_adding_sysbuild_immutable_mcuboot` to learn how to add MCUboot to an |NCS| application.
Some Matter samples include :term:`Device Firmware Update (DFU)` support out of the box, as listed in the :ref:`sample feature matrix table <matter_samples>`.

MCUboot minimal configuration
*****************************

MCUboot is by default configured to enable debug features, such as logs.
You can reduce the size of the bootloader image by disabling unnecessary features.

See the following files for the MCUboot minimal configuration used by :ref:`Matter samples <matter_samples>` in the |NCS|:

* :file:`prj.conf` file located in each sample's :file:`sysbuild/mcuboot` directory
* Board files located in each sample's :file:`sysbuild/mcuboot/boards` directory
* :file:`Kconfig.sysbuild` file located in each sample's directory.

This configuration allows to reduce the flash partition occupied by MCUboot to 24 kB.

.. _ug_matter_device_bootloader_partition_layout:

Partition layout
****************

.. include:: ../../../includes/pm_deprecation.txt

A bootloader is a critical component in a Matter device, ensuring secure firmware updates and authenticating new application images.
All Nordic Matter samples in the |NCS| use MCUboot as the primary bootloader, with configuration and partitioning adapted to application and device needs.

.. _ug_matter_hw_requirements_partition_dts_reference:

Consider the following when defining partitions for your end product:

* Use the default partition layout for your target SoC by including the base partition file :file:`<soc_name>_cpuapp_partitions.dtsi` located under the :file:`nrf/dts/samples/matter` directory.
  Include the base partition file in your :file:`boards/<board_name>.overlay` board file using the following line:

  .. code-block:: dts

     #include "<samples/matter/<soc_name>_cpuapp_partitions.dtsi>"

* To modify the partition layout, copy the whole content of the base partition file to your :file:`boards/<board_name>.overlay` board file and modify the partitions as needed.
  See :ref:`ug_matter_device_optimizing_memory_configuration` to learn how to adjust the partition sizes.

* Given the size of the Matter stack, it is usually not possible to fit both the primary and the secondary slot in the internal flash in order to store the current and the new firmware image, respectively.
  Instead, you should use the :ref:`external flash <ug_bootloader_external_flash>` to host the secondary slot.

  .. note::
      Remember to enable a proper flash driver when placing the secondary slot in the external flash.
      For example, if you develop your application on a Nordic Semiconductor's development kit that includes a QSPI NOR flash module, set the :kconfig:option:`CONFIG_NORDIC_QSPI_NOR` Kconfig option.

* When selecting the partition sizes, take into account that some of the partitions, such as settings and factory data ones, are not modified during the DFU process.
  This means that performing DFU from one firmware version to another using different partition sizes might not be possible, and you cannot change the partition sizes without reprogramming the device.
  Trying to perform DFU between applications that use incompatible partition sizes can result in unwanted application behavior, depending on which partitions are overlapping.
  In some cases, this might corrupt some partitions; in others, this can lead to a DFU failure.
* The MCUboot requires its ``slot0_partition`` and ``slot1_partition`` partitions to be located under offsets being aligned to the 4 kB flash page size.
  Selecting offset values that are not aligned to 4 kB for these partitions will lead to erase failures, and result in a DFU failure.

Partition names
===============

The following tables summarize the partitions by target.

**nRF52840 DK**

.. list-table::
   :header-rows: 1
   :widths: 28 45 27

   * - Devicetree node label
     - Purpose
     - Location
   * - ``boot_partition``
     - MCUboot bootloader.
     - SoC internal flash
   * - ``slot0_partition`` (``image-0``)
     - Primary application image (includes MCUboot swap metadata inside the region).
     - SoC internal flash
   * - ``factory_data_partition``
     - Matter factory data.
     - SoC internal flash
   * - ``storage_partition`` (``storage``)
     - Non-volatile settings Zephyr settings storage.
     - SoC internal flash
   * - ``slot1_partition`` (``image-1``)
     - Secondary application slot for MCUboot DFU.
     - External QSPI flash (``mx25r64``)

**nRF5340 DK & Nordic Thingy:53** (application core and external flash; network core uses ``flash1``)

.. list-table::
   :header-rows: 1
   :widths: 28 45 27

   * - Devicetree node label
     - Purpose
     - Location
   * - ``boot_partition``
     - MCUboot bootloader.
     - Application core internal flash
   * - ``slot0_partition`` (``image-0``)
     - Primary application image (includes MCUboot swap metadata inside the region).
     - Application core internal flash
   * - ``factory_data_partition``
     - Matter factory data.
     - Application core internal flash
   * - ``storage_partition``
     - Non-volatile settings Zephyr settings storage.
     - Application core internal flash
   * - ``slot1_partition`` (``image-1``)
     - Secondary application slot for MCUboot DFU.
     - External QSPI (``mx25r64``)
   * - ``slot3_partition`` (``image-3``)
     - Network core image update slot.
     - External QSPI (``mx25r64``)
   * - ``b0n_partition``
     - Immutable first-stage network core loader (B0n).
     - Network core flash (``flash1``)
   * - ``provision_partition``
     - B0n provisioning key storage.
     - Network core flash
   * - ``s0_partition`` (``image-0`` on cpunet)
     - Network core firmware.
     - Network core flash

**nRF54L15 & nRF54L10 & nRF54LM20 DKs**

.. list-table::
   :header-rows: 1
   :widths: 28 45 27

   * - Devicetree node label
     - Purpose
     - Location
   * - ``boot_partition``
     - MCUboot.
     - RRAM (``cpuapp_rram``)
   * - ``slot0_partition``
     - Primary image.
     - RRAM
   * - ``slot1_partition``
     - Secondary application slot for MCUboot DFU.
     - External QSPI (``mx25r64``) or RRAM for internal builds
   * - ``factory_data_partition``
     - Matter factory data.
     - RRAM
   * - ``storage_partition``
     - Non-volatile settings Zephyr settings storage.
     - RRAM
   * - ``slot0_s_partition``
     - TF-M secure application (TF-M code).
     - RRAM
   * - ``slot0_ns_partition``
     - TF-M non-secure application (User application).
     - RRAM
   * - ``tfm_storage_partition`` with ``tfm_its_partition``, ``tfm_otp_partition``, ``tfm_ps_partition``
     - TF-M secure storage partitions (TF-M builds only).
     - RRAM

Settings partition
==================

The nRF Connect platform in Matter uses Zephyr's :ref:`zephyr:settings_api` API to provide the storage capabilities to the Matter stack.
This requires that you define the ``settings_storage`` partition in the flash.
The recommended minimum size of the partition is 32 kB, but you can reserve even more space if your application uses the storage extensively.

The Zephyr settings storage is implemented by the :ref:`Zephyr NVS (Non-Volatile Storage) <zephyr:nvs_api>` or :ref:`ZMS (Zephyr Memory Storage) <zephyr:zms_api>` backends.
You can select either backend, and the selection affects several factors, such as the operational performance or memory lifetime.
To achieve the optimal experience, it is recommended to use:

* NVS backend for the flash-based nRF52 and nRF53 SoC families.
* ZMS backend for the RRAM- and MRAM-based nRF54 SoC families.

The settings backend uses multiple sectors of 4 kB each, and it must use the appropriate number of sectors to cover the entire settings partition area.
To configure the number of sectors used by the backend, set the corresponding Kconfig option to the desired value:

* :kconfig:option:`CONFIG_SETTINGS_NVS_SECTOR_COUNT` for the NVS
* :kconfig:option:`CONFIG_SETTINGS_ZMS_SECTOR_COUNT` for the ZMS

For example, to cover a settings partition of 32 kB in size, you require 8 sectors.

As you can see in :ref:`ug_matter_hw_requirements_layouts`, Matter samples in the |NCS| reserve exactly 32 kB for the ``settings_storage`` partition.

Factory data partition
======================

If you make a real Matter product, you also need the ``factory_data`` partition to store the factory data.
The factory data contains a set of immutable device identifiers, certificates and cryptographic keys, programmed onto a device at the time of the device fabrication.
For that partition one flash page of 4 kB should be enough in most use cases.

By default, the ``factory_data`` partition is write-protected with the :ref:`fprotect_readme` driver (``fprotect``).
The hardware limitations require that the write-protected areas are aligned to :kconfig:option:`CONFIG_FPROTECT_BLOCK_SIZE`.
For this reason, to effectively implement ``fprotect``, make sure that the partition layout of the application meets the following requirements:

* The ``factory_data`` partition is placed right after the ``app`` partition in the address space (that is, the ``factory_data`` partition offset must be equal to the last address of the ``app`` partition).
* The ``settings_storage`` partition size is a multiple of :kconfig:option:`CONFIG_FPROTECT_BLOCK_SIZE`, which may differ depending on the SoC in use.

See the following figure and check the :ref:`ug_matter_hw_requirements_layouts` to make sure your implementation is correct.

.. figure:: images/matter_memory_map_factory_data.svg
   :alt: Factory data partition implementation criteria for fprotect

   Factory data partition implementation criteria for fprotect

In case your memory map does not follow these requirements, you can still use the factory data implementation without the write protection by setting the :kconfig:option:`CONFIG_CHIP_FACTORY_DATA_WRITE_PROTECT` to ``n``, although this is not recommended.

See the :ref:`ug_matter_device_attestation_device_data_generating` section on the Device Attestation page for more information about the factory data in Matter.

Signing keys
************

MCUboot uses asymmetric cryptography to validate the authenticity of firmware.
The public key embedded in the bootloader image is used to validate the signature of a firmware image that is about to be booted.
If the signature check fails, MCUboot rejects the image and either:

* rolls back to the last valid firmware image if the fallback recovery has not been disabled using the MCUboot's :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_OVERWRITE_ONLY` Kconfig option.
* fails to boot.

.. note::
   To help you get started with MCUboot and ease working with sample applications, MCUboot comes with a default key pair for the firmware image validation.
   As the key pair is publicly known, it provides no protection against the image forgery.
   For this reason, when making a real product, it is of the greatest importance to replace it with a unique key pair, known only to the device maker.

   Read :ref:`ug_bootloader_adding_sysbuild_immutable_mcuboot_keys` to learn how to configure MCUboot to use a custom key pair.

Downgrade protection
********************

The :ref:`downgrade protection <ug_fw_update_image_versions_mcuboot_downgrade>` mechanism makes it impossible for an attacker to trick a user to install a firmware image older than the currently installed one.
The attacker might want to do this to reintroduce old security vulnerabilities that have already been fixed in newer firmware revisions.
You should enable the downgrade protection mechanism if you choose to enable MCUboot's :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_OVERWRITE_ONLY` Kconfig option, which disables the fallback recovery in case of a faulty upgrade.

.. _ug_matter_device_bootloader_image_compression:

Image compression
*****************

The :ref:`MCUboot image compression <mcuboot_image_compression>` feature allows you to reduce the size of the firmware image that is being installed.
This is done by compressing the image before it is written to the secondary slot.

Thanks to the compression, the secondary slot can be smaller than the primary one.
This is especially useful when you do not want to use external flash for the secondary slot, and you need to place the new image in the internal memory.

You can enable this feature by setting the following Kconfig options in your application's sysbuild configuration file:

* :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_OVERWRITE_ONLY` to ``y``
* :kconfig:option:`SB_CONFIG_MCUBOOT_COMPRESSED_IMAGE_SUPPORT` to ``y``

If your application has used external flash for the secondary slot and you want to stop using it, disable the following Kconfig options in your application's sysbuild configuration file:

* :kconfig:option:`SB_CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK` to ``n``
* :kconfig:option:`SB_CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY` to ``n``
