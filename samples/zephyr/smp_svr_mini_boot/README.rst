.. _mcuboot_minimal_configuration:

MCUboot minimal configuration
#############################

.. contents::
   :local:
   :depth: 2

The MCUboot minimal configuration sample provides the minimal and recommended settings for MCUboot on the nRF54L15 DK using the Zephyr's :zephyr:code-sample:`smp-svr` sample, where MCUboot is configured as a sub-image.

Overview
********

This sample shows how to configure MCUboot for secure boot and Device Firmware Update (DFU) capabilities using the Zephyr RTOS.
The MCUboot is configured to utilize hardware cryptography with the :ref:`ED25519 signature <ug_nrf54l_cryptography>` and the :ref:`Key Management Unit (KMU) <ug_nrf54l_developing_basics_kmu>` for secure key storage.
The setup uses LTO, direct-xip mode and disables non-essential functionalities to downsize the MCUboot non-volatile memory footprint.

The SMP server sample is configured to support Bluetooth® LE and shell for the MCUmgr protocol, which facilitates image management and OS commands.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Building and running
********************

.. |sample path| replace:: :file:`samples/zephyr/smp_svr_mini_boot`

.. include:: /includes/build_and_run.txt

Make sure you are building your project with the ``SB_CONFIG_MCUBOOT_GENERATE_DEFAULT_KMU_KEYFILE`` Kconfig option enabled.
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
