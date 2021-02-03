.. _bluetooth_mesh_thingy52_cli:

Bluetooth: Mesh Thingy:52 client
################################

.. contents::
   :local:
   :depth: 2

The Thingy:52 client injects RGB messages into a mesh network of Thingy:52 device nodes.
The client supports message injection with a range of different colors, a variable message delay, and a configurable speaker output.

.. note::
   This demo must be paired with :ref:`bluetooth_mesh_thingy52_srv` to show any functionality.

Overview
********

The Thingy:52 client supports two ways of injecting messages into the mesh network:

* Manual - The user manually selects the color and the delay of the message.
* Auto - The device injects a message with a random color and delay every two seconds.

The delay contained in a message can be in the range from ``200 ms`` to ``2000 ms``.
Both ways of message injection can be configured to support speaker output.

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
   |  Thingy:52       |
   +------------------+

The models are used for the following purposes:

* :ref:`bt_mesh_thingy52_mod_readme` instance controls the RGB messages.
* Config Server allows configurator devices to configure the node remotely.
* Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
  These callbacks trigger blinking of the LEDs.


Requirements
************

This demo project is made for Thingy:52 and will only run on a Thingy:52 device.

The demo requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

  * `nRF Mesh mobile app for Android`_
  * `nRF Mesh mobile app for iOS`_

An additional requirement is the :ref:`bluetooth_mesh_thingy52_srv` demo programmed on a separate Thingy:52 device, and configured according to the :ref:`bluetooth_mesh_thingy52_srv_testing` guide.


User interface
**************

Lightwell RGB LED:
   Shows the current manually selected RGB color for outgoing RGB messages.
   Also indicates the RGB message delay by its blinking frequency.

Small RGB LED:
   Shows if the speaker output for outgoing RGB messages is enabled.

The user input interface of this demo is created by combining the functionality of the Thingy:52 button and the integrated low-power accelerometer.
Together they create a button handler that can control different features depending on the orientation of the Thingy:52 device.

Low-power accelerometer:
   Detects the orientation of the Thingy:52 device.
   Combined with the button, it decides what input command to execute.

Button:
   Executes the input command related to the current orientation of the Thingy:52 device.

Thingy:52 orientations
======================

The behavior depends on how the Thingy:52 is oriented:

* Speaker orientation - Thingy:52 placed on the side, with Nordic logo facing towards you and the USB port on the left side.
* Color orientation - Thingy:52 placed on the side, with Nordic logo facing towards you and the USB port pointing up.
* Manual orientation - Thingy:52 laying flat, with Nordic logo facing the ground and the button pointing up.
* Auto orientation - Thingy:52 placed on the side, with Nordic logo facing towards you and the USB port on the right side.

Color picking
=============

The color-picking algorithm is made with `Google color picker`_ as a reference.
Place the dot in the color window in the top right corner, and slide the dot on the color line to pick a color.

Building and running
********************

.. |sample path| replace:: :file:`demos/bluetooth/mesh/thingy52/thingy52_cli`

.. include:: /includes/build_and_run.txt

Testing
=======

.. note::
   The Bluetooth mesh client for Thingy:52 cannot fully demonstrate its functionality on its own, and needs at least one device with the :ref:`bluetooth_mesh_thingy52_srv` demo running in the same mesh network.

After programming the demo to your Thingy:52, you can test it by using a smartphone with Nordic Semiconductor's nRF Mesh app installed.
Testing consists of provisioning the device and configuring it for communication with the mesh models.

Provisioning the device
-----------------------

.. |device name| replace:: :guilabel:`Thingy:52 Client`

.. include:: /includes/mesh_device_provisioning.txt

.. _bluetooth_mesh_thingy52_cli_config_models:

Configuring models
------------------

See :ref:`ug_bt_mesh_model_config_app` for details on how to configure the mesh models with the nRF Mesh mobile app.

Configure the Vendor model in the second element of the :guilabel:`Thingy:52 Client` node:

* Bind the model to :guilabel:`Application Key 1`.
* Set the publication parameters:

  * Destination/publish address: Set the :guilabel:`Publish Address` to the unicast address of a Thingy:52 Server on another device.
  * Retransmit count: Set the count to zero (:guilabel:`Disabled`), to prevent the model from sending each message multiple times.

You can now send RGB messages to the Thingy:52 Server node.

Enabling/disabling speaker output
---------------------------------

1. Position Thingy:52 in the Speaker orientation.
#. Press the Thingy:52 button.
   A small LED on the front side of Thingy:52 emitting the white light is indicating that the speaker output is enabled.

Injecting messages manually
---------------------------

1. Position Thingy:52 in the Color orientation.
#. Press and hold the Thingy:52 button.

   You should now see the color of the Lightwell RGB LED change slowly.
#. Release the button at the desired color.
#. Position Thingy:52 in the Manual orientation.
#. Press and hold the Thingy:52 button.

   You should now see the Lightwell RGB LED blinking.
   The frequency of the blinking represents the delay that the outgoing message will contain.
#. Release the button at the desired delay.

As soon as you release the button, the message is sent.

Injecting messages automatically
--------------------------------

1. Position Thingy:52 in the Auto orientation.
#. Press the Thingy:52 button.

   The auto-injection feature is now enabled.
#. Press the Thingy:52 button again to disable the auto-injection feature.


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
