.. _nrf_smp_svr_sample:

MCUboot SMP Server
##################

.. contents::
   :local:
   :depth: 2

The MCUboot SMP Server sample demonstrates firmware update using the Simple Management Protocol (SMP) with MCUboot.
It provides a starting point for working with multiple MCUboot configurations.
You can update the device over UART or Bluetooth® Low Energy.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

For update over UART, the `nRF Util development tool`_ ``mcu-manager`` command is recommended (``nrfutil mcu-manager``).

For update over Bluetooth Low Energy, you need the nRF Device Manager app:

* `nRF Device Manager mobile app for Android`_
* `nRF Device Manager mobile app for iOS`_

Overview
********

This sample implements an SMP server.
SMP is a basic transfer encoding used with the MCUmgr management protocol.
For more information about MCUmgr and SMP, see :ref:`device_mgmt`.

The sample supports the serial (UART) MCUmgr transport by default.

Bluetooth LE transport is supported when building with the overlay that enables it (for example, :file:`overlay-bt.conf`).

The sample is built with sysbuild and includes MCUboot as the bootloader.
Various build configurations are provided for different boards, MCUboot modes (including swap, direct-XIP and overwrite), signature types, and optional features such as encryption and compression, as well as external flash support.

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
   * - sample.dfu.smp_svr.serial
     - UART
     - Swap
     -
   * - sample.dfu.smp_svr.bt
     - Bluetooth LE
     - Swap
     -
   * - sample.dfu.smp_svr.mcuboot_flags.direct_xip_withrevert
     - UART
     - Direct-XIP
     -
   * - sample.dfu.smp_svr.encryption.rsa
     - UART
     - Swap
     - Encryption enabled, signature: RSA
   * - sample.dfu.smp_svr.encryption.ecdsa_p256
     - UART
     - Swap
     - Encryption enabled, signature: ECDSA P-256
   * - sample.dfu.smp_svr.encryption.ed25519
     - UART
     - Swap
     - Encryption enabled, signature: Ed25519
   * - sample.dfu.smp_svr.signature.none
     - UART
     - Swap
     -
   * - sample.dfu.smp_svr.signature.ed25519
     - UART
     - Swap
     - Signature using Ed25519
   * - sample.dfu.smp_svr.signature.ecdsa_p256
     - UART
     - Swap
     - Signature using ECDSA P-256
   * - sample.dfu.smp_svr.signature.pure
     - UART
     - Swap
     - Signature using Ed25519 (pure)
   * - sample.dfu.smp_svr.kmu.basic
     - UART
     - Swap
     - KMU-based signature (Ed25519)
   * - sample.dfu.smp_svr.nsib.kmu.basic
     - UART
     - Swap
     - NSIB with KMU-based signature
   * - sample.dfu.smp_svr.kmu.revocation
     - UART
     - Swap
     - KMU with key revocation
   * - sample.dfu.smp_svr.nrf_compress.basic
     - UART
     - Overwrite-only
     - Compressed image support
   * - sample.dfu.smp_svr.nrf_compress.encryption_rsa
     - UART
     - Overwrite-only
     - Compression and encryption (RSA)
   * - sample.dfu.smp_svr.nrf_compress.encryption_ecdsa_p256
     - UART
     - Overwrite-only
     - Compression and encryption (ECDSA P-256)
   * - sample.dfu.smp_svr.nrf_compress.encryption_ed25519
     - UART
     - Overwrite-only
     - Compression and encryption (Ed25519)
   * - sample.dfu.smp_svr.bt.nrf5340dk.ext_flash
     - Bluetooth LE
     - Swap
     - External flash
   * - sample.dfu.smp_svr.bt.nrf54l15
     - Bluetooth LE
     - Swap
     -
   * - sample.dfu.smp_svr.bt.nrf54l15dk.ext_flash
     - Bluetooth LE
     - Swap
     - External flash
   * - sample.dfu.smp_svr.bt.nrf54l15dk.ext_flash.pure_dts
     - Bluetooth LE
     - Swap
     - External flash (pure DTS, no partition manager)
   * - sample.dfu.smp_svr.bt.nrf54l15dk.ext_flash.sqspi
     - Bluetooth LE
     - Swap
     - External flash, communication through software QSPI on FLPR core
   * - sample.dfu.smp_svr.bt.nrf54h20dk
     - Bluetooth LE
     - Swap
     -
   * - sample.dfu.smp_svr.bt.nrf54h20dk.direct_xip_withrevert
     - Bluetooth LE
     - Direct-XIP
     -
   * - sample.dfu.smp_svr.serial.nrf54h20dk.ecdsa
     - UART
     - Swap
     - Signature using ECDSA P-256
   * - sample.dfu.smp_svr.bt.nrf54h20dk.direct_xip_withrequests
     - Bluetooth LE
     - Direct-XIP
     -
   * - sample.dfu.smp_svr.bt.nrf54h20dk.ext_flash
     - Bluetooth LE
     - Swap
     - External flash
   * - sample.dfu.smp_svr.ram_load
     - UART
     - Swap
     - RAM load
   * - sample.dfu.smp_svr.ram_load.kmu
     - UART
     - Swap
     - RAM load with KMU-based signature (Ed25519)

Building and running
********************

.. |sample path| replace:: :file:`samples/dfu/smp_svr`

.. include:: /includes/build_and_run.txt

The steps described above build the basic configuration with the UART transport and MCUboot using swap mode.
Other build configurations are available for different boards, MCUboot modes, signature types, and optional features.

When building a ``<configuration_name>`` configuration from the :file:`sample.yaml` file, you can use the following command:

.. code-block:: console

    west build -b <board_name> -T ./<configuration_name>

Alternatively, you can manually specify the ``extra_args`` parameters listed in the configuration description.
Pass each parameter with the ``-D`` prefix after ``--``:

.. code-block:: console

    west build -b <board_name> -- -D<extra_arg1> -D<extra_arg2> ...

For example, build the ``sample.dfu.smp_svr.bt.nrf54h20dk`` configuration by running the following command:

.. code-block:: console

    west build -b nrf54h20dk/nrf54h20/cpuapp -T ./sample.dfu.smp_svr.bt.nrf54h20dk

Alternatively, review the ``extra_args`` parameters defined for this configuration:

.. code-block:: console

    EXTRA_CONF_FILE="overlay-bt.conf"
    SB_CONFIG_NETCORE_IPC_RADIO=y
    SB_CONFIG_NETCORE_IPC_RADIO_BT_HCI_IPC=y
    mcuboot_CONFIG_NRF_SECURITY=y
    mcuboot_CONFIG_MULTITHREADING=y
    CONFIG_MCUMGR_GRP_IMG_ALLOW_CONFIRM_NON_ACTIVE_IMAGE_ANY=y

You can then build the configuration by explicitly passing these parameters:

.. code-block:: console

    west build -b nrf54h20dk/nrf54h20/cpuapp -- -DEXTRA_CONF_FILE="overlay-bt.conf" -DSB_CONFIG_NETCORE_IPC_RADIO=y -DSB_CONFIG_NETCORE_IPC_RADIO_BT_HCI_IPC=y -Dmcuboot_CONFIG_NRF_SECURITY=y -Dmcuboot_CONFIG_MULTITHREADING=y -DCONFIG_MCUMGR_GRP_IMG_ALLOW_CONFIRM_NON_ACTIVE_IMAGE_ANY=y

Testing
=======

The following steps are common for all configurations:

#. Build the sample.
#. Program the firmware to your development kit.
#. Observe the following output on the terminal:

   .. code-block:: console

      [00:00:00.007,978] <inf> smp_sample: build time: <BUILD TIME>

   ``<BUILD TIME>`` indicates the build time.

Performing DFU is specific to the configuration.

Testing over UART
-----------------

The following steps show how to perform DFU using the `nRF Util development tool`_ ``mcu-manager`` command over UART.
Not all steps apply to all configurations.
For example, if the overwrite only mode is used, no confirmation is needed.

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


   The number of images can differ depending on the configuration and board (for example, separate radio core, or merged/split images).

#. Build the second version of the sample.
   For simplicity, it is assumed to be built in the :file:`build_v2` directory.

#. Upload the second version of the image to the device using the SMP protocol:

   .. code-block:: console

      nrfutil mcu-manager serial image-upload --serial-port /dev/ttyACM0 --image-number 0 --firmware ./build_v2/smp_svr/zephyr/zephyr.signed.bin

#. List the images on the device again to get the hash of the second version of the image:

   .. code-block:: console

      nrfutil mcu-manager serial image-list --serial-port /dev/ttyACM0

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
      Image
        slot: 1
        version: 0.0.0
        hash: 35bd50242ad991aa5fd9374b33bc8ebcd2332fe26cae44f5f32172a62b7c4adcde1467f33b783e7c7090eb6dffc358bcb9a4efb8f50b99d587131418cd5da375
        bootable: true
        pending: false
        confirmed: false
        active: false
        permanent: false

#. Set the test flag for the second version of the image, using the acquired hash:

   .. code-block:: console

      nrfutil mcu-manager serial image-test --serial-port /dev/ttyACM0 --hash 35bd50242ad991aa5fd9374b33bc8ebcd2332fe26cae44f5f32172a62b7c4adcde1467f33b783e7c7090eb6dffc358bcb9a4efb8f50b99d587131418cd5da375

#. Reconnect to the terminal.

#. Reboot the device.

#. Verify that the build time in the terminal output reflects the new version.

#. To make the new version permanent, confirm the image:

   .. code-block:: console

      nrfutil mcu-manager serial image-confirm --serial-port /dev/ttyACM0 --hash 35bd50242ad991aa5fd9374b33bc8ebcd2332fe26cae44f5f32172a62b7c4adcde1467f33b783e7c7090eb6dffc358bcb9a4efb8f50b99d587131418cd5da375

Testing over Bluetooth LE
-------------------------

To perform DFU using the `nRF Connect Device Manager`_ mobile app over Bluetooth LE, follow these steps:

#. Start the `nRF Connect Device Manager`_ mobile app.
#. Follow the testing steps for the FOTA over Bluetooth LE.
   For more information, see the following documentation pages:

   * :ref:`Testing steps for FOTA over Bluetooth LE with nRF52840 <ug_nrf52_developing_ble_fota_steps_testing>`
   * :ref:`Testing steps for FOTA over Bluetooth LE with nRF5340 <ug_nrf53_developing_ble_fota_steps_testing>`
   * :ref:`Testing steps for FOTA over Bluetooth LE with nRF54L15 <ug_nrf54l_developing_ble_fota_steps_testing>`
   * :ref:`Testing steps for FOTA over Bluetooth LE with nRF54H20 <ug_nrf54h_developing_ble_fota_steps_testing>`

#. After the firmware update completes, check the UART output.
   Observe that the build time in the terminal output reflects the new version.

Dependencies
************

This sample uses the following |NCS| library:

* :ref:`MCUboot <mcuboot_index_ncs>`
