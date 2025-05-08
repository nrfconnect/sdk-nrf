.. _bluetooth_mesh_light_dim:

Bluetooth Mesh: Light dimmer and scene selector
###############################################

.. contents::
   :local:
   :depth: 2

The Bluetooth® Mesh light dimmer and scene selector sample demonstrates how to set up a light dimmer and scene selector application, and control dimmable LEDs with Bluetooth Mesh using the :ref:`bt_mesh_lvl_readme`, the :ref:`bt_mesh_onoff_readme`, and the :ref:`bt_mesh_scene_readme`.
The sample provides the following functionality:

  * On/off and dim up/down using one button
  * Scene recall/store of light levels with a second button

This sample demonstrates how to implement the following :ref:`ug_bt_mesh_nlc`:

  * Dimming Control NLC Profile
  * Basic Scene Selector NLC Profile

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
In addition to the on/off functionality, it allows changing the light level (brightness) of an LED light, and storing and recalling scenes of light levels.
To display any functionality, the sample must be paired with a device with the :ref:`mesh light fixture <bluetooth_mesh_light_lc>` sample running in the same mesh network.

The sample instantiates the following models:

 * :ref:`bt_mesh_lvl_cli_readme`
 * :ref:`bt_mesh_onoff_cli_readme`
 * :ref:`bt_mesh_scene_cli_readme`

Devices are nodes with a provisionee role in a mesh network.
Provisioning is performed using the `nRF Mesh mobile app`_.
This mobile application is also used to configure key bindings, and publication and subscription settings of the Bluetooth Mesh model instances in the sample.

.. tabs::

   .. group-tab:: nRF21, nRF52 and nRF53 DKs

      After provisioning and configuring the mesh models supported by the sample in the `nRF Mesh mobile app`_, **Button 1** on the Mesh Light Dimmer device can be used to control the configured network nodes' LEDs, while **Button 2** can be used to store and restore scenes on the network nodes.

      .. note::
        When running this sample on the :zephyr:board:`nrf52840dongle`, the scene selection functionality will not be available as the device only has one button.
        The single button of the dongle will be used for dimming and the on/off functionality as described for **Button 1** in this documentation.

   .. group-tab:: nRF54 DKs

      After provisioning and configuring the mesh models supported by the sample in the `nRF Mesh mobile app`_, **Button 0** on the Mesh Light Dimmer device can be used to control the configured network nodes' LEDs, while **Button 1** can be used to store and restore scenes on the network nodes.

      .. note::
        When running this sample on the :zephyr:board:`nrf52840dongle`, the scene selection functionality will not be available as the device only has one button.
        The single button of the dongle will be used for dimming and the on/off functionality as described for **Button 0** in this documentation.

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
   | Scene Client                  |
   +-------------------------------+

The models are used for the following purposes:

* The first element contains a Config Server and a Health Server.
  The Config Server allows configurator devices to configure the node remotely.
  The Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
  These callbacks trigger blinking of the LEDs.
* The third model in the first element is the Generic Level Client.
  The Generic Level Client controls the Generic Level Server in the target devices, deciding on parameters such as fade time and lighting level.
* The fourth model is the Generic OnOff Client which controls the Generic OnOff Server in the target devices, setting the LED state to ON or OFF.
* The last model is the Scene Client which controls the Scene Server in the target devices, storing or restoring scenes of the current LED states.


The model handling is implemented in :file:`src/model_handler.c`, which uses the :ref:`dk_buttons_and_leds_readme` library to control the buttons on the development kit.

User interface
**************

.. tabs::

   .. group-tab:: nRF21, nRF52 and nRF53 DKs

      Buttons:
        Can be used to input the out-of-band (OOB) authentication value during provisioning. All buttons have the same functionality during this procedure.

      Button 1:
        On press and release, **Button 1** will publish a Generic OnOff Set message using the configured publication parameters of its model instance, and toggle the LED on/off on a :ref:`mesh light fixture <bluetooth_mesh_light_lc>` device.

        On press and hold, **Button 1** will publish a Generic Level move set message using the configured publication parameters of its model instance and will continuously dim the LED lightness state on a :ref:`mesh light fixture <bluetooth_mesh_light_lc>` device.
        On release, the button publishes another Generic Level move set message with the ``delta`` parameter set to 0 and stops the continuous level change from the previous message.

      Button 2:
        On short press and release, **Button 2** will publish a Scene Restore message using the configured publication parameters of its model instance, and restore the LED state of all the targets to the values stored under the current scene number.
        Each press of the button recalls the next scene.
        The first press recalls scene 2, the next press recalls scene 3, and the sequence continues incrementally until it wraps around back to scene 1.


        On long press and release, **Button 2** will publish a Scene Store message using the configured publication parameters of its model instance, and store the current LED state of all the targets under the scene with the most recently recalled scene number.

        .. note::
          On the :zephyr:board:`nrf52840dongle`, the scene selection functionality will not be available as the device only has one button.

        .. tip::
          On Thingy:53, **Button 2** can be accessed by removing the top part of the casing.

      LEDs:
        Show the OOB authentication value during provisioning if the "Push button" OOB method is used.

        .. note::
          :zephyr:board:`thingy53` supports only one RGB LED.
          Each RGB LED channel is used as separate LED.

   .. group-tab:: nRF54 DKs

      Buttons:
        Can be used to input the out-of-band (OOB) authentication value during provisioning. All buttons have the same functionality during this procedure.

      Button 1:
        On press and release, **Button 0** will publish a Generic OnOff Set message using the configured publication parameters of its model instance, and toggle the LED on/off on a :ref:`mesh light fixture <bluetooth_mesh_light_lc>` device.

        On press and hold, **Button 0** will publish a Generic Level move set message using the configured publication parameters of its model instance and will continuously dim the LED lightness state on a :ref:`mesh light fixture <bluetooth_mesh_light_lc>` device.
        On release, the button publishes another Generic Level move set message with the ``delta`` parameter set to 0 and stops the continuous level change from the previous message.

      Button 2:
        On short press and release, **Button 1** publishes a Scene Restore message using the configured publication parameters of its model instance, and restores the LED state of all targets to the values stored under the current scene number.
        Each press of the button recalls the next scene.
        The first press recalls scene 2, the next press recalls scene 3, and the sequence continues incrementally until it wraps around back to scene 1.

        On long press and release, **Button 1** will publish a Scene Store message using the configured publication parameters of its model instance, and store the current LED state of all the targets under the scene with the most recently recalled scene number.

      LEDs:
        Show the OOB authentication value during provisioning if the "Push button" OOB method is used.

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

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

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

Configure the first element on the **Mesh Light Dimmer** node:

* Bind the following models to **Application Key 1**: Generic Level Client, Generic OnOff Client and Scene Client.
* Configure the publication for all models to the same group. This can be any group.

Configure the first element on the target **Mesh Light Fixture** node:

* Bind the following models to **Application Key 1**: Generic Level Server, Generic OnOff Server, Scene Server, and Scene Setup Server.
* Subscribe all models to the same group as set for publication for the **Mesh Light Dimmer**.

You should now be able to perform the following actions:

.. tabs::

   .. group-tab:: nRF21, nRF52 and nRF53 DKs

      * Press and release **Button 1** to see the LED on the target Mesh Light Fixture device toggle on and off.
      * Press and hold **Button 1** to see the light level of the LED on the target Mesh Light Fixture device slowly decrease or increase.
      * Short press and release **Button 2** to rotate through scenes, recalling each of them in turn.
      * Long press and release **Button 2** to store the current LED states as a scene with the current scene number.

   .. group-tab:: nRF54 DKs

      * Press and release **Button 0** to see the LED on the target Mesh Light Fixture device toggle on and off.
      * Press and hold **Button 0** to see the light level of the LED on the target Mesh Light Fixture device slowly decrease or increase.
      * Short press and release **Button 1** to rotate through scenes, recalling each of them in turn.
      * Long press and release **Button 1** to store the current LED states as a scene with the current scene number.

.. note::
  When controlling a Mesh Light Fixture device using the Mesh Light Dimmer device, the Light LC Server control will be temporarily disabled for the Mesh Light Fixture device.
  The control will be re-enabled after a certain time which can be configured using the :kconfig:option:`CONFIG_BT_MESH_LIGHT_CTRL_SRV_TIME_MANUAL` option.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_mesh_lvl_readme`
* :ref:`bt_mesh_onoff_readme`
* :ref:`bt_mesh_scene_readme`
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

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
