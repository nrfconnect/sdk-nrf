.. _ble_throughput:

Bluetooth: Throughput
#####################

The Bluetooth Throughput sample uses the :ref:`throughput_readme` to measure *Bluetooth* Low Energy throughput performance.
You can use it to determine the maximum throughput, or to experiment with different connection parameters and check their influence on the throughput.


Overview
********

The sample transmits data between two boards, the *tester* and the *peer*, and measures the throughput performance.
To do so, it uses the :ref:`throughput_readme`.

The sample demonstrates the interaction of the following connection parameters:

ATT_MTU size
   In *Bluetooth* Low Energy, the default Maximum Transmission Unit (MTU) is 23 bytes.
   When increasing this value, longer ATT payloads can be achieved, increasing ATT throughput.

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

You can experiment with different connection parameter values by reconfiguring the values using menuconfig and then compiling and programming the sample again to at least one of the boards.

.. note::
   In a *Bluetooth* Low Energy connection, the different devices negotiate the connection parameters that are used.
   If the configuration parameters for the devices differ, they agree on the lowest common denominator.

   By default, the sample uses the fastest connection parameters.
   If you change them to lower values, it is sufficient to program them to one of the boards.
   If you were to change them to higher values, you would need to program both boards again.


Requirements
************

* Two of the following development boards:

  * |nRF5340DK|

    If you use this board, you must add the following options to the network sample configuration::

       CONFIG_BT_CTLR_TX_BUFFER_SIZE=251
       CONFIG_BT_CTLR_DATA_LENGTH_MAX=251
       CONFIG_BT_RX_BUF_LEN=255

  * |nRF52840DK|
  * |nRF52DK|

  You can mix different boards.
* Connection to a computer with a serial terminal for each of the boards.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/throughput`

.. include:: /includes/build_and_run.txt


Testing
=======

After programming the sample to both boards, test it by performing the following steps:

1. Connect to both boards with a terminal emulator (for example, PuTTY).
   See :ref:`putty` for the required settings.
#. Reset both boards.
#. Observe that the boards establish a connection.
   When they are connected, one of them serves as *tester* and the other one as *peer*.
   The tester outputs the following information::

       Ready, press any key to start

#. Press a key in the terminal that is connected to the tester.
#. Observe the output while the tester sends data to the peer.
   At the end of the test, both tester and peer display the results of the test.


Sample output
==============

The result should look similar to the following output.

For the tester::

   ***** Booting Zephyr OS 1.12.99 *****
   [bt] [INF] hci_vs_init: HW Platform: Nordic Semiconductor (0x0002)
   [bt] [INF] hci_vs_init: HW Variant: nRF52x (0x0002)
   [bt] [INF] hci_vs_init: Firmware: Standard Bluetooth controller (0x00) Version 1.12 Build 99
   [bt] [INF] bt_dev_show_info: Identity: c5:ca:14:98:3b:90 (random)
   [bt] [INF] bt_dev_show_info: HCI: version 5.0 (0x09) revision 0x0000, manufacturer 0x05f1
   [bt] [INF] bt_dev_show_info: LMP: version 5.0 (0x09) subver 0xffff
   Bluetooth initialized
   Advertising successfully started
   Scanning successfully started
   Found a peer device c5:6f:8a:38:95:27 (random)
   Connected as master
   Conn. interval is 320 units
   MTU exchange pending
   MTU exchange successful
   Ready, press any key to start

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
   [local] sent 612684 bytes (598 KB) in 4042 ms at 1212 kbps
   [peer] received 612684 bytes (598 KB) in 2511 GATT writes at 1261557 bps
   Ready, press any key to start


For the peer::

   ***** Booting Zephyr OS 1.12.99 *****
   [bt] [INF] hci_vs_init: HW Platform: Nordic Semiconductor (0x0002)
   [bt] [INF] hci_vs_init: HW Variant: nRr (0x00) Version 1.12 Build 99
   [bt] [INF] bt_dev_show_info: Identity: c5:6f:8a:38:95:27 (random)
   [bt] [INF] bt_devized
   Advertising successfully started
   Scanning successfully started
   Found a peer device c5:ca:14:98:3b:90 (random)
   Connected as slave
   Conn. interval is 320 units

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
* :ref:`zephyr:kernel`:

  * ``include/kernel.h``

* ``include/sys/printk.h``
* ``include/zephyr/types.h``
* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/conn.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``


References
***********

For more information about the connection parameters that are used in this sample, see the following chapters in the `Bluetooth Core Specification`_:

* Vol 3, Part F, 3.2.8 Exchanging MTU Size
* Vol 6, Part B, 5.1.1 Connection Update Procedure
* Vol 6, Part B, 5.1.9 Data Length Update Procedure
* Vol 6, Part B, 5.1.10 PHY Update Procedure
