.. _bluetooth_mesh_thingy52_srv:

Bluetooth: Mesh Thingy:52 server
################################

.. contents::
   :local:
   :depth: 2

The Thingy:52 server demo offers a bundle of different applications utilizing Thingy:52's RGB LED and speaker to create games and artistic effects.

Overview
********

When paired up with the :ref:`bluetooth_mesh_thingy52_cli`, it offers the following use cases:

* RGB propagation - Create a mesh network of Thingy:52 servers that propagates RGB colors in various patterns to create artistic effects.
* Reaction game - Set up a relay of Thingy:52 servers, where the goal is to knock over the device that is currently lit by an RGB message.

When paired up with the :ref:`bluetooth_mesh_thingy52_music_cli`, it offers the following use case:

* Music real-time analyzer (RTA) - Use Thingy:52 server(s) to act as an RGB LED RTA that reflects music played into the client device.
  If less than three server devices are used, not all default audio frequency ranges can be displayed.

Provisioning
============

Provisioning is handled by the :ref:`bt_mesh_dk_prov`.

Models
======

The following table shows the Thingy:52 server composition data for this demo:

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

* :ref:`bt_mesh_thingy52_mod_readme` instance receives and relays the RGB messages.
* Config Server allows configurator devices to configure the node remotely.
* Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
  These callbacks trigger blinking of the LEDs.

Requirements
************

This demo project is made for Thingy:52 and will only run on a Thingy:52 device.

The demo requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

  * `nRF Mesh mobile app for Android`_
  * `nRF Mesh mobile app for iOS`_

An additional requirement is the :ref:`bluetooth_mesh_thingy52_srv` demo programmed on a separate Thingy:52 device and configured according to the :ref:`bluetooth_mesh_thingy52_srv_testing` guide.

User interface
**************

Large RGB LED:
   Shows the RGB color of the currently active RGB message in the device.

Speaker:
   Makes sound if the currently active RGB message in the device has the speaker enabled flag raised.

Low-power accelerometer:
   Detects the change in the orientation of the Thingy:52 device.
   Cancels any active RGB messages if the use case is reaction game or RGB propagation.

Building and running
********************

.. |sample path| replace:: :file:`demos/bluetooth/mesh/thingy52/thingy52_cli`

.. include:: /includes/build_and_run.txt

.. _bluetooth_mesh_thingy52_srv_testing:

Testing
=======

.. note::
   The Bluetooth mesh server for Thingy:52 cannot demonstrate its functionality on its own, and needs at least one device with either the :ref:`bluetooth_mesh_thingy52_cli` or the :ref:`bluetooth_mesh_thingy52_music_cli` demo running in the same mesh network.

After programming the demo to your Thingy:52, you can test it by using a smartphone with Nordic Semiconductor's nRF Mesh app installed.
Testing consists of provisioning the device and configuring it for communication with the mesh models.

.. _bluetooth_mesh_thingy52_srv_prov:

Provisioning the device
-----------------------

.. |device name| replace:: :guilabel:`Thingy:52 Server`

.. include:: /includes/mesh_device_provisioning.txt

.. _bluetooth_mesh_thingy52_srv_config_models:

Configuring models
------------------

See :ref:`ug_bt_mesh_model_config_app` for details on how to configure the mesh models with the nRF Mesh mobile app.

Configure the Vendor model in the second element of the :guilabel:`Thingy:52 Server` node:

* Bind the model to :guilabel:`Application Key 1`.
* Set the publication parameters:

  * Destination/publish address:
    For unicast, select an existing group or create a new one.
	For group address, set the :guilabel:`Publish Address` to the unicast address of a Thingy:52 Server on another device.

    .. note::
       A group represents one frequency range.
	   Servers displaying this range must subscribe to the same group.

  * Retransmit count: Set the count to zero (:guilabel:`Disabled`), to prevent the model from sending each message multiple times.
* Set the subscription parameters: Select an existing group or create a new one.

.. note::
   There is no requirement that every single Thingy:52 Server device should have publication or subscription parameters configured.
   A Thingy:52 server node with no publication configured will process all incoming RGB messages before discarding them.

.. _bluetooth_mesh_thingy52_srv_reaction_game_test:

Reaction game testing
---------------------

.. note::
   The reaction game only works when the speaker is off.
   When the speaker is on, the accelerometer gives wrong reading.

The following is an example of how the reaction game can be set up.

The user has four Thingy:52 devices.
The first device is set up and configured with the :ref:`bluetooth_mesh_thingy52_cli` demo.
The three remaining devices (servers A, B and C) are set up with the Thingy:52 server demo.

After provisioning and binding the application key to all three Thingy:52 server devices, set up the following publication scheme:

1. Set :guilabel:`Thingy:52 Client` to publish messages to :guilabel:`Server A`.
#. Set :guilabel:`Server A` to publish messages to :guilabel:`Server B`.
#. Set :guilabel:`Server B` to publish messages to :guilabel:`Server C`.
#. Set :guilabel:`Server C` to publish messages back to :guilabel:`Server A`.

See sections :ref:`bluetooth_mesh_thingy52_srv_config_models` (for servers) and :ref:`bluetooth_mesh_thingy52_cli_config_models` (for client) for more information about the configuration.

The above publication scheme creates a loop where the RGB message, injected from the Thingy:52 client, loops between servers A, B and C until the device currently holding the RGB message is knocked over, or the TTL value of the RGB message expires.
The TTL value in the :ref:`bluetooth_mesh_thingy52_cli` is ``100 hops`` by default.

This setup can now be tested by injecting RGB messages from the :ref:`bluetooth_mesh_thingy52_cli` device.

.. _bluetooth_mesh_thingy52_srv_rgb_relay_test:

RGB propagation testing
-----------------------

The following is an example of how the RGB propagation can be set up.

The user has four Thingy:52 devices.
The first device is set up and configured with the :ref:`bluetooth_mesh_thingy52_cli` demo.
The three remaining devices (servers A, B and C) are set up with the Thingy:52 server demo.

After provisioning and binding the application key to all three Thingy:52 server devices, set up the following publication/subscription scheme:

1. Set :guilabel:`Thingy:52 Client` to publish messages to :guilabel:`Server A`.
#. Create a new group :guilabel:`Group Address X` (see :ref:`ug_bt_mesh_model_config_app` for details).
#. Set :guilabel:`Server A` to publish messages to just created group :guilabel:`Group Address X`.
#. Set :guilabel:`Server B` to subscribe to :guilabel:`Group Address X`.
#. Set :guilabel:`Server C` to subscribe to :guilabel:`Group Address X`.

See sections :ref:`bluetooth_mesh_thingy52_srv_config_models` (for servers) and :ref:`bluetooth_mesh_thingy52_cli_config_models` (for client) for more information about the configuration.

The above publication/subscription scheme creates a relay where the RGB message injected from the Thingy:52 client jumps to server A, and then to Server B and C simultaneously, before disappearing.

This setup can now be tested by injecting RGB messages from the :ref:`bluetooth_mesh_thingy52_cli` device.

.. _bluetooth_mesh_thingy52_srv_music_test:

Music real-time analyzer (RTA)
------------------------------

The following is an example of how the music RTA can be set up.

The user has four Thingy:52 devices.
One device is set up and configured with the :ref:`bluetooth_mesh_thingy52_music_cli` demo.
The three remaining devices (server A, B and C) are set up with the Thingy:52 server demo.
Each server represents one audio frequency range.

After provisioning and binding the application key to all three Thingy:53 server devices, set up the following publication/subscription scheme:

1. The client models are set to publish messages to three different group addresses by creating groups :guilabel:`Group A`, :guilabel:`Group B`, and :guilabel:`Group C`.
#. Set :guilabel:`Server A` to subscribe to :guilabel:`Group A`.
#. Set :guilabel:`Server B` to subscribe to :guilabel:`Group B`.
#. Set :guilabel:`Server C` to subscribe to :guilabel:`Group C`.

See sections :ref:`bluetooth_mesh_thingy52_srv_config_models` (for servers) and :ref:`bluetooth_mesh_thingy52_cli_config_models` (for client) for more information about the configuration.

When the music client detects audio in the different frequency ranges, the RGB messages are sent to the corresponding servers.
This setup can now be tested by activatiing the microphone (pressing the button) on the :ref:`bluetooth_mesh_thingy52_music_cli` device.

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
