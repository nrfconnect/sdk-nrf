.. _bluetooth_mesh_sensor_client:

Bluetooth: Mesh sensor observer
###############################

.. contents::
   :local:
   :depth: 2

The Bluetooth mesh sensor observer sample demonstrates how to set up a basic Bluetooth mesh :ref:`bt_mesh_sensor_cli_readme` model application that gets sensor data from one :ref:`bt_mesh_sensor_srv_readme` model.
Four different sensor types are used to showcase different ways for the server to publish data.
In addition, the samples demonstrate usage of both :ref:`single-channel sensor types and sensor series types <bt_mesh_sensor_types_channels>`.

.. note::
   This sample must be paired with :ref:`bluetooth_mesh_sensor_server` to show any functionality.
   The observer has no sensor data, and is dependent on a mesh sensor to provide it.

Overview
********

This sample is split into the following source files:

* A :file:`main.c` file to handle initialization.
* One additional file for handling Bluetooth mesh models, :file:`model_handler.c`.

The following Bluetooth mesh sensor types are used in this sample:

* :c:var:`bt_mesh_sensor_present_dev_op_temp` - Published by the server according to its publishing period.
* :c:var:`bt_mesh_sensor_rel_runtime_in_a_dev_op_temp_range` - Periodically requested by the client.
* :c:var:`bt_mesh_sensor_presence_detected` - Published when a button is pressed on the server.
* :c:var:`bt_mesh_sensor_time_since_presence_detected` - Periodically requested by the client and published by the server according to its publishing period.


Provisioning
============

The provisioning is handled by the :ref:`bt_mesh_dk_prov`.
Use `nRF Mesh mobile app`_ for provisioning and configuring of models supported by the sample.

Models
======

The following table shows the mesh sensor observer composition data for this sample:

   +---------------+
   |  Element 1    |
   +===============+
   | Config Server |
   +---------------+
   | Health Server |
   +---------------+
   | Sensor Client |
   +---------------+

The models are used for the following purposes:

* Config Server allows configurator devices to configure the node remotely.
* Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
  These callbacks trigger blinking of the LEDs.
* Sensor Client gets sensor data from one or more :ref:`Sensor Server(s) <bt_mesh_sensor_srv_readme>`.

The model handling is implemented in :file:`src/model_handler.c`.
A :c:struct:`k_delayed_work` item is submitted recursively to periodically request sensor data.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :sample-yaml-rows:

The sample also requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

* `nRF Mesh mobile app for Android`_
* `nRF Mesh mobile app for iOS`_

Additionally, the sample requires the :ref:`bluetooth_mesh_sensor_server` sample application, programmed on a separate development kit and configured according to mesh sensor sample's :ref:`testing guide <bluetooth_mesh_sensor_server_testing>`.

User interface
**************

Buttons:
   Can be used to input the OOB authentication value during provisioning.
   All buttons have the same functionality during the provisioning procedure.

Terminal:
   All sensor values gathered from the server are printed over UART.
   For more details, see :ref:`gs_testing`.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/mesh/sensor_client`

.. include:: /includes/build_and_run.txt

.. _bluetooth_mesh_sensor_client_testing:

Testing
=======

.. note::
   The mesh sensor observer sample cannot demonstrate any functionality on its own, and needs a device with the :ref:`bluetooth_mesh_sensor_server` sample running in the same mesh network.
   Before testing the mesh sensor observer, go through the mesh sensor's :ref:`testing guide <bluetooth_mesh_sensor_server_testing>` with a different kit.

After programming the sample to your development kit, you can test it by using a smartphone with Nordic Semiconductor’s nRF Mesh app installed.
Testing consists of provisioning the device and configuring it for communication with the mesh models.

All sensor values gathered from the server are printed over UART.
For more details, see :ref:`gs_testing`.

Provisioning the device
-----------------------

.. |device name| replace:: :guilabel:`Mesh Sensor Observer`

.. include:: /includes/mesh_device_provisioning.txt

Configuring models
------------------

See :ref:`ug_bt_mesh_model_config_app` for details on how to configure the mesh models with the nRF Mesh mobile app.

Configure the Sensor Client model on the :guilabel:`Mesh Sensor Observer` node:

* Bind the model to :guilabel:`Application Key 1`.
* Set the publication parameters:

  * Destination/publish address: Select an existing group or create a new one, but make sure that the Sensor Server subscribes to the same group.
  * Retransmit count: Set the count to zero (:guilabel:`Disabled`), to avoid duplicate logging in the UART terminal.

* Set the subscription parameters: Select an existing group or create a new one, but make sure that the Sensor Server publishes to the same group.

The Sensor Client model is now configured and able to receive data from the Sensor Server.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_mesh_sensor_cli_readme`
* :ref:`bt_mesh_dk_prov`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* ``include/drivers/hwinfo.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``

* :ref:`zephyr:bluetooth_mesh`:

  * ``include/bluetooth/mesh.h``
