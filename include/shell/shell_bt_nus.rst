.. _shell_bt_nus_readme:

Nordic UART Service (NUS) shell transport
#########################################

The BLE GATT Nordic UART Service shell transport allows to receive shell commands remotely over *Bluetooth*.
It uses the :ref:`nus_service_readme`.

The NUS Service shell transport is used in the :ref:`shell_bt_nus` sample.

Setup
*****

Enable NUS shell transport in your application to be able to receive shell commands remotely.

To send shell commands to an application that uses this module, you need specific software, comparable to a terminal (for example, PuTTY) for UART communication.

.. _shell_bt_nus_host_tools:

Sending shell commands
**********************

The |NCS| provides two alternatives for sending shell commands from a host (for example, a PC) to the application that uses this module.
`bt_nus_shell.py`_ is a Python script that requires a terminal and a second development board.
`BLE Console`_ is a stand-alone application for Linux.

bt_nus_shell.py
===============

The script file ``scripts/shell/bt_nus_shell.py`` contains a cross-platform example host application, written in Python 2.

The script uses an additional Nordic Development Kit (for example, PCA10040) as a Bluetooth central device.
It connects to the specified device and forwards all NUS traffic to the network port.
You can then use a terminal application, for example PuTTY, to connect to that port (the default port is 8889) and use the shell.

The script requires ``nrfutil`` Python package to be installed.

To install it, open a terminal window and enter the following command:

.. parsed-literal::
   :class: highlight

   pip2 install --user nrfutil==4.0.0

The script requires the following parameters:

* ``com`` - port of the development board used by the script
* ``snr`` - SEGGER board ID
* ``family`` - chip family of the development board (for example, NRF52)
* ``name`` - advertising name of the device with the NUS shell

.. note::
   The script does not support reconnection.
   Therefore, you must restart both the script and the terminal application after each reconnection.

.. testing_start

Perform the following steps to use the ``bt_nus_shell.py`` script:

1. Connect a Nordic Development Kit (for example, PCA10040) to your PC.
#. Start the ``bt_nus_shell.py`` script with the correct parameters, for example::

       bt_nus_shell.py --snr 680834186 --name BT_NUS_shell --com COM115 --family NRF52
#. Open a terminal, for example PuTTY.
   Set the Connection Type to ``Raw`` and the  Destination Address to ``127.0.0.1:8889``.
#. Press Enter in the terminal.
   A console prompt is displayed.
#. Enter the commands that you want to execute on the remote shell.

.. testing_end

BLE Console
***********

The BLE Console is a stand-alone Linux application that uses a standard Bluetooth device (HCI dongle or built-in Bluetooth device) and the BlueZ stack to communicate over Bluetooth with the device that runs the NUS shell transport.

See :ref:`ble_console_readme` for more information.

API documentation
*****************

.. doxygengroup:: shell_bt_nus
   :project: nrf
   :members:
