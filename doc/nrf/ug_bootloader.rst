.. _ug_bootloader:

Secure bootloader chain
#######################

.. contents::
   :local:
   :depth: 2

The |NCS| provides a secure bootloader solution based on the chain of trust concept.

By using this bootloader chain, you can easily ensure that all code that is executed is authorized and your application is protected against running altered code.
If, for example, an attacker attempts to modify your application, or you have a bug in your code that overwrites parts of the application flash, the secure bootloader chain detects that the flash has been altered and your application does not start up.

Chain of trust
**************

A secure system depends on building and maintaining a chain of trust through all layers in the system.
Each step in this chain will guarantee that the next step can be trusted to have certain properties, because any illegal modification of a following step will be detected and the process halted.
The trustworthiness of each layer is guaranteed by the previous layer, all the way back to a property in the system referred to as a root of trust (RoT).

You can compare a chain of trust to the concept of a door.
You trust a door because you trust the lock because you trust the key because the key is in your pocket.
If you lose this key the chain of trust unravels and you no longer trust this door.

It is important to note that a key alone is not a RoT, although a RoT may include a key.
A RoT will consist of both hardware, software, and data components that must always behave in an expected manner, because any misbehavior cannot be detected.

.. _ug_bootloader_architecture:

Architecture
************

The secure bootloader chain consists of a sequence of images that are booted one after the other.
To establish a root of trust, the first image verifies the signature of the next image (which can be the application or another bootloader image).
If the next image is another bootloader image, that one must verify the image following it to maintain the chain of trust.
After all images in the bootloader chain have been verified successfully, the application starts.

The current implementation provides the first stage in the chain, the :ref:`bootloader`, and uses :doc:`mcuboot:index` as upgradable bootloader.

The following image shows an abstract representation of the memory layout, assuming that there are two bootloader images (one immutable, one upgradable) and one application:

.. figure:: images/bootloader_memory_layout.svg
   :alt: Memory layout

   Memory layout

For detailed information about the memory layout, see the partition configuration in the :file:`<build folder>/partitions.yml` file or run ``ninja partition_manager_report``.

.. _immutable_bootloader:

Immutable bootloader
====================

The first step in the chain of trust is an immutable first stage bootloader, in the following referred to as immutable bootloader.
The code abbreviates this bootloader as SB (for Secure Boot) or B0.

The immutable bootloader runs after every reset and establishes the root of trust by verifying the signature and metadata of the next image in the boot sequence.
If verification fails, the boot process stops.

This way, the immutable bootloader can guarantee that if the flash of the next image in the boot sequence (another bootloader or the application) has been tampered with in any way, that image will not start up.
So if an attacker attempts to take over the device by altering the flash, the device will not boot and not run the infected code.

The immutable bootloader cannot be updated or deleted from the device unless you erase the device.

There is no need to modify the immutable bootloader in any way before you program it. The default verification is recommended and suitable for all common user scenarios and includes the following checks:

Signature verification
   Verifies that the key used for signing the next image in the boot sequence matches one of the provided public keys.

Metadata verification
   Checks that the images are compatible.

.. _upgradable_bootloader:

Upgradable bootloader
=====================

Unlike the immutable bootloader, the upgradable bootloader can be updated through, for example, a Device Firmware Update (DFU).
This bootloader is therefore more flexible than the immutable bootloader.
It is protected by the root of trust in form of the immutable bootloader, and it must continue the chain of trust by verifying the next image in the boot sequence.

The upgradable bootloader should carry out the same signature and metadata verification as the immutable bootloader.
In addition, it can provide functionality for upgrading both itself and the following image in the boot sequence (in most cases, the application).

There are two partitions where the upgradable bootloader can be stored: slot 0 and slot 1 (also called *S0* and *S1*).
A new bootloader image is stored in the partition that is not currently used.
When booting, the immutable bootloader checks the version information for the images in slot 0 and slot 1 and boots the one with the highest version.
If this image is faulty and cannot be booted, the other partition will always hold a working image, and this one is booted instead.

Set the option :option:`CONFIG_BUILD_S1_VARIANT` when building the upgradable bootloader to automatically generate pre-signed variants of the image for both slots.
These signed variants can be used to perform an upgrade procedure through the :ref:`lib_fota_download` library.

.. _ug_bootloader_adding:

Adding a bootloader chain to your application
*********************************************

The |NCS| includes a sample implementation of an immutable bootloader.
Additionally, the |NCS| comes with a slightly modified version of :doc:`mcuboot:index`.

Both bootloaders can easily be included in your application using :ref:`ug_multi_image`.

Adding the immutable bootloader
===============================

To add the immutable bootloader to your application, set :option:`CONFIG_SECURE_BOOT` and add your private key file under :option:`CONFIG_SB_SIGNING_KEY_FILE`.
|how_to_configure|

See the documentation of the :ref:`bootloader` sample for more information.
The :ref:`bootloader_build_and_run` section has detailed instructions for adding the immutable bootloader as first stage of the secure bootloader chain.

To ensure that the immutable bootloader occupies as little flash as possible, apply the :file:`overlay-minimal-size.conf` Kconfig overlay file for the b0 image.
This can be done in the following way:

* Using cmake::

     cmake -GNinja -DBOARD=nrf52840dk_nrf5840 -Db0_OVERLAY_CONFIG=overlay-minimal-size.conf -DCONFIG_SECURE_BOOT=y ../

* Using west::

     west build -b nrf52840dk_nrf52840 zephyr/samples/hello_world -- -Db0_OVERLAY_CONFIG=overlay-minimal-size.conf -DCONFIG_SECURE_BOOT=y


Adding MCUboot as an upgradable bootloader
==========================================

To add MCUboot as upgradable bootloader to your application, set :option:`CONFIG_BOOTLOADER_MCUBOOT`.
|how_to_configure|

To make MCUboot upgradable, you must also add the immutable bootloader.
Set option :option:`CONFIG_SECURE_BOOT` to do this.

.. note::
   It is possible to include this bootloader without the immutable bootloader.
   In this case, MCUboot will act as an immutable bootloader.

It is possible for MCUboot to use the cryptographic functionality exposed by the immutable bootloader, reducing the flash usage for MCUboot to less than 16 kB.
To enable this configuration, apply the :file:`overlay-minimal-external-crypto.conf` Kconfig overlay file for the MCUboot image.
This can be done in the following way:

* Using cmake::

     cmake -GNinja -DBOARD=nrf52840dk_nrf5840 -Dmcuboot_OVERLAY_CONFIG=overlay-minimal-external-crypto.conf -DCONFIG_SECURE_BOOT=y -DCONFIG_BOOTLOADER_MCUBOOT=y ../

* Using west::

     west build -b nrf52840dk_nrf52840 zephyr/samples/hello_world -- -Dmcuboot_OVERLAY_CONFIG=overlay-minimal-external-crypto.conf -DCONFIG_SECURE_BOOT=y -DCONFIG_BOOTLOADER_MCUBOOT=y

See :doc:`mcuboot:index` for information about the default implementation of MCUboot.
:ref:`mcuboot:mcuboot_ncs` gives details on the integration of MCUboot in |NCS|.

You can configure MCUboot by setting configuration options for the ``mcuboot`` child image.

.. _ug_bootloader_flash:

Flash partitions used by MCUboot
--------------------------------

MCUboot requires two image slots: one that contains the application to be booted (the *primary slot*), and one where a new application can be stored before it is activated (the *secondary slot*).
See the *Image Slots* section in the :doc:`MCUboot documentation <mcuboot:design>` for more information.

The |NCS| variant of MCUboot uses the :ref:`partition_manager` to configure the flash partitions for these image slots.

In the default configuration, the Partition Manager dynamically sets up the partitions as required.
If you want to control where in memory the flash partitions are placed, you can define static partitions for your application.
See :ref:`ug_pm_static` for more information.

It is possible to use external flash as the storage partition for the secondary slot.
This requires a driver for the external flash that supports:

* Single-byte read and write
* Writing data from internal flash to external flash

See :ref:`pm_external_flash` for general information about how to set up partitions in external flash in the Partition Manager.
To configure MCUboot to use external flash for the secondary slot, update the :file:`ncs/bootloader/mcuboot/boot/zephyr/pm.yml` file to contain the following definition for ``mcuboot_secondary``::

   mcuboot_secondary:
       region: external_flash
       size: CONFIG_PM_EXTERNAL_FLASH_SIZE

The following example shows how to configure an application for the nRF52840 DK.
The nRF52840 DK comes with external flash that can be used for the secondary slot and that can be accessed using the QSPI NOR flash driver.

1. Append the following configuration to the :file:`ncs/bootloader/mcuboot/boot/zephyr/prj.conf` file::

      CONFIG_NORDIC_QSPI_NOR=y
      CONFIG_NORDIC_QSPI_NOR_FLASH_LAYOUT_PAGE_SIZE=4096
      CONFIG_NORDIC_QSPI_NOR_FLASH_ALLOW_STACK_USAGE_FOR_DATA_IN_FLASH=y
      CONFIG_MULTITHREADING=y
      CONFIG_BOOT_MAX_IMG_SECTORS=256
      CONFIG_PM_EXTERNAL_FLASH=y
      CONFIG_PM_EXTERNAL_FLASH_DEV_NAME="MX25R64"
      CONFIG_PM_EXTERNAL_FLASH_SIZE=0xf4000
      CONFIG_PM_EXTERNAL_FLASH_BASE=0

   These options enable the QSPI NOR flash driver, multi-threading (which is required by the flash driver), and the external flash of the nRF52840 DK.
#. Update the :file:`ncs/bootloader/mcuboot/boot/zephyr/pm.yml` file (as described above)::

      mcuboot_secondary:
          region: external_flash
          size: CONFIG_PM_EXTERNAL_FLASH_SIZE

#. Add the following configuration to the :file:`prj.conf` file in your application directory::

      CONFIG_NORDIC_QSPI_NOR=y
      CONFIG_NORDIC_QSPI_NOR_FLASH_LAYOUT_PAGE_SIZE=4096
      CONFIG_NORDIC_QSPI_NOR_FLASH_ALLOW_STACK_USAGE_FOR_DATA_IN_FLASH=y
      CONFIG_PM_EXTERNAL_FLASH=y
      CONFIG_PM_EXTERNAL_FLASH_DEV_NAME="MX25R64"
      CONFIG_PM_EXTERNAL_FLASH_SIZE=0xf4000
      CONFIG_PM_EXTERNAL_FLASH_BASE=0

   These options enable the QSPI NOR flash driver and the external flash of the nRF52840 DK.
   Multi-threading is enabled by default, so you do not need to enable it again.
