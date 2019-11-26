.. _bluetooth_mesh_light:

Bluetooth: Mesh Light
#####################

The Bluetooth Mesh Light sample demonstrates how to set up a basic Mesh
application and control LEDs with the Bluetooth Mesh, using the
:ref:`bt_mesh_onoff_readme`.

Overview
********

This sample is split into three source files: One handling provisioning, one
handling Mesh models, and a main file to tie the application together.

Provisioning
============

Provisioning is handled in :file:`src/prov_handler.c`.

Authentication
--------------

The provisioning handler provides the following Out Of Band (OOB)
authentication methods during provisioning:

#. Output: Display number
#. Output: Blink
#. Input: Push button

The first OOB method requires access to the application log through a serial
connection. If selected, the node will print an authentication number in its
serial log, which should be entered into the provisioner.

The second OOB method blinks the LEDs a certain number of times. The number of
blinks should be entered into the provisioner. The blinks occur at a frequency
of two blinks per second, and the sequence repeats every three seconds.

The third OOB method requires the user press a button the number of times
specified by the provisioner. After three seconds of inactivity, the button
count is sent to the provisioner.

The OOB methods can be changed by adding or removing ``input_actions`` and
``output_actions`` in the :cpp:type:`bt_mesh_prov` structure. It's also
possible for the provisioner to select "No authentication", and skip OOB
authentication altogether. This is not recommended, as it breaks Mesh network
security.

UUID
----

The provisioning handler uses :cpp:func:`hwinfo_get_device_id` from the
``hwinfo`` driver to generate a unique UUID for each device.

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

The :ref:`bt_mesh_onoff_srv_readme` instances in elements 1-4 control LEDs 1-4
respectively. The Config server allows configurator devices to configure the
node remotely. The Health server is used to call attention to the device
during provisioning.

The model handling is implemented in src/model_handler.c, which uses the
:ref:`dk_buttons_and_leds_readme` to control each LED on the board according
to the matching Generic OnOff server's received messages.

Requirements
************

* One of the following development boards:

  * |nRF52840DK|
  * |nRF52DK|
  * |nRF51DK|

* The Nordic Semiconductor nRF Mesh app for Android or iOS.

User interface
**************

Buttons:
   May be used to input the OOB authentication value during provisioning. All
   buttons have the same functionality during this procedure.

LEDs:
   Shows the OOB authentication value during provisioning if the "Push button"
   OOB method is used. Shows the OnOff state of the Generic OnOff server of
   the corresponding element.


Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/mesh/light`

.. include:: /includes/build_and_run.txt

.. _bluetooth_mesh_light_testing:

Testing
=======

After programming the sample to your board, you can test it by using the Nordic
Semiconductor nRF Mesh app for Android or iOS.

Provisioning
------------

Before communicating with the mesh models, the device must be provisioned. The
provisioning assigns an address range to the device, and adds it to the mesh
network. Provisioning is done through the nRF Mesh app:

#. Press "Add node" to start scanning for unprovisioned mesh devices.
#. Press the "Mesh Light" device to connect to it.
#. Press "Identify", then "Provision" to provision the device.
#. When prompted, select an OOB method and follow the instructions in the app.
#. Once the provisioning and initial configuration is complete, the app will
   go back to the Network screen. Press the **Mesh Light** node in the list.
   You'll see basic information about your mesh node and its configuration.
#. In the Mesh node view, expand the first element. It contains the list of
   models in the first element of the node.
#. Press the "Generic OnOff Server" model to see its configuration.
#. Models must be bound to application keys to be open to communication. Do
   this by pressing "BIND KEY" at the top of the screen. Select "Application
   Key 1" from the list.
#. You should now be able to control the first LED on the device by using the
   "Generic On Off Controls" in the model view. Pressing "ON" should make the
   first LED on the DK light up.

Repeat steps 7-9 for each of the elements on the node to control each of the
four LEDs on the DK.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_mesh_onoff_srv_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* ``include/drivers/hwinfo.h``
* :ref:`zephyr:kernel`:

  * ``include/kernel.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``

* :ref:`zephyr:bluetooth_mesh`:

  * ``include/bluetooth/mesh.h``
