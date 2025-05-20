.. _bluetooth_mesh_silvair_enocean:

Bluetooth Mesh: Silvair EnOcean
###############################

.. contents::
   :local:
   :depth: 2

You can use the :ref:`ug_bt_mesh` Silvair EnOcean sample to change the state of light sources on other devices within the same mesh network.
It also demonstrates how to use Bluetooth® Mesh models by using the Silvair EnOcean Proxy Server model in an application.

Use the Silvair EnOcean sample with the :ref:`bluetooth_mesh_light_lc` sample to demonstrate its function in a Bluetooth Mesh network.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You need at least two development kits:

* One development kit where you program this sample application (Silvair EnOcean Proxy Server model, including the Generic OnOff Client and the Generic Level Client)
* At least one development kit where you program the :ref:`bluetooth_mesh_light_lc` sample application (the server), and configure according to the mesh light fixture sample's :ref:`testing guide <bluetooth_mesh_light_ctrl_testing>`

For provisioning and configuring of the mesh model instances, the sample requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

* `nRF Mesh mobile app for Android`_
* `nRF Mesh mobile app for iOS`_

.. include:: /includes/tfm.txt

Overview
********

The Bluetooth Mesh Silvair EnOcean sample demonstrates how to set up mesh client model applications, and control LEDs with the Bluetooth Mesh using the :ref:`bt_mesh_onoff_readme` and :ref:`bt_mesh_lvl_readme` with use of an EnOcean switch.
To display any functionality, the sample must be paired with a device with the :ref:`bluetooth_mesh_light_lc` sample running in the same mesh network.

In both samples, devices are nodes with a provisionee role in a mesh network.
Provisioning is performed using the `nRF Mesh mobile app`_.
This mobile application is also used to configure key bindings, and publication and subscription settings of the Bluetooth Mesh model instances in the sample to enable them to communicate with the servers.

The Generic OnOff Client model and the Generic Level Client model are used for manipulating the Generic OnOff state and Generic Level state associated with the Generic OnOff Server model and Generic Level Server model respectively.
The Silvair EnOcean sample implements the Silvair EnOcean Proxy Server model that instantiates the Generic OnOff Client and the Generic Level Client models.

The sample uses a two (or one) button EnOcean switch to control the state of LED 1 on servers (implemented by the :ref:`bluetooth_mesh_light_lc` sample).
Two instances of the Generic OnOff Client model and two instances of the Generic Level Client model are instantiated in the Silvair EnOcean sample, one of each model for each button on the EnOcean switch that is used.
When a user presses or holds any of the buttons on the EnOcean switch, an OnOff/Level Set message is sent out to the configured destination address.

After provisioning and configuring the mesh models supported by the sample using the `nRF Mesh mobile app`_, you can control the LEDs on the other (server) development kits from the app.

Provisioning
============

Provisioning is handled by the :ref:`bt_mesh_dk_prov`.
It supports four types of out-of-band (OOB) authentication methods, and uses the Hardware Information driver to generate a deterministic UUID to uniquely represent the device.

Models
======

The following table shows the mesh Silvair EnOcean composition data for this sample:

.. table::
   :align: center

   =============================  ===================
   Element 1                      Element 2
   =============================  ===================
   Config Server                  Gen. Level Client
   Health Server                  Gen. OnOff Client
   Gen. DTT Server
   Gen. Level Client
   Gen. OnOff Client
   Silvair EnOcean Proxy Server
   =============================  ===================

The models are used for the following purposes:

* The Silvair EnOcean Proxy Server instantiates :ref:`bt_mesh_onoff_cli_readme` and :ref:`bt_mesh_lvl_cli_readme` on both elements, where each element is controlled by the buttons on the EnOcean switch.
* :ref:`bt_mesh_dtt_srv_readme` is used to control transition time of the Generic OnOff Server instances.
* Config Server allows configurator devices to configure the node remotely.
* Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device. These callbacks trigger blinking of the LEDs.

The model handling is implemented in :file:`src/model_handler.c`, which uses the :ref:`dk_buttons_and_leds_readme` library to detect button presses on the development kit.

The response from the target device updates the corresponding LED on the mesh Silvair EnOcean device.
When the target device is turned on or off, the corresponding LED will turn on or off accordingly.
If the light level is increased above `BT_MESH_LVL_MIN` on the target device, the corresponding LED will turn on.
The LED will turn off if the light level is equal to `BT_MESH_LVL_MIN`.

.. _bluetooth_mesh_silvair_enocean_user_interface:

User interface
**************

Development kit buttons:
      During the provisioning process, all buttons can be used for OOB input.
      Once the provisioning and configuration are completed, the buttons are not used.

EnOcean buttons:
      Pressing and releasing the button on the EnOcean switch publishes an OnOff message using the configured publication parameters of its model instance, and toggles the LED state on a :ref:`mesh light <bluetooth_mesh_light_lc>` device.
      Pressing and holding the button on the EnOcean switch publishes a Level message using the configured publication parameters of its model instance, and changes the emitted LED light level on the :ref:`mesh light <bluetooth_mesh_light_lc>` device.

LEDs:
   During the provisioning process, all LEDs are used to output the OOB actions.
   Once the provisioning and configuration are completed, the LEDs are used to reflect the status of actions, and they show the last known OnOff/Level state of the corresponding button.
   It will not change its emitted LED light level, it will only be on or off.

Configuration
*************

|config|

|nrf5340_mesh_sample_note|

Source file setup
=================

The Silvair EnOcean sample is split into the following source files:

* A :file:`main.c` file to handle initialization.
* One additional file for handling mesh models, :file:`model_handler.c`.

FEM support
===========

.. include:: /includes/sample_fem_support.txt

Building and running
********************

Make sure to enable the Bluetooth Mesh in |NCS| before building and testing this sample.
See :ref:`Bluetooth Mesh user guide <ug_bt_mesh>` for more information.

.. |sample path| replace:: :file:`samples/bluetooth/mesh/silvair_enocean`

.. include:: /includes/build_and_run_ns.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

.. _bluetooth_mesh_silvair_enocean_testing:

Testing
=======

.. note::
   The Silvair EnOcean sample cannot demonstrate any functionality on its own, and needs a device with the :ref:`bluetooth_mesh_light_lc` sample running in the same mesh network.

After programming the sample to your development kit, you can test it by using a smartphone with `nRF Mesh mobile app`_ installed.
Testing consists of provisioning the device and configuring it for communication with the mesh models.

Provisioning the device
-----------------------

.. |device name| replace:: :guilabel:`Mesh Silvair EnOcean`

.. include:: /includes/mesh_device_provisioning.txt

Configuring the EnOcean switch
------------------------------

See :ref:`bt_enocean_commissioning` for details on how to configure the EnOcean switch.

Configuring models
------------------

See :ref:`ug_bt_mesh_model_config_app` for details on how to configure the mesh models with the nRF Mesh mobile app.

Configure the :ref:`bt_mesh_onoff_cli_readme` and the :ref:`bt_mesh_lvl_cli_readme` models on each element on the **Mesh Silvair EnOcean** node:

* Bind the model to **Application Key 1**.
* Set the publication parameters:

  * Destination/publish address: Set the **Publish Address** to the first unicast address of the Mesh Light Fixture node.

.. note::
   Configuring the periodic publication and publication retransmission of these models has no effect.

Once the provisioning and the configuration of the client node and at least one of the server nodes are complete, you can use buttons on the EnOcean switch.
The buttons will control the LED lights on the associated servers, as described in :ref:`bluetooth_mesh_silvair_enocean_user_interface`.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_enocean_readme`
* :ref:`bt_mesh_onoff_cli_readme`
* :ref:`bt_mesh_lvl_cli_readme`
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
