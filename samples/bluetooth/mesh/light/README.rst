.. _bluetooth_mesh_light:

Bluetooth: Mesh Light
#####################

The Bluetooth Mesh Light sample demonstrates how to set up a basic Mesh server model application and control LEDs with the Bluetooth Mesh, using the :ref:`bt_mesh_onoff_readme`.

Overview
********

This sample is split into two source files:

* A main file to handle initialization.
* One additional file for handling Mesh models

Provisioning
============

The provisioning is handled the :ref:`bt_mesh_dk_prov` with the default configuration.

Models
======

This sample application has the following composition data:

.. table:: Mesh light composition data
   :align: center

   =================  =================  =================  =================
   Element 1          Element 2          Element 3          Element 4
   =================  =================  =================  =================
   Config Server      Gen. OnOff Server  Gen. OnOff Server  Gen. OnOff Server
   Health Server
   Gen. OnOff Server
   =================  =================  =================  =================

The :ref:`bt_mesh_onoff_srv_readme` instances in elements 1-4 control LEDs 1-4 respectively.
The Config server allows configurator devices to configure the node remotely.
The Health server is used to call attention to the device during provisioning.

The model handling is implemented in src/model_handler.c, which uses the :ref:`dk_buttons_and_leds_readme` to control each LED on the board according to the matching Generic OnOff server's received messages.

Requirements
************

* One of the following development boards:

  * |nRF5340DK|
  * |nRF52840DK|
  * |nRF52DK|
  * |nRF51DK|

* The Nordic Semiconductor nRF Mesh app for Android or iOS.

User interface
**************

Buttons:
   May be used to input the OOB authentication value during provisioning.
   All buttons have the same functionality during this procedure.

LEDs:
   Shows the OOB authentication value during provisioning if the "Push button" OOB method is used.
   Shows the OnOff state of the Generic OnOff server of the corresponding element.


Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/mesh/light`

.. include:: /includes/build_and_run.txt

.. _bluetooth_mesh_light_testing:

Testing
=======

After programming the sample to your board, you can test it by using the Nordic Semiconductor nRF Mesh app for Android or iOS.

Provisioning and configuration
------------------------------

Before communicating with the mesh models, the device must be provisioned.
The provisioning assigns an address range to the device, and adds it to the mesh network.
Provisioning is done through the nRF Mesh app for Android or iOS:

1. Press "Add node" to start scanning for unprovisioned mesh devices.
#. Press the "Mesh Light" device to connect to it.
#. Press "Identify", then "Provision" to provision the device.
#. When prompted, select an OOB method and follow the instructions in the app.
#. Once the provisioning and initial configuration is complete, the app will go back to the Network screen.
   Press the **Mesh Light** node in the list.
   You'll see basic information about your mesh node and its configuration.
#. In the Mesh node view, expand the first element.
   It contains the list of models in the first element of the node.
#. Press the "Generic OnOff Server" model to see its configuration.
#. Models must be bound to application keys to be open to communication.
   Do this by pressing "BIND KEY" at the top of the screen.
   Select "Application Key 1" from the list.
#. You should now be able to control the first LED on the device by using the "Generic On Off Controls" in the model view.
   Pressing "ON" should make the first LED on the DK light up.

Repeat steps 7-9 for each of the elements on the node to control each of the four LEDs on the DK.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_mesh_onoff_srv_readme`
* :ref:`bt_mesh_dk_prov`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* ``include/drivers/hwinfo.h``
* :ref:`zephyr:kernel`:

  * ``include/kernel.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``

* :ref:`zephyr:bluetooth_mesh`:

  * ``include/bluetooth/mesh.h``
