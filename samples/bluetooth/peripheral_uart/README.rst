.. _peripheral_uart:

Bluetooth: Peripheral UART
##########################

.. contents::
   :local:
   :depth: 2

The Peripheral UART sample demonstrates how to use the :ref:`nus_service_readme`.
It uses the NUS service to send data back and forth between a UART connection and a Bluetooth® LE connection, emulating a serial port over Bluetooth LE.

Overview
********

When connected, the sample forwards any data received on the RX pin of the UART 0 peripheral to the Bluetooth LE unit.
On Nordic Semiconductor's development kits, the UART 0 peripheral is typically gated through the SEGGER chip to a USB CDC virtual serial port.

Any data sent from the Bluetooth LE unit is sent out of the UART 0 peripheral's TX pin.

.. note::
   Thingy:53 uses second instance of USB CDC ACM class instead of UART 0, because it has no built-in SEGGER chip that could be used to gate UART 0.

.. _peripheral_uart_debug:

Debugging
=========

In this sample, the UART console is used to send and read data over the NUS service.
Debug messages are not displayed in this UART console.
Instead, they are printed by the RTT logger.

If you want to view the debug messages, follow the procedure in :ref:`testing_rtt_connect`.

.. note::
   On Thingy:53 debug logs are provided over the USB CDC ACM class serial port, instead of using RTT.

FEM support
***********

.. include:: /includes/sample_fem_support.txt

.. _peripheral_uart_minimal_ext:

Minimal sample variant
======================

You can build the sample with a minimum configuration as a demonstration of how to reduce code size and RAM usage.
This variant is available for resource-constrained boards.

See :ref:`peripheral_uart_sample_activating_variants` for details.

.. _peripheral_uart_cdc_acm_ext:

USB CDC ACM extension
=====================

For the boards with the USB device peripheral, you can build the sample with support for the USB CDC ACM class serial port instead of the physical UART.
This build uses the UART async adapter module that acts as a bridge between the USB CDC ACM that provides only the interrupt interface and the default sample configuration that uses the UART async API.

For more information about the adapter, see the :file:`uart_async_adapter` source files available in the :file:`peripheral_uart/src` directory.
See :ref:`peripheral_uart_sample_activating_variants` for details about how to build the sample with this extension.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuapp_ns, nrf52840dk_nrf52840, nrf52840dk_nrf52811, nrf52833dk_nrf52833, nrf52833dk_nrf52820, nrf52833dk_nrf52820, nrf52dk_nrf52832, nrf52dk_nrf52810, thingy53_nrf5340_cpuapp, nrf21540dk_nrf52840

.. note::
   * The boards ``nrf52dk_nrf52810``, ``nrf52840dk_nrf52811``, and ``nrf52833dk_nrf52820`` only support the `Minimal sample variant`_.
   * When used with :ref:`zephyr:thingy53_nrf5340`, the sample supports the MCUboot bootloader with serial recovery and SMP DFU over Bluetooth.
     Thingy:53 has no built-in SEGGER chip, so the UART 0 peripheral is not gated to a USB CDC virtual serial port.

The sample also requires a smartphone or tablet running a compatible application.
The `Testing`_ instructions refer to `nRF Connect for Mobile`_, but you can also use other similar applications (for example, `nRF Blinky`_ or `nRF Toolbox`_).

You can also test the application with the :ref:`central_uart` sample.
See the documentation for that sample for detailed instructions.

.. note::
   |thingy53_sample_note|

   The sample also enables an additional USB CDC ACM port that is used instead of UART 0.
   Because of that, it uses a separate USB Vendor and Product ID.

User interface
**************

The user interface of the sample depends on the hardware platform you are using.

Development kits
================

LED 1:
   Blinks with a period of 2 seconds, duty cycle 50%, when the main loop is running (device is advertising).

LED 2:
   On when connected.

Button 1:
   Confirm the passkey value that is printed in the debug logs to pair/bond with the other device.

Button 2:
   Reject the passkey value that is printed in the debug logs to prevent pairing/bonding with the other device.

Thingy:53
=========

RGB LED:
   The RGB LED channels are used independently to display the following information:

   * Red channel blinks with a period of 2 seconds, duty cycle 50%, when the main loop is running (device is advertising).
   * Green channel displays if device is connected.

Button:
   Confirm the passkey value that is printed in the debug logs to pair/bond with the other device.
   Thingy:53 has only one button, therefore the passkey value cannot be rejected by pressing a button.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/peripheral_uart`

.. include:: /includes/build_and_run.txt

.. _peripheral_uart_sample_activating_variants:

Activating sample extensions
============================

To activate the optional extensions supported by this sample, modify :makevar:`OVERLAY_CONFIG` in the following manner:

* For the minimal build variant, set :file:`prj_minimal.conf`.
* For the USB CDC ACM extension, set :file:`prj_cdc.conf`.

See :ref:`cmake_options` for instructions on how to add this option.
For more information about using configuration overlay files, see :ref:`zephyr:important-build-vars` in the Zephyr documentation.

.. _peripheral_uart_testing:

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Connect the device to the computer to access UART 0.
   If you use a development kit, UART 0 is forwarded as a COM port (Windows) or ttyACM device (Linux) after you connect the development kit over USB.
   If you use Thingy:53, you must attach the debug board and connect an external USB to UART converter to it.
#. |connect_terminal|
#. Optionally, you can display debug messages. See :ref:`peripheral_uart_debug` for details.
#. Reset the kit.
#. Observe that **LED 1** is blinking and that the device is advertising with the device name that is configured in :kconfig:`CONFIG_BT_DEVICE_NAME`.
#. Observe that the text "Starting Nordic UART service example" is printed on the COM listener running on the computer.
#. Connect to the device using nRF Connect for Mobile.
   Observe that **LED 2** is on.
#. Optionally, pair or bond with the device with MITM protection.
   This requires using the passkey value displayed in debug messages.
   See :ref:`peripheral_uart_debug` for details on how to access debug messages.
   To confirm pairing or bonding, press **Button 1** on the device and accept the passkey value on the smartphone.
#. In the application, observe that the services are shown in the connected device.
#. Select the UART RX characteristic value in nRF Connect.
   You can write to the UART RX and get the text displayed on the COM listener.
#. Type '0123456789' and tap :guilabel:`SEND`.
   Verify that the text "0123456789" is displayed on the COM listener.
#. To send data from the device to your phone or tablet, enter any text, for example, "Hello", and press Enter to see it on the COM listener.
   Observe that a notification is sent to the peer.
#. Disconnect the device in nRF Connect.
   Observe that **LED 2** turns off.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nus_service_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``boards/arm/nrf*/board.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:api_peripherals`:

   * ``incude/gpio.h``
   * ``include/uart.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``
