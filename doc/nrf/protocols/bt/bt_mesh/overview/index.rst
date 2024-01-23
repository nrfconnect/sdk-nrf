.. _ug_bt_mesh_overview_intro:

Bluetooth Mesh overview
#######################

The Bluetooth® Mesh protocol is a specification (see `Bluetooth Mesh protocol specification`_) developed and published by the Bluetooth Special Interest Group (SIG).
It allows one-to-one, one-to-many, and many-to-many communication, using the Bluetooth LE protocol to exchange messages between the nodes on the network.

Some of the features the Bluetooth Mesh in the |NCS| can offer are:

* Multi-hop (message relaying), extending the range beyond RF - supports up to 32767 devices in a network, with a maximum network diameter of 126 hops
* Multipath transmission, increasing the reliability
* Built-in security:

  * Multilevel encryption (network and application)
  * Privacy through obfuscating
  * Authentication
  * Replay protection
  * Trash-can attack protection through Key Refresh and node removal procedures
* Complete Bluetooth Mesh models implementation
* Complete Bluetooth Mesh Profile from Zephyr Project
* Rich samples for prototyping
* Roles configurable in Kconfig, among others

  * Relay
  * Friend
  * Low Power
  * Proxy
  * Provisioning (Provisioner/Provisionee)
* EnOcean Switch integration
* Standardized profiles for wireless lighting controls (NLC)
* Configuration and provisioning using nRF Mesh mobile app

The Bluetooth Mesh in the |NCS| supports all mandatory features of the `Bluetooth Mesh protocol specification`_ and the `Bluetooth Mesh model specification`_, as well as most of the optional features.
See :ref:`bt_mesh_models` for details about the models implemented in the |NCS|.
In addition, a number of features is supported by Zephyr's :ref:`zephyr:bluetooth_mesh`.

The following pages provide an overview of the Bluetooth Mesh in |NCS|.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   architecture
   topology
   models
   reserved_ids
   security
   coexistence
   nlc
