.. _bluetooth_mesh_sensor_server:

Bluetooth Mesh: Sensor
######################

.. contents::
   :local:
   :depth: 2

The Bluetooth® Mesh sensor sample demonstrates how to set up a basic mesh Sensor Server model application that provides sensor data to one :ref:`bt_mesh_sensor_cli_readme` model.
Eight different sensor types are used to showcase different ways for the server to publish data.
In addition, the samples demonstrate usage of both :ref:`single-channel sensor types and sensor series types <bt_mesh_sensor_types_channels>`, as well as how to add and write to a sensor setting.

.. note::
   This sample must be paired with the :ref:`bluetooth_mesh_sensor_client` sample to show any functionality.
   The mesh sensor provides the sensor data used by the observer.

This sample demonstrates how to implement the following :ref:`ug_bt_mesh_nlc`:

  * Ambient Light Sensor NLC Profile
  * Occupancy Sensor NLC Profile

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

For provisioning and configuring of the mesh model instances, the sample requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

* `nRF Mesh mobile app for Android`_
* `nRF Mesh mobile app for iOS`_

.. note::
   |thingy53_sample_note|

Additionally, the sample requires the :ref:`bluetooth_mesh_sensor_client` sample application.
The application needs to be programmed on a separate device, and configured according to the sensor observer sample's :ref:`testing guide <bluetooth_mesh_sensor_server_testing>`.

.. include:: /includes/tfm.txt

Overview
********

The following Bluetooth Mesh sensor types, and their settings, are used in this sample:

* On Sensor Server instance on Element 1:

  * :c:var:`bt_mesh_sensor_present_amb_light_level` - Periodically requested by the client, and published when a button is pressed on the server.

    * :c:var:`bt_mesh_sensor_gain` - Used as a setting for the :c:var:`bt_mesh_sensor_present_amb_light_level` sensor type to set the gain the ambient light sensor value is multiplied with.
    * :c:var:`bt_mesh_sensor_present_amb_light_level` - Used as a setting for the :c:var:`bt_mesh_sensor_present_amb_light_level` sensor type to calculate sensor gain based on measured reference ambient light level. This value does only have a set command.

* On Sensor Server instance on Element 2:

  * :c:var:`bt_mesh_sensor_presence_detected` - Published when a button is pressed on the server.

    * :c:var:`bt_mesh_sensor_motion_threshold` - Used as a setting for the :c:var:`bt_mesh_sensor_presence_detected` sensor type to set the time (0-10 seconds) before the presence is detected.

  * :c:var:`bt_mesh_sensor_time_since_presence_detected` - Periodically requested by the client and published by the server according to its publishing period (see :ref:`bluetooth_mesh_sensor_server_conf_models`).

* On Sensor Server instance on Element 3:

  * :c:var:`bt_mesh_sensor_motion_sensed` - Published when a button is pressed on the server.
  * :c:var:`bt_mesh_sensor_time_since_motion_sensed` - Periodically requested by the client and published by the server according to its publishing period (see :ref:`bluetooth_mesh_sensor_server_conf_models`).

* On Sensor Server instance on Element 4:

  * :c:var:`bt_mesh_sensor_people_count` - Periodically requested by the client, and published when a button is pressed on the server.

* On Sensor Server instance on Element 5:

  * :c:var:`bt_mesh_sensor_present_dev_op_temp` - Published by the server according to its publishing period (see :ref:`bluetooth_mesh_sensor_server_conf_models`), or periodically requested by the client.

    * :c:var:`bt_mesh_sensor_dev_op_temp_range_spec` - Used as a setting for the :c:var:`bt_mesh_sensor_present_dev_op_temp` sensor type to set the range of reported temperatures.

  * :c:var:`bt_mesh_sensor_rel_runtime_in_a_dev_op_temp_range` - Periodically requested by the client.

.. note::
   These values can be requested through shell commands by the :ref:`bluetooth_mesh_sensor_client`.

Moreover, the on-chip ``TEMP_NRF5`` temperature sensor is used for the nRF52 series, and the ``BME680`` temperature sensor for Thingy:53.

.. note::
  When running this sample on Thingy:53, some functionality will not be available as the device only has two buttons.
  The two buttons on Thingy:53 will be used for the ambient light sensor and presence detected sensor functionality as described for **Button 1** and **Button 2** in this documentation.
  **Button 2** can be accessed by removing the top part of the casing.

Provisioning
============

The provisioning is handled by the :ref:`bt_mesh_dk_prov`.
It supports four types of out-of-band (OOB) authentication methods, and uses the Hardware Information driver to generate a deterministic UUID to uniquely represent the device.

Use `nRF Mesh mobile app`_ for provisioning and configuring of models supported by the sample.

Models
======

The following table shows the Bluetooth Mesh sensor composition data for this sample:

.. table::
   :align: center

   ===================  ===================  ===================  ===================  ===================
   Element 1            Element 2            Element 3            Element 4            Element 5
   ===================  ===================  ===================  ===================  ===================
   Config Server        Sensor Server        Sensor Server        Sensor Server        Sensor Server
   Health Server        Sensor Setup Server  Sensor Setup Server  Sensor Setup Server  Sensor Setup Server
   Sensor Server
   Sensor Setup Server
   ===================  ===================  ===================  ===================  ===================

The models are used for the following purposes:

* Config Server allows configurator devices to configure the node remotely.
* Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
  These callbacks trigger blinking of the LEDs.
* Sensor Server instances provide sensor data to one or more :ref:`mesh sensor observers <bt_mesh_sensor_cli_readme>`.
* Sensor Setup Server instances are used for configuration of the corresponding Sensor Server instances.

The model handling is implemented in :file:`src/model_handler.c`.
It uses the TEMP_NRF5 or BME680 temperature sensor depending on the platform.

The sample has a descriptor related to the :c:var:`bt_mesh_sensor_present_dev_op_temp` sensor, which specifies tolerance values for the ``TEMP_NRF5`` temperature sensor calculated based on the `nRF52832 Temperature Sensor Electrical Specification`_.
The descriptor also specifies the temperature sensor's sampling type, which is :c:var:`BT_MESH_SENSOR_SAMPLING_INSTANTANEOUS`.

The :ref:`dk_buttons_and_leds_readme` library is used to detect button presses.

The :ref:`Zephyr settings API <zephyr:settings_api>` is used to persistently store the following settings given that :kconfig:option:`CONFIG_BT_SETTINGS` is enabled:

* The temperature range used in the :c:var:`bt_mesh_sensor_present_dev_op_temp` sensor
* The presence motion threshold used in the :c:var:`bt_mesh_sensor_presence_detected` sensor
* The ambient light level gain used in the :c:var:`bt_mesh_sensor_present_amb_light_level` sensor

User interface
**************

Buttons:
   Can be used to input the OOB authentication value during provisioning.
   All buttons have the same functionality during the provisioning procedure.

Once the provisioning procedure has completed, the buttons will have the following functionality:

.. tabs::

   .. group-tab:: nRF21 and nRF52 DKs

      Button 1:
        Simulates different ambient light sensor values.
        These dummy values represent raw values coming from an ambient light sensor.

      Button 2:
        Simulates presence detected.
        For how long the button has to be pressed before the presence is detected depends on the motion threshold.
        The motion threshold has five steps from 0 % (representing 0 seconds) to 100 % (representing 10 seconds) separated by 25 %-steps.

      Button 3:
        Simulates motion sensed.

      Button 4:
        Simulates different people count sensor values.
        These dummy values represent raw values coming from a people count sensor.

   .. group-tab:: nRF54 DKs

      Button 0:
        Simulates different ambient light sensor values.
        These dummy values represent raw values coming from an ambient light sensor.

      Button 1:
        Simulates presence detected.
        For how long the button has to be pressed before the presence is detected depends on the motion threshold.
        The motion threshold has five steps from 0 % (representing 0 seconds) to 100 % (representing 10 seconds) separated by 25 %-steps.

      Button 2:
        Simulates motion sensed.

      Button 3:
        Simulates different people count sensor values.
        These dummy values represent raw values coming from a people count sensor.

Configuration
*************

|config|

|nrf5340_mesh_sample_note|

Source file setup
=================

This sample is split into the following source files:

* A :file:`main.c` file to handle initialization.
* A file for handling mesh models, :file:`model_handler.c`.

FEM support
===========

.. include:: /includes/sample_fem_support.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/mesh/sensor_server`

.. include:: /includes/build_and_run_ns.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

.. _bluetooth_mesh_sensor_server_testing:

Testing
=======

.. note::
   The Bluetooth Mesh sensor sample cannot demonstrate any functionality on its own, and needs a device with the :ref:`bluetooth_mesh_sensor_client` sample running in the same mesh network.
   Before testing the sensor sample, go through the sensor observer sample's :ref:`testing guide <bluetooth_mesh_sensor_client_testing>` with a different development kit.

After programming the sample to your development kit, you can test it by using a smartphone with `nRF Mesh mobile app`_ installed.
Testing consists of provisioning the device, and configuring it for communication with the mesh models.

Provisioning the device
-----------------------

.. |device name| replace:: :guilabel:`Mesh Sensor`

.. include:: /includes/mesh_device_provisioning.txt

.. _bluetooth_mesh_sensor_server_conf_models:

Configuring models
------------------

See :ref:`ug_bt_mesh_model_config_app` for details on how to configure the mesh models with the nRF Mesh mobile app.

Configure Sensor Server model on each element on the **Mesh Sensor** node:

* Bind the model to **Application Key 1**.
* Set the publication parameters:

  * Destination/publish address: Select an existing group or create a new one, but make sure that the Sensor Client subscribes to the same group.
  * Retransmit count: Set the count to zero (**Disabled**), to avoid duplicate logging in the :ref:`bt_mesh_sensor_cli_readme`'s UART terminal.

* Set the subscription parameters: Select an existing group or create a new one, but make sure that the Sensor Client publishes to the same group.

The Sensor Server models are now configured and able to send data to the Sensor Client.

Configure the corresponding Sensor Setup Server model on each element on the **Mesh Sensor** node:

* Bind the model to **Application Key 1**.

* Set the subscription parameters: Select an existing group or create a new one, but make sure that the Sensor Client publishes to the same group.

The Sensor Setup Server models are now configured and able to receive sensor setting messages from the Sensor Client.

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

* :file:`include/drivers/hwinfo.h`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`

* :ref:`zephyr:bluetooth_mesh`:

  * :file:`include/bluetooth/mesh.h`

* :ref:`zephyr:settings_api`:

  * :file:`include/settings/settings.h`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
