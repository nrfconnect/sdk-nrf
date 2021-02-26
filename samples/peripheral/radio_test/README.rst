.. _radio_test:

Radio test
##########

.. contents::
   :local:
   :depth: 2

The Radio test sample demonstrates how to configure the radio in a specific mode and then test its performance.
The sample provides a set of predefined commands that allow you to configure the radio in three modes:

* Constant RX or TX carrier
* Modulated TX carrier
* RX or TX sweep

Overview
********

You can run the tests by connecting to the development kit through the serial port and sending shell commands.
Zephyr's :ref:`zephyr:shell_api` module is used to handle the commands.
At any time during the tests, you can dynamically set the radio parameters, such as output power, bit rate, and channel.
In sweep mode, you can set the time for which the radio scans each channel from 1 millisecond to 99 milliseconds, in steps of 1 millisecond.
The sample also allows you to send a data pattern to another development kit.

The sample starts with enabling the high frequency crystal oscillator and configuring the shell.
You can then start running commands to set up and control the radio.
See :ref:`radio_test_ui` for a list of available commands.

.. note::
   For the IEEE 802.15.4 mode, the start channel and the end channel must be within the channel range of 11 to 26.
   Use the ``start_channel`` and ``end_channel`` commands to control this setting.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpunet, nrf52840dk_nrf52840, nrf52dk_nrf52832, nrf21540dk_nrf52840

You can use any one of the development kits listed above.

.. note::
   On nRF5340 DK, the sample is designed to run on the network core.

The sample also requires one of the following testing devices:

  * Another development kit with the same sample.
    See :ref:`radio_test_testing_board`.
  * Another development kit connected to a PC with RSSI Viewer application (available in the `nRF Connect for Desktop`_).
    See :ref:`radio_test_testing_rssi`.

.. note::
   You can perform the radio test also using a spectrum analyzer.
   This method of testing is not covered by this documentation.

nRF21540 front-end module
=========================

.. |fem_file_path| replace:: :file:`samples/peripheral/radio_test`

.. include:: /includes/sample_fem_support.txt

The nRF21540 transmitted power gain, antenna and an activation delay can be configured using the user interface :ref:`radio_test_ui`.

.. _radio_test_ui:

User interface
**************
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
     - End the channel for the sweep.
   * - nRF21540
     - <sub_cmd>
     - Set nRF21540 Front-End-Module parameters.
   * - output_power
     - <sub_cmd>
     - Output power set.
   * - parameters_print
     -
     - Print current delay, channel, and other parameters.
   * - print_rx
     -
     - Print the received RX payload.
   * - start_channel
     - <channel>
     - Start the channel for the sweep or the channel for the constant carrier.
   * - start_duty_cycle_modulated_tx
     - <duty_cycle>
     - Duty cycle in percent (two decimal digits, between 01 and 99).
   * - start_rx
     -
     - Start RX.
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
   * - time_on_channel
     - <time>
     - Time on each channel (between 1 ms and 99 ms).
   * - toggle_dcdc_state
     - <state>
     - Toggle DC/DC converter state.
   * - transmit_pattern
     - <sub_cmd>
     - Set transmission pattern.

Building and running
********************
.. |sample path| replace:: :file:`samples/peripheral/radio_test`

.. include:: /includes/build_and_run.txt

.. note::
   On the |nRF5340DKnoref|, the Radio test sample is a standalone network sample that does not require any counterpart application sample.
   However, you must still program the application core to boot up the network core.
   You can use any sample for this, for example :ref:`nrf5340_empty_app_core`.

.. _radio_test_testing:

Testing
=======

After programming the sample to your development kit, you can test it in one of two ways.

.. note::
   For the |nRF5340DKnoref|, see :ref:`logging_cpunet` for information about the COM terminals on which the logging output is available.

.. _radio_test_testing_board:

Testing with another development kit
------------------------------------

1. Connect both development kits to the computer using a USB cable.
   The kits are assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal_both|
#. Run the following commands on one of the kits:
   #. Set the data rate with the ``data_rate`` command to ``ble_2Mbit``.
   #. Set the transmission pattern with the ``transmit_pattern`` command to ``pattern_11110000``.
   #. Set the radio channel with the ``start_channel`` command to 40.
#. Repeat all steps for the second kit.
#. On both kits, run the ``parameters_print`` command to confirm that the radio configuration is the same on both kits.
#. Set one kit in the Modulated TX Carrier mode using the ``start_tx_modulated_carrier`` command.
#. Set the other kit in the RX Carrier mode using the ``start_rx`` command.
#. Print the received data with the ``print_rx`` command and confirm that they match the transmission pattern (0xF0).

.. _radio_test_testing_rssi:

Testing with RSSI Viewer
------------------------

1. Connect the kit to the computer using a USB cable.
   The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal|
#. Set the start channel with the ``start_channel`` command to 20.
#. Set the end channel with the ``end_channel`` command to 60.
#. Set the time on channel with the ``time_on_channel`` command to 50ms.
#. Set the kit in the TX sweep mode using the ``start_tx_sweep`` command.
#. Start the RSSI Viewer application and select the kit to communicate with.
#. On the application chart, observe the TX sweep in the form of a wave that starts at 2420 MHz frequency and ends with 2480MHz.

Dependencies
************

This sample uses the following nrfx dependencies:

  * ``nrfx/drivers/include/nrfx_timer.h``
  * ``nrfx/hal/nrf_nvmc.h``
  * ``nrfx/hal/nrf_power.h``
  * ``nrfx/hal/nrf_radio.h``
  * ``nrfx/hal/nrf_rng.h``

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:device_model_api`:

   * ``drivers/clock_control.h``

* :ref:`zephyr:kernel_api`:

  * ``include/init.h``

* :ref:`zephyr:shell_api`:

  * ``include/shell/shell.h``
