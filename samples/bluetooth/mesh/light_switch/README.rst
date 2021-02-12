.. _bluetooth_mesh_light_switch:

Bluetooth: Mesh light switch
############################

.. contents::
   :local:
   :depth: 2

The Bluetooth mesh light switch sample demonstrates how to set up a basic mesh client model application, and control LEDs with the Bluetooth mesh using the :ref:`bt_mesh_onoff_readme`.

Overview
********

This sample is split into two source files:

* A :file:`main.c` file to handle initialization.
* One additional file for handling mesh models, :file:`model_handler.c`.

Provisioning
============

Provisioning is handled by the :ref:`bt_mesh_dk_prov`.

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

The models are used for the following purposes:

* :ref:`bt_mesh_onoff_cli_readme` instances in elements 1 to 4 are controlled by the buttons on the development kit.
* Config Server allows configurator devices to configure the node remotely.
* Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
  These callbacks trigger blinking of the LEDs.

The model handling is implemented in :file:`src/model_handler.c`, which uses the :ref:`dk_buttons_and_leds_readme` library to detect button presses on the development kit.

If the model is configured to publish to a unicast address, the model handler calls :c:func:`bt_mesh_onoff_cli_set` to turn the LEDs of a mesh light device on or off.
The response from the target device updates the corresponding LED on the mesh light switch device.
If the model is configured to publish to a group address, it calls :c:func:`bt_mesh_onoff_cli_set_unack` instead, to avoid getting responses from multiple devices at once.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuappns, nrf52840dk_nrf52840, nrf52dk_nrf52832, nrf52833dk_nrf52833

The sample requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

  * `nRF Mesh mobile app for Android`_
  * `nRF Mesh mobile app for iOS`_

An additional requirement is the :ref:`bluetooth_mesh_light` sample application programmed on a separate device, and configured according to the mesh light sample's :ref:`testing guide <bluetooth_mesh_light_testing>`.

User interface
**************

Buttons:
   Buttons are used to control the respective Generic OnOff Clients.
   When pressed, the button toggles the LED state on a :ref:`mesh light <bluetooth_mesh_light>` device.

LEDs:
   Show the last known OnOff state of the targeted :ref:`bluetooth_mesh_light` kit.


Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/mesh/light_switch`

.. include:: /includes/build_and_run.txt

.. _bluetooth_mesh_light_switch_testing:

Testing
=======

.. note::
   The light switch sample cannot demonstrate any functionality on its own, and needs a device with the :ref:`bluetooth_mesh_light` sample running in the same mesh network.
   Before testing mesh light switch, go through the mesh light's :ref:`testing guide <bluetooth_mesh_light_testing>` with a different kit.

After programming the sample to your development kit, you can test it by using a smartphone with Nordic Semiconductor's nRF Mesh app installed.
Testing consists of provisioning the device and configuring it for communication with the mesh models.

Provisioning the device
-----------------------

.. |device name| replace:: :guilabel:`Mesh Light Switch`

.. include:: /includes/mesh_device_provisioning.txt

Configuring models
------------------

See :ref:`ug_bt_mesh_model_config_app` for details on how to configure the mesh models with the nRF Mesh mobile app.

Configure the Generic OnOff Client model on each element on the :guilabel:`Mesh Light Switch` node:

* Bind the model to :guilabel:`Application Key 1`.
* Set the publication parameters:

  * Destination/publish address: Set the :guilabel:`Publish Address` to the first unicast address of the Mesh Light node.
  * Retransmit count: Set the count to zero (:guilabel:`Disabled`), to prevent the model from sending each button press multiple times.

You can now control the first LED on the mesh light device by pressing Button 1 on the mesh light switch development kit.

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
