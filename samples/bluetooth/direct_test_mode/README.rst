.. _direct_test_mode:

Bluetooth: Direct Test Mode
###########################

.. contents::
   :local:
   :depth: 2

This sample enables the Direct Test Mode functions described in `Bluetooth® Core Specification <Bluetooth Core Specification_>`_ (Vol. 6, Part F).
The actual encoding of the test commands and events is described in sections 3.3 and 3.4, respectively, of Vol. 6, Part F of this specification document.


Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Additionally, the sample requires one of the following testing devices:

* Dedicated test equipment, like an Anritsu MT8852 tester.
  See :ref:`direct_test_mode_testing_anritsu`.
* Another development kit with the same sample.
  See :ref:`direct_test_mode_testing_board`.
* A computer with the `Direct Test Mode app`_ available in the `nRF Connect for Desktop`_.
  See :ref:`direct_test_mode_testing_app`.

Overview
********

The sample uses Direct Test Mode (DTM) to test the operation of the following features of the radio:

* Transmission power and receiver sensitivity
* Frequency offset and drift
* Modulation characteristics
* Packet error rate
* Intermodulation performance

Test procedures are defined in the document `Bluetooth Low Energy RF PHY Test Specification <Bluetooth Low Energy RF PHY Test Specification_>`_.

You can carry out conformance tests using dedicated test equipment, such as the Anritsu MT8852 or similar, with an nRF5 running the DTM sample set as device under test (DUT).

Implementation
==============

The sample uses the :ref:`dtm_twowire_to_hci_readme` library to convert between 2-wire UART commands and events and Bluetooth LE HCI commands and events.
It also implements a transport module that uses Zephyr UART APIs to read and write 2-wire commands and events on the UART interface.

The sample application in :file:`src/main.c` runs the following loop:

1. Wait for a 2-wire UART command using the :c:func:`dtm_tw_transport_read` function.
#. Call the :c:func:`dtm_tw_to_hci_process_tw_cmd` function.

   * If the result is a 2-wire event, send the event immediately on UART using the :c:func:`dtm_tw_transport_write` function.
   * If the result is an HCI command, send it through ``bt_send()``, wait for the HCI event, generate the appropriate 2-wire event using the :c:func:`dtm_tw_to_hci_process_hci_event` function, then send the produced 2-wire event using :c:func:`dtm_tw_transport_write`.

This follows the conversion flow defined by the DTM conversion library.

Debugging
*********

In this sample, the UART console is used to exchange commands and events defined in the DTM specification.
Debug messages are not displayed in the UART console.
Instead, they are printed by the RTT logger.

If you want to view the debug messages, follow the procedure in :ref:`testing_rtt_connect`.
For more information about debugging in the |NCS|, see :ref:`debugging`.

.. note::
   The default configuration disables all logging, so you need to enable the logging options to view the debug messages.
   See the Configuration section for details.

Configuration
*************
|config|

Configuration options
=====================

The default sample configuration is in the :file:`prj.conf` file.

To enable RTT logging for this sample, set the following Kconfig options:

.. code-block:: console

  CONFIG_LOG=y
  CONFIG_LOG_PRINTK=y
  CONFIG_USE_SEGGER_RTT=y
  CONFIG_LOG_BACKEND_RTT=y

To set the log level for the transport module, set one of the following Kconfig options:

* :kconfig:option:`CONFIG_DTM_TW_TRANSPORT_LOG_LEVEL_DBG` for log level DEBUG
* :kconfig:option:`CONFIG_DTM_TW_TRANSPORT_LOG_LEVEL_INF` for log level INFO
* :kconfig:option:`CONFIG_DTM_TW_TRANSPORT_LOG_LEVEL_WRN` for log level WARNING
* :kconfig:option:`CONFIG_DTM_TW_TRANSPORT_LOG_LEVEL_ERR` for log level ERROR
* :kconfig:option:`CONFIG_DTM_TW_TRANSPORT_LOG_LEVEL_OFF` to disable logging

Further configuration options are available for the DTM 2-wire UART to HCI conversion library.
See :ref:`dtm_twowire_to_hci_readme` for details.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/direct_test_mode`

.. include:: /includes/build_and_run.txt

.. note::
   On the nRF5340 development kit, this sample requires the :ref:`nrf5340_remote_shell` sample on the application core.
   The Remote IPC shell sample is built and programmed automatically by default.

Testing
=======

After programming the sample to your development kit, you can test it in two main scenarios, as described in the following chapters.

.. note::
   For the |nRF5340DKnoref|, see :ref:`logging_cpunet` for information about the COM terminals on which the logging output is available.

.. _direct_test_mode_testing_anritsu:

Testing with a certified tester
-------------------------------

Conformance testing is performed using a certified tester, such as Anritsu MT8852.
The setup and test procedures depend on the tester used.
Details about the test operation are available in the tester documentation.
The tester handles sending DTM test commands and reading transmitted signals or sending received signals.

The application note `nAN34`_ describes two alternatives for setting up a production test with DTM using one of our old devices.

.. _direct_test_mode_testing_board:

Testing with another development kit
------------------------------------

To test with another development kit running the same sample, you need to send DTM 2-wire UART commands to the kits.
You can send these commands using one of the following methods:

.. _direct_test_mode_testing_app:

Other testing options
---------------------

**Option A: Using the Direct Test Mode app**

The `Direct Test Mode app`_ in `nRF Connect for Desktop`_ generates and sends commands automatically.

1. Connect both development kits to the computer using a USB cable.
#. Start the `Direct Test Mode app`_ in `nRF Connect for Desktop`_ and select one of the development kits.
#. Set the Transmitter mode and configure the test parameters (frequency, packet pattern, length, etc.).
   For example, use channel 37 (2402 MHz) with ``10101010`` packet pattern and 37-byte packet length.
#. Start the test.
   The app will send the ``TRANSMITTER_TEST`` command to the kit.
#. Observe the ``TEST_STATUS_EVENT`` response with the SUCCESS status field.
#. On the second kit, start the `Direct Test Mode app`_ and select it in the application.
#. Set the Receiver mode with matching test parameters.
#. Start the test.
   The app will send the ``RECEIVER_TEST`` command to the kit.
#. Observe the ``TEST_STATUS_EVENT`` response and the packet count increasing on the application chart.
#. Stop the test on the receiver kit, and observe the ``PACKET_REPORTING_EVENT`` with the received packet count.
#. Swap roles and repeat the test in the reverse direction.

**Option B: Sending raw DTM commands**

You can send raw DTM 2-wire UART commands using, for example, a serial terminal emulator.
See `Direct Test Mode terminal connection`_ for the required serial port settings and command format.

1. Connect both development kits to the computer using a USB cable.
   The computer assigns to each development kit a serial port.
   |serial_port_number_list|
#. Connect both kits with a terminal emulator.
   See `Direct Test Mode terminal connection`_ for the required settings.
#. Start ``TRANSMITTER_TEST`` on one kit by sending the ``0x80 0x96`` DTM command.
   This command will trigger TX activity on the 2402 MHz frequency (1st channel) with ``10101010`` packet pattern and 37-byte packet length.
#. Observe that you received the ``TEST_STATUS_EVENT`` packet in response with the SUCCESS status field: ``0x00 0x00``.
#. Start ``RECEIVER_TEST`` on the second kit by sending the ``0x40 0x96`` DTM command.
   Command parameters are identical to the ones used for the ``TRANSMITTER_TEST`` command.
#. Observe that you received the ``TEST_STATUS_EVENT`` packet in response with the SUCCESS status field: ``0x00 0x00``.
#. Finish RX testing on the second kit using the ``TEST_END`` DTM command by sending the ``0xC0 0x00`` packet.
#. Observe that you received the ``PACKET_REPORTING_EVENT`` packet in response, indicated by the most significant bit set to ``1``.
   The other 15 bits of the event contain the number of received packets, which means you can subtract ``0x8000`` from the event (big endian) to get the packet count.
   For example, the ``0xD6 0xAC`` message indicates that 22188 radio packets have been received.
#. Experiment with other combinations of commands and their parameters.
   The protocol is defined in `Bluetooth Core Specification <Bluetooth Core Specification_>`_ Vol. 6, Part F, 3 UART Test Interface.
#. To test the reverse direction, swap the TX and RX roles between the two kits and repeat the process.

Direct Test Mode terminal connection
------------------------------------

To send commands to and receive responses from the development kit that runs the Direct Test Mode sample, connect to it with RealTerm in Windows or Minicom in Linux.

The Bluetooth Low Energy DTM UART interface standard specifies the following configuration:

* Eight data bits
* No parity
* One stop bit
* No hardware flow control
* A selection of bit rates from 9600 to 1000000, one of which must be supported by the DUT.
  It might be possible to run other bit rates by experimenting with parameters.

.. note::
   The default bit rate of the DTM UART driver is 19200 bps, which is supported by most certified testers.

When using a 2-wire interface, you must send all commands as two-byte HEX numbers.
The responses must have the same format.

Connect with RealTerm (Windows)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The RealTerm terminal program offers a graphical interface for setting up your connection.

.. figure:: /images/realterm.png
   :alt: RealTerm start window

   The RealTerm start window

To test DTM with RealTerm, complete the following steps:

1. On the :guilabel:`Display` tab, set **Display As** to **Hex[space]**.

   .. figure:: /images/realterm_hex_display.png
      :alt: Set the RealTerm display format

#. Open the :guilabel:`Port` tab and configure the serial port parameters:

   a. Set the **Baud** to ``19200`` **(1)**.
   #. Select your J-Link serial port from the **Port** list **(2)**.
   #. Set the port status to ``Open`` **(3)**.

   .. figure:: /images/real_term_serial_port.png
      :alt: RealTerm serial port settings

#. Open the :guilabel:`Send` tab:

   a. Write the command as a hexadecimal number in the field **(1)**.
      For example, write ``0x00 0x00`` to send a **Reset** command.
   #. Click the :guilabel:`Send Numbers` button **(2)** to send the command.
   #. Observe the response in the DTM in area **(3)**.
      The response is encoded as hexadecimal numbers.

   .. figure:: /images/realterm_commands.png
      :alt: RealTerm commands sending

Connect with Minicom (Linux)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Minicom is a serial communication program that connects to the DTM device.

On the Linux operating system, install a Minicom terminal.
On Ubuntu, run:

.. code-block:: console

   sudo apt-get install minicom

1. Run the Minicom terminal:

   .. parsed-literal::
      :class: highlight

      sudo minicom -D *DTM serial port* -s

   For example:

   .. code-block:: console

      sudo minicom -D /dev/serial/by-id/usb-SEGGER_J-Link_000683580193-if00 -s

   The **-s** option switches you to Minicom setup mode.

#. Configure the Minicom terminal:

   .. figure:: /images/minicom_setup_window.png
      :alt: minicom configuration window

      Configuration window

   a. Select :guilabel:`Serial port setup` and set UART baudrate to ``19200``.

      .. figure:: /images/minicom_serial_port.png
         :alt: minicom serial port settings

   #. Select :guilabel:`Screen and keyboard` and press S on the keyboard to enable the **Hex Display**.
   #. Press Q on the keyboard to enable **Local echo**.

      .. figure:: /images/minicom_terminal_cfg.png
         :alt: minicom terminal screen and keyboard settings

   Minicom is now configured for receiving data.
   However, you cannot use it for sending DTM commands.

#. Send DTM commands:

   To send DTM commands, use ``echo`` with ``-ne`` options in another terminal.
   You must encode the data as hexadecimal numbers (\xHH, byte with hexadecimal value HH, 1 to 2 digits).

   .. parsed-literal::
      :class: highlight

      sudo echo -ne "*encoded command*" > *DTM serial port*

   To send a **Reset** command, for example, run the following command:

   .. code-block:: console

      sudo echo -ne "\x00\x00" > /dev/serial/by-id/usb-SEGGER_J-Link_000683580193-if00

Limitations
***********

This sample inherits the limitations of the DTM 2-wire UART to HCI conversion library.
See :ref:`dtm_twowire_to_hci_readme` (section Limitations).

Dependencies
************

This sample uses the following |NCS| library:

* :ref:`dtm_twowire_to_hci_readme`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:bluetooth_api`
* :ref:`zephyr:uart_api`
