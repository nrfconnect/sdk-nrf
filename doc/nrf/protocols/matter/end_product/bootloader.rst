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

Read :ref:`ug_bootloader_adding_immutable_mcuboot` to learn how to add MCUboot to an |NCS| application.
Some Matter samples include DFU support out of the box, as listed in the :ref:`sample feature matrix table <matter_samples>`.

MCUboot minimal configuration
*****************************

MCUboot is by default configured to enable debug features, such as logs.
You can reduce the size of the bootloader image by disabling unnecessary features.

See the `Kconfig.mcuboot.defaults`_ file for the MCUboot minimal configuration used by :ref:`Matter samples <matter_samples>` in the |NCS|.
This configuration allows to reduce the flash partition occupied by MCUboot to 24 kB.

Partition layout
****************

Each application that uses MCUboot must use :ref:`partition_manager` to define partitions of the flash memory.
This is needed for the bootloader to know where the current and the new firmware images are located in the flash.

There are multiple ways to define parititions using Partition Manager.
For example, each :ref:`Matter sample <matter_samples>` provides a :file:`pm_static_dfu.yml` file (one for each configuration) that statically defines the partition layout.
See :ref:`ug_matter_hw_requirements_layouts` to check the reference partition layout for each supported platform.

Given the size of the Matter stack, it will usually not be possible to fit both the primary and the secondary slot in the internal flash in order to store the current and the new firmware image, respectively.
Instead, you should use the :ref:`external flash <ug_bootloader_external_flash>` to host the secondary slot.

.. note::
   Remember to enable a proper flash driver when putting the secondary slot in the external flash.
   For example, if you develop your application on a Nordic Semiconductor's development kit that includes a QSPI NOR flash module, you can do this by setting the :kconfig:option:`CONFIG_NORDIC_QSPI_NOR` Kconfig option.

Settings partition
==================

The nRF Connect platform in Matter uses Zephyr's :ref:`zephyr:settings_api` API to provide the storage capabilities to the Matter stack.
This requires that you define the ``settings_storage`` partition in the flash.
The recommended minimum size of the partition is 16 kB, but you can reserve even more space if your application uses the storage extensively.

As you can see in the listing above, Matter samples in the |NCS| reserve exactly 16 kB for the ``settings_storage`` partition.

Factory data partition
======================

If you make a real Matter product, you also need the ``factory_data`` partition to store the factory data.
The factory data contains a set of immutable device identifiers, certificates and cryptographic keys, programmed onto a device at the time of the device fabrication.
For that partition one flash page of 4kB should be enough in most use cases.

See the :ref:`ug_matter_device_attestation_device_data_generating` section on the Device Attestation page for more information about the factory data in Matter.

Signing keys
************

MCUboot uses asymmetric cryptography to validate the authenticity of firmware.
The public key embedded in the bootloader image is used to validate the signature of a firmware image that is about to be booted.
If the signature check fails, MCUboot rejects the image and either:

* rolls back to the last valid firmware image if the fallback recovery has not been disabled using the MCUboot's :kconfig:option:`CONFIG_BOOT_UPGRADE_ONLY` Kconfig option.
* fails to boot.

.. note::
   To help you get started with MCUboot and ease working with sample applications, MCUboot comes with a default key pair for the firmware image validation.
   As the key pair is publicly known, it provides no protection against the image forgery.
   For this reason, when making a real product, it is of the greatest importance to replace it with a unique key pair, known only to the device maker.

   Read :ref:`ug_bootloader_adding_immutable_mcuboot_keys` to learn how to configure MCUboot to use a custom key pair.

Downgrade protection
********************

The :ref:`downgrade protection <ug_fw_update_image_versions_mcuboot_downgrade>` mechanism makes it impossible for an attacker to trick a user to install a firmware image older than the currently installed one.
The attacker might want to do this to reintroduce old security vulnerabilities that have already been fixed in newer firmware revisions.
You should enable the downgrade protection mechanism if you choose to enable MCUboot's :kconfig:option:`CONFIG_BOOT_UPGRADE_ONLY` Kconfig option, which disables the fallback recovery in case of a faulty upgrade.
