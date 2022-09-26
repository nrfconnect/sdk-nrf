.. _wifi_radio_sample_desc:

Sample description
##################

.. contents::
   :local:
   :depth: 2

The Wi-Fi Radio test sample demonstrates how to configure the Wi-Fi radio in a specific mode and then test its performance.
The sample provides a set of predefined commands that allow you to configure the radio in the following modes:

* Modulated carrier TX
* Modulated carrier RX

The sample also shows how to program the user region of FICR parameters on the development kit using a set of predefined commands.

Requirements
************

The sample supports the following development kit:

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
   Consequently, the write commands are destructive, and once written, the contents of the OTP registers cannot be reprogrammed.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/radio_test`

.. include:: /includes/build_and_run.txt

Currently, the following configurations are supported:

* 7002 DK + QSPI
* 7002 EK + SPIM


To build for the nRF7002 DK, use the ``nrf7002dk_nrf5340_cpuapp`` build target.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk_nrf5340_cpuapp

To build for the nRF7002 EK and nRF5340 DK, use the ``nrf5340dk_nrf5340_cpuapp`` build target with the ``SHIELD`` CMake option set to ``nrf7002_ek``.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf5340dk_nrf5340_cpuapp -- -DSHIELD=nrf7002_ek

See also :ref:`cmake_options` for instructions on how to provide CMake options.

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Test the sample either by configuring the Wi-Fi radio or by programming the FICR parameters:

   .. tabs::

      .. group-tab:: Configuring the Wi-Fi radio

         * To display the current configuration, use the following command:

           .. code-block:: console

              wifi_radio_test show_config

           The sample shows the following output:

           .. code-block:: console

              ************* Configured Parameters ***********
              rf_params = 00 00 00 00 00 00 2C 00 00 00 00 00 00 00 00 30 20 30 20 20 20 30 30 30 00 00 00 00 50 EC 00 00 00 00 00 00 00 00 00 00 00 00
              tx_pkt_tput_mode = 0
              tx_pkt_sgi = 0
              tx_pkt_preamble = 0
              tx_pkt_mcs = -1
              tx_pkt_rate = -1
              tx_pkt_gap = 200
              phy_calib_rxdc = 1
              phy_calib_txdc = 1
              phy_calib_txpow = 1
              phy_calib_rxiq = 1
              phy_calib_txiq = 1
              chnl_primary = 1
              tx_pkt_num = -1
              tx_pkt_len = 1400
              tx_power = 0
              he_ltf = 0
              he_gi = 0
              tx = 0
              rx = 0

         * To run a continuous Orthogonal frequency-division multiplexing (OFDM) TX traffic sequence with the following configuration:

           * Channel: 14
           * Frame duration: 5484 us
           * Inter-frame gap: 4200 us

           Execute the following sequence of commands:

           .. code-block:: console

              wifi_radio_test rx 0
              wifi_radio_test tx 0
              wifi_radio_test tx_pkt_tput_mode 0
              wifi_radio_test tx_pkt_len 4095
              wifi_radio_test chnl_primary 14
              wifi_radio_test tx_pkt_rate 6
              wifi_radio_test tx_power 0
              wifi_radio_test tx_pkt_gap 4200
              wifi_radio_test tx 1


         * To run a continuous Direct-sequence spread spectrum (DSSS) TX traffic sequence with the following configuration:

           * Channel: 1
           * Frame duration: 8500 us
           * Inter-frame gap: 8600 us

           Execute the following sequence of commands:

           .. code-block:: console

              wifi_radio_test rx 0
              wifi_radio_test tx 0
              wifi_radio_test tx_pkt_tput_mode 0
              wifi_radio_test tx_pkt_len 1024
              wifi_radio_test chnl_primary 1
              wifi_radio_test tx_pkt_rate 1
              wifi_radio_test tx_power 0
              wifi_radio_test tx_pkt_gap 8600
              wifi_radio_test tx 1

         See :ref:`wifi_radio_test_subcmds` for a list of available subcommands.

         .. note::

            * For regulatory certification, it is advisable to run the TX streams in Legacy OFDM or DSSS modes only (``wifi_radio_test tx_pkt_tput_mode 0``).
            * The frame duration can be calculated using the formula:

              .. code-block::

                 D = ((L * 8) / R ) + P

              where the following parameters are used:

              * D - Frame duration (us)
              * L - Frame length (bytes)
              * R - Data rate (Mbps)
              * P - PHY overhead duration (us) (Values: 24 us - Legacy OFDM, 192 us - DSSS)

      .. group-tab:: FICR/OTP programming

         * Use the following reference command interface to read or write the OTP params:

           .. code-block:: console

              wifi_radio_ficr_prog <subcommand> [Offset] [arg1] [arg2] .. [argN]

         * To display all the current FICR values, use the following command:

           .. code-block:: console

              wifi_radio_ficr_prog otp_read_params

           The sample shows the following output:

           .. code-block:: console

              OTP Region is open for R/W

              REGION_PROTECT0 = 0x50fa50fa
              REGION_PROTECT1 = 0x50fa50fa
              REGION_PROTECT2 = 0x50fa50fa
              REGION_PROTECT3 = 0x50fa50fa

              MAC0: Reg0 = 0x0036cef0
              MAC0: Reg1 = 0x00004a00
              MAC0 Addr  = f0:ce:36:00:00:4a

              MAC1 : Reg0 = 0x0036cef0
              MAC1 : Reg1 = 0x00004b00
              MAC1 Addr   = f0:ce:36:00:00:4b

              CALIB_XO = 0x2c

              CALIB_PDADJMCS7 = 0xffffffff

              CALIB_PDADJMCS0 = 0xffffffff

              CALIB_MAXPOW2G4 = 0xffffffff

              CALIB_MAXPOW5G0MCS7 = 0xffffffff

              CALIB_MAXPOW5G0MCS0 = 0xffffffff

              CALIB_RXGAINOFFSET = 0xffffffff

              CALIB_TXPOWBACKOFFT = 0xffffffff

              CALIB_TXPOWBACKOFFV = 0xffffffff

              REGION_DEFAULTS = 0xfffffff1

         * To read the status of OTP region, use the following command:

           .. code-block:: console

              wifi_radio_ficr_prog otp_get_status

           The sample shows the following output:

           .. code-block:: console

              Checking OTP PROTECT Region......
              OTP Region is open for R/W

              QSPI Keys are not programmed in OTP
              MAC0 Address is programmed in OTP
              MAC1 Address is programmed in OTP
              CALIB_XO is programmed in OTP
              CALIB_PDADJMCS7 not programmed in OTP
              CALIB_PDADJMCS0 is not programmed in OTP
              CALIB_MAXPOW2G4 is not programmed in OTP
              CALIB_MAXPOW5G0MCS7 is not programmed in OTP
              CALIB_MAXPOW5G0MCS0 is not programmed in OTP
              CALIB_RXGAINOFFSET is not programmed in OTP
              CALIB_TXPOWBACKOFFT is not programmed in OTP
              CALIB_TXPOWBACKOFFV is not programmed in OTP

         * To write different locations of the OTP and to program MAC0 address to F0:CE:36:00:00:4A, use the following command:

           .. code-block:: console

              wifi_radio_ficr_prog otp_write_params 0x120 0x0036CEF0 0x4A00

           The sample shows the following output:

           .. code-block:: console

              [00:24:25.200,622] <inf> otp_prog: OTP Region is open for R/W
              [00:24:25.200,653] <inf> otp_prog:
              [00:24:25.202,575] <inf> otp_prog: Written MAC address 1
              [00:24:25.202,575] <inf> otp_prog: mac addr 0 : Reg1 (0x128) = 0x36cef0
              [00:24:25.202,575] <inf> otp_prog: mac addr 0 : Reg2 (0x12c) = 0x4a00
              [00:24:25.202,606] <inf> otp_prog: Written REGION_DEFAULTS (0x154) : 0xfffffffb
              [00:24:25.203,002] <inf> otp_prog: Finished Writing OTP params

         See :ref:`wifi_radio_ficr_prog_subcmds` for a list of available subcommands.

Dependencies
************

This sample uses the following Zephyr library:

* :ref:`zephyr:shell_api`:

  * :file:`include/shell/shell.h`
