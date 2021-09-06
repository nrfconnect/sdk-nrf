.. _802154_phy_test:

IEEE 802.15.4 PHY Test Tool
###########################

.. contents::
   :local:
   :depth: 2

The IEEE 802.15.4 PHY Test Tool provides a solution for performing Zigbee RF Performance and PHY Certification tests, as well as a general evaluation of the integrated radio with IEEE 802.15.4 standard.

Overview
********

You can perform the testing by connecting to the development kit through the serial port and sending supported commands.

See the :ref:`802154_phy_test_ui` for the list of the available commands.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpunet, nrf52840dk_nrf52840, nrf21540dk_nrf52840

Conducting tests using the sample also requires a testing device, like another development kit running the same sample, set into DUT mode.
For more information, see :ref:`802154_phy_test_testing`.

.. note::
   You can perform the testing using other equipment, like spectrum analyzers, oscilloscopes, or RF power meters, but these methods are not covered by this documentation.

.. _802154_phy_test_ui:

Serial commands list
********************

This section lists the serial commands that are supported by the sample.

changemode - Change the device mode
===================================

It changes the device mode if BOTH modes are available.

   .. parsed-literal::
      :class: highlight

      custom changemode *<mode>*

The ``<mode>`` argument can assume one of the following values:

* ``0`` - DUT
* ``1`` - CMD

See the following example:

   .. parsed-literal::
      :class: highlight

      custom changemode *1*

lindication - LED indication
============================

It makes the CMD device control the LED indicating packet reception.

   .. parsed-literal::
      :class: highlight

      custom lindication *<value>*

The ``<value>`` argument can assume one of the following values:

* ``0`` - none
* ``1`` - LED packet reception indication

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lindication *1*

rping - Ping the DUT
====================

It makes the CMD device send a PING to the DUT device and wait for the reply.

   .. parsed-literal::
      :class: highlight

      custom rping

See the following example:

   .. parsed-literal::
      :class: highlight

      custom rping

lpingtimeout - Set the ping timeout
===================================

It makes the CMD device set the timeout in milliseconds for receiving the *pong* responses from the DUT device.

   .. parsed-literal::
      :class: highlight

      custom lpingtimeout *<timeout:1>* *<timeout:0>*

* The ``<timeout:1>`` value indicates the higher byte of the timeout.
* The ``<timeout:0>`` value indicates the lower byte of the timeout.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lpingtimeout *0* *255*

setchannel - Set the common radio channel for the DUT and CMD
=============================================================

It sets a common radio channel for the DUT and CMD devices, sends a PING, and waits for the response.

   .. parsed-literal::
      :class: highlight

      custom setchannel *<channel:3>* *<channel:2>* *<channel:1>* *<channel:0>*.

The four ``<channel:x>`` arguments are four octets defining the channel page and number.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom setchannel *0* *0* *8* *0*

lsetchannel - Set the CMD radio channel
=======================================

It sets radio channel of the CMD device.

   .. parsed-literal::
      :class: highlight

      custom lsetchannel *<channel:3>* *<channel:2>* *<channel:1>* *<channel:0>*.

The four ``<channel:x>`` arguments are four bytes defining the channel page and number.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lsetchannel *0* *0* *16* *0*

rsetchannel - Set the DUT radio channel
=======================================

It sets the radio channel of the DUT device.

   .. parsed-literal::
      :class: highlight

      custom rsetchannel *<channel>*

The ``<channel>`` argument indicates the selected channel’s number.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom rsetchannel *13*

lgetchannel - Get the CMD radio channel
=======================================

It gets the current configured channel of the CMD device.

   .. parsed-literal::
      :class: highlight

      custom lgetchannel

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lgetchannel

lsetpower - Set the CMD radio power
===================================

It sets the CMD device's TX power.

   .. parsed-literal::
      :class: highlight

      custom lsetpower *<mode:1>* *<mode:0>* *<power>*

* The ``<mode:1>`` and ``<mode:0>`` arguments are currently unsupported.
  Use ``0`` for both.
* The ``<power>`` argument indicates the TX power as a signed integer in dBm.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom rsetpower *0* *0* *-17*

lgetpower - Get the CMD radio power
===================================

It gets the current configured power of the CMD device.

   .. parsed-literal::
      :class: highlight

      custom lgetpower

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lgetpower

rgetpower - Get the DUT radio power
===================================

It gets the current configured power of the DUT device.

   .. parsed-literal::
      :class: highlight

      custom rgetpower

See the following example:

   .. parsed-literal::
      :class: highlight

      custom rgetpower

rstream - DUT modulated waveform transmission
=============================================

It commands the DUT device to start a modulated waveform transmission of a certain duration in milliseconds.

   .. parsed-literal::
      :class: highlight

      custom rstream *<duration:1>* *<duration:0>*

* The ``<duration:1>`` argument indicates the higher byte of the duration value.
* The ``<duration:0>`` argument indicates the lower byte of the duration value.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom rstream *255* *255*

rstart - Start the RX Test
==========================

It makes the DUT device start the RX test routine, clearing previous statistics.

   .. parsed-literal::
      :class: highlight

      custom rstart

See the following example:

   .. parsed-literal::
      :class: highlight

      custom rstart

rend - End the RX Test
======================

It makes the DUT device terminate the RX test routine.
The DUT device also sends the test results to the CMD device

   .. parsed-literal::
      :class: highlight

      custom rend

See the following example:

   .. parsed-literal::
      :class: highlight

      custom rend

find - Find the DUT
===================

It makes the CMD device cycle all the channels (11-26) trying to PING the DUT device.
It stops upon receiving a reply.

   .. parsed-literal::
      :class: highlight

      custom find

See the following example:

   .. parsed-literal::
      :class: highlight

      custom find

lgetcca - Clear Channel Assessment
==================================

It makes the CMD device perform a Clear Channel Assessment (CCA) with the requested mode and print the result.

   .. parsed-literal::
      :class: highlight

      custom lgetcca *<mode>*

The ``<mode>`` argument indicates the IEEE 802.15.4 CCA mode (1-3).

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lgetcca *1*

lsetcca - Clear Channel Assessment
==================================

If enabled, it makes the CMD device perform a Clear Channel Assessment (CCA) before each transmission.

   .. parsed-literal::
      :class: highlight

      custom lsetcca *<toggle>*

The ``<toggle>`` argument enables or disables the execution of a CCA before each transmission.
It can be set to the following values:

* ``1`` - enable CCA
* ``0`` - disable CCA

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lsetcca *1*

lgeted - Perform Energy Detection
=================================

It starts the energy detection and reports the result as 2 hexadecimal bytes.

   .. parsed-literal::
      :class: highlight

      custom lgeted

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lgeted

lgetlqi - Get Link Quality Indicator
====================================

It puts the CMD device in receive mode and makes it wait for a packet.
It then outputs the result as 2 hexadecimal bytes.

   .. parsed-literal::
      :class: highlight

      custom lgetlqi

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lgetlqi

lgetrssi - Measure RSSI
=======================

It gets the RSSI in dBm.

   .. parsed-literal::
      :class: highlight

      custom lgetrssi

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lgetrssi

lsetshort - Set short address
=============================

It sets the CMD device's short address.
It is used for frame filtering and acknowledgment transmission.

   .. parsed-literal::
      :class: highlight

      custom lsetshort *0x<short_address>*

The ``<short_address>`` argument indicates the IEEE 802.15.4 short, two-byte address.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lsetshort *0x00FF*

lsetextended - Set extended address
===================================

It sets the CMD device's extended address.
It is used for frame filtering and acknowledgment transmission.

   .. parsed-literal::
      :class: highlight

      custom lsetextended *0x<extended_address>*

The ``<extended_address>`` argument indicates the IEEE 802.15.4 long, 8-byte address.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lsetextended *0x000000000000FFFF*

lsetpanid - Set PAN id
======================

It sets the PAN id.
It is used for frame filtering and acknowledgment transmission.

   .. parsed-literal::
      :class: highlight

      custom lsetpanid *0x<panid>*

The ``<panid>`` argument indicates the two-bytes of the IEEE 802.15.4 PAN ID.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lsetpanid *0x000A*

lsetpayload - Set payload for burst transmission
================================================

It sets an arbitrary payload of a raw IEEE 802.15.4 packet.

   .. parsed-literal::
      :class: highlight

      custom lsetpayload *<length>* *<payload>*

* The ``<length>`` argument indicates the length of the payload in bytes.
* The ``<payload>`` argument indicates the bytes of the packet payload.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lsetpayload *5* *FFFFFFFFFF*

ltx - Burst transmission of packets
===================================

It starts the transmission of packets with a random (or previously defined) payload.

   .. parsed-literal::
      :class: highlight

      custom ltx *<number>* *<delay>*

* The ``<number>`` argument indicates the number of packets to be sent.
  Set to ``0`` for an infinite transmission.
* The ``<delay>`` argument indicates the delay in milliseconds between the transmissions.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom ltx *10* *1000*

ltxend - Stop the burst transmission of packets
===============================================

It makes the CMD device stop current burst transmission.

   .. parsed-literal::
      :class: highlight

      custom ltxend

See the following example:

   .. parsed-literal::
      :class: highlight

      custom ltxend

lstart - Start the continuous receive mode
==========================================

It makes the sample enter the continuous receive mode and print the received packet information over a serial connection.
The sample does not accept any other command until it receives ``custom lend``.

   .. parsed-literal::
      :class: highlight

      custom lstart

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lstart

lend - End the continuous receive mode
======================================

It makes the sample leave the continuous receive mode and print statistics

   .. parsed-literal::
      :class: highlight

      custom lend

The statistics are shown in the following format:

   .. parsed-literal::
      :class: highlight

      [total]0x%x%x%x%x [protocol]0x%x%x%x%x [totalLqi]0x%x%x%x%x [totalRssiMgnitude]0x%x%x%x%x

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lend

lsetantenna - Set CMD antenna id
================================

It sets the antenna used by the CMD device for both TX and RX operations.

   .. parsed-literal::
      :class: highlight

      custom lsetantenna *<antenna>*

The ``<antenna>`` argument indicates the antenna id.
It can either be ``0`` or ``1``.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lsetantenna *1*

lsetrxantenna - Set CMD antenna id for RX
=========================================

It sets the antenna used by the CMD device for RX operations.

   .. parsed-literal::
      :class: highlight

      custom lsetrxantenna *<antenna>*

The ``<antenna>`` argument indicates the antenna id.
It can either be ``0`` or ``1``.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lsetrxantenna *1*

lsettxantenna - Set CMD antenna id for TX
=========================================

It sets the antenna used by the CMD device for TX operations.

   .. parsed-literal::
      :class: highlight

      custom lsettxantenna *<antenna>*

The ``<antenna>`` argument indicates the antenna id.
It can either be ``0`` or ``1``.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lsettxantenna *1*

lgetrxantenna - Get CMD RX antenna id
=====================================

It gets the antenna used by the CMD device for RX operations.

   .. parsed-literal::
      :class: highlight

      custom lgetrxantenna

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lgetrxantenna

lgettxantenna - Get CMD TX antenna id
=====================================

It gets the antenna used by the CMD device for TX operations.

   .. parsed-literal::
      :class: highlight

      custom lgettxantenna

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lgettxantenna

lgetbestrxantenna - Get last best CMD RX antenna id selected by antenna diversity algorithm
===========================================================================================

It gets the last best antenna selected for RX operations by the antenna diversity algorithm.

   .. parsed-literal::
      :class: highlight

      custom lgetbestrxantenna

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lgetbestrxantenna

rsetantenna - Set DUT antenna id
================================

It sets the antenna used by the DUT device for both TX and RX operations.

   .. parsed-literal::
      :class: highlight

      custom rsetantenna *<antenna>*

The ``<antenna>`` argument indicates the antenna id.
It can either be ``0`` or ``1``.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom rsetantenna *1*

rsettxantenna - Set DUT TX antenna id
=====================================

It sets the antenna used by the DUT device for TX operations.

   .. parsed-literal::
      :class: highlight

      custom rsettxantenna *<antenna>*

The ``<antenna>`` argument indicates the antenna id.
It can either be ``0`` or ``1``.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom rsettxantenna *1*

rsetrxantenna - Set DUT RX antenna id
=====================================

It sets the antenna used by the DUT device for RX operations.

   .. parsed-literal::
      :class: highlight

      custom rsetrxantenna *<antenna>*

The ``<antenna>`` argument indicates the antenna id.
It can either be ``0`` or ``1``.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom rsetrxantenna *1*

rgetrxantenna - Get DUT TX antenna id
=====================================

It gets the antenna used by the DUT device for TX operations.

   .. parsed-literal::
      :class: highlight

      custom rgettxantenna

See the following example:

   .. parsed-literal::
      :class: highlight

      custom rgettxantenna

rgettxantenna - Get DUT RX antenna id
=====================================

It gets the antenna used by the DUT device for RX operations.

   .. parsed-literal::
      :class: highlight

      custom rgetrxantenna

See the following example:

   .. parsed-literal::
      :class: highlight

      custom rgetrxantenna

rgetbestrxantenna - Get last best DUT RX antenna id selected by antenna diversity algorithm
===========================================================================================

It gets the last best antenna selected for RX operations by the antenna diversity algorithm.

   .. parsed-literal::
      :class: highlight

      custom rgetbestrxantenna

See the following example:

   .. parsed-literal::
      :class: highlight

      custom rgetbestrxantenna

lcarrier - Unmodulated waveform (carrier) transmission
======================================================

It starts the transmission of the unmodulated carrier.

   .. parsed-literal::
      :class: highlight

      custom lcarrier *<pulse_duration>* *<interval>* *<transmission_duration>*

* The ``<pulse_duration>`` argument indicates the duration of the continuous signal transmission.
  It ranges between ``1`` and ``32767`` milliseconds.
* The ``<interval>`` argument indicates the duration of the interval between the pulses.
  It ranges between ``0`` and ``32767`` milliseconds.
* The ``<transmission_duration>`` argument indicates the upper limit for the command’s execution.
  It ranges between ``0`` and ``32767`` milliseconds.
  Set to ``0`` for infinite transmission.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lcarrier ``10`` ``200`` ``2000``

lstream - Modulated waveform transmission
=========================================

It starts a modulated waveform transmission.

   .. parsed-literal::
      :class: highlight

      custom lstream *<pulse_duration>* *<interval>* *<transmission_duration>*

* The ``<pulse_duration>`` argument indicates the duration of the continuous packet transmission.
  It ranges between ``1`` and ``32767`` milliseconds.
* The ``<interval>`` argument indicates the duration of the interval between the pulses.
  It ranges between ``0`` and ``32767`` milliseconds.
* The ``<transmission_duration>`` argument indicates the upper limit for the command’s execution.
  It ranges between ``0`` and ``32767`` milliseconds.
  Set to ``0`` for infinite transmission.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lstream *100* *200* *20000*

rhardwareversion - Get the DUT hardware version
===============================================

It gets the hardware version of the DUT device.

   .. parsed-literal::
      :class: highlight

      custom rhardwareversion

   .. parsed-literal::
      :class: highlight

      custom rhardwareversion

rsoftwareversion - Get the DUT software version
===============================================

It gets the software version of the DUT device.

   .. parsed-literal::
      :class: highlight

      custom rsoftwareversion

   .. parsed-literal::
      :class: highlight

      custom rsoftwareversion

lclk - High-frequency clock output on a selected pin
====================================================

It makes the CMD device disable or enable the high-frequency clock output on a selected pin.
The actual clock frequency depends on the SoC used.
It is the highest possible considering the GPIO and CLOCK modules possibilities.

   .. parsed-literal::
      :class: highlight

      custom lclk *<pin>* *<value>*

* The ``<pin>`` argument indicates the GPIO pin number.
  It ranges between ``0`` and the number of GPIO pins supported by the SoC.
* The ``<value>`` argument can assume one of the following values:

  * ``0`` - disabled
  * ``1`` - enabled

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lclk *10* *1*

lsetgpio - Set GPIO pin value
=============================

It makes the CMD device set the value of the selected GPIO out pin.

   .. parsed-literal::
      :class: highlight

      custom lsetgpio *<pin>* *<value>*

* The ``<pin>`` argument indicates the GPIO pin number.
  It ranges between ``0`` and the number of GPIO pins supported by the SoC.
* The ``<value>`` argument can assume one of the following values:

  * ``0`` - low
  * ``1`` - high

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lsetgpio *29* *0*

lgetgpio - Get GPIO pin value
=============================

It makes CMD reconfigure the selected GPIO pin to INPUT mode and read its value.

   .. parsed-literal::
      :class: highlight

      custom lgetgpio *<pin>*

* The ``<pin>`` argument indicates the GPIO pin number.
  It ranges between ``0`` and the number of GPIO pins supported by the SoC.

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lgetgpio *29*

lsetdcdc - Set DC/DC mode
=========================

It makes the CMD device disable or enable the DC/DC mode.
It has no effects on unsupported boards.

   .. parsed-literal::
      :class: highlight

      custom lsetdcdc *<value>*

* The ``<value>`` argument can assume one of the following values:

  * ``0`` - disabled
  * ``1`` - enabled

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lsetdcdc *1*

lgetdcdc - Get DC/DC mode
=========================

It gets the DC/DC mode of the CMD device.
It is always ``0`` for unsupported boards.

   .. parsed-literal::
      :class: highlight

      custom lgetdcdc

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lgetdcdc

lseticache - Set ICACHE configuration
=====================================

It makes the CMD device disable or enable the ICACHE

   .. parsed-literal::
      :class: highlight

      custom lseticache *<value>*

* The ``<value>`` argument can assume one of the following values:

  * ``0`` - disabled
  * ``1`` - enabled

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lseticache *1*

lgettemp - Read SoC temperature
===============================

It makes the CMD device print the SoC temperature in the format ``<.%02>``.

   .. parsed-literal::
      :class: highlight

      custom lgettemp

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lgettemp

lsleep - Transition radio to sleep mode
=======================================

It makes the CMD device put the radio in sleep mode.

   .. parsed-literal::
      :class: highlight

      custom lsleep

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lsleep

lreceive - Transition radio to receive mode
===========================================

It makes the CMD device put the radio in receive mode.

   .. parsed-literal::
      :class: highlight

      custom lreceive

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lreceive

lreboot - Reboots the device
============================

It reboots the device

   .. parsed-literal::
      :class: highlight

      custom lreboot

See the following example:

   .. parsed-literal::
      :class: highlight

      custom lreboot

Configuration
*************

|config|

FEM support
===========

.. include:: /includes/sample_fem_support.txt

.. note::
   The sample provides support for the *antenna diversity* feature on the nRF52840.
   You can enable the feature setting the :kconfig:`PTT_ANTENNA_DIVERSITY` option as ``enabled``.


Building and running
********************

.. |sample path| replace:: :file:`samples/peripheral/802154_phy_test`

.. include:: /includes/build_and_run.txt

.. note::
   On the |nRF5340DKnoref|, the IEEE 802.15.4 PHY Test Tool is a standalone network sample that does not require any counterpart application sample.
   However, you must still program the application core to boot up the network core, and forward the UART pins to the network core of the CMD device.
   The :ref:`nrf5340_empty_app_core` sample, which does both, is built and programmed automatically by default.

.. _802154_phy_test_testing:

Testing the sample
==================

After programming the sample to your development kit, test it by performing the following steps:

1. Connect the development kit to the computer using a USB cable.
   Use the development kit's programmer USB port (J2).
   The kits are assigned a COM port (in Windows) or a ttyACM device (in Linux), visible in the Device Manager or in the :file:`/dev` directory.
#. |connect_terminal|
#. If the sample is configured to support BOTH modes (the default setting), switch the development kit into CMD mode by sending the following command:

   .. parsed-literal::
      :class: highlight

      custom changemode *1*

#. On the bottom side of your development kit, locate the table describing the GPIO pin assignment to the LEDs.
   Read the numbers of the GPIO pins assigned to LED 1, 2, 3 or 4.
   For example, on the nRF52840DK, the LEDs are controlled by the pins ranging between P0.13 and P0.16.
#. The LEDs on nRF5340DK and nRF52840DK are in the ``sink`` configuration.
   To turn them on, you must set the respective pin's state to low to let the current flow through the LED, using the ``custom lsetgpio <pin> 0`` command, where ``<pin>`` is the number of the pin assigned for selected LED.
   See the following example for how to light up LED 1 on the nRF5340DK:

   .. parsed-literal::
      :class: highlight

      custom lsetgpio *28* 0

If the selected LED lights up, the sample works as expected and is ready for use.

.. _802154_phy_test_testing_board:

.. note::
   The serial communication does not utilize echo, and the timeout for receiving the entire command after receiving its first character is very short.
   To let the device properly receive the commands, use a terminal application that supports *line mode*, or send the entire command using commands like ``echo`` or ``printf``.

Performing radio tests without the serial interface
===================================================

1. Make sure that at least one of the development kits can be set into CMD mode and the other one to the DUT mode.
   The DUT device will not initialize the serial interface.
   The easiest way to achieve this is to flash both devices with the sample configured to support BOTH modes (default setting).
#. Connect both development kits to the computer using a USB cable.
   The kits are assigned a COM port (in Windows) or a ttyACM device (in Linux), visible in the Device Manager or the :file:`/dev` directory.
#. |connect_terminal|
#. If the samples are configured to support BOTH modes (the default setting), switch one of the development kits into CMD mode by sending the following command:

   .. parsed-literal::
      :class: highlight

      custom changemode *1*

#. Run the following command on the development kit running in CMD mode:

   .. parsed-literal::
      :class: highlight

      custom find

#. The development kit running in CMD mode should respond with one of the following 2 responses:

    * ``channel <ch> find <ack>`` - if the CMD device successfully communicates with the DUT device.
    * ``DUT NOT FOUND`` - if it could not exchange packets with the DUT device.

Refer to the :ref:`802154_phy_test_ui` for the complete list of the available commands.
