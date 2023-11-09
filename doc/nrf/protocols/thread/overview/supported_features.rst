.. _thread_ug_supported_features:

Supported Thread features
#########################

.. contents::
   :local:
   :depth: 2

The OpenThread implementation of the Thread protocol supports all features defined in the Thread 1.3.0 Specification that are required for the Thread 1.3 Certification program:

* All Thread networking layers:

  * IPv6
  * 6LoWPAN
  * IEEE 802.15.4 with MAC security
  * Mesh Link Establishment
  * Mesh Routing

* All device roles
* Border Router support
* Features introduced with Thread 1.2:

  * Coordinated Sampled Listening (CSL)
  * Link Metrics Probing
  * Multicast across Thread networks
  * Thread Domain unicast addressing

* Features introduced with Thread 1.3.0:

  * Service Registration Protocol (SRP) client
  * Transport Control Protocol (TCP)

In the |NCS|, you can choose which version of the Thread protocol to use in your application.
By default, the |NCS| supports Thread 1.3, which includes support for Thread 1.2.
You can enable and configure any Thread version by using :ref:`dedicated options <thread_ug_thread_specification_options>`.

.. _thread_ug_supported_features_v12:

Thread 1.2 features
*******************

The |NCS| implements all mandatory features from the Thread 1.2 Specification.

The features introduced with the Thread 1.2 Specification are fully backward-compatible with implementations using Thread 1.1 (more specifically, the Thread 1.1.1 Specification).
Thread 1.2 improves network scalability, responsiveness, density, and power consumption.
For more information about this Thread version, see the official `Thread 1.2 in Commercial White Paper`_ and the `Thread 1.2 Base Features`_ document.

.. note::
    See :ref:`thread_ug_thread_specification_options` for how to enable the 1.2 features that are currently available in the |NCS|.

.. _thread_ug_supported_features_csl:

Coordinated Sampled Listening (CSL)
===================================

Coordinated Sampled Listening defined in IEEE 802.15.4-2015 is introduced by Thread 1.2 Specification to provide low latency communication for :ref:`Sleepy End Devices (SEDs)<thread_ot_device_types>`.
Thread 1.2 routers are required to support synchronized CSL transmissions to children which require them.
These children are known as :ref:`Synchronized Sleepy End Devices (SSEDs)<thread_ot_device_types>`.

SSEDs are allowed to transmit frames normally at any time, but the routers should use the CSL transmission mechanism as long as the synchronization is maintained.
This allows an SSED to stay in a sleepy state more than 99% of the time and only turn on its radio periodically for a few hundreds of microseconds in order to receive frames from its parent.

The most common use case for SSEDs is for developing battery-powered actuators with low latency, such as window blinds.

Link Metrics Probing Protocol
=============================

Thread 1.2 Specification introduces Link Metrics Probing Protocol as a mechanism to gather information about the link quality with neighbors.

The available metrics are:

* Count of received PDUs.
* Exponential Moving Average of the LQI.
* Exponential Moving Average of the link margin (dB).
* Exponential Moving Average of the RSSI (dBm).

The main potential usage of this feature is to allow a battery powered device to lower its transmission power when the link quality with its parent is good enough.
This way the device can save energy on each transmitted frame.
The device interested in receiving link quality information is referred to as a Link Metrics Initiator, while the device delivering that information is referred to as a Link Metrics Subject.

Developers can make use of the OpenThread API to obtain information about the link quality in three ways:

* Single probe: a single-shot `Mesh Link Establishment (MLE)`_ command that returns the requested metrics in the reply.
* Forward Tracking Series: the Initiator configures the Subject to start tracking link quality information for every received frame.
  At any point, the Initiator retrieves the averaged values for that series by means of an MLE Data Request.
* Enhanced-ACK probing: the Initiator configures the Subject to include link metrics information as Information Elements in the Enh-ACKs that are generated in reply to IEEE 802.15.4-2015 frames sent by the Initiator.

Enhanced-ACK probing is the most power efficient method to retrieve link metrics since very little message overhead is required.
The |NCS| provides full Link Metrics support even for the :ref:`thread_architectures_designs_cp_rcp` architecture, which is the most technically challenging one since the radio driver must handle the injection of Information Elements on time to match the acknowledgment timing requirements.
The decision on how to interpret the link metrics information to adjust the transmission power is left to the application itself.

.. _ug_thread_multicast:

Multicast across Thread networks
================================

Thread 1.1 border routers have a limitation not to forward multicast traffic with a scope greater than realm-local.
For certain applications it could be useful to be able to control multicast groups from a host outside the Thread network.
This is achieved in Thread 1.2 by allowing Thread border routers to forward multicast traffic with a scope greater than realm-local in two ways:

* From the Thread network to the exterior network: as a configuration option in the border router, for every multicast group.
* From the exterior network to the Thread network: the Primary Backbone Router (PBBR) forwards only multicast traffic with a destination matching one of the multicast groups registered by Thread devices in its network.

For the second case, a Thread :ref:`Commissioner<thread_ot_commissioning_roles>` can be used as well to register allowed multicast groups on behalf of the devices.

The OpenThread stack will automatically handle the registration of multicast groups with a proper PBBR whenever they are configured in the device.

.. _ug_thread_domain:

Thread Domain unicast addressing
================================

Thread 1.2 Specification introduces the concept of Thread Domains.

A Thread Domain is a set of Thread Devices that receive and apply a common Thread Domain operational configuration.
The Thread Domain operational configuration enables Thread Devices to join and participate in larger interconnected `scopes <IPv6 Addressing Scopes_>`_ even extending beyond the limits of a single Thread network.
A user or network administrator may use functions of either Thread Commissioning or Thread Border Routers to set up a common Thread Domain operational configuration for Thread Devices.
The Thread Devices can belong to different Thread networks or `Partitions <Thread Partitions_>`_ that have potentially different per-network credentials.

.. _ug_thread_12_support_limitations:

Limitations for Thread 1.2 support
==================================

The Thread 1.2 Specification support has the following limitation:

* Due to code size limitation, the combination of complete set of Thread 1.2 features with the Bluetooth® LE multiprotocol support is not possible for the nRF52833 DKs.

.. _thread_ug_supported_features_v13:

Thread 1.3 features
*******************

For more information about this Thread version, see the official `Thread 1.3.0 Features White Paper`_.

.. note::
    See :ref:`thread_ug_thread_specification_options` for how to enable the 1.3 features that are currently available in the |NCS|.

DNS-based Service Discovery
===========================

Thread 1.3 Specification introduces DNS-SD Service Registration Protocol, which lets devices advertise the fact that they provide services while avoiding the use of multicast in the discovery.
The |NCS| provides the required SRP client functionality.

Transport Control Protocol
==========================

While the |NCS| has had TCP support through Zephyr (:ref:`IP stack supported features <zephyr:ip_stack_overview>`), the Thread 1.3 Specification defines a set of parameters and features that make TCP more efficient for the limited IEEE 802.15.4 networks.
An alternative TCP stack implementation incorporated from the OpenThread project can be enabled by users working on Thread-based TCP applications.

See the :file:`tcp.conf` configuration file in the :file:`snippets/tcp/` directory of the :ref:`ot_cli_sample` sample for an example how to enable the alternative TCP implementation.

Limitations for Thread 1.3 support
==================================

Transport Control Protocol (TCP) as defined by the Thread 1.3 Specification is only supported in experimental mode by the |NCS|.
