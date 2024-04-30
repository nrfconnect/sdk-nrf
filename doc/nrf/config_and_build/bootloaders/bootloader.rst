.. _ug_bootloader:

Secure bootloader chain
#######################

.. contents::
   :local:
   :depth: 2

The |NCS| secure bootloader solutions are built on the *chain of trust* concept.

By using this secure bootloader chain, you can ensure that all code being executed has been authorized and that your application is protected against running altered code.
If, for example, an attacker tries to modify your application, or you have a bug in your code that overwrites parts of the application firmware image, the secure bootloader chain detects that the firmware has been altered and your application does not start.

Select the structure of the secure bootloader chain based on your firmware update requirements:

* If you want the bootloader to perform firmware updates for only upgrading an application, then use the :ref:`single-stage, immutable bootloader <immutable_bootloader>` solution.
* If you want the bootloader to support firmware updates for both itself and the application, then use the :ref:`two-stage, upgradable bootloader <upgradable_bootloader>` solution.

.. _ug_bootloader_chain_of_trust:

Chain of trust
**************

A secure system depends on building and maintaining a chain of trust through all the layers in the system.
Each step in this chain guarantees that the next step can be trusted to have certain properties because any unauthorized modification of a subsequent step will be detected and the process halted.
Each layer guarantees the trustworthiness of the following layer, all the way back to a property in the system referred to as *root of trust* (RoT).

A RoT consists of hardware, software, and data components that must always behave as expected because any misbehavior cannot be detected.

Think of a chain of trust like a door, where the root of trust is the key:

1. You trust a door because you trust the lock.
#. You trust the lock because you trust the key.
#. You trust the key because it is in your pocket.

If you lose this key, you can no longer trust this door.

In the context of the bootloader, a step in the chain of trust is the boot of a specific image.
As such, the secure bootloader chain consists of a sequence of images that are booted one after the other.

For a system to establish a root of trust, the first image in the system verifies the signature of the next image, which can either be an application or another bootloader image.
If the next image is another bootloader image, that one must verify the image following it to maintain the chain of trust.
After all of the images in the bootloader chain have been verified successfully, the application starts.

.. _ug_bootloader_architecture:

Architecture
************

The |NCS| currently supports two implementations:

* The first implementation provides the first stage in the chain, the immutable :ref:`bootloader`, which could be either |NSIB| or :doc:`MCUboot <mcuboot:index-ncs>`.
  It does not support bootloader upgradability, but it is useful if you need just the capability to update your application.

  See the following image for an abstract representation of the memory layout for an application that uses only an immutable bootloader in its boot chain:

  .. figure:: images/bootloader_memory_layout_onestage.svg
     :alt: Memory layout

* The second implementation provides both the first stage in the chain, the immutable bootloader (|NSIB|), and uses :doc:`MCUboot <mcuboot:index-ncs>` as the upgradable second-stage bootloader.
  This implementation provides the capability to update both your bootloader and your application.
  This is useful when a bootloader performs additional complex operations, like using a software stack.

  See the following image for an abstract representation of the memory layout for an application that uses both an immutable and an upgradable bootloader in its boot chain:

  .. figure:: images/bootloader_memory_layout.svg
     :alt: Memory layout

By default, building an application with any bootloader configuration creates a :ref:`multi-image build <ug_multi_image>`, where the :ref:`partition_manager` manages its memory partitions.
In this case, bootloaders are built as child images.
When building an application with :ref:`Cortex-M Security Extensions (CMSE) enabled <app_boards_spe_nspe_cpuapp_ns>`, then :ref:`Trusted Firmware-M (TF-M) <ug_tfm>` is built with the image automatically.
From the bootloader perspective, the TF-M is part of the booted application image.

.. _ug_bootloader_flash_static_requirement:

Static partition requirement for DFU
====================================

By default, the Partition Manager generates the partition map dynamically.
As long as you are not using Device Firmware Updates (DFU), you can use the dynamic generation of memory partitions.

However, if you want to perform DFU, you must :ref:`define a static partition map <ug_pm_static>` because the dynamically generated partitions can change between builds.
This is important also when you use a precompiled HEX file as a child image instead of building it.
In such cases, the newly generated application images may no longer use a partition map that is compatible with the partition map used by the bootloader.
As a result, the newly built application image may not be bootable by the bootloader.

.. note::
   For detailed information about the memory layout used for the build, see the partition configuration in the :file:`partitions.yml` file, located in the build folder directory, or run ``ninja partition_manager_report``.
   You must enable the Partition Manager to make the :file:`partitions.yml` file and the ``partition_manager_report`` target available.

   The :file:`partitions.yml` file is present also if the Partition Manager generates the partition map dynamically.
   You can use this file as a base for your static partition map.

The memory partitions that must be defined in the static partition map depend on the selected bootloader chain.
For details, see :ref:`ug_bootloader_flash`.

.. _immutable_bootloader:

Immutable bootloader
====================

The first step in the chain of trust is a secure, immutable bootloader.
This bootloader can be used alone (as a single-stage bootloader) or together with a `Second-stage upgradable bootloader`_ (as a first-stage bootloader).

The immutable bootloader runs after every reset and establishes the root of trust by verifying the signature and metadata of the next image in the boot sequence.
If the verification fails, the boot process stops.
This way, the immutable bootloader can guarantee that the next image in the boot sequence will not start up if it has been tampered with in any way.
For example, if an attacker attempts to take over the device by altering the firmware, the device will not boot, and thus not run the infected code.

More specifically, the immutable bootloader always performs the following steps when it runs, regardless of any additional configuration:

1. Locking of the flash memory.

   To enable the RoT, the immutable bootloader locks the flash memory address range containing itself and its configuration using the hardware available on the given architecture.
   (The immutable bootloader cannot be modified or deleted without erasing the entire device.)

#. Selection of the next slot in the boot chain.

   The next stage in the boot chain can either be an application or another bootloader.
   Firmware images have a version number, and the bootloader will select the slot with the latest firmware.
   For more information about creating a second-stage bootloader, see :ref:`ug_bootloader_adding_upgradable`.

#. Verification of the next stage in the boot chain.

   The verification provided by this bootloader is recommended and suitable for all the most common user scenarios and includes the following checks:

   * Signature verification - Verifies that the key used for signing the next image in the boot sequence matches one of the provided public keys.

     During this stage, the bootloader checks that the image is authentic (comes only from its original author) and integral (it was not changed by accident).

   * Metadata verification - Checks that the images are compatible.

   .. caution::
      You must generate and use your own signing keys while in development and before deploying when using either MCUboot or the |NSIB| as an immutable bootloader.
      See :ref:`ug_fw_update_development_keys` for more information.

#. Booting of the next stage in the boot chain.

   All peripherals that have been used are reset and the next stage is booted.

Except for providing your own keys, there is no need to modify the immutable bootloader in any way before you program it.

The :ref:`bootloader capabilities table <app_bootloaders_support_table>` lists the bootloaders that you can use as an immutable bootloader.

.. _upgradable_bootloader:

Second-stage upgradable bootloader
==================================

If you also need the capability of updating the bootloader, you can add a second-stage upgradable bootloader to the bootloader chain.
It can be updated through either wired or :ref:`over-the-air (OTA) <lib_fota_download>` updates, unlike the immutable bootloader.

The immutable bootloader, acting as the root of trust, protects the upgradable bootloader, which must also continue the chain of trust by verifying the next image in the boot sequence.
For this reason, the immutable bootloader is responsible for upgrading the upgradable bootloader and verifying its metadata and image integrity.
For more information about how the immutable bootloader accomplishes this, see the :ref:`bootloader_flash_layout` section of the |NSIB|.

The upgradable bootloader carries out the same signature and metadata verification as the immutable bootloader.
Also, it can upgrade both itself and the following image in the boot sequence, which, in most cases, is an application.

.. caution::
   You should add a second-stage bootloader only when necessary by the design or firmware upgrade needs.
   Adding the second stage bootloader for no reason will lead to a degradation of the system's overall security, as attackers can exploit bugs that may exist in either bootloader.

The :ref:`bootloader capabilities table <app_bootloaders_support_table>` lists the bootloaders that you can use as an upgradable bootloader.

.. _upgradable_bootloader_presigned_variants:

Pre-signed variants
-------------------

When programming an upgradable bootloader, the build system can automatically generate pre-signed variants of the image verified by the |NSIB|.
The upgradable bootloader does not use pre-signed variants to update the application.

When building upgrade images for the image following the |NSIB| in the boot chain, like the upgradable bootloader or application, you must build with pre-signed variants.
Firmware update packages of the upgradable bootloader must contain images for both slots, since it may not be known which slot is in use by its current version while deployed in the field.
See the :ref:`bootloader_pre_signed_variants` section of the |NSIB| documentation for more details.

When not building firmware update packages, pre-signed variants are not strictly necessary but can be used as a backup mechanism in case the image in the primary slot becomes corrupted, for example from a bit-flip.
Having both slots programmed allows the immutable bootloader to invalidate the corrupt image and boot into a valid one.

.. _ug_bootloader_flash:

Flash memory partitions
=======================

Each bootloader handles flash memory partitioning differently.

After building the application, you can print a report of how the flash partitioning has been handled for a bootloader, or combination of bootloaders, by using :ref:`pm_partition_reports`.

.. _ug_bootloader_flash_b0:

|NSIB| partitions
-----------------

See :ref:`bootloader_flash_layout` for implementation-specific information about this bootloader.

.. _ug_bootloader_flash_mcuboot:

MCUboot partitions
------------------

For most applications, MCUboot requires two image slots:

* The *primary slot*, containing the application that will be booted.
* The *secondary slot*, where a new application can be stored before it is activated.

It is possible to use only the *primary slot* for MCUboot by using the ``CONFIG_SINGLE_APPLICATION_SLOT`` option.
This is particularly useful in memory-constrained devices to avoid providing space for two images.

See the *Image Slots* section in the :doc:`MCUboot documentation <mcuboot:design>` for more information.

The |NCS| variant of MCUboot uses the :ref:`partition_manager` to configure the flash memory partitions for these image slots.
In the default configuration, defined in :file:`bootloader/mcuboot/boot/zephyr/pm.yml`, the partition manager dynamically sets up the partitions as required for MCUboot.
For example, the partition layout for :file:`zephyr/samples/hello_world` using MCUboot on the ``nrf52840dk`` board would look like the following:

.. code-block:: console

    (0x100000 - 1024.0kB):
   +-----------------------------------------+
   | 0x0: mcuboot (0xc000)                   |
   +---0xc000: mcuboot_primary (0x7a000)-----+
   | 0xc000: mcuboot_pad (0x200)             |
   +---0xc200: mcuboot_primary_app (0x79e00)-+
   | 0xc200: app (0x79e00)                   |
   | 0x86000: mcuboot_secondary (0x7a000)    |
   +-----------------------------------------+

You can also store secondary slot images in external flash memory when using MCUboot.
See :ref:`ug_bootloader_external_flash` for more information.
