.. _bluetooth_central_dfu_smp:

.. ncs-sample::
   :title: Bluetooth: Central SMP Client

   The Central SMP Client sample demonstrates how to update the firmware of a remote device over Bluetooth® Low Energy using the Simple Management Protocol (SMP).
   It uses the :ref:`lib_dfu_target` with the SMP Bluetooth backend to transfer the firmware image to the target device.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample requires a second device that acts as the remote target.
The device must run an SMP server over Bluetooth LE, such as another development kit running the :zephyr:code-sample:`smp-svr` sample with the Bluetooth LE SMP transport enabled.

The sample looks for the firmware update image at :file:`/lfs1/update.bin`.
Before starting the update, upload a valid signed MCUboot update image for the remote target to this path.

To upload the firmware update image, use the nRF Device Manager mobile app or another client that supports the File System management group over SMP with Bluetooth LE:

* `nRF Device Manager mobile app for Android`_
* `nRF Device Manager mobile app for iOS`_

Overview
********

The :ref:`lib_dfu_target` transfers arbitrary data supplied by the application.
In a production application, the firmware update image can therefore come from any source the application can access, such as an SD card, external flash, or a web server.
For demonstration purposes, this sample uses a LittleFS partition in non-volatile memory as intermediate storage for the firmware update image.
To allow the image to be uploaded to that storage, the sample also acts as a Bluetooth LE peripheral that exposes an mcumgr SMP server with the File System management group.

On boot, the sample mounts the LittleFS file system and starts scanning as a central for the remote SMP server.
The sample automatically connects to any device that advertises the SMP Service UUID.
To connect based on other criteria, such as the device name, update the scan filter configuration in the :file:`prj.conf` and :file:`src/main.c` files.
For details about the available filter types, see :ref:`lib_nrf_bt_scan_readme_filters`.

To upload the firmware update image for the remote target, press the following button:

.. tabs::

   .. group-tab:: nRF53 DKs

      Press **Button 2**.

   .. group-tab:: nRF54 DKs

      Press **Button 1**.

The sample starts connectable advertising with the SMP service, so an mcumgr client such as the nRF Device Manager mobile app can connect and upload the signed image to :file:`/lfs1/update.bin` using the File System management group over SMP.
Advertising stops automatically once the host connects or after a 60-second timeout.
Press the same button again to restart advertising.

By default, the File System management group grants a connected mcumgr client access to every file in the file system.
The sample registers a file access hook (:kconfig:option:`CONFIG_MCUMGR_GRP_FS_FILE_ACCESS_HOOK`) that rejects any path other than :file:`/lfs1/update.bin`.
The SMP transport is also configured without authentication, so it accepts any client that is in range.

To start the update of the remote target, press the following button:

.. tabs::

   .. group-tab:: nRF53 DKs

      Press **Button 1**.

   .. group-tab:: nRF54 DKs

      Press **Button 0**.

The sample then performs the following operations:

1. Opens :file:`/lfs1/update.bin` on the LittleFS file system, checks that it starts with an MCUboot image header, and uses the file size as the exact size of the stored image.
#. Initializes the SMP DFU target and uploads the image to the remote device in chunks using the MCUmgr image management group over SMP.
#. Marks the uploaded image as pending (``schedule update``) and reboots the remote target so that MCUboot can apply the update.

.. note::
   The uploaded image is only marked for test, not confirmed.
   After the reboot, MCUboot swaps in and runs the new image once.
   The target firmware can call the :c:func:`boot_write_img_confirmed` function to confirm the running image and make it permanent.
   Otherwise, MCUboot reverts to the previous image on the next reset.

.. _bluetooth_central_dfu_smp_user_interface:

User interface
**************

.. tabs::

   .. group-tab:: nRF53 DKs

      Button 1:
         Start the firmware update of the remote target.

      Button 2:
         Start advertising so a host can upload the firmware update image using the File System management group over SMP.

   .. group-tab:: nRF54 DKs

      Button 0:
         Start the firmware update of the remote target.

      Button 1:
         Start advertising so a host can upload the firmware update image using the File System management group over SMP.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/central_smp_client`

.. include:: /includes/build_and_run_ns.txt

.. _bluetooth_central_dfu_smp_testing:

Testing
=======

|test_sample|

Button assignments depend on the development kit family.
See the :ref:`bluetooth_central_dfu_smp_user_interface` section for the assignments.

1. |connect_kit|
#. |connect_terminal|
#. Program a second development kit with the :zephyr:code-sample:`smp-svr` sample and enable the Bluetooth LE SMP transport.
   This test uses the :zephyr:code-sample:`smp-svr` sample, but any sample that provides an SMP server with image management support over Bluetooth LE can act as the remote target.
#. Observe and record the build time printed by the remote target.
#. Perform a pristine rebuild of the :zephyr:code-sample:`smp-svr` sample, but do not program the rebuilt image to the remote target.
   The pristine rebuild ensures that the build time printed by the firmware on startup is updated, making it easy to verify if the target application was updated.
   Use the resulting :file:`zephyr.signed.bin` file as the firmware update image.
#. Reset the Central SMP Client.
#. Observe that the text "Starting Bluetooth Central SMP Client sample" is printed on the COM listener running on the computer.
   The device starts scanning for peripherals with the SMP service.
#. Press the advertising button on the Central SMP Client, then upload the signed image to the LittleFS file system using the nRF Device Manager mobile app:

   a. Start the `nRF Device Manager mobile app for Android`_ or the `nRF Device Manager mobile app for iOS`_.
   #. Connect to the device that advertises as ``SMP Client Sample``.
   #. Open the File System management (**Files**) section, select the signed image file, and upload it to :file:`/lfs1/update.bin`.

#. Observe that the kits connect.
   After service discovery completes, the sample reports that the SMP DFU client is ready.
#. Press the firmware update button on the Central SMP Client.
   Observe messages similar to the following::

      Starting DFU of the remote target over BLE SMP
      Update image size: 262144 bytes
      Uploaded 4096 / 262144 bytes
      ...
      DFU completed. The target will reboot to apply the update.

#. Observe that the remote target reboots into the new firmware.
   Verify that the printed build time differs from the build time recorded before the update.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`dfu_smp_readme`
* :ref:`lib_dfu_target`
* :ref:`gatt_dm_readme`
* :ref:`nrf_bt_scan_readme`

It uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:flash_map_api`:

  * :file:`include/zephyr/storage/flash_map.h`

* :ref:`zephyr:file_system_api`:

  * :file:`include/zephyr/fs/fs.h`
  * :file:`include/zephyr/fs/littlefs.h`

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/gatt.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/uuid.h`

* :ref:`zephyr:mcumgr_smp_protocol_specification`
