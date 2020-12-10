.. _direct_test_mode:

Bluetooth: Direct Test Mode
###########################

.. contents::
   :local:
   :depth: 2

This sample enables the Direct Test Mode functions described in `Bluetooth Core Specification`_: Version 5.2, Vol. 6, Part F.

Overview
********

The sample uses Direct Test Mode (DTM) to test the operation of the following features of the radio:

* Transmission power and receiver sensitivity
* Frequency offset and drift
* Modulation characteristics
* Packet error rate
* Intermodulation performance

Test procedures are defined in the document `Bluetooth Low Energy RF PHY Test Specification`_: Document number RF-PHY.TS.p15

You can carry out conformance tests using dedicated test equipment, such as the Anritsu MT8852 or similar, with an nRF5 running the DTM sample set as device under test (DUT).

DTM sample
==========

The DTM sample includes two parts:

* The DTM, Direct Test Mode library, which manages the nRF radio and controls the standard DTM procedures.
* A sample that provides an external interface to the library.

You can find the source code of both parts here: :file:`samples/bluetooth/direct_test_mode/src`.

The DTM sample contains a driver for a 2-wire UART interface.
The driver maps two-octet commands and events to the DTM library, as specified by the Bluetooth Low Energy DTM specification.

.. figure:: /images/bt_dtm_dut.svg
   :alt: nRF5 with DTM as a DUT

The implementation is self-contained and requires no Bluetooth Low Energy protocol stack for its operation.
The MPU is initialized in the standard way.
The DTM library function ``dtm_init`` configures all interrupts, timers, and the radio.

``main.c`` may be replaced with other interface implementations, such as an HCI interface, USB, or another interface required by the Upper Tester.

The interface to the Lower Tester uses the antenna connector of the chosen development kit.
While in principle an aerial connection might be used, conformance tests cover the reading of the transmission power delivered by the DUT.
For this reason, a coaxial connection between the DUT and the Lower Tester is employed for all conformance testing.

DTM module interface
====================

The DTM function ``dtm_cmd_put`` implements the four commands defined by the Bluetooth Low Energy standard:

* ``TEST SETUP`` (called ``RESET`` in Bluetooth 4.0)
* ``RECEIVER_TEST``
* ``TRANSMITTER_TEST``
* ``TEST_END``

In the ``dtm_cmd_put`` interface, DTM commands are accepted in the 2-byte format.
Parameters such as: CMD code, Frequency, Length, or Packet Type are encoded within this command.

The following DTM events are polled using the ``dtm_event_get`` function:

* ``PACKET_REPORTING_EVENT``
* ``TEST_STATUS_EVENT`` [ ``SUCCESS`` | ``FAIL`` ]

.. figure:: /images/bt_dtm_engine.svg
   :alt: State machine overview of the DTM

Supported PHYs
==============

The DTM sample supports all four PHYs specified in DTM, but not all devices support all the PHYs.

.. list-table:: Supported PHYs
   :header-rows: 1

   * - PHY
     - nRF5340
   * - LE 1M
     - Yes
   * - LE 2M
     - Yes
   * - LE Coded S=8
     - Yes
   * - LE Coded S=2
     - Yes

Bluetooth Direction Finding support
===================================

The DTM sample supports all Bluetooth Direction Finding modes specified in DTM.

.. list-table:: Supported Bluetooth Direction Finding modes
   :header-rows: 1

   * - Direction Finding mode
     - nRF5340
   * - AoD 1 us slot
     - Yes
   * - AoD 2 us slot
     - Yes
   * - AoA
     - Yes

The following antenna switching patterns are possible:

* 1, 2, 3, ..., N
* 1, 2, 3, ..., N, N - 1, N - 2, ..., 1

The application supports a maximum of 19 antennas in the direction finding mode.
The RADIO can control up to 8 GPIO pins for the purpose of controlling the external antenna switches used in direction finding.

The antenna is chosen by writing consecutive numbers to the SWITCHPATTERN register.
This means that the antenna GPIO pins act like 8-bit registers.
In other words, for the first antenna, antenna pin 1 is active, for the second antenna, pin 2 is active, for the third antenna, pins 1 and 2 are active, and so on.

nRF21540 front-end module
=========================

.. |fem_file_path| replace:: :file:`samples/bluetooth/direct_test_mode`

.. include:: /includes/sample_fem_support.txt

You can configure the transmitted power gain and activation delay in nRF21540 using vendor-specific commands, see `Vendor-specific packet payload`_.

Vendor-specific packet payload
==============================

The Bluetooth Low Energy 2-wire UART DTM interface standard reserves the Packet Type, also called payload parameter, with binary value ``11`` for a Vendor Specific packet payload.

The DTM command is interpreted as a Vendor-Specific one when both the following conditions are met:

* Its CMD field is set to Transmitter Test, binary ``10``.
* Its PKT field is set to Vendor-Specific, binary ``11``.

Vendor specific commands can be divided into different categories as follows:

* If the Length field is set to ``0`` (symbol ``CARRIER_TEST``), an unmodulated carrier is turned on at the channel indicated by the Frequency field.
  It remains turned on until a ``TEST_END`` or ``RESET`` command is issued.
* If the Length field is set to ``1`` (symbol ``CARRIER_TEST_STUDIO``), this field value is used by the nRFgo Studio to indicate that an unmodulated carrier is turned on at the channel.
  It remains turned on until a ``TEST_END`` or ``RESET`` command is issued.
* If the Length field is set ``2`` (symbol ``SET_TX_POWER``), the Frequency field sets the TX power in dBm.
  The valid TX power values are specified in the product specification ranging from -40 to +4.
  0 dBm is the reset value.
  Only the 6 least significant bits will fit in the Length field.
  The two most significant bits are calculated by the DTM module.
  This is possible because the 6 least significant bits of all valid TX power values are unique.
  The TX power can be modified only when no Transmitter Test or Receiver Test is running.
* If the Length field is set to ``3``(symbol ``NRF21540_ANTENNA_SELECT``), the Frequency field sets the nRF21540 FEM antenna.
  The valid values are:

     * 0 - ANT1 enabled, ANT2 disabled
     * 1 - ANT1 disabled, ANT2 enabled

* If the Length field is set to ``4`` (symbol ``NRF21540_GAIN_SET``), the Frequency field sets the nRF21540 FEM TX gain value in arbitrary units.
  The valid gain values are specified in the nRF21540 product-specific range from 0 to 31.
* If the Length field is set to ``5`` (symbol ``NRF21540_ACTIVE_DELAY_SET``), the Frequency field sets the nRF21540 FEM activation delay in microseconds relative to a radio start.
  By default, this value is set to (radio ramp-up time - nRF21540 TX/RX settling time).
* All other values of Frequency and Length field are reserved.

The DTM-to-Serial adaptation layer
==================================

``main.c`` is an implementation of the UART interface specified in the `Bluetooth Core Specification`_: Vol. 6, Part F, Chap. 3.

The default selection of UART pins is defined in ``zephyr/boards/arm/board_name/board_name.dts``.
You can change the defaults using the symbols ``tx-pin`` and ``rx-pin`` in the DTS overlay file at the project level.

Debugging
*********

In this sample, the UART console is used to exchange commands and events defined in the DTM specification.
Debug messages are not displayed in this UART console.
Instead, they are printed by the RTT logger.

If you want to view the debug messages, follow the procedure in :ref:`testing_rtt_connect`.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpunet, nrf21540dk_nrf52840

Additionally, the sample requires one of the following testing devices:

  * Dedicated test equipment, like an Anritsu MT8852 tester.
    See :ref:`direct_test_mode_testing_anritsu`.
  * Another development kit with the same sample.
    See :ref:`direct_test_mode_testing_board`.
  * Another development kit connected to a PC with the Direct Test Mode sample available in the `nRF Connect for Desktop`_.
    See :ref:`direct_test_mode_testing_app`.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/direct_test_mode`

.. include:: /includes/build_and_run.txt

.. note::
   On the nRF5340 development kit, the Direct Test Mode sample is a standalone network sample.
   It does not require any counterpart application sample.
   However, you must still program the application core to boot up the network core.
   You can use any sample for this, for example, the :ref:`nrf5340_empty_app_core`.

The Bluetooth Low Energy DTM UART interface standard specifies:

* 8 data bits
* No parity
* 1 stop bit
* No hardware flow control
* A selection of bit rates from 9600 to 1000000, one of which must be supported by the DUT.
  It might be possible to run other bit rates by experimenting with parameters.

The default bit rate of the DTM UART driver is 19200 bps, which is supported by most certified testers.

Testing
=======

After programming the sample to your development kit, you can test it in the three following ways.

.. note::
   For the |nRF5340DKnoref|, see :ref:`logging_cpunet` for information about the COM terminals on which the logging output is available.

.. _direct_test_mode_testing_anritsu:

Testing with a certified tester
-------------------------------

Conformance testing is done using a certified tester.
The setup depends on the tester used, and details about the test operation must be found in the tester documentation.

Application note  `nAN34`_ describes two alternatives for setting up a production test with DTM using one of our old devices.

.. _direct_test_mode_testing_board:

Testing with another development kit
------------------------------------

1. Connect both development kits to the computer using a USB cable.
   The computer assigns to the development kit a COM port on Windows or a ttyACM device on Linux, which is visible in the Device Manager.
#. |connect_terminal_both|
#. Start ``TRANSMITTER_TEST`` by sending the ``0x80 0x96`` DTM command to one of the connected development kits.
   This command will trigger TX activity on the 2402 MHz frequency (1st channel) with ``10101010`` packet pattern and 37-byte packet length.
#. Observe that you received the ``TEST_STATUS_EVENT`` packet in response with the SUCCESS status field: ``0x00 0x00``.
#. Start ``RECEIVER_TEST`` by sending the ``0x40 0x96`` DTM command to the second development kit.
   Command parameters are identical to the ones used for the ``TRANSMITTER_TEST`` command.
#. Observe that you received the ``TEST_STATUS_EVENT`` packet in response with the SUCCESS status field: ``0x00 0x00``.
#. Finish RX testing using the ``TEST_END DTM`` command by sending the ``0xC0 0x00`` packet.
#. Observe that you received the ``PACKET_REPORTING_EVENT`` packet in response.
   For example, the ``0xD6 0xAC`` message indicates that 22188 Radio packets have been received.
#. Experiment with other combinations of commands and their parameters.

.. _direct_test_mode_testing_app:

Testing with nRF Connect for Desktop
------------------------------------

1. Connect the development kit to the computer using a USB cable.
   The computer assigns to the development kit a COM port on Windows or a ttyACM device on Linux, which is visible in the Device Manager.
#. |connect_terminal|
#. Start the ``TRANSMITTER_TEST`` by sending the ``0x80 0x96`` DTM command to the connected development kit.
   This command triggers TX activity on 2402 MHz frequency (1st channel) with ``10101010`` packet pattern and 37-byte packet length.
#. Observe that you received the ``TEST_STATUS_EVENT`` packet in response with the SUCCESS status field: ``0x00 0x00``.
#. Start the Direct Test Mode application in nRF Connect for Desktop and select the development kits to communicate with.
#. Set the Receiver mode and 37th channel in the test configuration menu.
#. Start the test.
#. On the application chart, observe that the number of RX packets is increasing for the 2402 MHz channel.
#. Stop the test.
#. Swap roles.
   Set the application to the RX mode and the connected development kit to the TX mode.

Dependencies
************

This sample uses the following nrfx dependencies:

  * ``nrfx/drivers/include/nrfx_timer.h``
  * ``nrfx/hal/nrf_nvmc.h``
  * ``nrfx/hal/nrf_radio.h``
  * ``nrfx/helpers/nrfx_gppi.h``

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:device_model_api`:

   * ``drivers/clock_control.h``
   * ``drivers/uart.h``
