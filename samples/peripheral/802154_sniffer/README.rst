.. _802154_sniffer:

IEEE 802.15.4 Sniffer
#####################

.. contents::
   :local:
   :depth: 2

The IEEE 802.15.4 Sniffer listens to a selected IEEE 802.15.4 channel (2.4GHz O-QPSK with DSSS) and integrates with the nRF 802.15.4 sniffer extcap for Wireshark.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The application can be used with the `nRF Sniffer for 802.15.4`_ extcap utility for the `Wireshark`_ network protocol analyzer.

Overview
********

The application presents the user with a command-line interface.

See the :ref:`802154_sniffer_commands` for the list of the available commands.

LED 1:
   When the capture is stopped the LED blinks with a period of 2 seconds with 50% duty cycle.
   When the capture is ongoing the LED blinks with a period of 0.5 seconds with 50% duty cycle.

LED 4:
   When the sniffer captures a packet the LED is toggled on and off.

.. _802154_sniffer_commands:

Serial commands list
********************

This section lists the serial commands that are supported by the sample.

channel - Change the radio channel
==================================

The command changes the IEEE 802.15.4 radio channel to listen on.

   .. parsed-literal::
      :class: highlight

      channel *<channel>*

The ``<channel>`` argument is an integer in the range between 11 and 26.

For example:

   .. parsed-literal::
      :class: highlight

      channel *23*

receive - start capturing packets
=================================

The ``receive`` command makes the sniffer enter the RX state and start capturing packets.

   .. parsed-literal::
      :class: highlight

      receive

The received packets will be printed to the command-line with the following format:

   .. parsed-literal::
      :class: highlight

      received: *<data>* power: *<power>* lqi: *<lqi>* time: *<timestamp>*

* The ``<data>`` is a hexidecimal string representation of the received packet.
* The ``<power>`` value is the signal power in dBm.
* The ``<lqi>`` value is the IEEE 802.15.4 Link Quality Indicator.
* The ``<timestamp>`` value is the absolute time of the received packet since the sniffer booted.

sleep - stop capturing packets
==============================

The ``sleep`` command disables the radio and ends the receive process.

   .. parsed-literal::
      :class: highlight

      sleep

Configuration
*************

|config|

Building and running
********************

.. |sample path| replace:: :file:`samples/peripheral/802154_sniffer`

.. include:: /includes/build_and_run.txt

.. _802154_sniffer_testing:

Testing the sample
==================

After programming the sample to your development kit, complete the following steps to test it:

1. Connect the development kit to the computer using a USB cable.
   Use the development kit's nRF USB port (**J3**).
   The kits are assigned a COM port (in Windows) or a ttyACM device (in Linux), visible in the Device Manager or in the :file:`/dev` directory.
#. |connect_terminal|
#. Switch to a radio channel with an ongoing radio traffic:

   .. parsed-literal::
      :class: highlight

      channel *23*

#. Start the capture process:

   .. parsed-literal::
      :class: highlight

      receive

   The **LED 1** will start blinking with shorter intervals.

#. If there is radio traffic on the selected channel, the sniffer should print the captured packets:

   .. parsed-literal::
      :class: highlight

      received: 49a85d41a5fffff4110f10270000369756e65619d09428a04b301951821db234460aa5ec4ff506631ef8adb22674683700 power: -39 lqi: 220 time: 15822687

   The **LED 4** will toggle its state when a frame is received.

#. Disable the capture:

   .. parsed-literal::
      :class: highlight

      sleep

   The **LED 1** will start blinking with longer intervals.

Dependencies
************

This sample uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:mpsl`
* :ref:`nrfxlib:nrf_802154`

This sample uses the following |NCS| libraries:

* :ref:`dk_buttons_and_leds_readme`

This sample uses the following Zephyr libraries:

* :ref:`zephyr:kernel_api`:

  * :file:`include/zephyr/kernel.h`
  * :file:`include/zephyr/sys/util.h`

* :ref:`zephyr:ieee802154_interface`:

  * :file:`include/zephyr/net/ieee802154_radio.h`

* :ref:`zephyr:shell_api`:

  * :file:`include/zephyr/shell/shell.h`
  * :file:`include/zephyr/shell/shell_uart.h`
