.. _thread_ug_supported_features:

Supported Thread features
#########################

.. contents::
   :local:
   :depth: 2

The OpenThread implementation of the Thread protocol supports all features defined in the Thread 1.1.1 Specification:

* All Thread networking layers:

  * IPv6
  * 6LoWPAN
  * IEEE 802.15.4 with MAC security
  * Mesh Link Establishment
  * Mesh Routing

* All device roles
* Border Router support

.. _thread_ug_supported_features_v12:

Support for Thread 1.2 Specification
************************************

The |NCS| is gradually implementing features from the Thread 1.2 Specification.

The features introduced with the Thread 1.2 Specification are fully backward-compatible with Thread 1.1 (more specifically, the Thread 1.1.1 Specification).
Thread 1.2 improves network scalability, responsiveness, density, and power consumption.
For more information about this Thread version, see the official `Thread 1.2 in Commercial White Paper`_ and the `Thread 1.2 Base Features`_ document.

In |NCS|, you can choose which version of the Thread protocol to use in your application.
By default, |NCS| supports Thread 1.1, but you can enable and configure Thread 1.2 by using :ref:`dedicated options <thread_ug_thread_1_2>`.

.. note::
    All Thread 1.2 mandatory functionalities are currently implemented, except for the full Border Router support.
    See :ref:`thread_ug_thread_1_2` for the list of 1.2 features that are currently available in |NCS|, with information about how to enable them.
    Currently, the :ref:`ot_cli_sample` sample is the only sample that provides an :ref:`ot_cli_sample_thread_v12`.

Limitations for Thread 1.2 support
==================================

The Thread 1.2 Specification support has the following limitation:

* Due to code size limitation, the combination of complete set of Thread 1.2 features with the Bluetooth® LE multiprotocol support is not possible for the nRF52833 DKs.

Coordinated Sampled Listening (CSL)
===================================

Coordinated Sampled Listening defined in IEEE 802.15.4-2015 is introduced by Thread 1.2 Specification to provide low latency communication for Sleepy End Devices.
Thread 1.2 routers are required to support synchronized CSL transmissions to children which require them, known as Synchronized Sleepy End Devices (SSEDs).
SSEDs are allowed to transmit frames normally at any time, but the routers should use the CSL transmission mechanism as long as the synchronization is maintained.
This allows an SSED to stay in a sleepy state more than 99% of the time and only turn on its radio periodically for a few hundreds of microseconds in order to receive frames from its parent.

The most common use case for SSEDs is for developing low latency battery-powered actuators, such as window blinds.

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
The device interested in receiving link quality information is referred to as Link Metrics Initiator, while the device delivering that information is referred to as Link Metrics Subject.

Developers can make use of the OpenThread API to obtain information about the link quality in three ways:

* Single probe: a single-shot MLE command that returns the requested metrics in the reply.
* Forward Tracking Series: the Initiator configures the Subject to start tracking link quality information for every received frame.
  At any point, the Initiator retrieves the averaged values for that series by means of an MLE Data Request.
* Enhanced-ACK probing: the Initiator configures the Subject to include link metrics information as Information Elements in the Enh-ACKs that are generated in reply to IEEE 802.15.4-2015 frames sent by the Initiator.

Enhanced-ACK probing is the most power efficient method to retrieve link metrics since very little message overhead is required.
|NCS| provides full Link Metrics support even for the RCP architecture, which is the most technically challenging one since the radio driver must handle the injection of Information Elements on time to match the acknowledgment timing requirements.
The decision on how to interpret the link metrics information to adjust the transmission power is left to the application itself.
