.. _ug_nrf54h20_mcuboot_dfu:

Configuring DFU and MCUboot
###########################

.. contents::
   :local:
   :depth: 2

This page provides an overview of Device Firmware Update (DFU) for the nRF54H Series devices, detailing the necessary steps, configurations, and potential risks involved in setting up secure boot and firmware updates.

On the nRF54H20 SoC, you can use MCUboot as a *standalone immutable bootloader*.
If you want to learn how to start using MCUboot in your application, refer to the :ref:`ug_bootloader_adding_sysbuild` page.
For full introduction to the bootloader and DFU solution, see :ref:`ug_bootloader_mcuboot_nsib`.

.. note::
   |NSIB| is not supported on the nRF54H20 SoC.

You must select a sample that supports DFU to ensure you can correctly test its functionality.
In the following subsections, the Zephyr's :zephyr:code-sample:`smp-svr` sample is used as an example.

Configuring MCUboot on the nRF54H20 DK
**************************************

To build your application with MCUboot, complete the following steps:

1. Navigate to the :file:`zephyr/samples/subsys/mgmt/mcumgr/smp_svr` directory.

#. Build the firmware:

   .. parsed-literal::
      :class: highlight

      west build -b nrf54h20dk/nrf54h20/cpuapp -p -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y

#. Flash the firmware onto the device:

   .. parsed-literal::
      :class: highlight

      west flash

NVM protections
***************

Non-volatile memory protection is crucial for ensuring the integrity and immutability of firmware code.
The protection disables read, write, or execute permissions when they are not required.

MCUboot can be protected from overwrites using the FPROTECT library.
If MCUboot is within 31 kB, the device will use RRAMC's region 4.
For sizes exceeding 31 kB, both regions 3 and 4 are used, provided that MCUboot is configured standalone.
To enable the protection mechanism for sizes between 31 kB and 62 kB, configure the :kconfig:option:`CONFIG_FPROTECT_ALLOW_COMBINED_REGIONS` Kconfig option.

RAM cleanup
***********

To prevent data leakage, MCUboot can clear out its RAM upon completion of execution.
This feature is controlled by the :kconfig:option:`CONFIG_SB_CLEANUP_RAM` Kconfig option.

Supported signatures
********************

MCUboot accommodates ed25519 and ed25519-pure signatures.
The pure signature is recommended, but you cannot use it with external memory.

Signature keys
**************

The :ref:`Key Management Unit (KMU)<ug_nrf54l_developing_basics_kmu>` retains the keys necessary for image signature verification, which must be uploaded simultaneously with the application during the flashing process.
Currently, encryption keys are not stored in the KMU.

Runtime revocation
******************

.. note::
   The support for this feature is currently :ref:`experimental <software_maturity>`.

MCUboot can invalidate image verification keys through the ``CONFIG_BOOT_KMU_KEYS_REVOCATION`` Kconfig option.
Enable this option during the MCUboot build process if there is a risk that images signed with a compromised key might contain critical vulnerabilities.
Revocation of keys is triggered during an update when a new image is signed with a newer key.

.. caution::
   You must enable the ``CONFIG_BOOT_KMU_KEYS_REVOCATION`` Kconfig option when creating your project.
   If this option is not activated initially, it will not be possible to enable it later, making this functionality unavailable and potentially exposing your project to security issues.

Key invalidation occurs after reboot, and the confirmed application invalidates keys of lower indices.
A valid signature verification must precede any key invalidation.
The last remaining key cannot be invalidated.

.. note::
   Once the application running in test mode confirms its stability, it reboots the device to enable MCUboot to invalidate the keys.
   The application should avoid collecting further firmware updates or performing any erase or write operations on the image storage partition before the reboot.

Image encryption
****************

MCUboot supports AES-encrypted images on the nRF54H20 SoC.

Image update types
******************

MCUboot supports various methods for updating firmware images.
For the nRF54H platform, you can use :ref:`swap and direct-xip modes<ug_bootloader_main_config>`.
