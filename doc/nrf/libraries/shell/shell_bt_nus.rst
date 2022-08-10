.. _shell_bt_nus_readme:

Nordic UART Service (NUS) shell transport
#########################################

.. contents::
   :local:
   :depth: 2

The Bluetooth® LE GATT Nordic UART Service shell transport allows you to receive shell commands remotely over *Bluetooth*.
It uses the :ref:`nus_service_readme`.

The NUS Service shell transport is used in the :ref:`shell_bt_nus` sample.

Setup
*****

Enable NUS shell transport in your application to be able to receive shell commands remotely.

.. _shell_bt_nus_host_tools:

Sending shell commands
**********************

.. testing_general_start

The |NCS| provides two alternatives for sending shell commands from a host, like a PC, to the application that uses this module:

* `bt_nus_shell.py`_. It is a Python script that requires a console application, like PuTTY, and a second development kit.
* `Bluetooth LE Console`_. It is a stand-alone application for Linux.

.. testing_general_end

bt_nus_shell.py
===============

.. testing_bt_nus_shell_intro_start

The script file :file:`scripts/shell/bt_nus_shell.py` contains a cross-platform example host application, written in Python 3.

The script uses an additional Nordic development kit, like the PCA10040, as a Bluetooth central device.
It connects to the specified device and forwards all NUS traffic to the network port.
You can then use a console application, like PuTTY, to connect to that port and use the shell.
The default port is set to ``8889``.

.. testing_bt_nus_shell_intro_end

To install the script dependencies, open your preferred command-line interface, and enter the following command in the |NCS| root directory:

   .. code-block:: console

      pip install --user -r scripts/shell/requirements.txt

The script requires the following parameters:

* ``com`` - the COM port of the development kit used by the script

Additionally, following parameters are option:

* ``snr`` - the SEGGER board ID
* ``family`` - the chip family of the development kit, for example, nRF52
* ``name`` - the advertising name of the device with the NUS shell
* ``port`` - the local network port

.. note::
   The script does not support reconnections.
   Therefore, you must restart both the script and the console application after each reconnection.

Perform the following steps to use the :file:`bt_nus_shell.py` script:

1. Connect a Nordic Development Kit, like the PCA10040, to your PC.
#. Start the :file:`bt_nus_shell.py` script with the correct parameters, for example:

   .. code-block:: console

      bt_nus_shell.py --name BT_NUS_shell --com COM237 --family NRF52 --snr 682560213

#. Open a console application, for example, PuTTY.
   Open a new session, setting the :guilabel:`Connection Type` to :guilabel:`Raw` and the :guilabel:`Destination Address` to ``127.0.0.1:8889``.
#. Press Enter in the terminal window.
   A console prompt is displayed.
#. Enter the commands that you want to execute in the remote shell.

Bluetooth LE Console
====================

The Bluetooth LE Console is a stand-alone Linux application that uses a standard Bluetooth device, like an HCI dongle or a built-in Bluetooth device, and the BlueZ stack to communicate over Bluetooth with the device that runs the NUS shell transport.

See :ref:`ble_console_readme` for more information.

API documentation
*****************

| Header file: :file:`include/shell/shell_bt_nus.h`
| Source file: :file:`subsys/shell/shell_bt_nus.c`

.. doxygengroup:: shell_bt_nus
   :project: nrf
   :members:
