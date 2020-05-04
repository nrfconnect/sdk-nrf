.. _bluetooth_mesh_sensor_server:

Bluetooth: Mesh sensor server
###############################

The Bluetooth Mesh sensor server sample demonstrates how to set up a basic Mesh sensor server model application that provides sensor data to one :ref:`Mesh sensor client <bt_mesh_sensor_cli_readme>` model.
Four different sensor types are used to showcase different ways for the server to publish data.
In addition, the samples demonstrate usage of both :ref:`single-channel sensor types and sensor series types <bt_mesh_sensor_types_channels>`.

.. note::
    This sample must be paired with :ref:`bluetooth_mesh_sensor_client` to show any functionality.
    The server provides the sensor data used by the client.

Overview
********

This sample is split into the following source files:

* A :file:`main.c` file to handle initialization.
* One additional file for handling Mesh models, :file:`model_handler.c`.

The following Mesh sensor types are used in this sample:

* :cpp:var:`bt_mesh_sensor_present_dev_op_temp` - Published by the server according to its publishing period (see :ref:`bluetooth_mesh_sensor_server_conf_models`).
* :cpp:var:`bt_mesh_sensor_rel_runtime_in_a_dev_op_temp_range` - Periodically requested by the client.
* :cpp:var:`bt_mesh_sensor_presence_detected` - Published when a button is pressed on the server board.
* :cpp:var:`bt_mesh_sensor_time_since_presence_detected` - Periodically requested by the client and published by the server according to its publishing period (see :ref:`bluetooth_mesh_sensor_server_conf_models`).

Moreover, the on-chip ``TEMP_NRF5`` temperature sensor is used.

Provisioning
============

The provisioning is handled by the :ref:`bt_mesh_dk_prov`.
Use `nRF Mesh mobile app`_ for provisioning and configuring of models supported by the sample.

Models
======

The following table shows the Mesh sensor server composition data for this sample:

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
* Mesh sensor server provides sensor data to one or more :ref:`Mesh sensor client(s) <bt_mesh_sensor_cli_readme>`.

The model handling is implemented in :file:`src/model_handler.c`, which uses the ``TEMP_NRF5`` temperature sensor, and :ref:`dk_buttons_and_leds_readme` to detect button presses.

Requirements
************

* One of the following development kits:

  * |nRF52840DK|
  * |nRF52DK|

* Smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

  * `nRF Mesh mobile app for Android`_
  * `nRF Mesh mobile app for iOS`_

* :ref:`bluetooth_mesh_sensor_client` sample application programmed on a separate device and configured according to the Mesh sensor client's :ref:`testing guide <bluetooth_mesh_sensor_client_testing>`.

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
   The Mesh sensor server sample cannot demonstrate any functionality on its own, and needs a device with the :ref:`bluetooth_mesh_sensor_client` sample running in the same Mesh network.
   Before testing the Mesh sensor server, go through the Mesh sensor client's :ref:`testing guide <bluetooth_mesh_sensor_client_testing>` with a different board.

After programming the sample to your board, you can test it by using a smartphone with Nordic Semiconductor’s nRF Mesh app installed.
Testing consists of provisioning the device and configuring it for communication with the mesh models.

Provisioning the device
-----------------------

The provisioning assigns an address range to the device, and adds it to the mesh network.
Complete the following steps in the nRF Mesh app:

1. Tap :guilabel:`Add node` to start scanning for unprovisioned mesh devices.
#. Select the :guilabel:`Mesh Sensor Server` device to connect to it.
#. Tap :guilabel:`Identify` and then :guilabel:`Provision` to provision the device.
#. When prompted, select an OOB method and follow the instructions in the app.

Once the provisioning is complete, the app returns to the Network screen.

.. _bluetooth_mesh_sensor_server_conf_models:

Configuring models
------------------

Complete the following steps in the nRF Mesh app to configure models:

1. On the Network screen, tap the :guilabel:`Mesh Sensor Server` node.
   Basic information about the mesh node and its configuration is displayed.
#. In the Mesh node view, expand the element.
   It contains the list of models in the first and only element of the node.
#. Tap :guilabel:`Sensor Server` to see the model's configuration.
#. Bind the model to application keys to make it open for communication:

   a. Tap :guilabel:`BIND KEY` at the top of the screen.
   #. Select :guilabel:`Application Key 1` from the list.

#. Set the publishing parameters:

   a. Tap :guilabel:`SET PUBLICATION`.
   #. Tap :guilabel:`Publish Address`.
   #. Select :guilabel:`Groups` from the drop-down menu.
   #. Select an existing group or create a new one.

      .. note::
            The sensor client must subscribe to the same group.

   #. Tap :guilabel:`OK`.
   #. Select a publishing period by using the :guilabel:`Interval` slider.
   #. Set the Retransmit Count to zero (:guilabel:`Disabled`) to avoid duplicate logging in the :ref:`sensor client's <bt_mesh_sensor_cli_readme>` UART terminal.
   #. Tap the confirmation button at the bottom right corner of the app to save the parameters.

#. Set subscription parameters:

   a. Tap :guilabel:`SUBSCRIBE`.
   #. Select an existing group or create a new one.

      .. note::
            The sensor client must publish to the same group.

   #. Tap :guilabel:`OK`.

#. Double-tap the back arrow button at the top left corner of the app to get back to the main application screen.

The sensor server model is now configured and able to send data to the client.

.. note::
    To enable server configuration by a client, an application key must be bound to the sensor setup server.
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
