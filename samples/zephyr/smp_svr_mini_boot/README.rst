.. _mcuboot_minimal_configuration:

MCUboot minimal configuration
#############################

.. contents::
   :local:
   :depth: 2

The MCUboot minimal configuration sample provides the minimal and recommended settings for MCUboot on :ref:`nRF54L15 DK <ug_nrf54l>` and :ref:`nRF54H20 DK <ug_nrf54h>` using the Zephyr's :zephyr:code-sample:`smp-svr` sample, where MCUboot is configured as a sub-image.

Overview
********

This sample shows how to configure MCUboot for secure boot and Device Firmware Update (DFU) capabilities using the Zephyr RTOS.
The MCUboot is configured to utilize hardware cryptography with the :ref:`ED25519 signature <ug_crypto_kmu_psa_key_programming_model>`. Additionally, for nRF54L15 DK  :ref:`Key Management Unit (KMU) <ug_kmu_guides_cracen_overview>` for secure key storage is used.
The setup uses LTO and disables non-essential functionalities to downsize the MCUboot non-volatile memory footprint.

The SMP server sample is configured to support Bluetooth® LE and shell for the MCUmgr protocol, which facilitates image management and OS commands.

To achieve minimal size, direct-xip mode can be used, though you can build the sample with the swap using move mode as well.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Building and running
********************

.. |sample path| replace:: :file:`samples/zephyr/smp_svr_mini_boot`

.. include:: /includes/build_and_run.txt

For nRF54L15 DK, make sure you are building your project with the :kconfig:option:`SB_CONFIG_MCUBOOT_GENERATE_DEFAULT_KMU_KEYFILE` Kconfig option enabled.

For direct-xip mode, you must build the sample with the :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP` Kconfig option enabled.
For swap using move mode, use the :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_SWAP_USING_MOVE` Kconfig option instead.

You will notice that the size of MCUboot is significantly reduced in comparison to the default configuration.

If you are using command line, you must run the ``west flash --erase`` command to enable KMU provisioning.

Testing
=======

After programming the device, complete the following steps to verify its functionality:

1. Connect to the device's serial console.
#. Reset the device.
#. Observe the boot logs confirming the successful start of the SMP Server application, as shown in the serial console output:

   .. code-block:: console

      *** Booting nRF Connect SDK v3.0.99-1d43aec8a694 ***
      *** Using Zephyr OS v4.1.99-a6a46c1
      [00:00:00.005,669] <inf> littlefs: FS at rram-controller@5004b000:0xc9000 is 4 0x1000-byte blocks...
      [00:00:00.007,960] <inf> smp_bt_sample: Advertising successfully started
      [00:00:00.007,978] <inf> smp_sample: build time: Jul  2 2025 13:59:16
      uart:~$

You can now test the sample with other functionalities.
For instance, you can rebuild the modified project and follow the steps on :ref:`how to perform a DFU <ug_nrf54l_developing_ble_fota_steps_testing>`.
