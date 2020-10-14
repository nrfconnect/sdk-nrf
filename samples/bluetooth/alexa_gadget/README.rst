.. _peripheral_alexa_gadgets:

Bluetooth: Alexa Gadgets Peripheral
###################################

.. contents::
   :local:
   :depth: 2

The Alexa Gadgets Peripheral sample demonstrates how a Bluetooth LE device can connect to an Amazon Echo device using the Alexa Gadgets Bluetooth Service and Profile.

Documentation for the Gadgets Service and Profile can be found at `Alexa Gadgets Bluetooth LE`_.

Before a Gadget can be connected to an Amazon Echo device, you must register it with Amazon.
See `Alexa Gadgets Setup`_ for information on how to do this.

Overview
********

When connected, the sample performs the handshake procedure and informs the peer of its supported capabilities.
Directives sent by the connected peer are printed as log messages when :option:`CONFIG_LOG` is set.
Additionally, when the "wakeword" State Update directive is received, LED 3 on the development kit is turned on.

Gadget capabilities
*******************

During the handshake procedure, the Gadget peripheral reports which capabilities it supports.
Capabilities determine which interface directives the Gadgets will receive, as listed in `Alexa Gadgets Interfaces`_.

The following table shows the mapping between capability configuration and supported interfaces:

.. list-table::
   :header-rows: 1
   :align: center

   * - Capability option
     - Interface
   * - ``CONFIG_GADGETS_CAPABILITY_ALERTS_ENABLE``
     - `Alerts interface`_
   * - ``CONFIG_GADGETS_CAPABILITY_MUSICDATA_ENABLE``
     - `Alexa.Gadget.MusicData interface`_
   * - ``CONFIG_GADGETS_CAPABILITY_SPEECHDATA_ENABLE``
     - `Alexa.Gadget.SpeechData interface`_
   * - ``CONFIG_GADGETS_CAPABILITY_STATELISTENER_ENABLE``
     - `Alexa.Gadget.StateListener interface`_
   * - ``CONFIG_GADGETS_CAPABILITY_NOTIFICATIONS_ENABLE``
     - `Alexa Gadgets Notifications interface`_

Gadget Custom Directives and Events
***********************************
Custom directives sent from the peer to the Gadget are propagated via the ``GADGETS_EVT_CUSTOM`` event.
Custom directives do not require defining any additional data format.

This sample includes a rudimentary custom event type that lets you send a JSON-formatted string with your chosen custom event name and namespace, using the function ``gadgets_profile_custom_event_json_send``.

By default, the sample is configured to respond to the "Color cycler" custom skill.
For instructions on how to create this custom skill, see `Alexa Gadgets Github color cycler`_.
Note that the skill code and configuration procedure is the same, even though the sample from this link uses a Raspberry Pi as a Gadget.

For more complex custom event types, you must define a custom event structure as described in `Alexa Gadgets Custom Event`_.
The custom event protobuf must be encoded using ``pb_encode()`` and the encoded buffer must be transmitted using ``gadgets_profile_custom_event_send``.

Requirements
************

The sample supports the following development kits:

.. include:: /includes/boardname_tables/sample_boardnames.txt
   :start-after: set2_start
   :end-before: set2_end

The sample also requires:

* An Amazon Echo device
* An Amazon developer account
* A registered Gadget (see `Alexa Gadgets Registration`_)

External dependencies
=====================

This sample has some specific external dependencies.

Tools
-----

This sample depends on an external protobuf compiler tool - `Nanopb`_.

This tool must be downloaded manually.

Download Nanopb version **0.4.2** for your platform from the `Nanopb Downloads`_ site and extract the downloaded file to either this sample directory, or some other location where CMake can find it via a standard system PATH.
Note that this sample directory is searched automatically for the presence of Nanopb.
For example, if you are using Windows, download and extract **nanopb-0.4.2-windows-x86.zip** to this sample directory.

Once the Nanopb tool has been downloaded, the build process is the same as for other samples.
The tool is invoked automatically by CMake.

Gadget registration
-------------------

When registering the Gadget, you will receive two identifier strings:

1) Device Amazon ID
2) Device secret

You must update the ``CONFIG_GADGETS_AMAZON_ID`` and ``CONFIG_GADGETS_DEVICE_SECRET`` configuration options in :file:`prj.conf` with the values that you received during registration.

User interface
**************

LED 1:
   On when connected.

LED 2:
   On when handshake with peer is completed.

LED 3:
   On when wakeword directive (``Alexa.Gadget.StateListener``) is received.

LED 4:
   Toggles when Color Cycler skill is used.

Button 4:
   Erases bond information when button is held during power cycle or reset.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/alexa_gadget`

.. include:: /includes/build_and_run.txt

.. _peripheral_alexa_gadgets_testing:

Testing
=======

After programming the sample to your board, test it by performing the following steps:

1. Optionally, set up log monitoring:

  a. Connect the board to the computer using a USB cable.
     The board is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
  b. |connect_terminal|

2. Reset the board.
3. Follow these instructions to pair your Echo device with the sample: `Alexa Gadgets Pairing`_.
4. Observe that LED 1 turns on to indicate that a connection has been established.
5. Observe that LED 2 turns on to indicate that the Alexa Gadgets handshake has completed.
6. Speak the "Alexa" wakeword to your Echo device and observe that LED 3 turns on.
7. If you are monitoring the log output from the COM port, observe log activity during Alexa queries.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`gadgets_service_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``boards/arm/nrf*/board.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``
