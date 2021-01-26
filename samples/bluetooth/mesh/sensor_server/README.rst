.. _bluetooth_mesh_sensor_server:

Bluetooth: Mesh sensor
######################

.. contents::
   :local:
   :depth: 2

The Bluetooth mesh sensor sample demonstrates how to set up a basic mesh Sensor Server model application that provides sensor data to one :ref:`bt_mesh_sensor_cli_readme` model.
Four different sensor types are used to showcase different ways for the server to publish data.
In addition, the sample demonstrates usage of both :ref:`single-channel sensor types and sensor series types <bt_mesh_sensor_types_channels>`.

.. note::
   This sample must be paired with the :ref:`bluetooth_mesh_sensor_client` sample to show any functionality.
   The mesh sensor provides the sensor data used by the observer.

Overview
********

This sample is split into the following source files:

* A :file:`main.c` file to handle initialization.
* One additional file for handling Bluetooth mesh models, :file:`model_handler.c`.

The following Bluetooth mesh sensor types are used in this sample:

* :c:var:`bt_mesh_sensor_present_dev_op_temp` - Published by the server according to its publishing period (see :ref:`bluetooth_mesh_sensor_server_conf_models`).
* :c:var:`bt_mesh_sensor_rel_runtime_in_a_dev_op_temp_range` - Periodically requested by the client.
* :c:var:`bt_mesh_sensor_presence_detected` - Published when a button is pressed on the server.
* :c:var:`bt_mesh_sensor_time_since_presence_detected` - Periodically requested by the client and published by the server according to its publishing period (see :ref:`bluetooth_mesh_sensor_server_conf_models`).

Moreover, the on-chip ``TEMP_NRF5`` temperature sensor is used.

Provisioning
============

The provisioning is handled by the :ref:`bt_mesh_dk_prov`.
Use `nRF Mesh mobile app`_ for provisioning and configuring of models supported by the sample.

Models
======

The following table shows the Bluetooth mesh sensor composition data for this sample:

   +---------------+
   |  Element 1    |
   +===============+
   | Config Server |
   +---------------+
   | Health Server |
   +---------------+
   | Sensor Server |
   +---------------+

The models are used for the following purposes:

* Config Server allows configurator devices to configure the node remotely.
* Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
  These callbacks trigger blinking of the LEDs.
* Sensor Server provides sensor data to one or more :ref:`mesh sensor observers <bt_mesh_sensor_cli_readme>`.

The model handling is implemented in :file:`src/model_handler.c`, which uses the ``TEMP_NRF5`` temperature sensor, and the :ref:`dk_buttons_and_leds_readme` library to detect button presses.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :sample-yaml-rows:

The sample also requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

* `nRF Mesh mobile app for Android`_
* `nRF Mesh mobile app for iOS`_

Additionally, the sample requires the :ref:`bluetooth_mesh_sensor_client` sample application.
The application needs to be programmed on a separate device, and configured according to the sensor observer sample's :ref:`testing guide <bluetooth_mesh_sensor_server_testing>`.

User interface
**************

Buttons:
   Can be used to input the OOB authentication value during provisioning.
   All buttons have the same functionality during the provisioning procedure.

Button 1:
   Simulate presence detected (after the provisioning procedure is finished).


Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/mesh/sensor_server`

.. include:: /includes/build_and_run.txt

.. _bluetooth_mesh_sensor_server_testing:

Testing
=======

.. note::
   The Bluetooth mesh sensor sample cannot demonstrate any functionality on its own, and needs a device with the :ref:`bluetooth_mesh_sensor_client` sample running in the same mesh network.
   Before testing the sensor sample, go through the sensor observer sample's :ref:`testing guide <bluetooth_mesh_sensor_client_testing>` with a different kit.

After programming the sample to your development kit, you can test it by using a smartphone with Nordic Semiconductor’s nRF Mesh app installed.
Testing consists of provisioning the device, and configuring it for communication with the mesh models.

Provisioning the device
-----------------------

.. |device name| replace:: :guilabel:`Mesh Sensor`

.. include:: /includes/mesh_device_provisioning.txt

.. _bluetooth_mesh_sensor_server_conf_models:

Configuring models
------------------

See :ref:`ug_bt_mesh_model_config_app` for details on how to configure the mesh models with the nRF Mesh mobile app.

Configure the Sensor Server model on the :guilabel:`Mesh Sensor` node:

* Bind the model to :guilabel:`Application Key 1`.
* Set the publication parameters:

  * Destination/publish address: Select an existing group or create a new one, but make sure that the Sensor Client subscribes to the same group.
  * Retransmit count: Set the count to zero (:guilabel:`Disabled`), to avoid duplicate logging in the :ref:`bt_mesh_sensor_cli_readme`'s UART terminal.

* Set the subscription parameters: Select an existing group or create a new one, but make sure that the Sensor Client publishes to the same group.

The Sensor Server model is now configured and able to send data to the Sensor Client.

.. note::
   To enable Sensor Server configuration by a Sensor Client, an application key must be bound to the Sensor Setup Server.
   This functionality must also be programmed in the :ref:`bt_mesh_sensor_cli_readme` device.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_mesh_sensor_srv_readme`
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
