.. _mcuboot_with_encryption:

MCUboot with Encryption Enabled
################################

.. contents::
   :local:
   :depth: 2

MCUboot reference sample demonstrating how to enable and use encryption.
The sample is based on SMP Server sample as DFU functionality is important part of demonstration.

Overview
********

The sample demonstrates how to build MCUboot with application image encryption enabled and upload encrypted images to target board.
MCUboot, in this sample, is not configured to be optimized down and has extensive debugging enabled, which allows to observe the boot process.
For review of optimizations please consider trying the :ref:`MCUboot minimal configuration <mcuboot_minimal_configuration>` sample.
The sample used with MCUboot is the MCUmgr SMP Server sample, which means that all the MCUmgr configuration options apply to it.

Platform specific information
*****************************

nrf54lXX platforms use ED25519 for signature verification and X25519 for random AES key exchange.

Security warnings
*****************

MCUboot sample has :kconfig:option:`CONFIG_FPROTECT` disabled in MCUboot only to make it easier for uploading sample without erasing entire device when MCUboot update is required, and user should consider turning the option on, for production builds, to prevent MCUboot being overwritten at run-time.
MCUboot sample does not use KMU by default.
MCUmgr sample has shell enabled.
Even with KMU enabled for MCUboot, asymmetric private key, used for transport encryption of random AES key, is compiled into MCUboot in a plain text form.
:kconfig:option:`CONFIG_BOOT_SWAP_SAVE_ENCTLV` should be enabled, in MCUboot configuration, in case
when DFU is performed to external storage device, to avoid random AES key, for currently swapped image, leaking.
MCUboot will not reject unencrypted image in secondary slot and is capable to boot it if signature verification passes.
MCUboot uses default encryption key, to change it, point :kconfig:option:`SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE` to your generated key.


Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Note that nrf54lv10dk has the SMP Server configuration cut down, in comparison to other platforms, to fit into application designated slot.

Sample specific Kconfig options
*******************************

Sample provides :kconfig:option:`SB_CONFIG_SAMPLE_MCUBOOT_ENCRYPTION_KMU`, by default set to `n`, that allows to enable KMU support for storage of signature keys.

Building and running
********************

.. |sample path| replace:: :file:`samples/zephyr/mcuboot_with_encryption`

.. include:: /includes/build_and_run.txt

By default, sample builds without KMU support and with the key built into the MCUboot binary.
To demonstrate working encryption at least two application builds are required.
Here we can achieve that by building the same sample with :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` set to version 1.0.0 and 2.0.0.
Build both applications using separate build directories.

Testing
=======

After programming the device with the first image, complete the following steps to verify its functionality:

1. Connect to the device's serial console.
#. Reset the device.
#. Observe the boot logs confirming the successful start of the SMP Server application, as shown in the serial console output:

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


Now you can check how encryption works. Depending on the selected upload method, you may either use the zephyr.signed.encrypted.bin from the second build image, or dfu_application.zip, which also contains an already encrypted image.
You can upload the second image to the device using one of the supported methods, such as Bluetooth (using MCUmgr), serial shell, or other DFU mechanisms. For more details on available upload methods and instructions, refer to the `Zephyr Device Firmware Upgrade (DFU) documentation <https://docs.zephyrproject.org/latest/guides/device_mgmt/dfu.html>`_.
After uploading, mark the image for test. After reboot you may see, on image list, that the version 2.0.0 has been booted for test, if you have named the image as such.
Unless you confirm the image, the reset will bring back the previous one. Note that image will be re-encrypted when it is swapped out of the boot slot.


Generating X25519 key
=====================

You may generate x25519 key using openssl command:

   .. code-block:: console

      openssl genpkey -algorithm X25519 -out your_new_private_key.pem

