.. _at_client_sample:

Cellular: AT Client
###################

.. contents::
   :local:
   :depth: 2

The AT Client sample demonstrates the asynchronous serial communication taking place over UART to the nRF9160 modem.
The sample enables you to use an external computer or MCU to send AT commands to the LTE-M/NB-IoT modem of your nRF9160 device.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The AT Client sample acts as a proxy for sending directives to the nRF9160 modem using AT commands.
This facilitates the reading of responses or analyzing of events related to the nRF9160 modem.
You can initiate the commands from a terminal or the `LTE Link Monitor`_, which is an application implemented as part of `nRF Connect for Desktop`_.

For more information on the AT commands, see the `AT Commands Reference Guide`_.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/at_client`

.. include:: /includes/build_and_run_ns.txt


Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Press the reset button on the nRF9160 DK to reboot the kit and start the AT Client sample.
#. :ref:`Connect to the nRF9160 DK with LTE Link Monitor<lte_connect>`.

   .. note::

      Make sure that **Automatic requests** is enabled in LTE Link Monitor.

#. Observe that initially the command :command:`AT+CFUN?` is automatically sent to the modem, which returns a value 4, indicating that the modem is in the offline mode.
#. Observe that the LTE Link Monitor terminal display also shows :command:`AT+CFUN=1` followed by ``OK`` indicating that the modem has changed to the normal mode.
#. Run the following commands from the LTE Link Monitor terminal:

   a. Enter the command: :command:`AT+CFUN?`.

      This command reads the current functional mode of the modem and triggers the command :command:`AT+CFUN=1` which sets the functional mode of the modem to normal.

   #. Enter the command :command:`AT+CFUN?` into the LTE Link Monitor terminal again.

      The UART/Modem/UICC/LTE/PDN indicators in the LTE Link Monitor side panel turn green.
      This command also automatically launches a series of commands like:

      * :command:`AT+CGSN=1`, which displays the product serial identification number (IMEI).
      * :command:`AT+CGMI`, which displays the manufacturer name.
      * :command:`AT+CGMM`, which displays the model identification name.
      * :command:`AT+CGMR`, which displays the revision identification.
      * :command:`AT+CEMODE`, which displays the current mode of operation.

   #. Enter the command: :command:`AT%XOPERID`.

      This command returns the network operator ID.

   #. Enter the command: :command:`AT%XMONITOR`.

      This command returns the modem parameters.

   #. Enter the command: :command:`AT%XTEMP?`.

      This command displays the current modem temperature.

   #. Enter the command: :command:`AT%CMNG=1`.

      This command displays a list of all certificates that are stored on your device.
      If you add the device to nRF Cloud, a CA certificate, a client certificate, and a private key with security tag 16842753 (which is the security tag for nRF Cloud credentials) are displayed.


Sample output
=============

Following is a sample output of the command :command:`AT%XMONITOR`:

.. code-block:: console

   AT%XMONITOR
   %XMONITOR: 5,"","","24201","76C1",7,20,"0102DA03",105,6400,53,24,"","11100000","11100000"
   OK


References
**********

`AT Commands Reference Guide`_


Dependencies
************

This sample uses the following |NCS| libraries:


* :ref:`lib_at_host` which includes:

  * :ref:`at_monitor_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
