.. _uart_terminal_script:

UART terminal script
####################

.. contents::
   :local:
   :depth: 2

The UART terminal script can be used with any setup that uses several development kits to start a terminal connection to all DKs at the same time.
This can save time needed to manually establish multiple connections, which speeds up :ref:`gs_testing`.

For example, when working with the :ref:`nrf53_audio_app` in the BIS mode with several receivers, you can use the script instead of manually opening a terminal connection to each DK.

Overview
********

Once started, the script checks for existing serial connections by running the ``nrfjprog --com`` command in the background.
It then starts a terminal connection for each DK's COM port with number corresponding to the one specified in the :file:`get_serial_ports.py` file.
By default, the terminal connection is opened for the COM port of the application core (``VCOM0``).

Requirements
************

The script source files are located in the :file:`scripts/uart_terminal` directory.

To use the script, make sure that you have the following software installed for your operating system:

* Windows: Python and PuTTy
* Linux: Python, Terminator, and Minicom

Using the script
****************

To start the script, run the ``python .\uart_terminal.py`` command in the :file:`script/uart_terminal` directory.

The script does not take any arguments.

After running the script, it will list all available VCOM ports for each connected development kit and open a terminal connection window for the desired port.

For example, if you have three DKs connected, a similar output might appear:

.. code-block:: console

   960385074    COM28    VCOM0
   960385074    COM29    VCOM1
   960385074    COM30    VCOM2
   1050166277    COM16    VCOM0
   1050166277    COM11    VCOM1
   960390887    COM18    VCOM0
   960390887    COM23    VCOM1
   960390887    COM22    VCOM2

In this list, ``VCOM0`` refers to the application core and ``VCOM1`` refers to the network core.
Using the default settings, the script will open a terminal window for each ``VCOM0`` port.

Changing the default port
=========================

If you want to change the port number for the terminal connection, edit the following lines in the :file:`get_serial_port.py` file in the :file:`scripts` directory by replacing ``VCOM0`` with the desired port number:

.. core-block:: python

   for line in output_decoded_lines:
    if "VCOM0" in line:
        info = line.split("    ")
        ports.append(info[1])
