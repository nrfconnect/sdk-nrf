.. _bt_mesh_chat_description:

Sample description
##################

.. contents::
   :local:
   :depth: 2

The Bluetooth® Mesh chat sample demonstrates how the mesh network can be used to facilitate communication between nodes by text, using the :ref:`bt_mesh_chat_client_model`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

* `nRF Mesh mobile app for Android`_
* `nRF Mesh mobile app for iOS`_

Overview
********

By means of the mesh network, the clients as mesh nodes can communicate with each other without the need of a server.
The mesh chat sample is mainly designed for group communication, but it also supports one-on-one communication, as well as sharing the nodes presence.

This sample is used in :ref:`ug_bt_mesh_vendor_model` as an example of how to implement a vendor model for the Bluetooth Mesh in |NCS|.

The clients are nodes with a provisionee role in a mesh network.
Provisioning is performed using the `nRF Mesh mobile app`_.
This mobile application is also used to configure key bindings, and publication and subscription settings of the Bluetooth Mesh model instances in the sample.
After provisioning and configuring the mesh models supported by the sample in the `nRF Mesh mobile app`_, you can communicate with other mesh nodes by sending text messages and obtaining their presence using the :ref:`shell module <shell_api>`.

Provisioning
============

The provisioning is handled by the :ref:`bt_mesh_dk_prov`.
It supports four types of out-of-band (OOB) authentication methods, and uses the Hardware Information driver to generate a deterministic UUID to uniquely represent the device.

Models
======

The following table shows the Bluetooth Mesh chat composition data for this sample:

   +---------------+
   |  Element 1    |
   +===============+
   | Config Server |
   +---------------+
   | Health Server |
   +---------------+
   | Chat Client   |
   +---------------+

The models are used for the following purposes:

* The :ref:`bt_mesh_chat_client_model` instance in the first element is used to communicate with the other Chat Client models instantiated on the other mesh nodes.
* Config Server allows configurator devices to configure the node remotely.
* Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
  These callbacks trigger blinking of the LEDs.

The model handling is implemented in :file:`src/model_handler.c`.

User interface
**************

Buttons:
   Can be used to input the OOB authentication value during provisioning.
   All buttons have the same functionality during this procedure.

LEDs:
   Show the OOB authentication value during provisioning if the "Push button" OOB method is used.

Terminal emulator:
   Used for the interaction with the sample.

Configuration
*************

|config|

Source file setup
=================

This sample is split into the following source files:

* A :file:`main.c` file to handle initialization.
* A file for handling the Chat Client model, :file:`chat_cli.c`.
* A file for handling Bluetooth Mesh models and communication with the :ref:`shell module <shell_api>`, :file:`model_handler.c`.

FEM support
===========

.. include:: /includes/sample_fem_support.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/mesh/chat`

.. include:: /includes/build_and_run.txt

.. _bluetooth_mesh_chat_testing:

Testing
=======

After programming the sample to your development kit, you can test it by using a smartphone with `nRF Mesh mobile app`_ installed.
Testing consists of provisioning the device and configuring it for communication with other nodes.

After configuring the device, you can interact with the sample using the terminal emulator.

Provisioning the device
-----------------------

.. |device name| replace:: :guilabel:`Mesh Chat`

.. include:: /includes/mesh_device_provisioning.txt

Configuring models
------------------

See :ref:`ug_bt_mesh_model_config_app` for details on how to configure the mesh models with the nRF Mesh mobile app.

Create a new group and name it *Chat Channel*, then configure the Vendor model on the **Mesh Chat** node:

* Bind the model to **Application Key 1**.
* Set the publication parameters:

  * Destination/publish address: Select the created group **Chat Channel**.
  * Publication interval: Set the interval to recommended value of 10 seconds.
  * Retransmit count: Change the count as preferred.

* Set the subscription parameters: Select the created group **Chat Channel**.

Interacting with the sample
---------------------------

1. Connect the development kit to the computer using a USB cable.
   The development kit is assigned a serial port.
   |serial_port_number_list|
#. |connect_terminal_specific_ANSI|
#. Enable local echo in the terminal to see the text you are typing.

After completing the steps above, a command can be sent to the sample.
The sample supports the following commands:

chat \-\-help
   Prints help message together with the list of supported commands.

chat presence set <presence>
   Sets presence of the current client.
   The following values are supported: available, away, dnd, inactive.

chat presence get <node>
   Gets presence of a specified chat client.

chat private <node> <message>
   Sends a private text message to a specified chat client.
   Remember to wrap the message in double quotes if it has 2 or more words.

chat msg <message>
   Sends a text message to the chat.
   Remember to wrap the message in double quotes if it has 2 or more words.

Whenever the node changes its presence, or the local node receives another model's presence the first time, you will see the following message:

.. code-block:: none

   <0x0002> is now available

When the model receives a message from another node, together with the message you will see the address of the element of the node that sent the message:

.. code-block:: none

   <0x0002>: Hi there!

The messages posted by the local node will have ``<you>`` instead of the address of the element:

.. code-block:: none

   <you>: Hello, 0x0002!
   <you> are now away

Private messages can be identified by the address of the element of the node that posted the message (enclosed in asterisks):

.. code-block:: none

   <you>: *0x0004* See you!
   <0x0004>: *you* Bye!

When the reply is received, you will see the following:

.. code-block:: none

   <0x0004> received the message

Note that private messages are only seen by those the messages are addressed to.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_mesh_dk_prov`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :ref:`zephyr:shell_api`:

  * :file:`include/shell.h`
  * :file:`include/shell_uart.h`

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`

* :ref:`zephyr:bluetooth_mesh`:

  * :file:`include/bluetooth/mesh.h`
