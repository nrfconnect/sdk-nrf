.. _ble_mesh_dfu_distributor:

Bluetooth Mesh: Device Firmware Update (DFU) distributor
########################################################

.. contents::
   :local:
   :depth: 2

The Bluetooth® Mesh DFU distributor sample demonstrates how device firmware can be distributed over a Bluetooth Mesh network.
The sample implements the Firmware Distribution role of the :ref:`Bluetooth Mesh DFU subsystem <zephyr:bluetooth_mesh_dfu>`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

For provisioning and configuring of the mesh model instances, the sample requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

* `nRF Mesh mobile app for Android`_
* `nRF Mesh mobile app for iOS`_

For uploading an image to the Distributor, this sample also requires a smartphone with Nordic Semiconductor's nRF Connect Device Manager mobile app installed in one of the following versions:

* `nRF Device Manager mobile app for Android`_
* `nRF Device Manager mobile app for iOS`_

Overview
********

The following are the key features of the sample:

* The sample is configured as an application for the MCUboot.
* The image management subsystem of the MCU manager (mcumgr) is used to upload firmware images to the Distributor.
* A set of shell commands is provided to control the firmware distribution over a Bluetooth Mesh network.
* Self-update is supported.

Provisioning
============

The sample supports provisioning over both the Advertising and the GATT Provisioning Bearers, PB-ADV and PB-GATT respectively.
The provisioning is handled by the :ref:`bt_mesh_dk_prov`.
It supports four types of out-of-band (OOB) authentication methods, and uses the Hardware Information driver to generate a deterministic UUID to uniquely represent the device.

Use `nRF Mesh mobile app`_ for provisioning and configuring of models supported by the sample.

Models
======

The following table shows the mesh composition data for this sample:

.. table::
   :align: left

   +------------------------------+------------------------+
   | Element 1                    | Element 2              |
   +==============================+========================+
   | Config Server                | BLOB Transfer Server   |
   +------------------------------+------------------------+
   | Health Server                | Firmware Update Server |
   +------------------------------+------------------------+
   | BLOB Transfer Client         |                        |
   +------------------------------+------------------------+
   | Firmware Update Client       |                        |
   +------------------------------+------------------------+
   | BLOB Transfer Server         |                        |
   +------------------------------+------------------------+
   | Firmware Distribution Server |                        |
   +------------------------------+------------------------+

The models are used for the following purposes:

* Config Server allows configurator devices to configure the node remotely.
* Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
  These callbacks trigger blinking of the LEDs.
* The Binary Large Object (BLOB) Transfer models, :ref:`zephyr:bluetooth_mesh_blob_srv` and :ref:`zephyr:bluetooth_mesh_blob_cli`, provide functionality for sending large binary objects from a single source to many Target nodes over the Bluetooth Mesh network.
  It is the underlying transport method for the DFU.
  BLOB Transfer Client and BLOB Transfer Server are instantiated on the primary element.
  An additional BLOB Transfer Server is instantiated on the secondary element.
* To implement the Firmware Distribution role, the sample instantiates the models :ref:`zephyr:bluetooth_mesh_dfd_srv` and :ref:`zephyr:bluetooth_mesh_dfu_cli`.
  The Firmware Distribution Server model and its base models are instantiated on the primary element.
  These models are also used to support the self-update of the sample.
* To implement the Target node functionality of the :ref:`zephyr:bluetooth_mesh_dfu` subsystem, the :ref:`zephyr:bluetooth_mesh_dfu_srv` model and its base models are instantiated on the secondary element.

Configuration
*************

|config|

Source file setup
=================

This sample is split into the following source files:

* A :file:`main.c` file to handle Bluetooth Mesh initialization, including the model handling for Device Composition Data, Health and Configuration Server models.
* File :file:`dfu_target.c` with the Target role implementation.
* File :file:`dfu_dist.c` with the Distributor role implementation.
* File :file:`smp_bt.c` implementing SMP Bluetooth service advertisement.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/mesh/mesh_dfu/distributor`

.. include:: /includes/build_and_run.txt

.. note::
   To prevent an unauthenticated access to the device over SMP, it is strongly recommended to enable the :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW_AUTHEN` option.
   This will enforce a remote device to initiate a pairing request before accessing SMP characteristics.
   See `SMP over Bluetooth authentication`_ for more information.

Testing
=======

.. _ble_mesh_dfu_distributor_provisioning:

Provisioning the device
-----------------------

.. |device name| replace:: :guilabel:`Mesh DFU Distributor`

.. include:: /includes/mesh_device_provisioning.txt

.. _ble_mesh_dfu_distributor_model_config:

Configuring models
------------------

See :ref:`ug_bt_mesh_model_config_app` for details on how to configure the mesh models with the nRF Mesh mobile app.

Configure the Firmware Distribution Server, Firmware Update Client, BLOB Transfer Server and BLOB Transfer Client models on the primary element on the **Mesh DFU Distributor** node:

* Bind each model to **Application Key 1**.

Configure the Firmware Update Server and BLOB Transfer Server models on the secondary element on the **Mesh DFU Distributor** node:

* Bind each model to **Application Key 1**.

Performing a Device Firmware Update
-----------------------------------

The Bluetooth Mesh defines the Firmware update Initiator role to control the firmware distribution.
This sample supports, but does not require an external Initiator to control the DFU procedure.
The Bluetooth Mesh DFU subsystem also provides a set of shell commands that can be used instead of the Initiator.
Follow the description in the :ref:`dfu_over_bt_mesh` guide on how to perform the firmware distribution without the Initiator.

The commands can be executed in two ways:

* Through the shell management subsystem of MCU manager (for example, using the nRF Connect Device Manager mobile application on Android or :ref:`Mcumgr command-line tool <dfu_tools_mcumgr_cli>`).
* By accessing the :ref:`zephyr:shell_api` module over RTT.

.. _ble_mesh_dfu_distributor_fw_image_upload:

Uploading a firmware image
--------------------------

A firmware image can be uploaded to the device in two ways:

* In-band, using BLOB Transfer models by an Initiator device.
* Out-of-band, using the image management subsystem.

For out-of-band upload, the sample uses the image management subsystem of the :ref:`zephyr:mcu_mgr`.
The management subsystem uses the Simple Management Protocol (SMP), provided by the Mcumgr library, to exchange commands and data between the SMP server (the sample device) and the SMP client.
This sample supports Bluetooth Low Energy and UART as the SMP transport.
See :ref:`zephyr:device_mgmt` for more information about Mcumgr and SMP.

In this sample, the device flash is split into fixed partitions using devicetree as defined in :zephyr_file:`nrf52840dk_nrf52840.dts<boards/nordic/nrf52840dk/nrf52840dk_nrf52840.dts>`  and :zephyr_file:`nrf54l15dk_nrf54l15_cpuapp.dts<boards/nordic/nrf54l15dk/nrf54l15dk_nrf54l15_cpuapp.dts>`.
The firmware image that is to be distributed over Bluetooth Mesh network should be stored at slot-1.
The sample uses :ref:`zephyr:flash_map_api` to read the firmware image from slot-1 when distributes it to Target nodes.

When the image is sent in-band, the Firmware Distribution Server will store the firmware image in slot-1.

To upload an image to slot-1 on the sample device out-of-band, use a smartphone with Nordic Semiconductor's `nRF Connect Device Manager`_ installed.

.. note::
   Because the same slot (slot-1) is used by the MCUboot bootloader for local DFU, do not request to test the image when uploading the firmware to the sample device.
   Otherwise, the bootloader will try swapping the distributor image with the uploaded one at the next reboot.

Copy the new image to the mobile phone.
Then, in the mobile app, do the following:

* Find and select the :guilabel:`Mesh DFU Distributor` device.
* Go to the :guilabel:`Image` tab.
* Tap the :guilabel:`Advanced` button in the right top corner.
  This will allow uploading the image to slot-1 without swapping the image on the Distributor.
* Under the :guilabel:`Image Upload` area, tap the :guilabel:`Select File` button and select the copied image.
* Tap the :guilabel:`Start` button.
* Select :guilabel:`target.signed.bin`.

Once the image upload is done, the :guilabel:`State` field is set to UPLOAD COMPLETE.

Changing the firmware distribution phase
----------------------------------------

When the firmware distribution phase changes, the sample will print a corresponding message.
For example, when the distribution is completed, the sample will print::

  Distribution phase changed to Completed

Self-update
-----------

This sample instantiates the DFU and BLOB Transfer Server models on its secondary element and thus can also be updated over Bluetooth Mesh by any other Distributor or by itself.

To update this sample, use the address of the secondary element of the sample as the address of the Target node.

When the Distributor updates itself, the DFU transfer will end immediately after start as the image is already stored on the device.

.. note::
   Do not add other Target nodes but the Distributor when performing a self-update.
   If the Firmware Distribution Server on the Distributor finds itself in the list of Target nodes, it skips the DFU transfer as the image is already stored on the device.
   Thus, other nodes will not receive the image.

When this sample is used as a Target, it behaves as described in :ref:`ble_mesh_dfu_target_upgrade`.

This sample also provides support for :ref:`dfu_over_ble`, so it is possible to self-update using the Simple Management Protocol (SMP).

SMP over Bluetooth authentication
=================================

By default, the SMP characteristics do not require authentication when using SMP over Bluetooth to access the :ref:`management subsystem <zephyr:mcu_mgr>`.
To prevent an unauthenticated access to the device over SMP, it is strongly recommended to enable the :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW_AUTHEN` option.
This will enforce a remote device to initiate a pairing request before accessing SMP characteristics.
See `Zephyr Bluetooth LE Security`_ for more details about securing the Bluetooth LE connection.

The sample supports the :ref:`bt_mesh_le_pair_resp_readme` model that allows sending a passkey over a mesh network when the Distributor has no means of displaying the passkey.
When the model and the :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW_AUTHEN` option are enabled while a remote device tries to read the SMP characteristics, the pairing request will be initiated and the sample will require the remote device to enter the passkey generated by the model.

To enable the LE pairing authentication with the LE Pairing Responder model support, set :makevar:`EXTRA_CONF_FILE` to :file:`overlay-smp-bt-auth.conf` file when building the sample.

Logging
=======

In this sample, the UART console is occupied by the shell module.
Therefore, it uses SEGGER RTT as a logging backend.
For the convenience, ``printk`` is also duplicated to SEGGER RTT.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_mesh_dk_prov`
* :ref:`dk_buttons_and_leds_readme`

It also requires :ref:`MCUboot <mcuboot_ncs>` and :ref:`zephyr:mcu_mgr`.

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:bluetooth_mesh`:

  * :file:`include/bluetooth/mesh.h`
