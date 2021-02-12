.. _thread_ot_commissioning:

OpenThread commissioning
########################

.. contents::
   :local:
   :depth: 2

Commissioning is the process that allows a new Thread device to join a Thread network.
The new device must be authenticated and authorized to become part of the network.

In the commissioning process, devices have different :ref:`thread_ot_commissioning_roles`.
The whole process can happen within the Thread network (connectivity within the Thread network) or can involve devices that are outside the network (other network provides connectivity, for example Ethernet or Wi-Fi).
This is the main difference between :ref:`thread_ot_commissioning_types_on-mesh` and :ref:`thread_ot_commissioning_types_external`, respectively.
The whole commissioning process benefits from :ref:`several security measures <thread_ot_commissioning_roles_authentication>`, especially the DTLS protocol.

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

The commissioning process goes through petitioning and joining.

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

.. _thread_ot_commissioning_cli:

Commissioning commands
**********************

See the following pages in the `OpenThread CLI Reference`_ on GitHub for an overview of available CLI commands:

* `Commissioner CLI commands`_
* `Joiner CLI commands`_

The NCP commands are available through `wpantund`_, which provides a `CLI wpantund wrapper`_ for `Spinel specification properties for Thread`_.

.. _thread_ot_commissioning_configuring_on-mesh:

Configuring on-mesh Thread commissioning
****************************************

You can configure on-mesh Thread commissioning using the :ref:`Thread CLI sample <ot_cli_sample>` or the :ref:`Thread NCP sample <ot_ncp_sample>` with two devices to form a Thread network.
One device will act as a Commissioner and the other will be a Joiner.

.. note::
    Before you start the configuration process, make sure you are familiar with :ref:`Thread commissioning concepts <thread_ot_commissioning>`, especially :ref:`thread_ot_commissioning_types_on-mesh`.

.. _thread_ot_commissioning_configuring_on-mesh_requirements:

Requirements
============

To configure on-mesh Thread commissioning, you need at least two development kits that are compatible with either the CLI or the NCP samples.
Check the sample documentation pages for the list of compatible development kits.

.. _thread_ot_commissioning_configuring_on-mesh_flashing:

.. rst-class:: numbered-step

Programming the DKs
===================

Program both development kits with the :ref:`Thread CLI sample <ot_cli_sample>` or program both of them with the :ref:`Thread NCP sample <ot_ncp_sample>`.
See the sample's page for details.

After programming the DKs and turning them on, both devices will be pre-commissioned and will form a Thread network.
This network needs to be manually disabled.

.. _thread_ot_commissioning_configuring_on-mesh_disabling:

.. rst-class:: numbered-step

Disabling the Thread network
============================

The |NCS|'s Thread CLI and NCP samples come with the autostart feature, which means that the devices will form the network automatically without user intervention.
To properly observe the commissioning process, it is recommended to form a new Thread network manually.

To disconnect from the network before starting the commissioning process, run the following command on both devices for the sample of your choice:

.. tabs::

   .. group-tab:: CLI

      .. code-block:: console

         uart:~$ ot thread stop

   .. group-tab:: NCP

      .. code-block:: console

         wpanctl:device_if> leave
         Leaving current WPAN. . .

.. _thread_ot_commissioning_configuring_on-mesh_forming:

.. rst-class:: numbered-step

Setting up the Commissioner
===========================

One of the two devices must become the Leader and Commissioner of the randomly generated network.
Complete the following steps for the sample of your choice:

1. Form a network by running the following commands:

   .. tabs::

      .. group-tab:: CLI

         .. code-block:: console

            uart:~$ ot dataset init new
            Done
            uart:~$ ot dataset commit active
            Done
            uart:~$ ot ifconfig up
            Done
            uart:~$ ot thread start
            Done

      .. group-tab:: NCP

         .. code-block:: console

            wpanctl:leader_if> form "My_Network" -c 11
            Forming WPAN "My_Network" as node type "router", channel:11
            Successfully formed!

#. View the newly generated network settings by running the following command:

   .. tabs::

      .. group-tab:: CLI

         .. code-block:: console

            uart:~$ ot dataset
            Active Timestamp: 1
            Channel: 23
            Channel Mask: 07fff800
            Ext PAN ID: 36dd32babd209538
            Mesh Local Prefix: fd51:51f2:fb58:c849/64
            Master Key: 0278f75cb81f04834f09b5fc095852d6
            Network Name: OpenThread-8299
            PAN ID: 0x8299
            PSKc: 658f3f958bade7db07a36c3fbf2fa2c9
            Security Policy: 0, onrcb
            Done

      .. group-tab:: NCP

         .. code-block:: console

            wpanctl:leader_if> status
            leader_if => [
                  "NCP:State" => "associated"
                  "Daemon:Enabled" => true
                  "NCP:Version" => "OPENTHREAD/20191113-01053-g07f430dac; NONE; Oct  7 2020 14:54:25"
                  "Daemon:Version" => "0.08.00d (0.07.01-347-gf2e1501; Oct  7 2020 11:40:50)"
                  "Config:NCP:DriverName" => "spinel"
                  "NCP:HardwareAddress" => [F4CE368F9ED56701]
                  "NCP:Channel" => 11
                  "Network:NodeType" => "leader"
                  "Network:Name" => "My_Network"
                  "Network:XPANID" => 0x7ADF737288F9FCB0
                  "Network:PANID" => 0xC4BB
                  "IPv6:MeshLocalAddress" => "fd7a:df73:7288:0:29a8:87b5:3fb7:5142"
                  "IPv6:MeshLocalPrefix" => "fd7a:df73:7288::/64"
                  "com.nestlabs.internal:Network:AllowingJoin" => false
            ]

#. Retrieve the ``EUI64`` identifier from the Joiner by running the following command:

   .. tabs::

      .. group-tab:: CLI

         .. code-block:: console

            uart:~$ ot eui64
            f4ce3687a6e4f6e8
            Done

      .. group-tab:: NCP

         .. code-block:: console

            wpanctl:joiner_if> getprop NCP:HardwareAddress
            NCP:HardwareAddress = [F4CE36710E768C63]

#. Enable the Commissioner role in the Leader device by running the following command:

   .. tabs::

      .. group-tab:: CLI

         .. code-block:: console

            uart:~$ ot commissioner start
            Done

      .. group-tab:: NCP

         .. code-block:: console

            wpanctl:leader_if> commissioner start
            Commissioner started

#. Set up a pre-shared key for the Joiner device by running the following command:

   .. tabs::

      .. group-tab:: CLI

         .. code-block:: console

            uart:~$ ot commissioner joiner add f4ce3687a6e4f6e8 N0RD1C
            Done

      .. group-tab:: NCP

         .. code-block:: console

            wpanctl:leader_if> commissioner joiner-add F4CE36710E768C63 3600 N0RD1C
            Added Joiner F4:CE:36:71:0E:76:8C:63, timeout:3600, PSKd:"N0RD1C"

   See :ref:`thread_ot_commissioning_roles_authentication` for encoding limitations.

.. _thread_ot_commissioning_configuring_on-mesh_joining:

.. rst-class:: numbered-step

Adding the Joiner
=================

Now that the Commissioner is ready, complete the following steps for the sample of your choice:

1. Start the joining process in the Joiner device by running the following commands:

   .. tabs::

      .. group-tab:: CLI

         .. code-block:: console

            uart:~$ ot ifconfig up
            Done
            uart:~$ ot joiner start N0RD1C
            Done

         After a couple of seconds, the following message appears:

         .. code-block:: console

            Join success

      .. group-tab:: NCP

         .. code-block:: console

            wpanctl:joiner_if> joiner --join N0RD1C
            Starting joiner commissioning, PSKd:"N0RD1C" ...
            Successfully joined!

   The Joiner starts broadcasting Discovery Requests on all available channels.
   When the Commissioner receives a Discovery Request, it responds to the sender.
   After the response, a DTLS session is established to securely authenticate the Joiner and exchange the network credentials.
#. After a successful joining process, attach the newly added device to the Thread network by running the following command:

   .. tabs::

      .. group-tab:: CLI

         .. code-block:: console

            uart:~$ ot thread start
            Done

      This command starts the Thread network and automatically attaches the device to it.

      .. group-tab:: NCP

         .. code-block:: console

            wpanctl:joiner_if> attach
            Resuming saved WPAN. . .

Both devices are now able to ping each other.
