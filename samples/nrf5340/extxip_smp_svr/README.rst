.. _smp_svr_ext_xip:

.. ncs-sample::
   :title: nRF5340: SMP Server with external XIP

   This sample demonstrates how to split an application that partially resides on internal flash and Quad Serial Peripheral Interface (QSPI) flash by using Execute in place (XIP) and the Simple Management Protocol (SMP) server.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

To update over UART or shell, use the `nRF Util development tool`_ ``mcu-manager`` command (``nrfutil mcu-manager``).

To update over Bluetooth® Low Energy, install the nRF Device Manager app:

* `nRF Device Manager mobile app for Android`_
* `nRF Device Manager mobile app for iOS`_

Alternatively, you can use the `nRF Connect for Mobile`_ app for SMP over Bluetooth LE.
See :ref:`bootloader_quick_start` for an overview of DFU tools.

Do not use the deprecated Go :file:`mcumgr` command-line tool.
For more information, see :ref:`dfu_tools_mcumgr_cli` and :ref:`zephyr:mcumgr_tools_libraries`.

Overview
********

SMP is a basic transfer protocol used by the MCUmgr management library.
This sample shows how to divide an application into internal and external parts using the SMP server.
The internal part operates from the SoC's built-in storage, and the external part runs directly from external memory, using the XIP capability through a QSPI peripheral.

For more information about MCUmgr and SMP, see :ref:`device_mgmt`.

The sample enables the splitting by:

* Enabling the split XIP image feature through the :kconfig:option:`SB_CONFIG_QSPI_XIP_SPLIT_IMAGE` sysbuild Kconfig option.
  This Kconfig option sets a build-system level support for image division and adapts the image-signing support for MCUboot.
* Using the project's CMake file which describes relocation of certain libraries or objects to an external XIP area.
* Using a linker script that describes the external QSPI area.
  (The execution code from the external memory should link to it.)
* Using devicetree memory maps in the :file:`dts` folder for appropriate flash partitioning.

For details on SMP features implemented by the sample, see the Zephyr :zephyr:code-sample:`smp-svr` documentation.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf5340/extxip_smp_svr`
.. include:: /includes/build_and_run.txt

.. parsed-literal::
   :class: highlight

   west build -b *board_target*

The |NCS| build system generates all essential binaries, including the application for internal and external QSPI flash, the networking stack, and MCUboot.
The build process involves signing all the application binaries except for MCUboot which does not require signing in this configuration.

.. note::
   When building for the nRF5340 non-secure (TF-M) target, the direct-xip build is not supported.

To upload MCUboot and a bundle of images to the nRF5340 SoC, use the ``west flash`` command.

Testing
=======

After programming the sample, reset the board and check the serial console output.
You should see a log line similar to the following:

.. code-block:: console

   smp_sample: build time: <BUILD TIME>

``<BUILD TIME>`` indicates the build time of the running firmware.

The following sections describe how to update firmware over shell (UART) or Bluetooth LE.
Both transports use the SMP server in the application.

Upload and map image to MCUboot slot
------------------------------------

For this sample configuration, MCUmgr supports uploading up to three target images.
This allows you to update all parts of the firmware.

When uploading to a specific image, pass the image index to the SMP client.
If you omit the index, the client assumes image ``0``.
The default upload target is the MCUboot secondary slot for the first image pair.

Use the partition node to map MCUboot slots to ``<image>`` for the client:

    +---------------------+--------+---------------+------------------------------+
    | Partition node      | Slot   | <image> index |       Firmware part          |
    +=====================+========+===============+==============================+
    | ``slot1_partition`` | slot-1 |       0       | Internal application part    |
    +---------------------+--------+---------------+------------------------------+
    | ``slot3_partition`` | slot-3 |       1       | Networking                   |
    +---------------------+--------+---------------+------------------------------+
    | ``slot5_partition`` | slot-5 |       2       | QSPI application part        |
    +---------------------+--------+---------------+------------------------------+

When selecting a non-default image index, the client must not send a full erase of slot-1 before the upload.
The legacy Go :file:`mcumgr` tool required the ``-e`` flag for this reason.
``nrfutil mcu-manager`` and `nRF Connect Device Manager`_ handle image selection without that limitation.

Signed update binaries are generated in the build directory.
See :ref:`app_build_output_files` and :ref:`child_parent_to_sysbuild_migration_filename_changes` for file names.
For this sample, the typical paths are the following:

* Internal application: :file:`build/smp_svr_ext_xip/zephyr/zephyr.internal.signed.bin`
* Network core (when enabled): :file:`build/signed_by_mcuboot_and_b0_<net_core_app_name>.bin`
* QSPI application part: :file:`build/smp_svr_ext_xip/zephyr/zephyr.external.signed.bin`

.. note::

   The image indices ``0`` and ``2`` correspond to the internally and externally linked parts of the application.
   These two images are linked.
   You must always upload them together.
   Skip image ``1`` when building a configuration without the network core.

Testing over shell (UART)
-------------------------

The sample enables SMP over the shell backend on the application core UART.
Use ``nrfutil mcu-manager serial`` on a computer to upload images and control the device.

#. Disconnect the serial terminal if it conflicts with SMP traffic on the same port.
#. List images on the device.
   Replace ``<serial_port>`` with your port (for example, :file:`/dev/ttyACM0` or :file:`COM3`).

   .. code-block:: console

      nrfutil mcu-manager serial image-list --serial-port <serial_port>

#. Build a new version of the sample (for example, in the :file:`build_v2` directory) with a higher image version than the firmware on the device.

#. Upload each signed image that is part of the update.
   Repeat the command for every image you need to update, setting ``--image-number`` and ``--firmware`` accordingly:

   .. code-block:: console

      nrfutil mcu-manager serial image-upload --serial-port <serial_port> --image-number 0 --firmware build_v2/extxip_smp_svr/zephyr/zephyr.internal.signed.bin
      nrfutil mcu-manager serial image-upload --serial-port <serial_port> --image-number 1 --firmware build_v2/signed_by_mcuboot_and_b0_<net_core_app_name>.bin
      nrfutil mcu-manager serial image-upload --serial-port <serial_port> --image-number 2 --firmware build_v2/extxip_smp_svr/zephyr/zephyr.external.signed.bin

#. List the images again and note the hash of each uploaded image in the secondary slot:

   .. code-block:: console

      nrfutil mcu-manager serial image-list --serial-port <serial_port>

   You can also print the hash of a local file with :file:`imgtool`:

   .. code-block:: console

      imgtool verify build_v2/smp_svr_ext_xip/zephyr/zephyr.internal.signed.bin

#. Mark each uploaded image for test, using the hash from the previous step:

   .. code-block:: console

      nrfutil mcu-manager serial image-test --serial-port <serial_port> --hash <hash>

   Apply this step to every image that constitutes the complete application.

#. Reset the device:

   .. code-block:: console

      nrfutil mcu-manager serial reset --serial-port <serial_port>

#. Reconnect to the serial terminal and verify that ``<BUILD TIME>`` in the log output reflects the new firmware.

.. note::

   The sample uses MCUboot overwrite-only mode.
   Regardless of whether an image is marked for test or confirm, the incoming image overwrites the previous content during the update.

Testing over Bluetooth® Low Energy
----------------------------------

Use the `nRF Connect Device Manager`_ mobile app on a phone or tablet to interact with the SMP server over Bluetooth LE.
This is the recommended method on hosts where a desktop SMP client over Bluetooth is unavailable (for example, Windows without a supported BLE SMP CLI).

#. Install the app:

   * `nRF Device Manager mobile app for Android`_
   * `nRF Device Manager mobile app for iOS`_

#. Copy the signed update binaries from the build directory to your mobile device.

#. Open the app, scan for the device, and connect.
   The default advertised name matches the Zephyr SMP server sample (``Zephyr`` unless you changed :kconfig:option:`CONFIG_BT_DEVICE_NAME`).

#. Verify SMP connectivity using the echo feature in the app, or proceed directly with a firmware update.

#. For each image you need to update:

   a. Open the :guilabel:`Image` tab.
   #. Tap :guilabel:`Advanced` in the top-right corner so you can select the target image index and upload mode.
   #. Tap :guilabel:`Select File` and choose the signed binary for that image (internal, network core, or QSPI part).
   #. Select image index ``0``, ``1``, or ``2`` to match the table above.
   #. Tap :guilabel:`Start` and complete the upload.

   Upload the internal (``0``) and QSPI (``2``) application parts together when updating the split application.
   Include the network core image (``1``) only when your build includes the network core.

#. Mark each uploaded image for test and reset the device from the app (or use the :guilabel:`Reset` action under basic SMP controls).

#. Verify on the serial console that ``<BUILD TIME>`` reflects the new firmware.

For a general walkthrough of FOTA over Bluetooth LE with the mobile app, see :ref:`ug_nrf53_developing_ble_fota_steps_testing`.

Dependencies
************

The sample uses the following Zephyr library:

* :ref:`zephyr:mcu_mgr`

The memory layout is defined in the sample devicetree files located in the :file:`dts` folder.
See :ref:`devicetree-based partitioning <zephyr:dt-guide>` for more information.
