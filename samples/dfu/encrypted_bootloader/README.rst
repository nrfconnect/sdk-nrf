.. _encrypted_bootloader:

Encrypted bootloader
####################

.. contents::
   :local:
   :depth: 2

The Encrypted bootloader sample demonstrates secure device firmware update (DFU) with image encryption enabled for both the application and MCUboot.

This sample uses the :zephyr:code-sample:`smp-svr` project as its application by directly importing the project's sources in the main :file:`CMakeLists.txt` file.

For a simpler setup that only encrypts application images (single MCUboot instance, no b0 chain), see :ref:`mcuboot_with_encryption`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

For update over UART, the `nRF Util development tool`_ ``mcu-manager`` command is recommended (``nrfutil mcu-manager``).

Overview
********

This sample implements an SMP server and demonstrates how to perform encrypted updates of the application and MCUboot.

To keep the sample simple, only the serial (UART) MCUmgr transport is provided.
To extend the sample to support other transports, add the appropriate overlay files.

To support the MCUboot updates, the sample uses a two-stage boot chain:

* b0 - Validates and selects decrypted MCUboot images from either the ``s0_partition`` or ``s1_partition`` partitions.
* MCUboot - Validates and selects an appropriate destination slot of the encrypted update candidate stored inside the ``slot1_partition`` partition.
  The detection of the update candidate type may be based on either the load address or the UUID values in the image header.
  Once the destination slot is determined, the MCUboot swaps and decrypts the encrypted update candidate into the corresponding primary slot (either ``slot0_partition`` or ``slot1_partition`` partition).

Both stages use the same encryption and signature machinery.
Images are signed with ED25519 and encrypted for transport with X25519/AES as described in :ref:`mcuboot_wrapper` (Encrypted images).

The various sample build configurations are defined in the :file:`sample.yaml` file.
The following table describes the available configurations.

.. note::
   Not all configuration and board combinations are supported.
   Refer to the :file:`sample.yaml` file for the list of supported boards for each configuration.

.. list-table::
   :widths: auto
   :header-rows: 1

   * - Configuration
     - Transport
     - MCUboot mode
     - Other features
   * - sample.dfu.encrypted_mcuboot.load_addr
     - UART
     - Swap
     - Uses load address to determine the update candidate.
   * - sample.dfu.encrypted_mcuboot.uuid
     - UART
     - Swap
     - Uses UUID to determine the update candidate.

Platform-specific information
=============================

The nRF54L Series platforms use the following cryptographic algorithms:

* ED25519 for digital signature verification.
* X25519 for securely exchanging AES encryption keys during image updates.

Building and running
********************

.. |sample path| replace:: :file:`samples/dfu/encrypted_bootloader`

.. include:: /includes/build_and_run.txt

The steps described build the basic configuration with the UART transport and MCUboot using load address to determine the update candidate.
Other build configurations are available for different boards, MCUboot modes, signature types, and optional features.

When building a ``<configuration_name>`` configuration from the :file:`sample.yaml` file, you can use the following command:

.. code-block:: console

    west build -b <board_name> -T ./<configuration_name>

Testing
=======

Complete the following steps to perform DFU using the `nRF Util development tool`_ ``mcu-manager`` command over UART:

1. Build the sample.
#. Program the firmware to your development kit.
#. Observe the following output on the terminal:

   .. code-block:: console

      [00:00:00.007,978] <inf> smp_sample: build time: <BUILD TIME>

   ``<BUILD TIME>`` indicates the build time.

#. Disconnect the terminal to avoid conflicts.
   The SMP communication uses the same serial port as the log output.
   Some terminal emulators might not require this step.
#. List the images on the device.
   The serial port might differ depending on your development kit and operating system.

   .. code-block:: console

       nrfutil mcu-manager serial image-list --serial-port <serial_port>

   The output resembles the following example:

   .. code-block:: console

      Image
        slot: 0
        version: 0.0.0
        hash: ef8cc97cf2c873bde983a3a133cbc7468ef14cd42092ee131b00275aea0429522f256020129a51f9309078b1ca310ed1f76a091c484a496f8d3f7fd565d13468
        bootable: true
        pending: false
        confirmed: true
        active: true
        permanent: false

   The MCUboot image is not listed by MCUmgr commands, because it is not a regular MCUboot image.
   You can identify the active MCUboot version and slot based on the first stage bootloader logs:

   .. code-block:: console

      *** Booting nRF Connect SDK v3.3.99-74687ea51dbd ***
      *** Using Zephyr OS v4.4.0-ab3701f67f47 ***
      Fprotect disabled. No protection applied.
      LCS-awareness disabled.
      Attempting to boot slot 0.
      Attempting to boot from address 0x10800.
      I: Trying to get Firmware version
      I: Firmware signature verified.
      Firmware version 2

#. Rebuild the sample with a new version.

   * Set the application version using the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` Kconfig option.
   * Set the MCUboot version using the :kconfig:option:`CONFIG_FW_INFO_FIRMWARE_VERSION` Kconfig option.

   You can set both by providing additional arguments to the build command:

   .. code-block:: console

      west build -b <board_name> -T ./<configuration_name> -- -DCONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION="1.0.0+0" -Dmcuboot_CONFIG_FW_INFO_FIRMWARE_VERSION=3

   The sample uses auto-generated keys for signing and encryption.
   To keep the new build compatible with the original one, do not build the second version using the pristine option.

#. Upload the encrypted second version of the MCUboot image to the device using the SMP protocol:

   .. code-block:: console

      nrfutil mcu-manager serial image-upload --serial-port /dev/ttyACM1 --image-number 1 --firmware ./build/signed_by_mcuboot_and_b0_mcuboot_s1_variant.bin

#. List the images on the device again to get the hash of the second version of the MCUboot image:

   .. code-block:: console

      nrfutil mcu-manager serial image-list --serial-port /dev/ttyACM1

   The output resembles the following example:

   .. code-block:: console

      Image
         slot: 0
         version: 0.0.0
         hash: b9f9f450cf5674a58a5bf2d99afe56f145c5e17e0dbb771c5abbf26610af0a05ffb4ef5452cc315a0ce084287152b6d3a94998470487a070490685c0dc8b9819
         bootable: true
         pending: false
         confirmed: true
         active: true
         permanent: false
      Image
         slot: 1
         version: 0.0.0
         hash: 631f61740f3ecf5955570a9b608364555df93af1f3fd0250e7ed9433dd9f97abc61e888f6916bc831fa07f916e89301d51d059a61a712e487b0aace4832c0ec5
         bootable: true
         pending: false
         confirmed: false
         active: false
         permanent: false

#. Set the confirmed flag for the second version of the image, using the acquired hash:

   .. code-block:: console

      nrfutil mcu-manager serial image-confirm --serial-port /dev/ttyACM1 --hash 631f61740f3ecf5955570a9b608364555df93af1f3fd0250e7ed9433dd9f97abc61e888f6916bc831fa07f916e89301d51d059a61a712e487b0aace4832c0ec5

   The MCUboot updates do not support the test flag.
   If the MCUboot update candidate for an inactive slot is deteted, the slot is overwritten with the new image.

#. Reconnect to the terminal.
#. Reboot the device.
#. Verify that the new MCUboot version is running.

   The device should first boot the old MCUboot:

   .. code-block:: console

      *** Booting nRF Connect SDK v3.3.99-74687ea51dbd ***
      *** Using Zephyr OS v4.4.0-ab3701f67f47 ***
      Fprotect disabled. No protection applied.
      LCS-awareness disabled.
      Attempting to boot slot 0.
      Attempting to boot from address 0x10800.
      I: Trying to get Firmware version
      I: Firmware signature verified.
      Firmware version 2

   Detect the update candidate in the application's secondary slot:

   .. code-block:: console

      *** Booting MCUboot v2.3.0-dev-f11a4d2b3a17 ***
      *** Using nRF Connect SDK v3.3.99-74687ea51dbd ***
      *** Using Zephyr OS v4.4.0-ab3701f67f47 ***
      I: Starting bootloader
      I: LCS-awareness disabled, skipping LCS check
      I: Primary image: magic=bad, swap_type=0x3, copy_done=0x2, image_ok=0x2
      I: Secondary image: magic=good, swap_type=0x3, copy_done=0x3, image_ok=0x1
      I: Boot source: none
      I: Primary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
      I: Secondary image: magic=good, swap_type=0x3, copy_done=0x3, image_ok=0x1

   Decrypt the MCUboot update candidate and swap it into the inactive slot using NSIB routines:

   .. code-block:: console

      I: Image index: 1, Swap type: perm
      I: Starting swap using nsib algorithm.

   After the swap is complete, the device will reboot itself to switch to the new MCUboot version:

   .. code-block:: console

      *** Booting nRF Connect SDK v3.3.99-74687ea51dbd ***
      *** Using Zephyr OS v4.4.0-ab3701f67f47 ***
      Fprotect disabled. No protection applied.
      LCS-awareness disabled.
      Attempting to boot slot 1.
      Attempting to boot from address 0x25800.
      I: Trying to get Firmware version
      I: Firmware signature verified.
      Firmware version 3

#. Disconnect the terminal to avoid conflicts.
   The SMP communication uses the same serial port as the log output.
   Some terminal emulators might not require this step.
#. Upload the encrypted second version of the application image to the device using the SMP protocol:

   .. code-block:: console

      nrfutil mcu-manager serial image-upload --serial-port /dev/ttyACM1 --image-number 0 --firmware ./build/encrypted_bootloader/zephyr/zephyr.signed.encrypted.bin

#. List the images on the device again to get the hash of the second version of the application image:

   .. code-block:: console

      nrfutil mcu-manager serial image-list --serial-port /dev/ttyACM1

   The output resembles the following example:

   .. code-block:: console

      Image
         slot: 0
         version: 0.0.0
         hash: b9f9f450cf5674a58a5bf2d99afe56f145c5e17e0dbb771c5abbf26610af0a05ffb4ef5452cc315a0ce084287152b6d3a94998470487a070490685c0dc8b9819
         bootable: true
         pending: false
         confirmed: true
         active: true
         permanent: false
      Image
         slot: 1
         version: 1.0.0
         hash: b192841fbc9a158ff374c56ce0922ed13683d50cce50e32915984df077043ea802da9679bd651e57ce652f39f08cb034385fe135510731dd015770b526ff1bfd
         bootable: true
         pending: false
         confirmed: false
         active: false
         permanent: false

#. Set the test flag for the second version of the image, using the acquired hash:

   .. code-block:: console

      nrfutil mcu-manager serial image-test --serial-port /dev/ttyACM1 --hash b192841fbc9a158ff374c56ce0922ed13683d50cce50e32915984df077043ea802da9679bd651e57ce652f39f08cb034385fe135510731dd015770b526ff1bfd

#. Reconnect to the terminal.
#. Reboot the device.
#. Verify that the build time in the terminal output reflects the new version.
#. To make the new version permanent, confirm the image:

   .. code-block:: console

      nrfutil mcu-manager serial image-confirm --serial-port /dev/ttyACM1 --hash b192841fbc9a158ff374c56ce0922ed13683d50cce50e32915984df077043ea802da9679bd651e57ce652f39f08cb034385fe135510731dd015770b526ff1bfd

Dependencies
************

The sample depends on following subsystems and libraries:

* :ref:`MCUboot <mcuboot_index_ncs>`
* :ref:`zephyr:mcu_mgr`
* :ref:`nrf_security`
