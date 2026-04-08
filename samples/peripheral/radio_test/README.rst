.. _radio_test:

Radio test (short-range)
########################

.. contents::
   :local:
   :depth: 2

The Radio test sample demonstrates how to configure the 2.4 GHz short-range radio (Bluetooth® LE, IEEE 802.15.4 and proprietary) in a specific mode and then test its performance.
The sample provides a set of predefined commands that allow you to configure the radio in three modes:

* Constant RX or TX carrier
* Modulated TX carrier
* RX or TX sweep

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You can use any one of the development kits listed above.

.. note::
   On nRF5340 DK and nRF7002 DK, the sample is designed to run on the network core and requires the :ref:`nrf5340_remote_shell` running on the application core.
   This sample uses the :ref:`shell_ipc_readme` library to forward shell data through the physical UART interface of the application core.

The sample also requires one of the following testing devices:

  * Another development kit with the same sample.
    See :ref:`radio_test_testing_board`.
  * Another development kit connected to a PC with the `RSSI Viewer app`_ (available in the `nRF Connect for Desktop`_).
    See :ref:`radio_test_testing_rssi`.

.. note::
   You can perform the radio test also using a spectrum analyzer.
   This method of testing is not covered by this documentation.

Front-end module
================

.. include:: /includes/sample_dtm_radio_test_fem.txt

You can configure the front-end module (FEM) transmitted power control, antenna output, and activation delay using the main shell commands of the :ref:`radio_test_ui`.

.. note::
   Each front-end module (FEM) has different capabilities and operating modes, so some commands may not be supported by a specific FEM and those supported may work differently on different FEMs.

Skyworks front-end module
=========================

.. include:: /includes/sample_dtm_radio_test_skyworks.txt

You can configure the Skyworks front-end module (FEM) antenna output and activation delay using the main shell commands of the :ref:`radio_test_ui`.

Overview
********

To run the tests, connect to the development kit through the serial port and send shell commands.
Zephyr's :ref:`zephyr:shell_api` module is used to handle the commands.
At any time during the tests, you can dynamically set the radio parameters, such as output power, bit rate, and channel.
In sweep mode, you can set the time for which the radio scans each channel from one millisecond to 99 milliseconds, in steps of one millisecond.
The sample also allows you to send a data pattern to another development kit.

The sample first enables the high frequency crystal oscillator and configures the shell.
You can then start running commands to set up and control the radio.
See :ref:`radio_test_ui` for a list of available commands.

.. note::
   For the IEEE 802.15.4 mode, the start channel and the end channel must be within the channel range of 11 to 26.
   Use the ``start_channel`` and ``end_channel`` commands to control this setting.


.. _radio_test_ui:

Command Descriptions
********************

User interface
==============
.. list-table:: Main shell commands (in alphabetical order)
   :header-rows: 1

   * - Command
     - Argument
     - Description
   * - cancel
     -
     - Cancel the sweep or the carrier.
   * - data_rate
     - <sub_cmd>
     - Set the data rate.
   * - end_channel
     - <channel>
     - End channel for the sweep (in MHz, as difference from 2400 MHz).
   * - fem
     - <sub_cmd>
     - Set front-end module (FEM) parameters.
   * - output_power
     - <sub_cmd>
     - Output power set.
       If a front-end module is attached and the :option:`CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC` Kconfig option is enabled, it has the same effect as the ``total_output_power`` command.
   * - parameters_print
     -
     - Print current delay, channel, and other parameters.
   * - print_channel_sequence
     -
     - Print the custom sequence set with the set_channel_sequence command
   * - print_rx
     -
     - Print the received RX payload.
   * - set_channel_sequence
     - <sequence of up to 80 channels>
     - Set a custom channel sequence for the start_tx_with_sleep command
   * - set_channel_sequence_hopping_mode
     - <hopping_mode>
     - Set the hopping mode for the channel sequence.
       Sequential (default order) | random <seed>
   * - start_channel
     - <channel>
     - Start channel for the sweep or the channel for the constant carrier (in MHz, as difference from 2400 MHz).
   * - start_duty_cycle_modulated_tx
     - <duty_cycle>
     - Duty cycle as a percentage (two decimal digits, ranging from 01 to 90).
   * - start_rx
     - <packet_num>
     - Start RX (continuous RX mode is used if no argument is provided).
   * - start_rx_sweep
     -
     - Start the RX sweep.
   * - start_tx_carrier
     -
     - Start the TX carrier.
   * - start_tx_modulated_carrier
     - <packet_num>
     - Start the modulated TX carrier (continuous TX mode is used if no argument is provided).
   * - start_tx_sweep
     -
     - Start the TX sweep.
   * - start_tx_sweep_with_sleep
     - <tx_time> (us) <sleep_time> (us)
     - Start TX sweep with a controlled sleep cycle
   * - start_tx_sweep_with_sleep_modulated
     - <tx_time> (us) <sleep_time> (us)
     - Start modulated TX sweep with a controlled sleep cycle
   * - time_on_channel
     - <time>
     - Time on each channel in ms (between 1 and 99).
   * - toggle_dcdc_state
     - <state>
     - Toggle DC/DC converter state.
   * - transmit_pattern
     - <sub_cmd>
     - Set transmission pattern.
   * - total_output_power
     - <tx output power>
     - Set total output power in dBm.
       This value includes SoC output power and front-end module gain.


cancel
======

The ``cancel`` command is used to stop an ongoing TX or RX test.

data_rate
=========

The ``data_rate`` command sets the data rate used for any modulated transmission.

.. list-table:: Accepted data rates for the ``data_rate`` command
   :header-rows: 1

   * - Data rate
     - Description
   * - ``nrf_1Mbit``
     - 1 Mbit/s Nordic proprietary radio mode
   * - ``nrf_2Mbit``
     - 2 Mbit/s Nordic proprietary radio mode
   * - ``nrf_4Mbit_BT06``
     - 4 Mbps Nordic proprietary radio mode (BT=0.6/h=0.5)
   * - ``nrf_4Mbit_BT04``
     - 4 Mbps Nordic proprietary radio mode (BT=0.4/h=0.5)
   * - ``ble_1Mbit``
     - 1 Mbit/s Bluetooth Low Energy
   * - ``ble_2Mbit``
     - 2 Mbit/s Bluetooth Low Energy
   * - ``ble_lr125Kbit``
     - Long range 125 kbit/s TX, 125 kbit/s and 500 kbit/s RX
   * - ``ble_lr500Kbit``
     - Long range 500 kbit/s TX, 125 kbit/s and 500 kbit/s RX
   * - ``ieee802154_250Kbit``
     - IEEE 802.15.4-2006 250 kbit/s

.. note::

   Not all data rates are supported on all platforms.
   What data rates a given platform support is covered at <link_to_device_documentation?>

Usage example
-------------

.. code-block:: console

  uart:~$ data_rate ble_1Mbit

end_channel
===========

The ``end_channel`` command sets the end channel for the TX and RX sweeps.
The channel represents an offset, in MHz, from 2400 MHz.
It must be between ``0`` and ``80``, where ``0`` corresponds to 2400 MHz and ``80`` corresponds to 2480 MHz.

fem
===

The ``fem`` command is used to set front-end module (FEM) parameters.
It is invoked with a sub-command to determine what parameters are being controlled.

.. list-table:: ``fem`` command sub-command list
   :header-rows: 1

   * - Command
     - Description
   * - ``tx_power_control``
     - Set the FEM TX power control to a given value. This value is defined by the FEM in use, and this can only be used when the :ref:`CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC <CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC>` Kconfig option is disabled.
   * - ``antenna``
     - Set the FEM antenna in use.
   * - ``ramp_up_time``
     - Set the FEM ramp up time in microseconds.


output_power
============

The ``output_power`` command sets the output power for all TX commands.
The command also accounts for front-end module if FEM is attached and automatic power control is enabled.

.. list-table:: Accepted power levels for the ``output_power`` command
   :header-rows: 1

   * - Power level
     - Description
   * - pos8dBm
     - TX power: +8 dBm
   * - pos7dBm
     - TX power: +7 dBm
   * - pos6dBm
     - TX power: +6 dBm
   * - pos5dBm
     - TX power: +5 dBm
   * - pos4dBm
     - TX power: +4 dBm
   * - pos3dBm
     - TX power: +3 dBm
   * - pos2dBm
     - TX power: +2 dBm
   * - pos1dBm
     - TX power: +1 dBm
   * - pos0dBm
     - TX power: 0 dBm
   * - neg1dBm
     - TX power: -1 dBm
   * - neg2dBm
     - TX power: -2 dBm
   * - neg3dBm
     - TX power: -3 dBm
   * - neg4dBm
     - TX power: -4 dBm
   * - neg5dBm
     - TX power: -5 dBm
   * - neg6dBm
     - TX power: -6 dBm
   * - neg7dBm
     - TX power: -7 dBm
   * - neg8dBm
     - TX power: -8 dBm
   * - neg9dBm
     - TX power: -9 dBm
   * - neg10dBm
     - TX power: -10 dBm
   * - neg12dBm
     - TX power: -12 dBm
   * - neg14dBm
     - TX power: -14 dBm
   * - neg16dBm
     - TX power: -16 dBm
   * - neg18dBm
     - TX power: -18 dBm
   * - neg20dBm
     - TX power: -20 dBm
   * - neg22dBm
     - TX power: -22 dBm
   * - neg28dBm
     - TX power: -28 dBm
   * - neg40dBm
     - TX power: -40 dBm
   * - neg46dBm
     - TX power: -46 dBm

.. note::

   Not all TX power levels are supported on all platforms.
   What TX power levels a given platform support is covered at <link_to_device_documentation?>

Usage example
-------------

.. code-block:: console

  uart:~$ output_power pos0dBm

parameters_print
================

The ``parameters_print`` command prints the currently configured parameters.

.. list-table:: Parameters printed and command to configure the parameter
   :header-rows: 1

   * - Printed parameter
     - Command to modify
   * - Data rate
     - ``data_rate``
   * - TX power
     - ``output_power``
   * - Transmission pattern
     - ``transmit_pattern``
   * - Start channel
     - ``start_channel``
   * - End channel
     - ``end_channel``
   * - Time on each channel
     - ``time_on_channel``
   * - Duty cycle
     - ``start_duty_cycle_modulated_tx``

print_channel_sequence
======================

The ``print_channel_sequence`` commnd prints the currently configured channel sequence,
configured with the ``set_channel_sequence`` command, as well as how many channels are in the sequence.

print_rx
========

The ``print_rx`` command prints the received RX payload and number of packets received.

set_channel_sequence
====================

The ``set_channel_sequence`` command sets a custom channel sequence for the ``start_tx_with_sleep`` and ``start_tx_sweep_with_sleep_modulated`` command.
The channel sequence can contain up to 80 channels between ``0`` and ``80``.
Each channel represents an offset, in MHz, from 2400 MHz.
They must be between ``0`` and ``80``, where ``0`` corresponds to 2400 MHz and ``80`` corresponds to 2480 MHz.

Usage example
-------------

Configure the channel sequence to only have one channel so transmission repeats on the same frequency.

.. code-block:: console

  uart:~$ set_channel_sequence 10
  uart:~$ print_channel_sequence
  Channel Sequence length: 1
  Channel Sequence: [10]


Configure a new channel sequence with multiple channels.

.. code-block:: console

  uart:~$ set_channel_sequence 0 2 4 6 8 10
  uart:~$ print_channel_sequence
  Channel Sequence length: 6
  Channel Sequence: [0, 2, 4, 6, 8, 10]


Configure a channel sequence that is not strictly ascending with multiple channels.

.. code-block:: console

  uart:~$ set_channel_sequence 0 2 4 6 8 10 8 6 4 2
  uart:~$ print_channel_sequence
  Channel Sequence length: 10
  Channel Sequence: [0, 2, 4, 6, 8, 10, 8, 6, 4, 2]


.. note::

   Using the print_channel_sequence is not required, but it makes verifying the configured channel sequence easier.

start_channel
=============

The ``start_channel`` command sets the start channel for the ``start_rx_sweep`` and ``start_tx_sweep`` commands.
The command also configures the transmission channel for the ``start_rx``, ``start_tx_carrier`` and ``start_tx_modulated_carrier`` commands.

The channel represents an offset, in MHz, from 2400 MHz.
It must be between ``0`` and ``80``, where ``0`` corresponds to 2400 MHz and ``80`` corresponds to 2480 MHz.

start_duty_cycle_modulated_tx
=============================

The ``start_duty_cycle_modulated_tx`` command starts a modulated carrier with a configurable duty cycle.
The data rate and transmission pattern are controlled with the ``data_rate`` and ``transmit_pattern`` commands, respectively,
while the duty cycle is controlled when invoking the ``start_duty_cycle_modulated_tx`` as a value between 1 and 90 as a percentage.

Usage example
-------------

To configure the device to transmit on ``ble_1Mbit`` with a random transmit pattern and a 50% duty cycle, use the following command:

.. code-block:: console

  uart:~$ data_rate ble_1Mbit
  uart:~$ transmit_pattern pattern_random
  uart:~$ start_duty_cycle_modulated_tx 50

start_rx
========

The ``start_rx`` starts an RX reception window.
With additional configuration during invocation, the RX window expects a specified number of packets.
Without configuration, the device enters a continuous RX reception mode.

Usage example
-------------

To configure the device to receive a specified number of packets, here 50, use the following command:

.. code-block:: console

  uart:~$ start_rx 50

To configure the radio to enter continuous RX mode, use the following command:

.. code-block:: console

  uart:~$ start_rx

start_rx_sweep
==============

The ``start_rx_sweep`` command starts an RX sweep.
The RX sweep starts at the channel configured with ``start_channel`` and increments by 1 MHz until it reaches the end channel configured with ``end_channel``.
When the sweep reaches the end channel, it restarts at the start channel.
The RX sweep remains on each channel for the number of milliseconds specified by ``time_on_channel``.

Usage example
-------------

To configure an RX sweep starting at 2400 MHz and ending at 2480 MHz, with 10 ms on each channel, use the following command:

.. code-block:: console

  uart:~$ start_channel 0
  uart:~$ end_channel 80
  uart:~$ time_on_channel 10
  uart:~$ start_rx_sweep

start_tx_carrier
================

The ``start_tx_carrier`` command starts a continuous, unmodulated TX carrier on the channel configured with ``start_channel``.

start_tx_modulated_carrier
==========================

The ``start_tx_modulated_carrier`` command starts a modulated TX carrier on the channel configured with ``start_channel``.
If no argument is given, the TX carrier transmits continuously.
If an argument is given, the TX carrier transmits the specified number of packets.
The data rate and transmission pattern are controlled using the ``data_rate`` and ``transmit_pattern`` commands, respectively.

Usage example
-------------

To configure a continuous modulated TX carrier at 2400 MHz on the ``ble_2Mbit`` PHY with a random transmission pattern, use the following commands:

.. code-block:: console

  uart:~$ start_channel 0
  uart:~$ data_rate ble_2Mbit
  uart:~$ transmit_pattern pattern_random
  uart:~$ start_tx_modulated_carrier

To configure a modulated TX carrier at 2440 MHz on the ``ble_1Mbit`` PHY with a random transmission pattern that sends 50 packets, use the following commands:

.. code-block:: console

  uart:~$ start_channel 40
  uart:~$ data_rate ble_1Mbit
  uart:~$ transmit_pattern pattern_random
  uart:~$ start_tx_modulated_carrier 50

start_tx_sweep
==============

The ``start_tx_sweep`` command starts a TX sweep.
The TX sweep starts at the channel configured with ``start_channel`` and increments by 1 MHz until it reaches the end channel configured with ``end_channel``.
When the sweep reaches the end channel, it restarts at the start channel.
The TX sweep remains on each channel for the number of milliseconds configured with ``time_on_channel``.

Usage example
-------------

To configure a a TX sweep starting at 2400 MHz and ending at 2480 MHz, with 10 ms on each channel, use the following commands:

.. code-block:: console

  uart:~$ start_channel 0
  uart:~$ end_channel 80
  uart:~$ time_on_channel 10
  uart:~$ start_tx_sweep

start_tx_sweep_with_sleep
=========================

The ``start_tx_sweep_with_sleep`` command starts a TX sweep with a configurable sleep time.
The channel sequence used for the sweep is configured with the ``set_channel_sequence`` command.
The sweep will start at the first channel in the channel sequence and iterates through the sequence until it reaches the last channel in the sequence.
When the sweep reaches the last channel in the sequence, it restarts at he start of the sequence.
The time spent on each channel is configured by setting the TX time and sleep time.
This is done when invoking the ``start_tx_sweep_with_sleep`` command providing the TX time and sleep time in microseconds.

.. note::
   The default TX sweep with sleep starts at 2404 MHz and increments by 1 MHz up to 2478 MHz.
   When it reaches 2478 MHz, it restarts at 2404 MHz.
   It does not transmit on 2425, 2426, and 2427 MHz.
   This is done to avoid the standard BLE advertising channels.

Usage example
-------------

To configure a TX sweep with an 80 µs TX window and a 160 µs sleep interval on each channel, use the following command.
This configuration results in a sweep that spends 240 µs on each channel, corresponding to a 33.33% duty cycle.

.. code-block:: console

  uart:~$ start_tx_sweep_with_sleep 80 160

To configure a TX sweep that uses every other channel with a 80 µs TX window and a 240 µs sleep interval on channel, use the following command.
This configuration results in a sweep that spends 320 µs on each channel, corresponding to a 25% duty cycle.

.. code-block:: console

  uart:~$ set_channel_sequence 0 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 32 34 36 38 40 42 44 46 48 50 52 54 56 58 60 62 64 66 68 70 72 74 76 78 80
  uart:~$ start_tx_sweep_with_sleep 80 240

start_tx_sweep_with_sleep_modulated
===================================

The ``start_tx_sweep_with_sleep_modulated`` command starts a modulated TX sweep with a configurable sleep time.
The channel sequence used for the sweep is configured with the ``set_channel_sequence`` command.
The sweep will start at the first channel in the channel sequence and iterates through the sequence until it reaches the last channel in the sequence.
When the sweep reaches the last channel in the sequence, it restarts at he start of the sequence.
The time spent on each channel is configured by setting the TX time and sleep time.
This is done when invoking the ``start_tx_sweep_with_sleep`` command providing the TX time and sleep time in microseconds.

.. note::
   The default TX sweep with sleep starts at 2404 MHz and increments by 1 MHz up to 2478 MHz.
   When it reaches 2478 MHz, it restarts at 2404 MHz.
   It does not transmit on 2425, 2426, and 2427 MHz.
   This is done to avoid the standard BLE advertising channels.

Usage example
-------------

To configure a TX sweep with an 80 µs TX window and a 160 µs sleep interval on each channel, use the following command.
This configuration results in a sweep that spends 240 µs on each channel, corresponding to a 33.33% duty cycle.

.. code-block:: console

  uart:~$ start_tx_sweep_with_sleep_modulated 80 160

To configure a TX sweep that uses every other channel with a 80 µs TX window and a 240 µs sleep interval on channel, use the following command.
This configuration results in a sweep that spends 320 µs on each channel, corresponding to a 25% duty cycle.

.. code-block:: console

  uart:~$ set_channel_sequence 0 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 32 34 36 38 40 42 44 46 48 50 52 54 56 58 60 62 64 66 68 70 72 74 76 78 80
  uart:~$ start_tx_sweep_with_sleep_modulated 80 240

time_on_channel
===============

The ``time_on_channel`` command configures the time, in milliseconds, on each channel for standard TX and RX sweeps

toggle_dcdc_state
=================

The ``toggle_dcdc_state`` command is used to toggle the DC/DC state.
This command functions differently if ``NRF_POWER_HAS_DCDCEN_VDDH`` or ``NRF_POWER_HAS_DCDCEN`` is set.

Usage example
-------------

To toggle the DC/DC state when ``NRF_POWER_HAS_DCDCEN_VDDH`` is set, use the following command.

.. code-block:: console

   uart:~$ toggle_dcdc_state 1

To toggle the DC/DC VDDH state when ``NRF_POWER_HAS_DCDCEN_VDDH`` is set, use the following command.

.. code-block:: console

   uart:~$ toggle_dcdc_state 0

To toggle the DC/DC state when ``NRF_POWER_HAS_DCDCEN`` is set, use the following command.

.. code-block:: console

   uart:~$ toggle_dcdc_state


transmit_pattern
================

The ``transmit_pattern`` command is used to configure the transmission pattern for modulated transmissions.

.. list-table:: Selectable transmit patterns
   :header-rows: 1

   * - Transmit pattern
     - Description
   * - pattern_random
     - Transmitt a random sequence
   * - pattern_11110000
     - Transmit 11110000 repeating
   * - pattern_11001100
     - Transmit 11001100 repeating

total_output_power
==================

The ``total_output_power`` command sets the total output power of the device in dBm.
This command accounts for any FEM module gain as well as the SoC output power.

Usage example
-------------

To configure the total ouput power to 10 dBm, use the following command.

.. code-block:: console

  uart:~$ total_output_power 10

TX output power
===============

This sample has a few commands that you can use to test the device output power.
The behavior of the commands vary depending on the hardware configuration and Kconfig options as follows:

* Radio Test without front-end module support:

  * The ``output_power`` command sets the SoC output command with a subcommand set.
    The output power is set directly in the radio peripheral.

* Radio Test with front-end module support in default configuration (the :option:`CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC` Kconfig option is enabled):

  * The ``output_power`` command sets the total output power, including front-end module gain.
  * The ``total_output_power`` command sets the total output power, including front-end module gain with a value in dBm unit provided by user.
  * For these commands, the radio peripheral and FEM transmit power control is calculated and set automatically to meet your requirements.
  * If an exact output power value cannot be set, a lower value is used.

* Radio Test with front-end module support and manual TX output power control (the :option:`CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC` Kconfig option is disabled):

  * The ``output_power`` command sets the SoC output command with a subcommands set.
  * The ``fem`` command with the ``tx_power_control`` subcommand sets the front-end module transmit power control to a value for given specific front-end module.
  * You can use this configuration to perform tests on your hardware design.

Configuration
*************

|config|

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/peripheral/radio_test/Kconfig`) :

.. options-from-kconfig::
   :show-type:

Building and running
********************

.. |sample path| replace:: :file:`samples/peripheral/radio_test`

.. include:: /includes/build_and_run.txt

.. note::
   On the nRF5340 or nRF7002 development kit, the Radio Test sample requires the :ref:`nrf5340_remote_shell` sample on the application core.
   The Remote IPC shell sample is built and programmed automatically by default.

Pin debugging of RADIO events
================================

The sample can be built with an extra overlay file to enable pin debugging of RADIO events for nR54L series devices with the following command:

.. code-block:: console

   west build -bnrf54l15dk/nrf54l15/cpuapp -p -- -DEXTRA_DTC_OVERLAY_FILE=pin_debug_54l.overlay


With pin debugging enabled two GPIOs will be configured to toggle on RADIO events:
- One pin is set high on the ``RADIO->EVENTS_READY`` and low on the ``RADIO->EVENTS_DISABLED``.
- One pin is set high on the ``RADIO->EVENTS_ADDRESS`` and low on the ``RADIO->EVENTS_END``.

The pins used for debugging are configured in :file:`samples/peripheral/radio_test/pin_debug_54l.overlay`.


Remote USB CDC ACM Shell variant
================================

This sample can run the remote IPC Service Shell through the USB on the nRF5340 DK application core.
For example, when building on the command line, use the following command:

.. code-block:: console

   west build samples/peripheral/radio_test -b nrf5340dk/nrf5340/cpunet -- -DFILE_SUFFIX=usb

You can also build this sample with the remote IPC Service Shell and support for the front-end module.
You can use the following command:

.. code-block:: console

   west build samples/peripheral/radio_test -b nrf5340dk/nrf5340/cpunet -- -DSHIELD=nrf21540ek -DFILE_SUFFIX=usb

.. note::
   You can also build the sample with the remote IPC Service Shell for the |nRF7002DKnoref| using the ``nrf7002dk/nrf5340/cpunet`` board target in the commands.

.. _radio_test_testing:

Testing
=======

After programming the sample to your development kit, complete the following steps to test it in one of the following two ways:

.. note::
   For the |nRF5340DKnoref| or |nRF7002DKnoref|, see :ref:`logging_cpunet` for information about the COM terminals on which the logging output is available.

.. _radio_test_testing_board:

Testing with another development kit
------------------------------------

Complete the following steps:

1. Connect both development kits to the computer using a USB cable.
   The kits are assigned serial ports.
   |serial_port_number_list|
#. |connect_terminal_both_ANSI|
#. Run the following commands on one of the kits:

   a. Set the data rate with the ``data_rate`` command to ``ble_2Mbit``.
   #. Set the transmission pattern with the ``transmit_pattern`` command to ``pattern_11110000``.
   #. Set the radio channel with the ``start_channel`` command to 40.

#. Repeat all steps for the second kit.
#. On both kits, run the ``parameters_print`` command to confirm that the radio configuration is the same on both kits.
#. Set one kit in the Modulated TX Carrier mode using the ``start_tx_modulated_carrier`` command.
#. Set the other kit in the RX Carrier mode using the ``start_rx`` command.
#. Print the received data with the ``print_rx`` command and confirm that they match the transmission pattern (0xF0).

.. _radio_test_testing_rssi:

Testing with the RSSI Viewer app
--------------------------------

Complete the following steps:

1. Connect the kit to the computer using a USB cable.
   The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal_ANSI|
#. Set the start channel with the ``start_channel`` command to 20.
#. Set the end channel with the ``end_channel`` command to 60.
#. Set the time on channel with the ``time_on_channel`` command to 50 ms.
#. Set the kit in the TX sweep mode using the ``start_tx_sweep`` command.
#. Start the `RSSI Viewer app`_ and select the kit to communicate with.
#. On the application chart, observe the TX sweep in the form of a wave that starts at 2420 MHz frequency and ends with 2480 MHz.

Dependencies
************

This sample uses the following |NCS| libraries:

  * :ref:`shell_ipc_readme`
  * :ref:`fem_al_lib`

This sample has the following nrfx dependencies:

  * :file:`nrfx/drivers/include/nrfx_timer.h`
  * :file:`nrfx/hal/nrf_power.h`
  * :file:`nrfx/hal/nrf_radio.h`

The sample also has the following nrfxlib dependency:

  * :ref:`nrfxlib:mpsl_fem`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:device_model_api`:

   * :file:`drivers/clock_control.h`

* :ref:`zephyr:kernel_api`:

  * :file:`include/init.h`

* :ref:`zephyr:shell_api`:

  * :file:`include/shell/shell.h`
