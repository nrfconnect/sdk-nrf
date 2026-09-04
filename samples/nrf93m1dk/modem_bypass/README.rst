.. _nrf93m1dk_modem_bypass:

.. ncs-sample::
   :title: nRF93M1 DK: Modem bypass

   This sample configures the modem UART switch in bypass mode, forwarding it to the USB CDC-ACM VCOM port.
   This allows a host PC can send AT commands directly to the modem.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The sample configures the nRF54L15 host core to directly  route the modem serial UART to the USB virtual COM port.
It also handles modem power sequencing on startup using a deferred work item.

The following GPIOs are exposed through the buttons on the development kit:

* **Button 1** - Triggers the ``POWER_KEY`` signal to the modem.
* **Button 2** - Triggers the ``RESET`` signal to the modem.

The UART baud rate is 115200.

Configuration
*************

|config|

Sysbuild Kconfig options
========================

When building for the ``nrf93m1dk/nrf54l15/cpuflpr`` board target using sysbuild, you can combine this sample with the :ref:`radio_test` running on the application core.

To enable this, use the following sysbuild configuration options:

* Set :kconfig:option:`SB_CONFIG_APPCORE_RADIO_TEST` to ``y``.
* Set :kconfig:option:`SB_CONFIG_VPR_LAUNCHER` to ``n``.

For example, when building on the command line, enter the following command:

.. code-block:: console

   west build -p -b nrf93m1dk/nrf54l15/cpuflpr --sysbuild -- -DSB_CONFIG_APPCORE_RADIO_TEST=y -DSB_CONFIG_VPR_LAUNCHER=n


Building and running
********************

.. |sample path| replace:: :file:`samples/nrf93m1dk/modem_bypass`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to the development kit, complete the following steps:

1. Connect the kit to the host PC using a USB cable.
   The kit enumerates as a USB CDC-ACM serial port.
#. Open a serial terminal (for example, nRF Connect Serial Terminal) at 115200 baud.
#. Send an AT command, for example, ``AT``, and verify that the modem responds with ``OK``.
#. Press **Button 1** briefly to trigger the modem ``POWER_KEY`` signal.
#. Press **Button 2** to trigger the modem reset signal.
