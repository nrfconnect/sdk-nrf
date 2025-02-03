.. _ug_bt_mesh_overview_intro:

Bluetooth Mesh overview
#######################

The Bluetooth® Mesh protocol is a specification (see `Bluetooth Mesh protocol specification`_) developed and published by the Bluetooth Special Interest Group (SIG).
It allows one-to-one, one-to-many, and many-to-many communication, using the Bluetooth LE protocol to exchange messages between the nodes on the network.

Compared to traditional Bluetooth Low Energy connections, the Bluetooth Mesh protocol offers the following features:

* Multi-hop (message relaying), extending the range beyond RF - supports up to 32767 devices in a network, with a maximum network diameter of 126 hops
* Multipath transmission, increasing the reliability
* Built-in security through:

  * Multilevel encryption (network and application)
  * Privacy through obfuscating
  * Authentication
  * Replay protection
  * Trash-can attack protection through Key Refresh and node removal procedures

The Bluetooth Mesh technology is covered by the following set of specifications:

* `Bluetooth Mesh protocol specification`_ v1.1
* `Bluetooth Mesh model specification`_ v1.1
* `Mesh Device Firmware Update Model`_ v1.0
* `Mesh Binary Large Object Transfer Model`_ v1.0
* `Ambient Light Sensor NLC Profile`_ v1.0.1
* `Basic Lightness Controller NLC Profile`_ v1.0.1
* `Basic Scene Selector NLC Profile`_ v1.0.1
* `Dimming Control NLC Profile`_ v1.0.1
* `Energy Monitor NLC Profile`_ v1.0.1
* `Occupancy Sensor NLC Profile`_ v1.0.1

Bluetooth Mesh in the |NCS| supports all mandatory features and almost all optional features of the `Bluetooth Mesh protocol specification`_.
The following features are supported:

* Node role (Advertising and GATT bearer)
* Provisioner role (Advertising bearer only)
* Relay feature
* Proxy feature (for connectivity to smart phones and tablets)
* Friend and Low Power features
* Configuration and health foundation models
* Remote Provisioning models
* Bridge Configuration models
* Private Beacons
* On-Demand Private GATT Proxy
* SAR Enhancements
* Opcode Aggregator models
* Large Composition Data models
* Enhanced Provisioning Authentication algorithm

All models from the `Bluetooth Mesh model specification`_ are supported.
See :ref:`bt_mesh_models` for details about the following models implemented in the |NCS|:

* Generic OnOff
* Generic Level
* Generic Default Transition Time
* Generic Power OnOff
* Generic Power Level
* Generic Battery
* Generic Location
* Generic Property
* Light Lightness
* Light Lightness Control
* Light CTL
* Light xyL
* Light HSL
* Sensor
* Time
* Scene
* Scheduler

The following features related to Mesh Device Firmware Upgrade are supported:

* Mesh Device Firmware Update Models

  * Except HTTPS, and OOB firmware upload
  * Except Firmware Distributor Client model
  * Vendor-specific mechanism of firmware upload over the Simple Management Protocol
    (SMP) service on distributor is supported

* Mesh Binary Large Object Transfer (BLOB) Models

Additionally, the following features are provided:

* Chat (vendor model)
* Silvair EnOcean Switch Mesh Proxy Server (vendor model)
* Support for over-the-air secure background DFU (point-to-point)
* Configuration and provisioning using the nRF Mesh mobile app

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
