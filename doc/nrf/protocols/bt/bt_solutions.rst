.. _ug_bt_solution:

Bluetooth solution areas
########################

.. contents::
   :local:
   :depth: 2

Bluetooth® technology provides full stack support for a wide and expanding range of use cases.

Audio streaming
***************

The |NCS| supports the Bluetooth LE Audio standard, including `Auracast™`_ (broadcast) audio.
See :ref:`nrf53_audio_app` to get started.

Note that Nordic Semiconductor SoCs do not support Bluetooth Classic Audio, which is based on the Bluetooth BR/EDR radio.

Data transfer
*************

Data transfer is the main use case area for Bluetooth LE and covers use cases like wearables, PC peripherals, and health-related devices.
Supported use cases for Bluetooth LE are implemented as Services and Profiles on top of the Generic Attribute Profile (GATT).

Support in the |NCS|:

* :ref:`nrf_desktop`: Reference design for a Bluetooth Human Interface Device (HID) solution.
  Includes desktop mouse, gaming mouse, keyboard, and dongle.
* :ref:`ble_samples` (|NCS|): Bluetooth samples, most of which are data transfer related.
  The following samples are recommended to get started with Bluetooth:

  * :ref:`peripheral_lbs`: This sample implements controls for buttons and LEDs on various hardware platforms from Nordic Semiconductor.
    The GATT Service is not a Bluetooth standard, so the sample is a good template for implementing vendor-specific profiles and services.

  * :ref:`peripheral_uart` and :ref:`central_uart`: These samples implement a Nordic-defined GATT service and profile that give a simple TX/RX generic data pipe, providing UART communication over Bluetooth LE.

* :zephyr:code-sample-category:`bluetooth` (Zephyr Project): The Zephyr Project offers additional Bluetooth samples.
  These samples are not guaranteed to work as part of the |NCS|, but are helpful as starting point for a relevant use case.

Device networks
***************

Bluetooth Mesh is a technology that enables large-scale device networks in a many-to-many (m:m) topology, with focus on reliability and security.
The support in the |NCS| includes the Mesh Profile, Mesh Models, Bluetooth Networked Lighting Control (NLC) profiles, and related application examples.
See the :ref:`ug_bt_mesh` main page for further details.

Periodic Advertising with Response (PAwR) allows a central device to communicate with any number of devices in a star
topology.
The feature is support in :ref:`softdevice_controller`, and the related Encrypted Advertising (EAD) feature is
supported in Zephyr Host.
The main use case, Electronic Shelf-Labels (ESL), has an experimental implementation as the `nrf-esl-bluetooth`_ application in the Nordic Playground.

Location services
*****************

In addition to the various forms of data transfer, Bluetooth technology includes methods for device positioning.

Presence
========

Bluetooth advertising is an easy way to indicate presence of a device to a nearby scanner.
These kinds of solutions are known as Bluetooth beacons and work as a foundation for many solutions for tracking and item finding.

Nordic Semiconductor offers full implementation of two solutions for item finding:

* Google Find My Device Network: See the :ref:`fast_pair_locator_tag` sample for an implementation of a tag device.
* Apple Find My Network: See :ref:`integrations` page on how to apply for access.

Direction
=========

The |NCS| includes 4 samples related to Bluetooth Angle-of-Arrival (AoA) technology, which allows to find the
direction from which a Bluetooth signal is transmitted:

* :ref:`direction_finding_connectionless_rx`
* :ref:`direction_finding_connectionless_tx`
* :ref:`bluetooth_direction_finding_central`
* :ref:`direction_finding_peripheral`

Distance
========

Channel sounding is a feature in the Bluetooth specification that allows distance measurement between two devices.
The measurement is based on round-trip timing (RTT) and phase-based ranging (PBR).
The following samples can be used to evaluate the feature:

* :ref:`channel_sounding_ras_reflector`
* :ref:`channel_sounding_ras_initiator`
