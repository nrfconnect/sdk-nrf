.. _snippet_ot_usb:

OpenThread snippet for USB transport (ot-usb)
=============================================

.. contents::
   :local:
   :depth: 2

To build with this snippet, follow the instructions in the :ref:`using-snippets` page.
When building with west, run the following command:

.. code-block:: console

   west build -- -D<project_name>_SNIPPET=ot-usb

Overview
********

This snippet enables USB CDC ACM transport for the OpenThread stack.
When using this snippet, all communication with the OpenThread stack is done over USB.

.. note::

    This snippet is not supported on boards without a USB peripheral (for example, nRF54L15 DK).

Configuration
*************

To change the USB device product string, set the :kconfig:option:`CONFIG_CDC_ACM_SERIAL_PRODUCT_STRING` Kconfig option in your application's :file:`prj.conf` file.
