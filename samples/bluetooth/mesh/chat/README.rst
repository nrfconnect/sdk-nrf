.. _bt_mesh_chat:

Bluetooth: Mesh chat
####################

.. contents::
   :local:
   :depth: 2

The Bluetooth mesh chat sample demonstrates how the mesh network can be used to facilitate communication between nodes by text, using the :ref:`bt_mesh_chat_client_model`.
By means of the mesh network, the clients as mesh nodes can communicate with each other without the need of a server.
The sample is mainly designed for group communication, but it also supports one-on-one communication, as well as sharing the nodes presence.

This sample is used in :ref:`ug_bt_mesh_vendor_model` as an example of how to implement a vendor model for the Bluetooth mesh in |NCS|.

.. toctree::
   :maxdepth: 1
   :glob:
   :caption: Subpages:

   chat_cli.rst

Overview
********

This sample is split into four source files:

* A :file:`main.c` file to handle initialization.
* A file for handling the Chat Client model, :file:`chat_cli.c`.
* A file for handling Bluetooth mesh models and communication with the :ref:`shell module <shell_api>`, :file:`model_handler.c`.

After provisioning and configuring the Bluetooth mesh models supported by the sample in the `nRF Mesh mobile app`_, you can communicate with other mesh nodes by sending text messages and obtaining their presence using the :ref:`shell module <shell_api>`.

Provisioning
============

The provisioning is handled by the :ref:`bt_mesh_dk_prov`.

Models
======

The following table shows the Bluetooth mesh chat composition data for this sample:

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

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :sample-yaml-rows:

The sample also requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

* `nRF Mesh mobile app for Android`_
* `nRF Mesh mobile app for iOS`_

User interface
**************

Buttons:
   Can be used to input the OOB authentication value during provisioning.
   All buttons have the same functionality during this procedure.

LEDs:
   Show the OOB authentication value during provisioning if the "Push button" OOB method is used.

Terminal emulator:
   Used for the interraction with the sample.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/mesh/chat`

.. include:: /includes/build_and_run.txt

.. _bluetooth_mesh_chat_testing:

Testing
=======

After programming the sample to your development kit, you can test it by using a smartphone with Nordic Semiconductor's nRF Mesh app installed.
Testing consists of provisioning the device and configuring it for communication with other nodes.

After configuring the device, you can interact with the sample using the terminal emulator.

Provisioning the device
-----------------------

.. |device name| replace:: :guilabel:`Mesh Chat`

.. include:: /includes/mesh_device_provisioning.txt

Configuring models
------------------

See :ref:`ug_bt_mesh_model_config_app` for details on how to configure the mesh models with the nRF Mesh mobile app.

Create a new group and name it *Chat Channel*, then configure the Vendor model on the :guilabel:`Mesh Chat` node:

* Bind the model to :guilabel:`Application Key 1`.
* Set the publication parameters:

  * Destination/publish address: Select just created group :guilabel:`Chat Channel`.
  * Publication interval: Set the interval to recommended value of 10 seconds.
  * Retransmit count: Change the count as preferred.

* Set the subscription parameters: Select just created group :guilabel:`Chat Channel`.

Make sure to configure the parameters for each mesh node in the mesh network.

Interacting with the sample
---------------------------

1. Connect the kit to the computer using a USB cable.
   The kit is assigned a COM port (Windows), ttyACM device (Linux) or tty.usbmodem (MacOS).
#. |connect_terminal_specific|
#. Enable local echo in the terminal to see the text you are typing.

After completing the steps above, a command can be sent to the sample.
The sample supports the following commands:

chat --help
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

  * ``include/kernel.h``

* :ref:`zephyr:shell_api`:

  * ``include/shell.h``
  * ``include/shell_uart.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``

* :ref:`zephyr:bluetooth_mesh`:

  * ``include/bluetooth/mesh.h``
