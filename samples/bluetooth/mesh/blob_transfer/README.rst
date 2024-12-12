.. _ble_mesh_blob_xfer:

Bluetooth Mesh: Binary Large Object (BLOB) Transfer
###################################################

.. contents::
   :local:
   :depth: 2

The Bluetooth® Mesh BLOB Transfer sample demonstrates how binary large object data can be transferred over a Bluetooth Mesh network.
The sample implements the BLOB Transfer Server/Client roles of the :ref:`Bluetooth Mesh BLOB subsystem <zephyr:bluetooth_mesh_blob>`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

For provisioning and configuring of the mesh model instances, the sample requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

* `nRF Mesh mobile app for Android`_
* `nRF Mesh mobile app for iOS`_

Overview
********

Mesh networks can transfer large amounts of data from one node to others using the Binary Large Object Transfer Model.
In addition to 1:1 transfer, it supports group communication to enable 1:n transfer.

The BLOB Transfer Model is typically used for the DFU model, but it can be used to transfer large data such as sensing data, lightness tuning tables, etc. that require large amounts of data.

This sample implements the methods to write and read data to and from the flash area through Shell control, and to send and receive BLOB data between multiple nodes.

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
   | Config Server                | BLOB Transfer Client   |
   +------------------------------+------------------------+
   | Health Server                |                        |
   +------------------------------+------------------------+
   | BLOB Transfer Server         |                        |
   +------------------------------+------------------------+

The models are used for the following purposes:

* Config Server allows configurator devices to configure the node remotely.
* Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
  These callbacks trigger blinking of the LEDs.
* The Binary Large Object (BLOB) Transfer models, :ref:`zephyr:bluetooth_mesh_blob_srv` and :ref:`zephyr:bluetooth_mesh_blob_cli`, provide functionality for sending large binary objects from a single source to many Target nodes over the Bluetooth Mesh network.
  BLOB Transfer Server is instantiated on the primary element.
  BLOB Transfer Client is instantiated on the secondary element.

The model handling is implemented in :file:`src/model_handler.c` and :file:`src/blob_xfer.c`.

User interface
**************

Terminal emulator:
   Used for the interaction with the sample.

Configuration
*************

|config|

Source file setup
=================

This sample is split into the following source files:

* A :file:`src/main.c` file to handle Bluetooth Mesh initialization.
* File :file:`src/model_handler.c` including the model handling for Device Composition Data, Health and Configuration Server models.
* File :file:`src/blob_xfer.c` with the BLOB Transfer Server/Client model handlers and APIs of Sender/Receiver implementation.
* File :file:`src/blob_shell.c` with the :ref:`shell module <shell_api>` for communicating with users.

Partition Manager
=================

This sample use static configuration for Partition Manager.
The partition information is created according to :file:`pm_static.yml`.
If you modify this file, you need to modify `STORAGE_START` / `STORAGE_END` / `STORAGE_ALIGN` in :file:`src/blob_xfer.h`` and `BLOB_AREA_ID` in :file:`src/blob_xfer.c` to the appropriate values.


Configuration options
=====================

The following configuration parameters are associated with the BLOB Transfer:

.. _CONFIG_BT_MESH_BLOB_TX_MAX_TARGETS:

CONFIG_BT_MESH_BLOB_TX_MAX_TARGETS - Max targets configuration
   Maximum number of targets to send blob over mesh.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/mesh/blob_transfer`

.. include:: /includes/build_and_run.txt

.. _bluetooth_mesh_blob_transfer_testing:

Testing
=======

This sample has been tested with the nRF52840 DK (nrf52840dk_nrf52840) board.

.. _ble_mesh_blob_transfer_provisioning:

Provisioning the device
-----------------------

.. |device name| replace:: :guilabel:`Mesh BLOB Transfer`

.. include:: /includes/mesh_device_provisioning.txt

.. _ble_mesh_blob_transfer_config:

Configuring models
------------------

See :ref:`ug_bt_mesh_model_config_app` for details on how to configure the mesh models with the nRF Mesh mobile app.

Create a new group and name it BLOB Channel, then configure the BLOB Transfer Server model on the primary element on the **Mesh BLOB Transfer** receiver nodes:

* Bind the model to **Application Key 1**.
* Set the subscription parameters: Select the created group BLOB Channel.

Configure the BLOB Transfer Client model on the secondary element on the **Mesh BLOB Transfer** sender node:

* Bind the model to **Application Key 1**.

Interacting with the sample
---------------------------

1. Connect the development kit to the computer using a USB cable.
   The development kit is assigned a COM port (Windows), ttyACM device (Linux) or tty.usbmodem (MacOS).
#. |connect_terminal_specific_ANSI|
#. Enable local echo in the terminal to see the text you are typing.

After completing the steps above, a command can be sent to the sample.
The sample supports the following commands:

blob \-\-help
   Print help message together with the list of supported commands.

**Flash APIs**

blob read <offset> <size>
   Read and print data stored in flash.

blob write <offset> <size>
   Write dummy data to flash.

blob hash <offset> <size>
   Print a hash value for data stored in FLASH.

**BLOB Sender APIs**

blob send init <offset> <size>
   Specify the flash area where the data to be transferred is stored.

blob send target add <addr> [addr, ...]
   Add unicast addresses of the target element with a BLOB client model.

blob send target show
   Print the target addresses to send BLOB data to.

blob send target clean
   Clear all target addresses.

blob send start <blob_id> [-t ttl=DEFAULT_TTL] [-b base_timestamp=2] [-g multicast_address=UNASSIGNED_ADDR] [-k appkey_index=0] [-m {push|pull}=push]
   Start sending BLOB data to <blob_id>.

blob send cancel
   Cancel BLOB Transfer sending process.

blob send suspend
   Suspend BLOB Transfer sending process.

blob send resume
   Resume BLOB Transfer sending process.

**BLOB Receiver APIs**

blob receive init <offset>
   Specify the flash area where data will be stored.

blob receive start <blob_id> [-t ttl=DEFAULT_TTL] [-b base_timestamp=2]
   Ready to receive BLOB data to <blob_id>.

blob receive cancel
   Cancel BLOB Transfer sending process.

blob receive progress
   Print BLOB Transfer progress.

**BLOB Receiver node example**

The receiver node determines the offset in flash to store the blob data and prepares to start receiving blobs with **<blob_id>**.

.. code-block:: console

   uart:~$ blob receive init 0x3000
   Set flash to receive blob: offset: 0x3000
   uart:~$ blob receive start abcdef
   Set blob receive ID: 0x0000000000abcdef, ttl: 0xff, base_timeout: 2

When a sender starts BLOB Trasnfer, data is being stored in flash.
After successful reception, a hash value is output for the received data, and you can check the data with the read command.

.. code-block:: console

   ...
   BLOB Transfer RX Start
   BLOB Transfer RX End: Success, id(0000000000abcdef)
   Hash value of read data: 0x7a08da46
   uart:~$ blob read 0x3000 0x500
   0x00003000 to 0x0000301f: c0ffeec0ffeec0ffeec0ffeec0ffeec0ffeec0ffeec0ffeec0ffeec0ffeec0ff
   0x00003020 to 0x0000303f: eec0ffeec0ffeec0ffeec0ffeec0ffeec0ffeec0ffeec0ffeec0ffeec0ffeec0
   ...

**BLOB Sender node example**

The sender node write BLOB data into the flash.

.. code-block:: console

   uart:~$ blob write 0x1000 0x500
   Erase flash: offset: 0x1000, size: 0x1000
   Write data to flash (size: 0x500)
   Hash value of written data: 0x7a08da46

The sender adds one or more targets' addresses of the element with BLOB Server model.

.. code-block:: console

   uart:~$ blob send target add 0x101A
   Added target 101a
   uart:~$ blob send target show
   1 Target: 0x101a

The sender node determines the offset and size in flash to send blob data.

.. code-block:: console

   uart:~$ blob send init 0x1000 0x500
   Set flash to send blob: offset: 0x1000, size: 0x500

The sender sets multicast address with created group BLOB Channel and starts BLOB Trasnfer with **<blob_id>**.
If no group is specified, the data is sent to each receiver as a unicast address.
The sender receives the Capabilities of the receivers specified as targets and sends BLOB data according to those parameters.
If it terminates successfully, `Success` is output.

.. code-block:: console

   uart:~$ blob send start abcdef -g D000
   Set blob send ID: 0x0000000000abcdef, ttl: 0xff, base_timeout: 2, group: d000, appkey_index: 0, mode: 1
   Hash value of read data: 0x7a08da46
   BLOB Transfer TX Caps
   BLOB Transfer TX Success

**Common of BLOB Transfer model**

Whenever the node uses **BLOB Transfer Client/Server** models before provisioning, you will see the following message:

.. code-block:: none

   The mesh node is not provisioned. Please provision the mesh node before using the blob transfer.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_mesh_dk_prov`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:shell_api`:

  * :file:`include/shell.h`
  * :file:`include/shell_uart.h`

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`

* :ref:`zephyr:bluetooth_mesh`:

  * :file:`include/bluetooth/mesh.h`
