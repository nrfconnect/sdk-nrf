.. _mesh_architecture:

Bluetooth Mesh stack architecture
#################################

.. contents::
   :local:
   :depth: 2

The Bluetooth® Mesh stack in |NCS| is an extension of the Zephyr Bluetooth Mesh stack.
The Zephyr Bluetooth Mesh stack implements the Bluetooth Mesh profile specification (see :ref:`zephyr:bluetooth_mesh`), while |NCS| provides additional model implementations from the Bluetooth Mesh model specification on top of the :ref:`zephyr:bluetooth_mesh_access` API.

.. figure:: images/bt_mesh_basic_architecture.svg
   :alt: Graphical representation of basic mesh stack architecture

   Basic architecture of the mesh stack

The mesh stack's structure corresponds to the structure of the Bluetooth Mesh specification and follows the same naming conventions:

* Bluetooth Mesh components in |NCS|

  * :ref:`mesh_architecture_models` - The Bluetooth Mesh models present and implement device behavior.

* Bluetooth Mesh components in Zephyr

  * :ref:`mesh_architecture_access` - The Bluetooth Mesh access layer organizes models and communication.
  * :ref:`Core <mesh_architecture_core>` - The core Bluetooth Mesh layer takes care of encryption and message relaying.
  * :ref:`mesh_architecture_provisioning` - The Bluetooth Mesh provisioning protocol is used for adding devices to the network.

See the official `Bluetooth Mesh glossary`_ for definitions of the most important Bluetooth Mesh-related terms used in this documentation.
You can also read about :ref:`basic Bluetooth Mesh concepts <mesh_concepts>` for a concise introduction to the Bluetooth Mesh.

.. _mesh_architecture_flow:

Bluetooth Mesh network data flow
********************************

The following figure demonstrates how the data packets flow between mesh nodes and their layers within the mesh stack structure.

.. figure:: images/bt_mesh_data_packet_flow.svg
   :alt: Basic data flow within a mesh network in the |NCS| Bluetooth Mesh

   Basic data flow within a mesh network in the |NCS| Bluetooth Mesh

As an example, consider a Bluetooth Mesh network with three devices/mesh network nodes and the process that takes place on each node when the light switch on the source node is pressed, as discussed in the following sections.

Source node with a light switch
===============================

The following process takes place at this stage:

  1. The application calls the light switch model's publish function.
  #. The model includes an on/off message in a publishing packet with an opcode and sends it to the access layer.
  #. The access layer fetches the necessary publish parameters, like destination address, encryption keys and time to live value (TTL value), and passes the packet to the transport layer, the highest of the core layers.
  #. The transport layer then encrypts the message with the selected application key, and splits the message into segments if necessary.
     Each segment is passed to the network layer, which attaches a network header with a sequence number and encrypts the packet with the network key before passing it to the Bluetooth LE Controller.
  #. The Bluetooth LE Controller includes the network message in an advertisement packet, and schedules a time slot for the packet to be broadcasted.

Relay node
==========

The following process takes place at this stage:

  1. The broadcast is received by all mesh nodes within range, and is passed from their bearer layers to their network layers.
  #. The network packet is decrypted, and if the receiving node is not its destination, the packet's TTL value is decreased by one, before being re-encrypted with the same network key and passed back to the Bluetooth LE Controller to be relayed.

Destination node with a light bulb
==================================

The following process takes place at this stage:

  1. Once the packet is relayed to the destination light bulb node, its network layer will decrypt the packet and pass it to the transport layer.
  #. Once all transport layer segments are received in this manner, the assembled message is decrypted with an application key, and passed on to the access layer.
  #. The access layer checks the opcode, application key and destination address, and passes the message to all eligible models.
  #. If one of these models is a light bulb model, the model parses the contents of the message, and notifies the application to turn the light bulb on or off.

The light bulb model may respond to acknowledge the transmission, following the same procedure back to the light switch node, which can notify the application that the on/off message was received.

.. _mesh_architecture_models:

Models
******

The models define the behavior and communication formats of all data that is transmitted across the mesh.
Equivalent to Bluetooth LE's GATT services, the Bluetooth Mesh models are independent, immutable implementations of specific behaviors or services.
All mesh communication happens through models, and any application that exposes its behavior through the mesh must channel the communication through one or more models.

The Bluetooth Mesh specification defines a set of immutable models for typical usage scenarios, but vendors are also free to implement their own models.

You can read more about the Bluetooth Mesh models in |NCS| in :ref:`bt_mesh_models`.

.. _mesh_architecture_access:

Access
******

The access layer controls the device's model composition.
It holds references to:

* Models that are present on the device
* Messages these models accept
* Configuration of these models

As the device receives mesh messages, the access layer finds which models the messages are for and forwards them to the model implementations.
The access layer is implemented in Zephyr.
For more information about the access layer, see :ref:`zephyr:bluetooth_mesh_access`.

.. _mesh_architecture_core:

Bluetooth Mesh core
*******************

Consisting of a network and a transport layer, the Bluetooth Mesh core module provides the mesh-specific transport for the messages.

The transport layer provides in-network security by encrypting mesh packets with *application keys*, and splitting them into smaller segments that can go on air.
The transport layer re-assembles incoming packet segments and presents the full mesh message to the access layer.

The network layer encrypts each transport layer packet segment with a *network key*, and populates the source and destination address fields.
When receiving a mesh packet, the network layer decrypts the message, inspects the source and destination addresses, and decides whether the packet is intended for this device and whether the network layer should relay it.

The Bluetooth Mesh core provides protection against malicious behavior and attacks against the mesh network through two-layer encryption, replay protection, and packet header obfuscation.
The Bluetooth Mesh core is implemented in Zephyr.
Read more about the Bluetooth Mesh core API in :ref:`zephyr:bluetooth_mesh_core`.

.. _mesh_architecture_provisioning:

Provisioning
************

Provisioning is the act of adding a device to a mesh network.
The Provisioning module takes care of both sides of this process, by implementing a provisioner role (the network owner) and a provisionee role (the device to add).

The mesh stack supports provisioning of a device directly through the PB-ADV/PB-GATT provisioning bearer, which can only happen between a provisioner and a provisionee that are within radio range of each other.
The Bluetooth Mesh provisioning protocol is implemented in Zephyr.
For more information about the provisioning process and the API, see :ref:`zephyr:bluetooth_mesh_provisioning`.
