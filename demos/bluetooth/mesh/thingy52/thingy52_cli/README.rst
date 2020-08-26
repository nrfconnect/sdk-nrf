.. _bluetooth_mesh_thingy52_cli:

Bluetooth: Mesh Thingy:52 Client
################################

The Thingy:52 client demo is meant to work together with the :ref:`bluetooth_mesh_thingy52_srv` demo. It is used to inject
RGB messages into a mesh network consisting of Thingy:52 Server nodes. The client supports message injection with a range
of different colors, variable message delay, and configurable speaker output.


Overview
********

The Thingy:52 client supports two ways of injecting messages into the mesh network:

* Manual - The user manually selects the color and delay of the message.
* Auto - When activated by the user the device will inject a message with a random color and delay
  every two seconds.

The delay contained in a message can be in the range from ``200 ms`` to ``2000 ms``. Both ways of
message injection can be configured to support speaker output.

Provisioning
============

Provisioning is handled by the :ref:`bt_mesh_dk_prov`.

Models
======

The following table shows the Thingy:52 client composition data for this demo:

.. table::
   :align: center

   +------------------+
   |   Element 1      |
   +==================+
   |  Config Server   |
   +------------------+
   |  Health Server   |
   +------------------+
   | Thingy:52 Client |
   +------------------+

The models are used for the following purposes:

* :ref:`bt_mesh_thingy52_mod_readme` instance to control RGB messages.
* Config Server allows configurator devices to configure the node remotely.
* Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
  These callbacks trigger blinking of the LEDs.


Requirements
************

This demo project is made for Thingy:52 and will only run on a Thingy:52 device.

The demo requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

  * `nRF Mesh mobile app for Android`_
  * `nRF Mesh mobile app for iOS`_

An additional requirement is the :ref:`bluetooth_mesh_thingy52_srv` demo programmed on a separate Thingy:52
device, and configured according to the :ref:`bluetooth_mesh_thingy52_srv_testing` guide.


User interface
**************

Lightwell RGB LED:
   Show the current manually selected RGB color for outgoing RGB messages. Also indicates RGB message delay by blinking
   frequency.

Small RGB LED:
   Show if speaker output for outgoing RGB messages is enabled

The user input interface of this demo is created by combining the functionality of the Thingy:52 button and the
integrated low power accelerometer. Together they create a button handler that can control different features
depending on the orientation of the Thingy:52 device.

Low Power accelerometer:
   Used to detect the orientation of the Thingy:52 device. Combined with the button it desides what iput action to
   execute.

Button:
   Executes input command related to the current orientation of the Thingy:52 device.

Thingy:52 orientations
======================

How the Thingy:52 is oriented decides the behaviour:

* Speaker orientation - Thingy:52 placed on the side, with Nordic logo facing towards you. USB port on the left side.
* Color orientation - Thingy:52 placed on the side, with Nordic logo facing towards you. USB port pointing up.
* Manual orientation - Thingy:52 laying flat, with Nordic logo facing the ground. Button pointing up.
* Auto orientation - Thingy:52 placed on the side, with Nordic logo facing towards you. USB port on the right side.

Color picking
=============

The color picking algorihm is made with Google color picker as reference:

https://www.google.com/search?q=color+picker

Slide the dot on the color line with the dot in the color window placed in the top right corner.

Building and running
********************

.. |sample path| replace:: :file:`demos/bluetooth/mesh/thingy52/thingy52_cli`

.. include:: /includes/build_and_run.txt

Testing
=======

.. note::
   The Bluetooth mesh client for Thingy:52 cannot fully demonstrate its functionality on its own,
   and needs at least one device with the :ref:`bluetooth_mesh_thingy52_srv` demo running in the same mesh network.

After programming the demo to your Thingy:52, you can test it by using a smartphone with Nordic Semiconductor's nRF Mesh app installed.
Testing consists of provisioning the device and configuring it for communication with the mesh models.

Provisioning the device
-----------------------

The provisioning assigns an address range to the device, and adds it to the mesh network.
Complete the following steps in the nRF Mesh app:

1. Tap :guilabel:`Add node` to start scanning for unprovisioned mesh devices.
#. Select the :guilabel:`Thingy:52 Client` device to connect to it.
#. Tap :guilabel:`Identify` and then :guilabel:`Provision` to provision the device.
#. When prompted, select an OOB method and follow the instructions in the app.

Once the provisioning is complete, the app returns to the Network screen.

.. _bluetooth_mesh_thingy52_cli_config_models:

Configuring models
------------------

Complete the following steps in the nRF Mesh app to configure models:

1. On the Network screen, tap the :guilabel:`Thingy:52 Client` node.
   Basic information about the mesh node and its configuration is displayed.
#. In the Mesh node view, expand the second element.
   It contains the list of models in the second element of the node.
#. Tap :guilabel:`Vendor Model` to see the model's configuration.
#. Bind the model to application keys to make it open for communication:

   1. Tap :guilabel:`BIND KEY` at the top of the screen.
   #. Select :guilabel:`Application Key 1` from the list.

#. Configure the Client model publish parameters, which define how the model will send its messages:

   1. Tap :guilabel:`SET PUBLICATION`.
   #. Set the Publish Address to the unicast address of a Thingy:52 Server model on another device.
   #. Set the Retransmit Count to zero (:guilabel:`Disabled`) to prevent the model from sending each message multiple times.
   #. Leave the rest of the publish parameters at their default values.
   #. Tap :guilabel:`APPLY` to confirm the configuration.

You are now be able to send RGB messages to the Thingy:52 server node.

Enable/disable speaker output
-----------------------------

1. Flip the Thingy:52 to the Speaker orientation.
#. Press the Thingy:52 button. Speaker output enabled is indicated by a small led on the front of the Thingy:52 emitting white light.

Inject message manually
-----------------------

1. Flip the Thingy:52 to the Color orientation.
#. Press and hold the Thingy:52 button. You should now see the color of the lightwell RGB LED change slowly. Release the button at the desired color.
#. Flip the Thingy:52 to the Manual orientation.
#. Press and hold the Thingy:52 button. You should now see the lightwell RGB LED blinking. The frequency of the blinking represents the delay that the outgoing message will contain.
   Release the button at the desired delay.
#. As soon as you release the button the message will be sent.

Inject message automatically
---------------------------

1. Flip the Thingy:52 to the Auto orientation.
#. Press the Thingy:52 button. The auto injection feature is now enabled.
#. Press the Thingy:52 button again to disable the auto injection feature.


Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_mesh_dk_prov`

It also uses the following custom libraries:

* :ref:`bt_mesh_thingy52_mod_readme`
* :ref:`bt_mesh_thingy52_orientation_handler`

In addition, it uses the following Zephyr libraries:

* ``include/drivers/hwinfo.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``

* :ref:`zephyr:bluetooth_mesh`:

  * ``include/bluetooth/mesh.h``
