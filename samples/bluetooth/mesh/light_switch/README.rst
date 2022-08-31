.. _bluetooth_mesh_light_switch:

Bluetooth: Mesh light switch
############################

.. contents::
   :local:
   :depth: 2

The :ref:`ug_bt_mesh` light switch sample can be used to change the state of light sources on other devices within the same mesh network.
It also demonstrates how to use Bluetooth® mesh models by using the Generic OnOff Client model in an application.

Use the light switch sample with the :ref:`bluetooth_mesh_light` sample to demonstrate its function in a Bluetooth mesh network.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You need at least two development kits:

* One development kit where you program this sample application (the client)
* One (or more) development kit(s) where you program the :ref:`bluetooth_mesh_light` sample application (the server(s)), and configure according to the mesh light sample's :ref:`testing guide <bluetooth_mesh_light_testing>`

For provisioning and configuring of the mesh model instances, the sample requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

* `nRF Mesh mobile app for Android`_
* `nRF Mesh mobile app for iOS`_

.. note::
   |thingy53_sample_note|

.. include:: /includes/tfm.txt

Overview
********

The Bluetooth mesh light switch sample demonstrates how to set up a mesh client model application, and control LEDs with the Bluetooth mesh using the :ref:`bt_mesh_onoff_readme`.
To display any functionality, the sample must be paired with a device with the :ref:`bluetooth_mesh_light` sample running in the same mesh network.

In both samples, devices are nodes with a provisionee role in a mesh network.
Provisioning is performed using the `nRF Mesh mobile app`_.
This mobile application is also used to configure key bindings, and publication and subscription settings of the Bluetooth mesh model instances in the sample to enable them to communicate with the servers.

The Generic OnOff Client model is used for manipulating the Generic OnOff state associated with the Generic OnOff Server model.
The light switch sample implements the Generic OnOff Client model.

The sample instantiates up to four instances of the Generic OnOff Client model to control the state of LEDs on servers (implemented by the :ref:`bluetooth_mesh_light` sample).
One instance of the Generic OnOff Client model is instantiated in the light switch sample for each button available on the development kit that is used.
When a user presses any of the buttons, an OnOff Set message is sent out to the configured destination address.

After provisioning and configuring the mesh models supported by the sample using the `nRF Mesh mobile app`_, you can control the LEDs on the other (server) development kit(s) from the app.

Provisioning
============

Provisioning is handled by the :ref:`bt_mesh_dk_prov`.
It supports four types of out-of-band (OOB) authentication methods, and uses the Hardware Information driver to generate a deterministic UUID to uniquely represent the device.

Models
======

The following table shows the mesh light switch composition data for this sample:

.. table::
   :align: center

   =================  =================  =================  =================
   Element 1          Element 2          Element 3          Element 4
   =================  =================  =================  =================
   Config Server      Gen. OnOff Client  Gen. OnOff Client  Gen. OnOff Client
   Health Server
   Gen. OnOff Client
   =================  =================  =================  =================

.. note::
   When used with :ref:`zephyr:thingy53_nrf5340`, Elements 3 and 4 are not available.
   :ref:`zephyr:thingy53_nrf5340` supports only two buttons.

The models are used for the following purposes:

* :ref:`bt_mesh_onoff_cli_readme` instances in available elements are controlled by the buttons on the development kit.
* Config Server allows configurator devices to configure the node remotely.
* Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
  These callbacks trigger blinking of the LEDs.

The model handling is implemented in :file:`src/model_handler.c`, which uses the :ref:`dk_buttons_and_leds_readme` library to detect button presses on the development kit.

If the model is configured to publish to a unicast address, the model handler calls :c:func:`bt_mesh_onoff_cli_set` to turn the LEDs of a mesh light device on or off.
The response from the target device updates the corresponding LED on the mesh light switch device.
If the model is configured to publish to a group address, it calls :c:func:`bt_mesh_onoff_cli_set_unack` instead, to avoid getting responses from multiple devices at once.

.. _bluetooth_mesh_light_switch_user_interface:

User interface
**************

Buttons:
      During the provisioning process, the buttons can be used for OOB input.
      Once the provisioning and configuration are completed, the buttons are used to initiate certain actions and control the respective Generic OnOff Client instances.
      When pressed, the button publishes an OnOff message using the configured publication parameters of its model instance, and toggles the LED state on a :ref:`mesh light <bluetooth_mesh_light>` device.

LEDs:
   During the provisioning process, on board LEDs are used to output the OOB actions.
   Once the provisioning and configuration are completed, the LEDs are used to reflect the status of actions, and they show the last known OnOff state of the corresponding button.

.. note::
   :ref:`zephyr:thingy53_nrf5340` supports only one RGB LED.
   Each RGB LED channel is used as separate LED.

Configuration
*************

|config|

Source file setup
=================

The light switch sample is split into the following source files:

* :file:`main.c` used to handle initialization.
* :file:`model_handler.c` used to handle mesh models.
* :file:`thingy53.c` used to handle preinitialization of the :ref:`zephyr:thingy53_nrf5340` board.
  Only compiled when the sample is built for :ref:`zephyr:thingy53_nrf5340` board.

FEM support
===========

.. include:: /includes/sample_fem_support.txt

Building and running
********************

Make sure to enable the Bluetooth mesh in |NCS| before building and testing this sample.
See :ref:`Bluetooth mesh user guide <ug_bt_mesh>` for more information.

.. |sample path| replace:: :file:`samples/bluetooth/mesh/light_switch`

.. include:: /includes/build_and_run_ns.txt

.. _bluetooth_mesh_light_switch_testing:

Testing
=======

.. note::
   The light switch sample cannot demonstrate any functionality on its own, and needs a device with the :ref:`bluetooth_mesh_light` sample running in the same mesh network.
   Before testing mesh light switch, go through the mesh light's :ref:`testing guide <bluetooth_mesh_light_testing>` with a different development kit.

After programming the sample to your development kit, you can test it by using a smartphone with `nRF Mesh mobile app`_ installed.
Testing consists of provisioning the device and configuring it for communication with the mesh models.

Provisioning the device
-----------------------

.. |device name| replace:: :guilabel:`Mesh Light Switch`

.. include:: /includes/mesh_device_provisioning.txt

Configuring models
------------------

See :ref:`ug_bt_mesh_model_config_app` for details on how to configure the mesh models with the nRF Mesh mobile app.

Configure the Generic OnOff Client model on each element on the **Mesh Light Switch** node:

* Bind the model to **Application Key 1**.
* Set the publication parameters:

  * Destination/publish address: Set the **Publish Address** to the first unicast address of the Mesh Light node.
  * Retransmit count: Set the count to zero (**Disabled**), to prevent the model from sending each button press multiple times.

Once the provisioning and the configuration of the client node and at least one of the server nodes are complete, you can use buttons on the client development kit.
The buttons will control the LED lights on the associated servers, as described in :ref:`bluetooth_mesh_light_switch_user_interface`.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_mesh_onoff_cli_readme`
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
