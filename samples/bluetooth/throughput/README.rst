.. _ble_throughput:

Bluetooth: Throughput
#####################

.. contents::
   :local:
   :depth: 2

.. note::
   The implementation and usage of this sample has changed considerably with |NCS| v1.5.0.

The Bluetooth Throughput sample uses the :ref:`throughput_readme` to measure *Bluetooth* Low Energy throughput performance.
You can use it to determine the maximum throughput, or to experiment with different connection parameters and check their influence on the throughput.

Overview
********

The sample transmits data between two development kits, the *tester* and the *peer*, and measures the throughput performance.
It uses the :ref:`throughput_readme` for this.
To run the tests, connect to the kit using the serial port and send shell commands.
Zephyr's :ref:`zephyr:shell_api` module is used to handle the commands.

The sample demonstrates the interaction of the following connection parameters:

ATT_MTU size
   In *Bluetooth* Low Energy, the default Maximum Transmission Unit (MTU) is 23 bytes.
   When increasing this value, longer ATT payloads can be achieved, increasing ATT throughput.

.. note::
   To configure the ATT_MTU size, use menuconfig and compile and program the sample again.

Data length
   In *Bluetooth* Low Energy, the default data length for a radio packet is 27 bytes.
   Data length extension allows to use larger radio packets, so that more data can be sent in one packet, increasing throughput.

Connection interval
   The connection interval defines how often the devices must listen on the radio.
   When increasing this value, more packets may be sent in one interval, but if a packet is lost, the wait until the retransmission is longer.

Physical layer (PHY) data rate
   Starting with Bluetooth 5, the over-the-air data rate in Bluetooth Low Energy can exceed 1 Ms/s (mega symbol per second), which allows for faster transmission.
   In addition, it is possible to use coded PHY (available on the nRF52840 SoC only) for long-range transmission.

By default, the following connection parameter values are used:

.. list-table:: Default parameter values
   :header-rows: 1

   * - Parameter
     - Value
   * - ATT_MTU size
     - 247 bytes
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
   In a *Bluetooth* Low Energy connection, the different devices negotiate the connection parameters that are used.
   If the configuration parameters for the devices differ, they agree on the lowest common denominator.

   By default, the sample uses the fastest connection parameters.
   You can change them to different valid values without a need to program both kits again.

.. note::
   When you have set the LE Connection Interval to high values and need to change the PHY or the Data Length in the next test, the PHY Update procedure or Data Length Update procedure can take several seconds.


Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuappns, nrf52840dk_nrf52840, nrf52833dk_nrf52833, nrf52dk_nrf52832

You can use any two of the development kits listed above and mix different development kits.

.. include:: /includes/hci_rpmsg_overlay.txt

The sample also requires a connection to a computer with a serial terminal for each of the development kits.

User interface
**************

Button 1:
   Set the board into a master (tester) role.

Button 2:
   Set the board into a slave (peer) role.


Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/throughput`

.. include:: /includes/build_and_run.txt


Testing
=======

After programming the sample to both kits, test it by performing the following steps:

1. Connect to both kits with a terminal emulator (for example, PuTTY).
   See :ref:`putty` for the required settings.
#. Reset both kits.
#. Press Button 1 on the kit to set the kit into master (tester) role.
#. Press Button 2 on the other kit to set the kit into slave (peer) mode.
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

   *** Booting Zephyr OS build v2.4.0-ncs1-1715-g3366927a5498  ***
   Starting Bluetooth Throughput example
   I: SoftDevice Controller build revision:
   I: 7a 01 b4 17 68 14 99 b6 |z...h...
   I: 6a d1 f2 fd fe 59 63 e3 |j....Yc.
   I: 43 ca fb 5c             |C..\
   I: HW Platform: Nordic Semiconductor (0x0002)
   I: HW Variant: nRF52x (0x0002)
   I: Firmware: Standard Bluetooth controller (0x00) Version 122.46081 Build 256825
   I: Identity: D9:85:73:DC:7D:D4 (random)
   I: HCI: version 5.2 (0x0b) revision 0x1154, manufacturer 0x0059
   I: LMP: version 5.2 (0x0b) subver 0x1154
   Bluetooth initialized

   Press button 1 on the master board.
   Press button 2 on the slave board.


   uart:~$
   Master role. Starting scanning
   Filters matched. Address: D2:71:97:84:DE:B2 (random) connectable: 1
   Connected as master
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

                       ^.-.^                               ^..^
                    ^-/ooooo+:.^                       ^.--:+syo/.
                 ^-/oooooooooooo+:.                 ^.-:::::+yyyyyy+:^
              ^-/+oooooooooooooooooo/-^          ^.-::::::::/yyyyyyyhhs/-
           ^-:/++++oooooooooooooooooooo+:.   ^.-::::::::::::/yyyyyyyhhhhhho:^
         ^::///++++oooooooooooooooooooooooo//:::::::::::::::/yyyyyyyhhhhhddds
         -::://+++ooooooooooooooooooooooooooooo+/:::::::::::/yyyyyyyhhhhhdddd^
         -::::::/++ooooooooooooooooooooooooooooooo+/::::::::/yyyyyyyhhhhhdddd^
         -:::::::::/+ooooooooooooooooooooooooooooossso+/::::/yyyyyyyhhhhhdddd^
         -::::::::::::/+oooooooooooooooooooooooooossssssso+//yyyyyyyhhhhhdddd^
         -::::::::::::::::/+ooooooooooooooooooooooossssssssssyyyyyyyhhhhhdddd.
         -:::::::::::::::::::/+oooooooooooooooooooossssssssssyyyyyyyhhhhhdddd.
         -:::::::::::::::::::::::/+ooooooooooooooosssssssssssyyyyyyyhhhhhdddd.
         -::::::::::::::::::::::::::/+ooooooooooooossssssssssyyyyyyyhhhhhdddd.
         -::::::::::::::::::::::::::::::/+ooooooooossssssssssyyyyyyyhhhhhdddd-
         -:::::::::::::::::::::::::::::::::/+ooooosssssssssssyyyyyyyhhhhhdddd-
         -:::::::::::::::::::::::::::::::::::::/+oossssssssssyyyyyyyhhhhhdddd:
         -::::::::::::::::::::::::::::::::::::::::/+ossssssssyyyyyyyhhhhhdddd:
         -::::::::::::::::::::::::::::::::::::::::::::/osssssyyyyyyyhhhhhdddd:
         -:::::::::::::::::::::::::::::::::::::::::::::::/+ossyyyyyyhhhhhdddd:
         -:::::::::::::::::o+/:::::::::::::::::::::::::::::::+oyyyyyhhhhhdddd:
         -:::::::::::::::::ossyso/::::::::::::::::::::::::::::::/osyhhhhhdddd/
         -:::::::::::::::::ossyyyyys+:::::::::::::::::::::::::::::::+shhhdddd/
         -:::::::::::::::::ossyyyyhhhhyo/::::::::::::::::::::::::::::::/oyddd/
         .-::::::::::::::::ossyyyyhhhhddddy/-::::::::::::::::::::::::::::::+y:
           ^.-:::::::::::::ossyyyyhhhhdhs/.  ^.--:::::::::::::::::::::::::-.^
              ^.--:::::::::ossyyyyhhy+-^         ^.-::::::::::::::::::--.^
                  ^.-::::::ossyyyo/.                ^^.-:::::::::::-.^
                     ^..-::oss+:^                       ^.-:::::-.^
                         ^.:.^                             ^^.^^

   Done
   [local] sent 612684 bytes (598 KB) in 3890 ms at 1260 kbps
   [peer] received 612684 bytes (598 KB) in 2511 GATT writes at 1395626 bps

   Type 'config' to change the configuration parameters.
   You can use the Tab key to autocomplete your input.
   Type 'run' when you are ready to run the test.


For the peer::

   *** Booting Zephyr OS build v2.4.0-ncs1-1715-g3366927a5498  ***
   Starting Bluetooth Throughput example
   I: SoftDevice Controller build revision:
   I: 7a 01 b4 17 68 14 99 b6 |z...h...
   I: 6a d1 f2 fd fe 59 63 e3 |j....Yc.
   I: 43 ca fb 5c             |C..\
   I: HW Platform: Nordic Semiconductor (0x0002)
   I: HW Variant: nRF52x (0x0002)
   I: Firmware: Standard Bluetooth controller (0x00) Version 122.46081 Build 256825
   I: Identity: D2:71:97:84:DE:B2 (random)
   I: HCI: version 5.2 (0x0b) revision 0x1154, manufacturer 0x0059
   I: LMP: version 5.2 (0x0b) subver 0x1154
   Bluetooth initialized

   Press button 1 on the master board.
   Press button 2 on the slave board.


   uart:~$
   Slave role. Starting advertising
   Connected as slave
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
   [local] received 612684 bytes (598 KB) in 2511 GATT writes at 1261557 bps


Dependencies
*************

This sample uses the following |NCS| libraries:

* :ref:`throughput_readme`

In addition, it uses the following Zephyr libraries:

* ``include/console.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* ``include/sys/printk.h``
* ``include/zephyr/types.h``
* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/conn.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``
* :ref:`zephyr:shell_api`:

  * ``include/shell/shell.h``


References
***********

For more information about the connection parameters that are used in this sample, see the following chapters in the `Bluetooth Core Specification`_:

* Vol 3, Part F, 3.2.8 Exchanging MTU Size
* Vol 6, Part B, 5.1.1 Connection Update Procedure
* Vol 6, Part B, 5.1.9 Data Length Update Procedure
* Vol 6, Part B, 5.1.10 PHY Update Procedure
