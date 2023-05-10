.. _bluetooth_mesh_light_dim:

Bluetooth: Mesh light dimmer controller
#######################################

.. contents::
   :local:
   :depth: 2

The Bluetooth® mesh light dimmer sample demonstrates how to set up a light dimmer application, and control a dimmable LED with Bluetooth mesh using the :ref:`bt_mesh_lvl_readme` and the :ref:`bt_mesh_onoff_readme`.
The sample provides the controller with both the on/off and the dim up/down functionality, with a use of a single button.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

  * `nRF Mesh mobile app for Android`_
  * `nRF Mesh mobile app for iOS`_

.. note::
   |thingy53_sample_note|

.. include:: /includes/tfm.txt

Overview
********

This sample can be used to control the state of light sources.
In addition to the on/off functionality, it allows changing the light level (brightness) of an LED light.
To display any functionality, the sample must be paired with a device with the :ref:`mesh light fixture <bluetooth_mesh_light_lc>` sample running in the same mesh network.

The sample instantiates the following models:

 * :ref:`bt_mesh_lvl_cli_readme`
 * :ref:`bt_mesh_onoff_cli_readme`

Devices are nodes with a provisionee role in a mesh network.
Provisioning is performed using the `nRF Mesh mobile app`_.
This mobile application is also used to configure key bindings, and publication and subscription settings of the Bluetooth mesh model instances in the sample.
After provisioning and configuring the mesh models supported by the sample in the `nRF Mesh mobile app`_, **Button** 1 on the Mesh Light Dimmer device can be used to control the configured network nodes' LEDs.

Provisioning
============

The provisioning is handled by the :ref:`bt_mesh_dk_prov`.
It supports four types of out-of-band (OOB) authentication methods, and uses the Hardware Information driver to generate a deterministic UUID to uniquely represent the device.

Models
======

The following table shows the composition data for the light dimmer sample:

.. table::
   :align: center

   +-------------------------------+
   | Element 1                     |
   +===============================+
   | Config Server                 |
   +-------------------------------+
   | Health Server                 |
   +-------------------------------+
   | Gen. Level Client             |
   +-------------------------------+
   | Gen. OnOff Client             |
   +-------------------------------+

The models are used for the following purposes:

* The first element contains a Config Server and a Health Server.
  The Config Server allows configurator devices to configure the node remotely.
  The Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
  These callbacks trigger blinking of the LEDs.
* The third model in the first element is the Generic Level Client.
  The Generic Level Client controls the Generic Level Server in the target devices, deciding on parameters such as fade time and lighting level.
* The last model is the Generic OnOff Client which controls the Generic OnOff Server in the target devices, setting the LED state to ON or OFF.


The model handling is implemented in :file:`src/model_handler.c`, which uses the :ref:`dk_buttons_and_leds_readme` library to control the buttons on the development kit.

User interface
**************

Buttons:
   Can be used to input the out-of-band (OOB) authentication value during provisioning. All buttons have the same functionality during this procedure.

Button 1:
   On press and release, **Button 1** will publish a Generic OnOff Set message using the configured publication parameters of its model instance, and toggle the LED on/off on a :ref:`mesh light fixture <bluetooth_mesh_light_lc>` device.

   On press and hold, **Button 1** will publish a Generic Level move set message using the configured publication parameters of its model instance and will continuously dim the LED lightness state on a :ref:`mesh light fixture <bluetooth_mesh_light_lc>` device. On release, the button publishes another Generic Level move set message with the ``delta`` parameter set to 0 and stops the continuous level change from the previous message.

LEDs:
   Show the OOB authentication value during provisioning if the "Push button" OOB method is used.

.. note::
   :ref:`zephyr:thingy53_nrf5340` supports only one RGB LED.
   Each RGB LED channel is used as separate LED.

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

.. |sample path| replace:: :file:`samples/bluetooth/mesh/light_dimmer`

.. include:: /includes/build_and_run_ns.txt

.. _bluetooth_mesh_light_dimmer_testing:

Testing
=======

After programming the sample to your development kit, you can test it by using a smartphone with `nRF Mesh mobile app`_ installed.
Testing consists of provisioning the device and configuring it for communication with the mesh models.

Provisioning the device
-----------------------

.. |device name| replace:: :guilabel:`Mesh Light Dimmer`

.. include:: /includes/mesh_device_provisioning.txt

Configuring models
------------------

See :ref:`ug_bt_mesh_model_config_app` for details on how to configure the mesh models with the nRF Mesh mobile app.

Configure the Generic Level Client model on the first element on the **Mesh Light Dimmer** node:

* Bind the model to **Application Key 1**.
* Configure the publication to any group.

Configure the first element on the target **Mesh Light Fixture** node:

* Bind the following models to **Application Key 1**: Generic Level Server and Generic OnOff Server.
* Subscribe both models to the same group as set for publication for the **Mesh Light Dimmer**.

You should now be able to perform the following actions:

* Press and release **Button 1** to see the LED on the target Mesh Light Fixture device toggle on and off.
* Press and hold **Button 1** to see the light level of the LED on the target Mesh Light Fixture device slowly decrease or increase.

.. note::
  When controlling a Mesh Light Fixture device using the Mesh Light Dimmer device, the Light LC Server control will be temporarily disabled for the Mesh Light Fixture device.
  The control will be re-enabled after a certain time which can be configured using the :kconfig:option:`CONFIG_BT_MESH_LIGHT_CTRL_SRV_TIME_MANUAL` option.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_mesh_lvl_readme`
* :ref:`bt_mesh_onoff_readme`
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

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
