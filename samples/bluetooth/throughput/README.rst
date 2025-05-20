.. _ble_throughput:

Bluetooth: Throughput
#####################

.. contents::
   :local:
   :depth: 2

The Bluetooth® Throughput sample uses the :ref:`throughput_readme` to measure Bluetooth Low Energy throughput performance.
You can use it to determine the maximum throughput, or to experiment with different connection parameters and check their impact on the throughput.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

You can use any two of the development kits listed above and mix different development kits.

The sample also requires a connection to a computer with a serial terminal |ANSI| for each of the development kits.

Overview
********

The sample transmits data between two development kits, the *tester* and the *peer*, and measures the throughput performance.
It uses the :ref:`throughput_readme` for this.
To run the tests, connect to the kit using the serial port and send shell commands.
Zephyr's :ref:`zephyr:shell_api` module is used to handle the commands.

The sample demonstrates the interaction of the following connection parameters:

ATT_MTU size
   In Bluetooth Low Energy, the default Maximum Transmission Unit (MTU) is 23 bytes.
   When increasing this value, longer ATT payloads can be achieved, increasing the ATT throughput.

   .. note::
      To configure the ATT_MTU size, use menuconfig and compile and program the sample again.

Data length
   In Bluetooth Low Energy, the default data length for a radio packet is 27 bytes.
   Data length extension allows to use larger radio packets, so that more data can be sent in one packet, increasing the throughput.

Connection interval
   The connection interval defines how often the devices must listen to the radio.
   When increasing this value, more packets may be sent in one interval, but if a packet is lost, the wait until the retransmission is longer.

Physical layer (PHY) data rate
   Starting with Bluetooth 5, the over-the-air data rate in Bluetooth Low Energy can exceed 1 Ms/s (mega symbols per second), which allows for faster transmission.
   In addition, you can use coded PHY (available on select nRF SoCs) for long-range transmission.

By default, the following connection parameter values are used:

.. list-table:: Default parameter values
   :header-rows: 1

   * - Parameter
     - Value
   * - ATT_MTU size
     - 498 bytes
   * - Data length
     - 251 bytes
   * - Connection interval
     - 320 units (400 ms)
   * - PHY data rate
     - 2 Ms/s

Changing connection parameter values
====================================

To experiment with different connection parameter values, reconfigure the values using the :ref:`zephyr:shell_api` interface before running a test.

You can adjust the following parameters:

* PHY
* LE Data Length
* LE Connection interval

.. note::
   In a Bluetooth Low Energy connection, the different devices negotiate the connection parameters that are used.
   If the configuration parameters for the devices differ, they agree on the lowest common denominator.

   By default, the sample uses the fastest connection parameters.
   You can change them to different valid values without a need to program both kits again.

   When you have set the LE Connection Interval to high values and need to change the PHY or the Data Length in the next test, the PHY Update or Data Length Update procedure can take several seconds.

User interface
**************

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      Button 1:
         Set the board into a central (tester) role.

      Button 2:
         Set the board into a peripheral (peer) role.

   .. group-tab:: nRF54 DKs

      Button 0:
         Set the board into a central (tester) role.

      Button 1:
         Set the board into a peripheral (peer) role.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/throughput`

.. include:: /includes/build_and_run_ns.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

.. include:: /includes/nRF54H20_erase_UICR.txt

Testing
=======

After programming the sample to both kits, complete following steps to test it:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. |connect_terminal_both_ANSI|
      #. Reset both kits.
      #. Press **Button 1** on the first development kit or type ``central`` in the terminal connected to the first kit to set it into the central (tester) role.
      #. Press **Button 2** on the second development kit or type ``peripheral`` in the terminal connected to the second kit to set it into the peripheral (peer) role.
      #. Observe that the kits establish a connection.
         The tester outputs the following information::

            Type 'config' to change the configuration parameters.
            You can use the Tab key to autocomplete your input.
            Type 'run' when you are ready to run the test.

      #. Type ``config print`` in the terminal to print the current configuration.
         Type ``config`` in the terminal to configure the test parameters to your choice.
         Use the Tab key for auto-completion and to view the options available for a parameter.
      #. Type ``run`` in the terminal to start the test.
      #. Observe the output while the tester sends data to the peer.
         At the end of the test, both tester and peer display the results of the test.
      #. Repeat the test after changing the parameters.
         Observe how the throughput changes for different sets of parameters.

   .. group-tab:: nRF54 DKs

      1. |connect_terminal_both_ANSI|
      #. Reset both kits.
      #. Press **Button 0** on the first development kit or type ``central`` in the terminal connected to the first kit to set it into the central (tester) role.
      #. Press **Button 1** on the second development kit or type ``peripheral`` in the terminal connected to the second kit to set it into the peripheral (peer) role.
      #. Observe that the kits establish a connection.
         The tester outputs the following information::

            Type 'config' to change the configuration parameters.
            You can use the Tab key to autocomplete your input.
            Type 'run' when you are ready to run the test.

      #. Type ``config print`` in the terminal to print the current configuration.
         Type ``config`` in the terminal to configure the test parameters to your choice.
         Use the Tab key for auto-completion and to view the options available for a parameter.
      #. Type ``run`` in the terminal to start the test.
      #. Observe the output while the tester sends data to the peer.
         At the end of the test, both tester and peer display the results of the test.
      #. Repeat the test after changing the parameters.
         Observe how the throughput changes for different sets of parameters.

Sample output
==============

The result should look similar to the following output.

For the tester::

   *** Booting Zephyr OS build v3.0.99-ncs1  ***
   Starting Bluetooth Throughput sample
   I: SoftDevice Controller build revision:
   I: 33 78 2a 18 20 f5 61 61 |3x*. .aa
   I: a6 8b 77 60 62 83 39 2a |..w`b.9*
   I: 7c f1 14 e4             ||...


   : HW Platform: Nordic Semiconductor (0x0002)
   I: HW Variant: nRF52x (0x0002)
   I: Firmware: Standard Bluetooth controller (0x00) Version 51.10872 Build 1643454488
   I: Identity: D7:D6:AD:A2:50:62 (random)
   I: HCI: version 5.3 (0x0c) revision 0x1136, manufacturer 0x0059
   I: LMP: version 5.3 (0x0c) subver 0x1136
   Bluetooth initialized

   Press button 1 or type "central" on the central board.
   Press button 2 or type "peripheral" on the peripheral board.
   uart:~$
   Central. Starting scanning
   Filters matched. Address: DC:D6:E5:EE:75:9A (random) connectable: 1
   Connected as central
   Conn. interval is 320 units
   Service discovery completed
   MTU exchange pending
   MTU exchange successful

   Type 'config' to change the configuration parameters.
   You can use the Tab key to autocomplete your input.
   Type 'run' when you are ready to run the test.
   run

   ==== Starting throughput test ====
   PHY update pending
   LE PHY updated: TX PHY LE 2M, RX PHY LE 2M
   LE Data length update pending
   LE data len updated: TX (len: 251 time: 2120) RX (len: 251 time: 2120)
   The test is in progress and will require around 20 seconds to complete.
   Done
   [local] sent 1242945 bytes (1213 KB) in 7292 ms at 1363 kbps
   [peer] received 1242945 bytes (1213 KB) in 2511 GATT writes at 1415583 bps

   Type 'config' to change the configuration parameters.
   You can use the Tab key to autocomplete your input.
   Type 'run' when you are ready to run the test.


For the peer::

   *** Booting Zephyr OS build v3.0.99-ncs1  ***
   Starting Bluetooth Throughput sample
   I: SoftDevice Controller build revision:
   I: 33 78 2a 18 20 f5 61 61 |3x*. .aa
   I: a6 8b 77 60 62 83 39 2a |..w`b.9*
   I: 7c f1 14 e4             ||...


   : HW Platform: Nordic Semiconductor (0x0002)
   I: HW Variant: nRF52x (0x0002)
   I: Firmware: Standard Bluetooth controller (0x00) Version 51.10872 Build 1643454488
   I: Identity: DC:D6:E5:EE:75:9A (random)
   I: HCI: version 5.3 (0x0c) revision 0x1136, manufacturer 0x0059
   I: LMP: version 5.3 (0x0c) subver 0x1136
   Bluetooth initialized

   Press button 1 or type "central" on the central board.
   Press button 2 or type "peripheral" on the peripheral board.
   uart:~$
   Peripheral. Starting advertising
   Connected as peripheral
   Conn. interval is 320 units
   LE PHY updated: TX PHY LE 2M, RX PHY LE 2M

   =============================================================================
   =============================================================================
   =============================================================================
   =============================================================================
   =============================================================================
   =============================================================================
   =============================================================================
   ===========================================================
   [local] received 1242945 bytes (1213 KB) in 2511 GATT writes at 1415583 bps


Dependencies
*************

This sample uses the following |NCS| libraries:

* :ref:`throughput_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/console.h`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :file:`include/sys/printk.h`
* :file:`include/zephyr/types.h`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/gatt.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/uuid.h`
* :ref:`zephyr:shell_api`:

  * :file:`include/shell/shell.h`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`

References
***********

For more information about the connection parameters that are used in this sample, see the following chapters in the `Bluetooth Core Specification`_:

* Vol 3, Part F, 3.2.8 Exchanging MTU Size
* Vol 6, Part B, 5.1.1 Connection Update Procedure
* Vol 6, Part B, 5.1.9 Data Length Update Procedure
* Vol 6, Part B, 5.1.10 PHY Update Procedure
