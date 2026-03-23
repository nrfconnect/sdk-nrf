.. _mcuboot_with_encryption:

MCUboot with encryption enabled
###############################

.. contents::
   :local:
   :depth: 2

The MCUboot with encryption enabled sample demonstrates secure device firmware update (DFU) using MCUboot with encryption enabled.
You will learn how to build encrypted images and deploy them to supported development kits, protecting application code from unauthorized access during updates.
This sample does not contain its own application code.
Instead, it focuses on configuring encryption in MCUboot and generating encrypted DFU images.
To provide a working example, the sample uses the :zephyr:code-sample:`smp-svr` project as its application by directly importing the project's sources in the main :file:`CMakeLists.txt` file.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. note::

   On the nRF54LV10 DK, the :zephyr:code-sample:`smp-svr` configuration is trimmed to fit the application slot size.
   For the nRF54L15 DK, an overlay is used to increase the size of the MCUboot partition.
   It is necessary because the enabled logging features require more space than the default partitioning provides.

Overview
********

This sample provides a practical starting point for using MCUboot with application image encryption enabled.
It walks you through the process of building encrypted firmware images and updating them securely on a supported development kit.

MCUboot in this sample is built with enhanced debug output, making it easier to observe and understand the secure boot process.
For more information on minimal builds and optimization, see the :ref:`MCUboot minimal configuration <mcuboot_minimal_configuration>` documentation.

The firmware update protocol is handled by the :ref:`MCUmgr SMP server<zephyr:mcu_mgr>`, so you can use all related configuration options.

Platform-specific information
=============================

The nRF54L Series platforms use the following cryptographic algorithms:

* ED25519 for digital signature verification.
* X25519 for securely exchanging AES encryption keys during image updates.

.. _mcuboot_with_encryption_config:

Configuration
*************

|config|

Configuration options
=====================

You can use the following Kconfig options to configure the sample:

* :kconfig:option:`CONFIG_FPROTECT` - This option is disabled by default.
  It enables flash protection for the MCUboot code.
  You can disable it for development and enable it for production purposes to prevent MCUboot overwriting at runtime.
  This option is not available on the nRF54H20 DK.
  You can enable the flash protection on this platform using IronSide services (see :ref:`ug_nrf54h20_ironside_protect` for more details).
* :kconfig:option:`CONFIG_MCUBOOT_LOG_LEVEL_DBG` - This option is enabled by default.
  It allows you to easily verify that MCUboot is starting up.
  Disable it for production builds.
* :kconfig:option:`CONFIG_BOOT_SWAP_SAVE_ENCTLV` - Enable this option in the MCUboot configuration if you are performing DFU to an external storage device.
  This ensures that the random AES key used for the currently swapped image is not exposed.
* :kconfig:option:`SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE` - MCUboot uses a default encryption key.
  To override it, adjust this option by setting a path to your custom encryption key file.
* :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` - Use this option to set the application image version for software updates.

To configure the sample to use KMU crypto storage, add ``-DSB_EXTRA_CONF_FILE=sysbuild_kmu.conf`` to the build command line.
This option is supported only on nRF54L Series devices.
This option brings in sysbuild configuration file that selects two additional options:

* :kconfig:option:`SB_CONFIG_MCUBOOT_SIGNATURE_USING_KMU` - This option enables KMU support.
* :kconfig:option:`SB_CONFIG_MCUBOOT_GENERATE_DEFAULT_KMU_KEYFILE` - This option uses the build system to generate and prepare a bundle of default KMU keys to be used with the sample.

  .. caution::
     Do not use this option in a production build.
     This is a security risk.

To configure the sample to use ITS crypto storage, add the ``-DSB_EXTRA_CONF_FILE=sysbuild_its.conf`` option to the build command.
This option is supported only on nRF54H Series devices.
This option brings in sysbuild configuration file that selects one additional option:

* :kconfig:option:`SB_CONFIG_MCUBOOT_SIGNATURE_USING_ITS` - This option enables ITS support.
* :kconfig:option:`SB_CONFIG_MCUBOOT_GENERATE_DEFAULT_KEY_FILE` - This option uses the build system to generate and prepare a bundle of default ITS keys to be used with the sample.

Signature key
*************

Even with encryption enabled, MCUboot relies on signature keys to verify images at each boot.
On some devices, you can store the public signature key in one of the following ways:

* Compile it into the device firmware.
* Use the crypto storage.

If your application is not using crypto storage, you can set a key with the :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_KEY_FILE` Kconfig option.
In this case, MCUboot can only use one signature key file compiled into the firmware.

On devices that store keys in crypto storage, the number of stored keys depends on the specific device and may range from one to several.
Typically, three slots are reserved for storing MCUboot signature keys.
To use MCUboot with crypto storage, you must provision a set of keys to the device in addition to compiling in support for crypto storage.
If you use KMU for signature key storage, follow the instructions in :ref:`ug_kmu_provisioning_overview` to provision the keys.
If you use ITS for signature key storage, follow the instructions in the :ref:`ug_nrf54h20_keys` page to provision the keys.

Security considerations
***********************

See the list of best practices, security-related options, and recommended settings when configuring the sample:

* For secure production builds:

  * Enable flash memory protection.
    Use either :kconfig:option:`CONFIG_FPROTECT` or the IronSide memory protection service, depending on the SoC.
    Also enable :kconfig:option:`CONFIG_BOOT_SWAP_SAVE_ENCTLV`.
  * Manage keys independently instead of relying on automatic key generation.
    As such, do not use :kconfig:option:`SB_CONFIG_MCUBOOT_GENERATE_DEFAULT_KMU_KEYFILE` or :kconfig:option:`SB_CONFIG_MCUBOOT_GENERATE_DEFAULT_KEY_FILE`.

  See the :ref:`mcuboot_with_encryption_config` section for details.

* MCUmgr's shell is enabled by default, allowing you to manage commands using a serial terminal.
* MCUboot accepts unencrypted images in the secondary slot if signature verification passes.
  For higher security, use encrypted images wherever possible.
* KMU key handling:

  * By default, KMU is not used to store keys in MCUboot.
  * If KMU is enabled, the asymmetric private key used for transport encryption of the random AES key is still compiled into MCUboot in plain text.
    Use hardware-backed key storage in production.
  * Do not leave encryption keys or private keys in plain text inside the MCUboot binary.

* ITS key handling:

  * By default, MCUboot does not use ITS to store keys.
  * If you enable ITS, MCUboot still compiles the asymmetric private key for transport encryption of the random AES key into the binary in plain text.
    Use hardware-backed key storage in production.
  * Do not store encryption keys or private keys in plain text in the MCUboot binary.

Building and running
********************

.. |sample path| replace:: :file:`samples/dfu/mcuboot_with_encryption`

.. include:: /includes/build_and_run.txt

By default, the sample builds with KMU support on platforms that support it (nRF54L Series).
Otherwise, the signature key is embedded in the MCUboot binary.
To see the encryption workflow, you must build two application images with different version numbers (for example, set the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` Kconfig option to ``1.0.0`` and ``2.0.0``), using separate build directories.

Testing
=======

After programming the device with the first image, verify its functionality:

1. Connect to the device's serial console.
#. Reset the device.
#. Observe the boot output confirming successful start of the SMP Server application:

   .. code-block:: console

      *** Booting MCUboot v2.3.0-dev-0d263faf6e55 ***
      *** Using nRF Connect SDK v3.1.99-4f1d50b83bfb ***
      *** Using Zephyr OS v4.2.99-164b47d30942 ***

      *** Booting nRF Connect SDK v3.1.99-4f1d50b83bfb ***
      *** Using Zephyr OS v4.2.99-164b47d30942 ***
      [00:00:00.075,512] <inf> bt_sdc_hci_driver: SoftDevice Controller build revision:
                                                  9d dc 12 f9 d0 01 5e 7e  af 5b 84 59 45 12 69 4e
      |......^~ .[.YE.iN
                                                  5e dc 0b 2f                                      |^../
      [00:00:00.076,807] <inf> bt_hci_core: HW Platform: Nordic Semiconductor (0x0002)
      [00:00:00.076,822] <inf> bt_hci_core: HW Variant: nRF54Lx (0x0005)
      [00:00:00.076,836] <inf> bt_hci_core: Firmware: Standard Bluetooth controller (0x00) Version
      157.4828 Build 1577177337
      [00:00:00.077,217] <inf> bt_hci_core: HCI transport: SDC
      [00:00:00.077,269] <inf> bt_hci_core: Identity: C0:0A:89:ED:8F:B9 (random)
      [00:00:00.077,285] <inf> bt_hci_core: HCI: version 6.1 (0x0f) revision 0x30c0, manufacturer 0x0059
      [00:00:00.077,299] <inf> bt_hci_core: LMP: version 6.1 (0x0f) subver 0x30c0
      [00:00:00.077,765] <inf> smp_bt_sample: Advertising successfully started

#. To test encrypted updates, upload the second (encrypted) image using any supported method, such as MCUmgr over Bluetooth, serial shell, or another DFU transport.
   For more information on DFU, see the :ref:`Zephyr Device Firmware Upgrade documentation <zephyr:dfu>`.
#. After uploading the image, mark the new image for test.
#. On reboot, verify that the new firmware version is running.
   If the image is not confirmed, the bootloader will revert to the previous version.
   The image will then be re-encrypted if swapped out of the boot slot.

.. note::

   This sample also accepts unencrypted firmware updates.
   If you upload an unencrypted and properly signed image, MCUboot will successfully boot it.

Further information on image encryption and generating keys
===========================================================

More information on MCUboot support for image encryption can be found in the :ref:`mcuboot_wrapper` section under Encrypted images.
Users should familiarize themselves with this section, as they will need to generate their own keys for encryption key exchange.

.. note::

   The keys used in this sample are publicly known.
   Do not use them in your product under any circumstances.

To learn how to upload custom keys to KMU, see the :ref:`ug_nrf54l_dfu_config` documentation page.
To learn how to upload custom keys to ITS, see the :ref:`ug_nrf54h20_keys` documentation page.

Dependencies
************

The sample depends on following subsystems and libraries:

* :ref:`MCUboot <mcuboot_index_ncs>`
* :ref:`zephyr:mcu_mgr`
* :ref:`nrf_security`
