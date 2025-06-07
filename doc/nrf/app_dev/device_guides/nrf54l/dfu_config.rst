.. _ug_nrf54l_dfu_config:

Configuring DFU and MCUboot
###########################

This page provides an overview of Device Firmware Update (DFU) for the nRF54L Series devices, detailing the necessary steps and configurations for setting up secure boot and firmware updates.

NSIB and MCUboot bootloader settings
************************************

You can combine the NSIB with MCUboot, or use MCUboot as a standalone bootloader.
If you wish to know how to start using the available bootloaders in your application, see the :ref:`ug_bootloader_adding_sysbuild` page.
For full introduction to the bootloader and DFU solution, see :ref:`ug_bootloader_mcuboot_nsib`.

You must select a sample that supports DFU to ensure you can correctly test its functionality.
In the following subsections, the Zephyr's :zephyr:code-sample:`smp-svr` sample is used as an example.

Configuring NSIB and MCUboot
============================

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

Non-volatile memory protections are crucial for ensuring the integrity and immutability of firmware code.
These protections are designed to disable read, write, or execute permissions when they are not required.
There are two types of protection mechanisms:

* NSIB protection - The ``SB_CONFIG_SECURE_BOOT_BOOTCONF_LOCK_WRITES`` Kconfig option enables a lock on NSIB from power-up, which is only removable through a chip erase.
  This protection uses a single 31kB RRAMC’s region (region 3), activated at IC’s power-up if the UICR is programmed accordingly.
  Enabling this lock, also disables the :kconfig:option:`CONFIG_FPROTECT_ALLOW_COMBINED_REGIONS` Kconfig option, restricting FPROTECT to sizes that are up to 31kB.
* MCUboot protection - MCUboot can be protected from overwrites using the FPROTECT library.
  If MCUboot is within 31kB, the device will use RRAMC’s region 4.
  For sizes exceeding 31kB, both regions 3 and 4 are used, provided MCUboot is configured standalone (without NSIB).
  To determine protections for sizes between 31kB and 62kB, configure the :kconfig:option:`CONFIG_FPROTECT_ALLOW_COMBINED_REGIONS` Kconfig option.

RAM cleanup
***********

To prevent data leakage, both NSIB and MCUboot can clear out their RAM upon completion of execution.
This feature is controlled by the :kconfig:option:`CONFIG_SB_CLEANUP_RAM` Kconfig option.

Supported signatures
********************

MCUboot accommodates ed25519 and ed25519-pure signatures.
The latter signature is recommended, but it cannot be used with external memories.
NSIB supports only the ed25519-pure signature, which is hardcoded.

Signature keys
**************

:ref:`Key Management Unit (KMU)<ug_nrf54l_developing_basics_kmu>` retains the keys necessary for image signature verification, which must be uploaded simultaneously with the application during the flashing process.
Currently, encryption keys are not stored in KMU.

.. note::
   NSIB regenerates its key with each build unless it is specified in the command-line.
   This could result to unexpected behavior.

Runtime revocation
******************

.. note::
   The support for this feature is currently :ref:`experimental <software_maturity>`.

MCUboot can invalidate image verification keys through the ``CONFIG_BOOT_KMU_KEYS_REVOCATION`` Kconfig option.
You should enable this option if images signed with a compromised key contain critical vulnerabilities.
Key invalidation occurs after reboot, and the confirmed application invalidates keys of lower indices.
A valid signature verification must precede any key invalidation.
The last remaining key cannot be invalidated.

Images encryption
*****************

For detailed information on ECIES-X25519 support, refer to the *Encrypting MCUboot images with AES and ECIES-X25519 key exchange* documentation page. <LINK TBA>

Image update types
******************

For updating firmware images, both :ref:`swap and direct-xip modes<ug_bootloader_main_config>` are recommended.
