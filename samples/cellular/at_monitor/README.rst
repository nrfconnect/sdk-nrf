.. _at_monitor_sample:

Cellular: AT monitor
####################

.. contents::
   :local:
   :depth: 2

The AT monitor sample demonstrates how to use the :ref:`at_monitor_readme` library and define AT monitors to receive AT notifications from the :ref:`nrfxlib:nrf_modem`.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample defines two AT monitors, one for network status notifications (``+CEREG``) and one for received signal quality parameters notifications (``+CESQ``) through the :c:func:`AT_MONITOR` macro.
The sample then subscribes to both notifications and switches the modem to function mode one to register to the network.
While the device is registering to the network, the sample uses one of the AT monitors to determine if the registration is complete and monitors the signal quality using the other monitor.
Once the device registers with the network, the sample reads the modem PSM mode status, enables it, and reads the PSM mode status again.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/at_monitor`

.. include:: /includes/build_and_run_ns.txt


Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Power on or reset your nRF9160 DK.
#. Observe that the sample starts and connects to the LTE network, while displaying both the network registration status and signal quality during the process.
#. Observe that the sample displays the PSM status twice on the terminal, once when it is disabled, and once when it is enabled.
#. Observe that the sample completes with a message on the terminal.

Sample Output
=============

The sample shows the following output:

.. code-block:: console

	AT Monitor sample started
	Subscribing to notifications
	Connecting to network
	Resuming link quality monitor for AT notifications
	Waiting for network
	Link quality: -61 dBm
	Network registration status: searching
	Link quality: -59 dBm
	Network registration status: home
	Network connection ready
	Pausing link quality monitor for AT notifications
	Reading PSM info...
	  PSM: disabled
	Enabling PSM
	Reading PSM info...
	  PSM: enabled
	Periodic TAU string: 00000110
	Active time string: 00100001
	Modem response:
	+CEREG: 1,1
	OK
	Shutting down modem
	Network registration status: no network
	Bye

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`at_monitor_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
