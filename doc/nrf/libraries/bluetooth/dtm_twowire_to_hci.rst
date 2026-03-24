.. _dtm_twowire_to_hci_readme:

DTM 2-wire UART to HCI Converter
################################

.. contents::
   :local:
   :depth: 2

The DTM 2-wire to HCI Conversion library provides a bridge that fills the gap between a DTM 2-wire UART tester and a Bluetooth® LE Controller.
It allows the application to convert DTM 2-wire UART commands into either 2-wire UART events or HCI commands, and subsequently HCI events into 2-wire UART events.

Overview
********

`Bluetooth Core Specification`_, Vol 6, Part F, 2.1 Test sequences declares both an HCI and a 2-wire UART interface for Direct Test Mode (DTM), as shown in the following table:

+-------------------------------------+-------------------------------------+-------------------------------------+
| RF Test command/event               | HCI command/event                   | 2-wire UART command/event           |
+=====================================+=====================================+=====================================+
| ``LE_Transmitter_Test`` command     | ``HCI_LE_Transmitter_Test`` command | ``LE_Transmitter_Test`` command     |
+-------------------------------------+-------------------------------------+-------------------------------------+
| ``LE_Receiver_Test`` command        | ``HCI_LE_Receiver_Test`` command    | ``LE_Receiver_Test`` command        |
+-------------------------------------+-------------------------------------+-------------------------------------+
| ``LE_Test_End`` command             | ``HCI_LE_Test_End`` command         | ``LE_Test_End`` command             |
+-------------------------------------+-------------------------------------+-------------------------------------+
| ``LE_Status`` event                 | ``HCI_Command_Complete`` event      | ``LE_Test_Status`` event            |
+-------------------------------------+-------------------------------------+-------------------------------------+
| ``LE_Packet_Report`` event          | ``HCI_Command_Complete`` event      | ``LE_Packet_Report`` event          |
+-------------------------------------+-------------------------------------+-------------------------------------+

Many Bluetooth LE Controllers support DTM over HCI, eliminating the need to maintain a custom radio backend in DTM firmware.
However, a large portion of RF testing tools and equipment still rely on the 2-wire UART interface, such as the Direct Test Mode app in nRF Connect for Desktop.
This library provides a translation layer between these interfaces by converting incoming 2-wire UART commands into HCI commands or 2-wire UART events, and converting any resulting HCI events back into 2-wire UART events.

Command and event translation follows the mappings presented in the table.
When an error is detected in either a 2-wire UART command or an HCI event, the library produces an ``LE_Test_Status`` event with an error status.
In addition to the commands in the table, the 2-wire UART interface defines the ``LE_Test_Setup`` command.
This command has no direct HCI equivalent and is mostly used to configure parameters for upcoming tests.
The library buffers these parameters so they can be applied in future ``HCI_LE_Transmitter_Test`` or ``HCI_LE_Receiver_Test`` commands.

Configuration
*************

To enable this library, use the :kconfig:option:`CONFIG_DTM_TWOWIRE_TO_HCI`.

Check and adjust the following Kconfig options:

* :kconfig:option:`CONFIG_DTM_TWOWIRE_TO_HCI_RECEIVER_TEST_VERSION` - Sets the version for HCI command ``HCI_LE_Receiver_Test``.
* :kconfig:option:`CONFIG_DTM_TWOWIRE_TO_HCI_TRANSMITTER_TEST_VERSION` - Sets the version for HCI command ``HCI_LE_Transmitter_Test``.

Also configure the chosen Bluetooth LE Controller to support DTM HCI commands.
For the SoftDevice Controller and Zephyr Bluetooth LE Controller, use the :kconfig:option:`CONFIG_BT_CTLR_DTM_HCI` Kconfig option.

Usage
*****

The flow of the application could look like this:

1. Wait for a 2-wire UART command from the tester.
2. Call the :c:func:`dtm_tw_to_hci_process_tw_cmd` function to process the command.

   * If a 2-wire UART event was generated:

     a. Send the 2-wire UART event to the tester.
     #. Return to **Step 1**.

   * If an HCI command was generated:

     a. Send the HCI command to the controller.
     #. Call the :c:func:`dtm_tw_to_hci_process_hci_event` function to process the resulting HCI event.
     #. Send the generated 2-wire UART event to the tester.
     #. Return to **Step 1**.

Limitations
***********

The following limitations apply to this library:

* Not thread-safe.
  The caller must ensure that calls to the library are serialized, for example by using a mutex or by calling from a single thread.

* Multiple contexts are not supported.
  The DTM test configuration is buffered in the library when 2-wire UART ``LE_Test_Setup`` commands are processed.
  If commands from multiple testers are accepted, they might start tests with unintended configuration.

* Response events to 2-wire UART ``LE_Test_Setup`` commands might generate inaccurate success events.
  ``LE_Test_Setup`` commands that configure the test do not communicate with the controller, as in HCI, the test configuration is passed with the command that starts the test.
  The library generates a success event for these ``LE_Test_Setup`` commands if all parameters are within the protocol specification, but the controller might have further limitations that are unknown to the library.
  This results in the ``LE_Receiver_Test`` or ``LE_Transmitter_Test`` command generating an error event instead of the ``LE_Test_Setup`` command that set the invalid configuration.

* CTE or antenna switching features of the DTM 2-wire UART protocol are not supported.
  Attempting to use these features will result in an error event.

API documentation
*****************

| Header file: :file:`include/bluetooth/dtm_twowire/dtm_twowire_to_hci.h`
| Source file: :file:`lib/dtm_twowire/dtm_twowire_to_hci.c`

.. doxygengroup:: dtm_twowire_to_hci
