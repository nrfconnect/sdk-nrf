.. _wifi_radio_test_sd:

Wi-Fi: Wi-Fi Bluetooth LE Radio test (Single domain)
####################################################

.. contents::
   :local:
   :depth: 2

The Wi-Fi Bluetooth LE Radio test (Single domain) sample demonstrates how to use the radio test for both Wi-Fi® and Bluetooth® LE protocols using a single domain image, that is, both running on the same core (application core).
The sample shows how to use the radio test subcommands to configure the parameters and run the radio test.

The sample also supports programming the Factory Information Configuration Registers (FICR) fields defined in the nRF7002 one-time programmable (OTP) memory.

The sample demonstrates how to configure the Wi-Fi radio in a specific mode and then test its performance.
It provides a set of predefined commands that allow you to configure the radio in the following modes:

* Modulated carrier TX
* Modulated carrier RX
* Tone transmission
* IQ sample capture at ADC output

The sample also shows how to program the user region of FICR parameters on the development kit using a set of predefined commands.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

To run the tests, connect to the development kit through the serial port and send shell commands.
Zephyr's :ref:`zephyr:shell_api` module is used to handle the commands.

You can start running ``wifi_radio_test`` subcommands to set up and control the radio.
See :ref:`wifi_radio_test_subcmds` for a list of available subcommands.

In the Modulated carrier RX mode, you can use the ``get_stats`` subcommand to display the statistics.
See :ref:`wifi_radio_test_stats` for a list of available statistics.

You can use ``wifi_radio_ficr_prog`` subcommands to read or write OTP registers.
See :ref:`wifi_radio_ficr_prog_subcmds` for a list of available subcommands.

.. note::

   All the FICR registers are stored in the one-time programmable (OTP) memory.
   Consequently, the write commands are destructive. Once written, the contents of the OTP registers cannot be reprogrammed.

.. _wifi_radio_sd_sample_building_and_running:

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/radio_test/single_domain`

.. include:: /includes/build_and_run.txt

Currently, the following configurations are supported:

* nRF7002 DK + QSPI
* nRF7002 EK + SPIM

To build for the nRF7002 DK, use the ``nrf7002dk/nrf5340/cpuapp`` board target.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk/nrf5340/cpuapp

To build for the nRF7002 EK and nRF5340 DK, use the ``nrf5340dk/nrf5340/cpuapp`` board target with the ``SHIELD`` CMake option set to ``nrf7002ek``.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf5340dk/nrf5340/cpuapp -- -DSHIELD=nrf7002ek

See also :ref:`cmake_options` for instructions on how to provide CMake options.

See the below links for detailed documentation on the sample and its features.

* Testing : :ref:`wifi_radio_test_testing`
* Peripheral radio test: :ref:`radio_test`
* Radio test subcommands: :ref:`wifi_radio_subcommands`
* FICR programming subcommands: :ref:`wifi_ficr_prog`
