.. _ug_nrf54l_dfu_config:

Configuring DFU and MCUboot
###########################

.. contents::
   :local:
   :depth: 2

This page provides an overview of Device Firmware Update (DFU) for the nRF54L Series devices, detailing the necessary steps, configurations, and potential risks involved in setting up secure boot and firmware updates.

NSIB and MCUboot bootloader settings
************************************

You can combine the NSIB with MCUboot, or use MCUboot as a standalone bootloader.
If you want to learn how to start using the available bootloaders in your application, refer to the :ref:`ug_bootloader_adding_sysbuild` page.
For full introduction to the bootloader and DFU solution, see :ref:`ug_bootloader_mcuboot_nsib`.

You must select a sample that supports DFU to ensure you can correctly test its functionality.
In the following subsections, the Zephyr's :zephyr:code-sample:`smp-svr` sample is used as an example.

Configuring NSIB with MCUboot
=============================

.. note::

   The nRF54LM20A SoC currently does not support this configuration.

To configure and build firmware using NSIB and MCUboot, complete the following steps:

1. Navigate to the :file:`zephyr/samples/subsys/mgmt/mcumgr/smp_svr` directory.

#. Build the firmware with the following configurations:

   .. parsed-literal::
      :class: highlight

      west build -b *board_target* -p -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y -DSB_CONFIG_SECURE_BOOT_APPCORE=y

#. Provision the device and upload the key:

   .. parsed-literal::
      :class: highlight

      west ncs-provision upload -k ./build/GENERATED_NON_SECURE_SIGN_KEY_PRIVATE.pem -s nrf54l15 --keyname BL_PUBKEY

#. Flash the firmware onto the device:

   .. parsed-literal::
      :class: highlight

      west flash

Configuring MCUboot only
========================

To build with MCUboot only, complete the following steps:

1. Navigate to the :file:`zephyr/samples/subsys/mgmt/mcumgr/smp_svr` directory.

#. Build the firmware:

   .. parsed-literal::
      :class: highlight

      west build -b nrf54l15dk/nrf54l15/cpuapp -p -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y

#. Flash the firmware onto the device:

   .. parsed-literal::
      :class: highlight

      west flash

.. note::
   The standalone NSIB configuration is not supported.

NVM protections
***************

Non-volatile memory protection is crucial for ensuring the integrity and immutability of firmware code.
The protection is designed to disable read, write, or execute permissions when they are not required.
There are two types of protection mechanisms:

* NSIB protection - The :kconfig:option:`SB_CONFIG_SECURE_BOOT_BOOTCONF_LOCK_WRITES` Kconfig option enables a lock on NSIB from power-up, which is only removable through a chip erase.
  This protection mechanism uses a single 31 kB resistive random access memory controller's (RRAMC) region 3, activated at IC’s power-up if the UICR is programmed accordingly.
  Enabling this lock, also disables the :kconfig:option:`CONFIG_FPROTECT_ALLOW_COMBINED_REGIONS` Kconfig option, restricting FPROTECT to sizes that are up to 31 kB.
* MCUboot protection - MCUboot can be protected from overwrites using the FPROTECT library.
  If MCUboot is within 31 kB, the device will use RRAMC’s region 4.
  For sizes exceeding 31 kB, both regions 3 and 4 are used, provided that MCUboot is configured standalone (without NSIB).
  To enable the protection mechanism for sizes between 31 kB and 62 kB, configure the :kconfig:option:`CONFIG_FPROTECT_ALLOW_COMBINED_REGIONS` Kconfig option.

FPROTECT is enabled by default on the nR54L platform.
The build system automatically selects the appropriate setup based on the inclusion of NSIB and MCUboot.

RAM cleanup
***********

To prevent data leakage, both NSIB and MCUboot can clear out their RAM upon completion of execution.
This feature is controlled by the :kconfig:option:`CONFIG_SB_CLEANUP_RAM` Kconfig option.

Supported signatures
********************

MCUboot accommodates ed25519 and ed25519-pure signatures.
The latter signature is recommended, but you cannot use it with external memory.
NSIB supports only the ed25519-pure signature, which is hardcoded.

Signature keys
**************

The :ref:`Key Management Unit (KMU)<ug_kmu_guides_cracen_overview>` retains the keys necessary for image signature verification, which must be uploaded simultaneously with the application during the flashing process.
Currently, encryption keys are not stored in the KMU.
In the case of nRF54LM20A SoC, keys are compiled into the bootloader.

.. note::
   NSIB regenerates its key with each build unless it is specified in the command line.
   This could result in unexpected behavior.

Runtime revocation
******************

.. note::
   The support for this feature is currently :ref:`experimental <software_maturity>`.

MCUboot can invalidate image verification keys through the ``CONFIG_BOOT_KMU_KEYS_REVOCATION`` Kconfig option.
Enable this option during the MCUboot build process if there is a risk that images signed with a compromised key might contain critical vulnerabilities.
The revocation of keys is triggered during an update when a new image is signed with a newer key.

.. warning::
   You must enable the ``CONFIG_BOOT_KMU_KEYS_REVOCATION`` Kconfig option when creating your project.
   If this option is not activated initially, it will not be possible to enable it later, making this functionality unavailable and potentially exposing your project to security issues.

Key invalidation occurs after reboot, and the confirmed application invalidates keys of lower indices.
A valid signature verification must precede any key invalidation.
The last remaining key cannot be invalidated.

.. note::
   Once the application running in test mode confirms its stability, it will reboot the device to enable MCUboot to invalidate the keys.
   Until this reboot occurs, the application should avoid collecting further firmware updates or performing any erase or write operations on the image storage partition.

Images encryption
*****************

MCUboot supports AES-encrypted images on the nRF54L15 SoC, using ECIES-X25519 for key exchange.
For detailed information on ECIES-X25519 support, refer to the :ref:`ug_nrf54l_ecies_x25519` documentation page.

Image update types
******************

MCUboot supports various methods for updating firmware images.
For the nRF54L platform, you can use :ref:`swap and direct-xip modes<ug_bootloader_main_config>`.
