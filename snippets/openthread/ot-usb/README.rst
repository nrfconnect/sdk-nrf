.. _snippet_ot_usb:
.. _snippet-ot-usb:

OpenThread snippet for USB transport (ot-usb)
#############################################

.. contents::
   :local:
   :depth: 2

The ``ot-usb`` snippet enables USB CDC ACM transport for the OpenThread stack.
When using this snippet, all communication with the OpenThread stack is done over USB.

Overview
********

The snippet adds a CDC ACM UART instance and selects it as the console, the shell UART, and the OpenThread UART in devicetree.
It also enables the next USB device stack (:kconfig:option:`CONFIG_USB_DEVICE_STACK_NEXT`) and the UART line control API (:kconfig:option:`CONFIG_UART_LINE_CTRL`).

.. note::

    This snippet is not supported on boards without a USB peripheral.
    For example, it does not support the ``nrf54l15dk/nrf54l15/cpuapp`` and ``nrf54l15dk/nrf54l10/cpuapp`` board targets, because these platforms do not have a USB peripheral.

Configuration
*************

To change the USB device product string, set the :kconfig:option:`CONFIG_CDC_ACM_SERIAL_PRODUCT_STRING` Kconfig option in your application's :file:`prj.conf` file.

Usage
*****

To build with this snippet, follow the instructions in the :ref:`zephyr:using-snippets` page.
When building with west, run the following command:

.. code-block:: console

   west build -- -D<project_name>_SNIPPET=ot-usb
