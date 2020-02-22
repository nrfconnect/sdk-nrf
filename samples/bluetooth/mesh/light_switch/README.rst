.. _bluetooth_mesh_light_switch:

Bluetooth: Mesh Light Switch
############################

The Bluetooth Mesh Light Switch sample demonstrates how to set up a basic Mesh client model application and control LEDs with the Bluetooth Mesh, using the :ref:`bt_mesh_onoff_readme`.

Overview
********

This sample is split into two source files:

* A main file to handle initialization.
* One additional file for handling Mesh models

Provisioning
============

Provisioning is handled by the :ref:`bt_mesh_dk_prov`.

Models
======

This sample application has the following composition data:

.. table:: Mesh light composition data
   :align: center

   =================  =================  =================  =================
   Element 1          Element 2          Element 3          Element 4
   =================  =================  =================  =================
   Config Server      Gen. OnOff Client  Gen. OnOff Client  Gen. OnOff Client
   Health Server
   Gen. OnOff Client
   =================  =================  =================  =================

The :ref:`bt_mesh_onoff_cli_readme` instances in elements 1-4 are controlled by the buttons on the Device Kit.
The Config server allows configurator devices to configure the node remotely.
The Health server is used to call attention to the device during provisioning.

The model handling is implemented in src/model_handler.c, which uses the :ref:`dk_buttons_and_leds_readme` to detect button presses on the board.

The model handler calls :cpp:func:`bt_mesh_onoff_cli_set` to turn the LEDs of a Mesh Light device on or off.
As we pass a response buffer to the function, it will will block until the model receives a response from the target device.
If the function returns successfully, the response buffer will contain the response from the Mesh Light.
In this example, the contents of the response will be ignored, as we handle all incoming OnOff status messages in the status callback.
Note that the response structure is still required, as the Generic OnOff Client model won't request a response from the Server if we pass ``NULL`` in the response parameter.

The button handling is deferred to its own button handling thread to avoid blocking the system workqueue.
As the call to :cpp:func:`bt_mesh_onoff_cli_set` blocks until it receives a response, the caller might be blocked for seconds, which would severely impact other activities on the device.
This button handling thread is a loop that waits for a semaphore signaled by the button handler callback, and triggers calls to the appropriate Generic OnOff Client model.

Requirements
************

* One of the following development boards:

  * |nRF52840DK|
  * |nRF52DK|

* The Nordic Semiconductor nRF Mesh app for Android or iOS.
* The :ref:`bluetooth_mesh_light` sample application programmed on a separate device, and configured according to the Mesh Light sample's :ref:`bluetooth_mesh_light_testing` guide.

User interface
**************

Buttons:
   Buttons are used to control their own Generic OnOff Clients.
   When pressed, the button will toggle the LED state on a :ref:`bluetooth_mesh_light` device.

LEDs:
   Shows the last known OnOff state of the targeted :ref:`bluetooth_mesh_light` board.


Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/mesh/light_switch`

.. include:: /includes/build_and_run.txt

.. _bluetooth_mesh_light_switch_testing:

Testing
=======

.. important::
   The Light Switch sample cannot demonstrate any functionality on its own, and needs a device with the :ref:`bluetooth_mesh_light` sample running in the same mesh network.
   Before testing the Mesh Light Switch, go through the Mesh Light's :ref:`bluetooth_mesh_light_testing` guide with a different board.

After programming the sample to your board, you can test it by using the Nordic Semiconductor nRF Mesh app for Android or iOS.

Provisioning and configuration
------------------------------

Before communicating with the mesh models, the device must be provisioned.
The provisioning assigns an address range to the device, and adds it to the mesh network.
Provisioning is done through the nRF Mesh app:

1. Press "Add node" to start scanning for unprovisioned mesh devices.
#. Press the "Mesh Light Switch" device to connect to it.
#. Press "Identify", then "Provision" to provision the device.
#. When prompted, select an OOB method and follow the instructions in the app.
#. Once the provisioning and initial configuration is complete, the app will go back to the Network screen.
   Press the **Mesh Light Switch** node in the list.
   You'll see basic information about your mesh node and its configuration.
#. In the Mesh node view, expand the first element.
   It contains the list of models in the first element of the node.
#. Press the "Generic OnOff Client" model to see its configuration.
#. Models must be bound to application keys to be open to communication.
   Do this by pressing "BIND KEY" at the top of the screen.
   Select "Application Key 1" from the list.
#. In addition to the application key binding, Client models need publish parameters to be configured.
   The publish parameters define how the model should send its messages.
   Press "SET PUBLICATION" to configure the publish parameters.
#. Set the Publish Address to the first unicast address of the Mesh Light node.
   Set the Retransmit Count to zero ("Disabled") to prevent the model from sending each button press multiple times.
   The rest of the publish parameters can be left at their default values.
   Press "APPLY" to accept the configuration.
#. You should now be able to control the first LED on the Mesh Light device by pressing Button 1 on the Mesh Light Switch device kit.

Repeat steps 7-10 for each of the elements on the node to control each of the four LEDs on the Mesh Light device.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_mesh_onoff_cli_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* ``include/drivers/hwinfo.h``
* :ref:`zephyr:kernel`:

  * ``include/kernel.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``

* :ref:`zephyr:bluetooth_mesh`:

  * ``include/bluetooth/mesh.h``
