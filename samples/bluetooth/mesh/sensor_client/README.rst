.. _bluetooth_mesh_sensor_client:

Bluetooth Mesh: Sensor observer
###############################

.. contents::
   :local:
   :depth: 2

The Bluetooth® Mesh sensor observer sample demonstrates how to set up a basic Bluetooth Mesh :ref:`bt_mesh_sensor_cli_readme` model application that gets sensor data from one :ref:`bt_mesh_sensor_srv_readme` model.
Eight different sensor types are used to showcase different ways for the server to publish data.
In addition, the samples demonstrate usage of both :ref:`single-channel sensor types and sensor series types <bt_mesh_sensor_types_channels>`, as well as how to add and write to a sensor setting.

.. note::
   This sample must be paired with :ref:`bluetooth_mesh_sensor_server` to show any functionality.
   The observer has no sensor data, and is dependent on a mesh sensor to provide it.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

* `nRF Mesh mobile app for Android`_
* `nRF Mesh mobile app for iOS`_

Additionally, the sample requires the :ref:`bluetooth_mesh_sensor_server` sample application, programmed on a separate development kit and configured according to mesh sensor sample's :ref:`testing guide <bluetooth_mesh_sensor_server_testing>`.

Overview
********

The following Bluetooth Mesh sensor types, and their settings, are used in this sample:

* :c:var:`bt_mesh_sensor_present_dev_op_temp` - Periodically requested by the client and published by the server according to its publishing period.

  * :c:var:`bt_mesh_sensor_dev_op_temp_range_spec` - Used as a setting for the :c:var:`bt_mesh_sensor_present_dev_op_temp` sensor type to set the range of reported temperatures.

* :c:var:`bt_mesh_sensor_rel_runtime_in_a_dev_op_temp_range` - Periodically requested by the client.
* :c:var:`bt_mesh_sensor_presence_detected` - Published when a button is pressed on the server.

  * :c:var:`bt_mesh_sensor_motion_threshold` - Used as a setting for the :c:var:`bt_mesh_sensor_presence_detected` sensor type to set the time (0-10 seconds) before the presence is detected.

* :c:var:`bt_mesh_sensor_time_since_presence_detected` - Periodically requested by the client and published by the server according to its publishing period.
* :c:var:`bt_mesh_sensor_motion_sensed` - Published when a button is pressed on the server.
* :c:var:`bt_mesh_sensor_time_since_motion_sensed` - Periodically requested by the client and published by the server according to its publishing period.
* :c:var:`bt_mesh_sensor_present_amb_light_level` - Periodically requested by the client and published by the server according to its publishing period, and published when a button is pressed on the server.

  * :c:var:`bt_mesh_sensor_gain` - Used as a setting for the :c:var:`bt_mesh_sensor_present_amb_light_level` sensor type to set the gain the ambient light sensor value is multiplied with.
  * :c:var:`bt_mesh_sensor_present_amb_light_level` - Used as a setting for the :c:var:`bt_mesh_sensor_present_amb_light_level` sensor type to calculate sensor gain based on measured reference ambient light level. This value does only have a set command.

* :c:var:`bt_mesh_sensor_people_count` - Published when a button is pressed on the server.

Provisioning
============

The provisioning is handled by the :ref:`bt_mesh_dk_prov`.
It supports four types of out-of-band (OOB) authentication methods, and uses the Hardware Information driver to generate a deterministic UUID to uniquely represent the device.

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
* Sensor Client gets sensor data from one or more :ref:`Sensor Servers <bt_mesh_sensor_srv_readme>`.

The model handling is implemented in :file:`src/model_handler.c`.
A :c:struct:`k_work_delayable` item is submitted recursively to periodically request sensor data.

User interface
**************

Buttons:
   Can be used to input the OOB authentication value during provisioning.
   All buttons have the same functionality during the provisioning procedure.

Once the provisioning procedure has completed, the buttons will have the following functionality:

.. tabs::

   .. group-tab:: nRF21 and nRF52 DKs

      Button 1:
         Sends a get message for the :c:var:`bt_mesh_sensor_dev_op_temp_range_spec` setting of the :c:var:`bt_mesh_sensor_present_dev_op_temp` sensor.

      Button 2:
         Sends a set message for the :c:var:`bt_mesh_sensor_dev_op_temp_range_spec` setting of the :c:var:`bt_mesh_sensor_present_dev_op_temp` sensor, switching between the ranges specified in the :c:var:`temp_ranges` variable.

      Button 3:
         Sends a get message for a descriptor, requesting information about the :c:var:`bt_mesh_sensor_present_dev_op_temp` sensor.

      Button 4:
         Sends a set message for the :c:var:`bt_mesh_sensor_motion_threshold` setting of the :c:var:`bt_mesh_sensor_presence_detected` sensor, switching between the ranges specified in the :c:var:`presence_motion_threshold` variable.

   .. group-tab:: nRF54 DKs

      Button 0:
         Sends a get message for the :c:var:`bt_mesh_sensor_dev_op_temp_range_spec` setting of the :c:var:`bt_mesh_sensor_present_dev_op_temp` sensor.

      Button 1:
         Sends a set message for the :c:var:`bt_mesh_sensor_dev_op_temp_range_spec` setting of the :c:var:`bt_mesh_sensor_present_dev_op_temp` sensor, switching between the ranges specified in the :c:var:`temp_ranges` variable.

      Button 2:
         Sends a get message for a descriptor, requesting information about the :c:var:`bt_mesh_sensor_present_dev_op_temp` sensor.

      Button 3:
         Sends a set message for the :c:var:`bt_mesh_sensor_motion_threshold` setting of the :c:var:`bt_mesh_sensor_presence_detected` sensor, switching between the ranges specified in the :c:var:`presence_motion_threshold` variable.

Terminal:
   All sensor values gathered from the server are printed over UART.
   For more details, see :ref:`testing`.

.. note::
   Some sensor and setting values need to be get/set through shell commands, as there is not enough buttons on the board for all sensor and setting values.

Configuration
*************

|config|

Source file setup
=================

This sample is split into the following source files:

* A :file:`main.c` file to handle initialization.
* One additional file for handling Bluetooth Mesh models, :file:`model_handler.c`.

FEM support
===========

.. include:: /includes/sample_fem_support.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/mesh/sensor_client`

.. include:: /includes/build_and_run.txt

.. _bluetooth_mesh_sensor_client_testing:

Testing
=======

.. note::
   The mesh sensor observer sample cannot demonstrate any functionality on its own, and needs a device with the :ref:`bluetooth_mesh_sensor_server` sample running in the same mesh network.
   Before testing the mesh sensor observer, go through the mesh sensor's :ref:`testing guide <bluetooth_mesh_sensor_server_testing>` with a different development kit.

After programming the sample to your development kit, you can test it by using a smartphone with `nRF Mesh mobile app`_ installed.
Testing consists of provisioning the device and configuring it for communication with the mesh models.

All sensor values gathered from the server are printed over UART.
For more details, see :ref:`testing`.

Provisioning the device
-----------------------

.. |device name| replace:: :guilabel:`Mesh Sensor Observer`

.. include:: /includes/mesh_device_provisioning.txt

Configuring models
------------------

See :ref:`ug_bt_mesh_model_config_app` for details on how to configure the mesh models with the nRF Mesh mobile app.

Configure the Sensor Client model on the **Mesh Sensor Observer** node:

* Bind the model to **Application Key 1**.
* Set the publication parameters:

  * Destination/publish address: Select an existing group or create a new one, but make sure that the Sensor Server subscribes to the same group.
  * Retransmit count: Set the count to zero (**Disabled**), to avoid duplicate logging in the UART terminal.

* Set the subscription parameters: Select an existing group or create a new one, but make sure that the Sensor Server publishes to the same group.

The Sensor Client model is now configured and able to receive data from the Sensor Server.

Interacting with the sample through shell
-----------------------------------------

1. Connect the development kit to the computer using a USB cable.
   The development kit is assigned a serial port.
   |serial_port_number_list|
#. |connect_terminal_specific_ANSI|
#. Enable local echo in the terminal to see the text you are typing.
#. Enable mesh shell by typing ``mesh init``

After completing the steps above, a command can be given to the client.
See :ref:`bt_mesh_sensor_cli_readme` and :ref:`bluetooth_mesh_shell` for information about shell commands.

SensorID/SettingID used in the shell commands are:

* :c:var:`bt_mesh_sensor_present_dev_op_temp` - 0x0054
* :c:var:`bt_mesh_sensor_dev_op_temp_range_spec` - 0x0013
* :c:var:`bt_mesh_sensor_rel_runtime_in_a_dev_op_temp_range` - 0x0064
* :c:var:`bt_mesh_sensor_presence_detected` - 0x004D
* :c:var:`bt_mesh_sensor_time_since_presence_detected` - 0x0069
* :c:var:`bt_mesh_sensor_motion_threshold` - 0x0043
* :c:var:`bt_mesh_sensor_present_amb_light_level` - 0x004E
* :c:var:`bt_mesh_sensor_gain` - 0x0074

For example, to set the sensor gain for present ambient light level to 1.1, write the following::

   mesh models sensor setting-set 0x004E 0x0074 1.1

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_mesh_sensor_cli_readme`
* :ref:`bt_mesh_dk_prov`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/drivers/hwinfo.h`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`

* :ref:`zephyr:bluetooth_mesh`:

  * :file:`include/bluetooth/mesh.h`

* :ref:`bluetooth_mesh_shell`
