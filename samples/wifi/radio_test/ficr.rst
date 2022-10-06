.. _wifi_ficr_prog:

Wi-Fi: FICR programming
#######################

.. contents::
   :local:
   :depth: 2

The Wi-Fi Radio Factory Information Configuration Registers (FICR) programming
sample demonstrates how to program the user region of FICR parameters on the
development kit using a set of predefined commands.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

To run the commands, connect to the development kit through the serial port and
send shell commands. Zephyr's :ref:`zephyr:shell_api` module is used to handle
the commands.

You can start running ``wifi_radio_ficr_prog`` subcommands to read or write OTP
registers. See :ref:`wifi_radio_ficr_prog_subcmds` for a list of available
subcommands.

.. note::

   All the FICR registers are stored in the one-time programmable (OTP) memory.
   Consequently, the write commands are destructive, and once written, the
   contents of the OTP registers cannot be re-programmed.

User interface
**************

``wifi_radio_ficr_prog`` is the Wi-Fi radio ficr_prog command and it supports the following subcommands:

.. _wifi_radio_ficr_prog_subcmds:

Wi-Fi radio FICR subcommands
============================

.. list-table:: Wi-Fi radio ficr subcommands
   :widths: 15 15 10 30 70
   :header-rows: 1

   * - Subcommand
     - Register
     - Offset
     - Argument(s)
     - Description
   * - otp_get_status
     - N/A
     - N/A
     - N/A
     - Reads out the OTP status of user region and status of each field (programmed or not).
   * - otp_read_params
     - N/A
     - N/A
     - N/A
     - Reads out all the OTP parameters (excluding QSPI_KEY which cannot be read).
   * - otp_write_params
     - REGION_PROTECT
     - 0x100
     - arg
     - | arg = 0x50FA50FA : Enable R/W permission
       | arg = 0x00000000 : Lock Region from Writing
       | arg = Others : Invalid
       | All four REGION_PROTECT registers are written with this 32-bit argument.
   * - otp_write_params
     - QSPI_KEY
     - 0x110
     - arg1 arg2 arg3 arg4
     - All four key arguments are 32-bit values forming the required 128-bit (Q)SPI key for encryption.
   * - otp_write_params
     - MAC_ADDRESS0
     - 0x120
     - arg1 arg2
     - If MAC address is AA:BB:CC:DD:EE:FF, then arg1=0xDDCCBBAA and arg2=0xFFEE.
   * - otp_write_params
     - MAC_ADDRESS1
     - 0x128
     - arg1 arg2
     - If MAC address is AA:BB:CC:DD:EE:FF, then arg1=0xDDCCBBAA and arg2=0xFFEE.
   * - otp_write_params
     - CALIB_XO
     - 0x130
     - arg
     - arg is 7-bit unsigned XO value. Bits [31:7] unused. Adjusts capacitor bank, 0 : Lowest capacitance (Highest frequency), 255 : Highest capacitance (Lowest frequency).
   * - otp_write_params
     - CALIB_PDADJMCS7
     - 0x134
     - arg
     - arg is 32-bit power detector adjustment for MCS7 for channel 7, 36, 100, 165 (channel 7 in bits[7:0] - signed).
   * - otp_write_params
     - CALIB_PDADJMCS0
     - 0x138
     - arg
     - arg is 32-bit power detector adjustment for MCS0 for channel 7, 36, 100, 165 (channel 7 in bits[7:0] - signed).
   * - otp_write_params
     - CALIB_MAXPOW2G4
     - 0x13C
     - arg
     - arg is 32-bit value specifying maximum output power for DSSS, MCS0, MCS7 (DSSS in bits[7:0] - unsigned). Measured in channel 7. Bits [31:24] unused.
   * - otp_write_params
     - CALIB_MAXPOW5G0MCS7
     - 0x140
     - arg
     - arg is 32-bit value specifying maximum output power for MCS7 in channel 36, 100, 165 (channel 36 in bits[7:0] - unsigned). Bits [31:24] unused.
   * - otp_write_params
     - CALIB_MAXPOW5G0MCS0
     - 0x144
     - arg
     - arg is 32-bit value specifying maximum output power for MCS0 in channel 36, 100, 165 (channel 36 in bits[7:0] - unsigned). Bits [31:24] unused.
   * - otp_write_params
     - CALIB_RXGAINOFFSET
     - 0x148
     - arg
     - arg is 32-bit value specifying RX gain offset for channel 7, 36, 100, 165 (channel 7 in bits[7:0] - signed).
   * - otp_write_params
     - CALIB_TXPOWBACKOFFT
     - 0x14C
     - arg
     - arg is 32-bit value specifying TX power backoff versus temperature. Bits[7:0] 2.4G at +80 degree Celsius, bits[15:8] 2.4G at -20 degree Celsius, bits[23:16] 5.0G at +80 degree Celsius, bits[31:24] 5.0G at -20 degree Celsius. Each 8-bit field is a signed value.
   * - otp_write_params
     - CALIB_TXPOWBACKOFFV
     - 0x150
     - arg
     - arg is 32-bit value specifying TX power backoff versus voltage. Backoff in 2.4GHz, 5GHz bands  at very low voltage. Backoff in 2.4GHz, 5GHz bands  at low voltage. Each 8-bit field is signed value.
   * - otp_write_params
     - REGION_DEFAULTS
     - 0x154
     - arg
     - | Register default enable control.
       | arg is 32-bit value where a specific bit is set to ``0`` implies that the corresponding OTP field is programmed. The following list shows the bit and field mapping:
       |
       | bit 0  : QSPI_KEY
       | bit 1  : MAC0 Address
       | bit 2  : MAC1 Address
       | bit 3  : CALIB_XO
       | bit 4  : CALIB_PDADJMCS7
       | bit 5  : CALIB_PDADJMCS0
       | bit 6  : CALIB_MAXPOW2G4
       | bit 7  : CALIB_MAXPOW5G0MCS7
       | bit 8  : CALIB_MAXPOW5G0MCS0
       | bit 9  : CALIB_RXGAINOFFSET
       | bit 10 : CALIB_TXPOWBACKOFFT
       | bit 11 : CALIB_TXPOWBACKOFFV
       | bit 12-31 : Reserved



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
#. Use the following reference command interface to read or write the OTP params:

   .. code-block:: console

      wifi_radio_ficr_prog <subcommand> [Offset] [arg1] [arg2] .. [argN]

#. Test the sample by running the following commands:

   a. To display all the current FICR values, use the following command:

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


   #. To read the status of OTP region, use the following command:

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

   #. To write different locations of the OTP and to program MAC0 address to F0:CE:36:00:00:4A, use the following command:

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


Dependencies
************

This sample uses the following Zephyr library:

* :ref:`zephyr:shell_api`:

  * ``include/shell/shell.h``
