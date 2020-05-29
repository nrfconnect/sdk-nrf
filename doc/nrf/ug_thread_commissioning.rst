.. _thread_ot_commissioning:

OpenThread commissioning
########################

Commissioning is the process that allows a new Thread device to join a Thread network.
The new device must be authenticated and authorized to become part of the network.

In the commissioning process, devices have different :ref:`thread_ot_commissioning_roles`.
The whole process can happen within the Thread network (connectivity within the Thread network) or can involve devices that are outside the network (other network provides connectivity, for example Ethernet or Wi-Fi).
This is the main difference between :ref:`thread_ot_commissioning_types_on-mesh` and :ref:`thread_ot_commissioning_types_external`, respectively.
The whole commissioning process benefits from :ref:`several security measures <thread_ot_commissioning_roles_authentication>`, especially the DTLS protocol.

This page describes the following commissioning topics:

.. contents::
    :local:
    :depth: 2

Once familiar with these concepts, read the following page:

.. toctree::
   :maxdepth: 1

   ug_thread_commissioning_configuring.rst

.. _thread_ot_commissioning_roles:

Commissioning roles
*******************

During the commissioning process, the devices involved are assigned one of the following roles:

* Leader - Watches over the network configuration, allows Commissioner Candidate to become Commissioner, and ensures there is only one active Commissioner.
* Commissioner Candidate - Role for a device capable of becoming the Commissioner, but has not yet been assigned the role of Commissioner.
* Commissioner - Authenticates the Joiner and provides it with network credentials required to join the network.
  The Commissioner can be part of the network or it can be outside the network (see :ref:`thread_ot_commissioning_types`).
* Joiner - Role for a new device that wants to join the Thread network.
  It exchanges messages with the Commissioner through the directly connected Joiner Router that serves as a proxy.
* Joiner Router - Role for a router device that is one hop away from the Joiner device in the Thread network and is the sole device connected with the Joiner.
  Responds to the Discovery Request of the Joiner.
  Moreover, when chosen by the Joiner, it passes subsequent communication in a secure manner.
* Border Router - Role for a device equipped with at least two interfaces (for example Thread and Wi-Fi, Ethernet, LTE etc.) that forwards data between a Thread network and a non-Thread network.
  It can also be an interface for the Commissioner.

  * Border Agent function, usually combined with the Border Router role, accepts :ref:`petitions <thread_ot_commissioning_phases>` from the Commissioner candidate.
    It also relays commissioning messages between Thread Network and a Commissioner.
    If a Commissioner is a part of the network, the same node is both the Commissioner and the Border Agent.

  .. note::
        While the Border Router is typically used only in the external commissioning process, the Nordic Border Router, which is based on the OpenThread Border Router (OTBR), supports both on-mesh and external commissioning.

Once a device is assigned one of the roles in the network, it can combine it with other roles.
An exception is the Joiner role, which is exclusive to the Joiner and cannot be combined with other roles.
For example:

* Border Router can also have the Joiner Router role in :ref:`thread_ot_commissioning_types_external`.
* Joiner Router can also have the Commissioner role in :ref:`thread_ot_commissioning_types_on-mesh`.

For details about scenarios that include devices with multiple roles, see `Thread Group's Commissioning White Paper`_.

See :ref:`thread_ot_commissioning_cli` for the list of commands used to assign the commissioning roles.

.. _thread_ot_commissioning_types:

On-mesh and external commissioning
**********************************

The commissioning in OpenThread can be either on-mesh or external.
Native commissioning is not supported by OpenThread.

.. _thread_ot_commissioning_types_on-mesh:

On-mesh Thread commissioning
============================

In the on-mesh Thread commissioning, the commissioning takes place inside the Thread network.
The Thread Leader approves a Commissioner connected either to the Thread network (on-mesh Commissioner) or to a Thread device, and accepts it into the Thread network.
Border Agent then authenticates it.
After authentication, the Commissioner instructs the Joiner Router to transfer Thread network credentials to the Joiner.

In this type of commissioning, Thread network credentials are transferred between devices over the radio.
At the end of its own authentication process, the Joiner :ref:`joins <thread_ot_commissioning_phases_joining>` the Thread network and becomes an active device that communicates with other Thread devices.

For security purposes, the on-mesh Thread commissioning requires exchanging a DTLS handshake between Commissioner and Joiner.
See :ref:`thread_ot_commissioning_roles_authentication` for more information.

.. figure:: /images/Thread_on-mesh_commissioning.svg
   :alt: On-mesh Thread commissioning

   On-mesh Thread commissioning

For information about how to configure on-mesh Thread Commissioning, see :ref:`thread_ot_commissioning_configuring_on-mesh`.

.. _thread_ot_commissioning_types_external:

External Thread commissioning
=============================

In the external Thread commissioning, the commissioning involves a Commissioner device connected to a network other than the Thread network, like Wi-Fi or Ethernet.
This external Commissioner (for example, a mobile phone) commissions new devices onto the network using the Thread Border Router as forwarding interface.

For security purposes, the external Thread commissioning requires exchanging a DTLS handshake.
The following DTLS sessions are established:

* Between Commissioner and Border Agent
* Between Commissioner and Joiner

See :ref:`thread_ot_commissioning_roles_authentication` for related information.

.. figure:: /images/Thread_external_commissioning.svg
   :alt: External Thread commissioning

   External Thread commissioning

.. _thread_ot_commissioning_phases:

Commissioning phases
********************

The commissioning process has the following phases:

.. contents::
    :local:
    :depth: 1

.. _thread_ot_commissioning_phases_petitioning:

Petitioning
===========

Petitioning concerns the Commissioner role.

Petitioning occurs in both commissioning scenarios. The Commissioner Candidate that is either connected to an external network (external candidate) or is part of the network (on-mesh candidate) must petition the Leader of the Thread network through the Thread Border Agent to become the only authorized Commissioner.

The petitioning involves up to two phases:

* In the external commissioning scenario, the potential commissioner exchanges a DTLS authentication handshake with the Thread Border Router to prove its eligibility and set up a secure connection.
* In both scenarios, the potential commissioner sends a petition to the Thread Leader through the Thread Border Router.

The Leader accepts the petition based on only one criterium: whether there is already an active Commissioner in the Thread network.
If there is none, the petition is accepted.
If the petition is rejected, a rejection message is sent with the ID of the active Commissioner.

After the petition is accepted by the Leader:

* In the external commissioning scenario, the connection is established and all subsequent communication between the Commissioner and other Thread devices is done through the Border Agent.
* In both scenarios, the new Commissioner becomes the only authorized Commissioner.
* In both scenarios, a periodic message is sent to keep the secure commissioning session open.

.. _thread_ot_commissioning_phases_joining:

Joining
=======

Joining concerns the Joiner role.

Joining occurs in both commissioning scenarios.
It involves the following phases:

* The Joiner, that is a potential new device in the Thread network, sends a Discovery Request message on every channel.
* The Joiner Router receives the message and answers with the Discovery Response message.
  This message contains network identifiers and Steering Data in the payload.
* The Joiner uses the information received from the Joiner Router to discover the correct network to connect to.

After the Joiner received the payload from the Joiner Router:

* The connection is established.
* The secure communication session continues, with a periodic message sent to keep it open.

.. _thread_ot_commissioning_roles_authentication:

Security, authentication, and credentials
*****************************************

To avoid a situation in which rogue devices join the Thread network, the communication between Commissioner and Joiner (in both scenarios), and Commissioner and Border Agent (in external commissioning) is secured with the Datagram Transport Layer Security (DTLS) authentication protocol session.
The session is established automatically.

Also the communication between Joiner and Joiner Router is secured, but only when Joiner Router sends network credentials to Joiner using one time key generated by the Commissioner.

During commissioning, the on-mesh Thread Commissioner possesses the network master key by default, while the external Thread Commissioner never gains possession of the network master key.

The commissioning uses the following passwords and credentials:

* Commissioning Credential - Passphrase known by the Leader and shared with the Commissioner of the network.

  .. note::
        The Commissioning Credential has 6 bytes minimum and 255 bytes maximum and is composed in the UTF-8 format, without character exclusions.

* Commissioning Key (PSKc) - Pre-Shared Key for the Commissioner based on the Commissioning Credential which is used to establish the Commissioner Session between the Commissioner and Border Agent.
  All devices in the Thread network store the PSKc.
* Joining Device Credential (PSKd) - Passphrase for authenticating a new Joiner device, used to establish a secure session between the Commissioner and the Joiner.
  When encoded in binary, this passphrase is referred to as Pre-Shared Key for the Device.

  .. note::
        The Joining Device Credential is composed of at least 6 and no more than 32 uppercase alphanumeric ASCII characters (base32-thread, 0 to 9 and A to Y, with the exclusion of I, O, Q, and Z).

For details and full overview of security credentials, see `Thread protocol specification`_, table 8.2.

----

.. _thread_ot_commissioning_cli:

Commissioning CLI commands
**************************

See the following pages in the `OpenThread CLI Reference`_ on GitHub for an overview of available CLI commands:

* `Commissioner CLI commands`_
* `Joiner CLI commands`_

----

Copyright disclaimer
    |Google_CCLicense|
