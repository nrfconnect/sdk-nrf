.. _bluetooth_mesh_thingy52_srv:

Bluetooth: Mesh Thingy:52 Server
################################

The Thingy:52 Server demo offers a bundle of different applications that utilizes Thingy:52's RGB LED and speaker to create games and artistic effects.


Overview
********

Paired up with the :ref:`bluetooth_mesh_thingy52_cli` we have the following usecases:

* RGB Propagation - Create a mesh network of Thingy:52 Server that propegates RGB colors in various patterns to create artistic effects.
* Reaction Game - Set up a relay of Thingy:52 Servers, where the goal is to knock over the device that is currently lit by an RGB message.

Paired up with the :ref:`bluetooth_mesh_thingy52_music_cli` we have the following usecase:

* Music Real Time Analyzer (RTA) - Use Thingy:52 Server(s) to act as an RGB LED RTA that reflects music played into the client device. If less than three server devices are used, not all default audio frequency ranges can be displayed.

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
   | Thingy:52 Server |
   +------------------+

The models are used for the following purposes:

* :ref:`bt_mesh_thingy52_mod_readme` instance to receive and relay RGB messages.
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
device and configured according to the :ref:`bluetooth_mesh_thingy52_srv_testing` guide.


User interface
**************

Large RGB LED:
   Show the RGB color of the currently active RGB message in the device.

Speaker:
   Makes sound if the currently active RGB message in the device has the speaker enabled flag raised.

Low Power accelerometer:
   Used to detect if orientation of the Thingy:52 device has changed. Cancels any active RGB messages if the
   use case is Reaction Game or RGB Propagation mode.

Building and running
********************

.. |sample path| replace:: :file:`demos/bluetooth/mesh/thingy52/thingy52_cli`

.. include:: /includes/build_and_run.txt

.. _bluetooth_mesh_thingy52_srv_testing:

Testing
=======

.. important::
   The Mesh server for Thingy:52 cannot demonstrate its functionality on its own, and needs at least one device
   with either the :ref:`bluetooth_mesh_thingy52_cli` demo, or the :ref:`bluetooth_mesh_thingy52_music_cli` running
   in the same mesh network.

After programming the demo to your Thingy:52, you can test it by using a smartphone with Nordic Semiconductor's nRF Mesh app installed.
Testing consists of provisioning the device and configuring it for communication with the mesh models.

.. _bluetooth_mesh_thingy52_srv_prov:

Provisioning the device
-----------------------

The provisioning assigns an address range to the device, and adds it to the mesh network.
Complete the following steps in the nRF Mesh app:

1. Tap :guilabel:`Add node` to start scanning for unprovisioned mesh devices.
#. Select the :guilabel:`Thingy:52 Server` device to connect to it.
#. Tap :guilabel:`Identify` and then :guilabel:`Provision` to provision the device.
#. When prompted, select an OOB method and follow the instructions in the app.

Once the provisioning is complete, the app returns to the Network screen.

.. _bluetooth_mesh_thingy52_srv_gen_conf_model:

General configuration of models
-------------------------------

Complete the following steps in the nRF Mesh app to configure models:

1. On the Network screen, tap the :guilabel:`Thingy:52 Server` node.
   Basic information about the mesh node and its configuration is displayed.
#. In the Mesh node view, expand the second element.
   It contains the list of models in the second element of the node.
#. Tap :guilabel:`Vendor Model` to see the model's configuration.
#. Bind the model to application keys to make it open for communication:

   1. Tap :guilabel:`BIND KEY` at the top of the screen.
   #. Select :guilabel:`Application Key 1` from the list.


.. _bluetooth_mesh_thingy52_srv_pub_sub:

Publication and Subscription configuration
------------------------------------------

.. note::
   There is no requirement that every single Thingy:52 Server device has publication or subscription configured.
   A Thingy:52 Server that has no publication configured will process every incoming RGB message like normal, and then
   the message is discarded.

**Publication**:

1. On the Network screen, tap the :guilabel:`Thingy:52 Server` node.
   Basic information about the mesh node and its configuration is displayed.
#. In the Mesh node view, expand the second element.
   It contains the list of models in the second element of the node.
#. Tap :guilabel:`Vendor Model` to see the model's configuration.
#. Tap :guilabel:`SET PUBLICATION`.
#. Tap :guilabel:`Publish Address`.
#. Choose one of the following publishing alternatives:

   a. For unicast:

      1. Select :guilabel:`Groups` from the drop-down menu.
      #. Select an existing group or create a new one.
      #. Tap :guilabel:`OK`.

   #. For Group address:

      1. Select :guilabel:`Unicast Address` from the drop-down menu.
      #. Set the Publish Address to the unicast address of a Thingy:52 Server model on another device.
      #. Tap :guilabel:`OK`.

#. Set the Retransmit Count to zero (:guilabel:`Disabled`) to prevent the model from sending each message multiple times.
#. Leave the rest of the publish parameters at their default values.
#. Tap :guilabel:`APPLY` to confirm the configuration.

**Subscription**:

1. On the Network screen, tap the :guilabel:`Thingy:52 Server` node.
   Basic information about the mesh node and its configuration is displayed.
#. In the Mesh node view, expand the second element.
   It contains the list of models in the second element of the node.
#. Tap :guilabel:`Vendor Model` to see the model's configuration.
#. Tap :guilabel:`SUBSCRIBE`.
#. Tap :guilabel:`Publish Address`.
#. Select :guilabel:`Groups` from the drop-down menu.
#. Select an existing group or create a new one.
#. Tap :guilabel:`OK` to confirm the configuration.

.. _bluetooth_mesh_thingy52_srv_reaction_game_test:

Reaction Game Testing
---------------------

.. note::
   This game only works when the speaker is off. This is because the accelerometer gives wrong reading when the speaker is on.

The following is an example of how the reaction game can be set up:

The user has four Thingy:52 devices. The first device is set up and configured with the :ref:`bluetooth_mesh_thingy52_cli`.

The three remaining device (Server A, B and C) are set up with the Thingy:52 Server Demo. After completing :ref:`bluetooth_mesh_thingy52_srv_prov`
and :ref:`bluetooth_mesh_thingy52_srv_gen_conf_model`
steps for all three Server devices, we set up the following publication scheme using the :ref:`bluetooth_mesh_thingy52_srv_pub_sub` (server)
and :ref:`bluetooth_mesh_thingy52_cli_config_models` (client) guides:

1. The Thingy:52 Client is set to publish messages to :guilabel:`Server A`.
#. :guilabel:`Server A` is set to publish messages to :guilabel:`Server B`.
#. :guilabel:`Server B` is set to publish messages to :guilabel:`Server C`.
#. :guilabel:`Server C` is set to publish messages back to :guilabel:`Server A`.

This creates a loop where the RGB message injected from the Thingy:52 Client will loop between Server A, B and C until the device currently
holding the RGB message is knocked over, or the RGB message TTL value expires. The TTL value in the :ref:`bluetooth_mesh_thingy52_cli`
is ``100 hops`` by default.

This setup can now be tested by injecting RGB messages from the :ref:`bluetooth_mesh_thingy52_cli` device.

.. _bluetooth_mesh_thingy52_srv_rgb_relay_test:

RGB Propagation Testing
-----------------------

The following is an example of how the RGB Propagation can be set up:

The user has four Thingy:52 devices. The first device is set up and configured with the :ref:`bluetooth_mesh_thingy52_cli`.

The three remaining device (Server A, B and C) are set up with the Thingy:52 Server Demo. After completing :ref:`bluetooth_mesh_thingy52_srv_prov`
and :ref:`bluetooth_mesh_thingy52_srv_gen_conf_model`
steps for all three Server devices, we set up the following publication scheme using the :ref:`bluetooth_mesh_thingy52_srv_pub_sub` (server)
and :ref:`bluetooth_mesh_thingy52_cli_config_models` (client) guides:

1. The Thingy:52 Client is set to publish messages to :guilabel:`Server A`.
#. We create :guilabel:`Group Address X`.
#. :guilabel:`Server A` is set to publish messages to :guilabel:`Group Address X`.
#. :guilabel:`Server B` is set to subscribe to :guilabel:`Group Address X`.
#. :guilabel:`Server C` is set to subscribe to :guilabel:`Group Address X`.

This creates a relay where the RGB message injected from the Thingy:52 Client will jump to Server A, then to Server B and C simoutaniously,
before disappearing.

This setup can now be tested by injecting RGB messages from the :ref:`bluetooth_mesh_thingy52_cli` device.

.. _bluetooth_mesh_thingy52_srv_music_test:

Music Real Time Analyzer (RTA)
------------------------------

The following is an example of how the music RTA can be set up:

The user has four Thingy:52 devices. One device is set up and configured with the :ref:`bluetooth_mesh_thingy52_music_cli`.

The three remaining devices (server A, B and C) are set up with the Thingy:52 Server Demo. Each server will represend one audio frequency range.
After completing :ref:`bluetooth_mesh_thingy52_srv_prov` and :ref:`bluetooth_mesh_thingy52_srv_gen_conf_model`
steps for all three Server devices, we set up the following publication scheme using the :ref:`bluetooth_mesh_thingy52_srv_pub_sub` (server)
and :ref:`bluetooth_mesh_thingy52_music_cli_config_models` (client) guides:

1. The client models are set to publish messages to three different group addresses by creating :guilabel:`Group A` :guilabel:`Group B` and :guilabel:`Group C`.
#. :guilabel:`Server A` is set to subscribe to :guilabel:`Group A`.
#. :guilabel:`Server B` is set to subscribe to :guilabel:`Group B`.
#. :guilabel:`Server C` is set to subscribe to :guilabel:`Group C`.

When the music client now detects audio in the different frequency ranges, RGB messages will be sent to the corresponding servers.

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
