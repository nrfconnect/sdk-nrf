.. _gs_testing:

Testing a sample application
############################

Follow the instructions in the **Testing** section of the documentation for the sample to ensure that the application runs as expected.

Information about the current state of the application is usually provided through the LEDs and/or via UART.

.. _putty:

How to connect with PuTTY
*************************

To see the UART output, connect to the board with a terminal emulator, for example, PuTTY.

Connect with the following settings:

 * Baud rate: 115200
 * 8 data bits
 * 1 stop bit
 * No parity
 * HW flow control: None

.. _lte_connect:

How to connect with LTE Link Monitor
************************************

To connect to nRF9160-based boards (for example, the nRF9160 DK or Nordic Thingy:91), you can also use `LTE Link Monitor`_, which is implemented in `nRF Connect for Desktop`_.
This application is used to establish LTE communication with the nRF9160 modem through AT commands, and it also displays the UART output.

To connect to the nRF9160-based board with LTE Link Monitor, perform the following steps:

1. Launch the LTE Link Monitor app.

   .. note::

      Make sure that **Automatic requests** is enabled in LTE Link Monitor.

#. Connect the nRF9160-based board to the PC with a USB cable.
#. Power on the nRF9160-based board.
#. Click **Select Device** and select the particular board entry from the drop-down list in the LTE Link Monitor.
#. Observe that the LTE Link monitor app starts AT communication with the modem of the nRF9160-based board and shows the status of the communication in the display terminal.
   The app also displays any information that is logged on UART.

   .. note::

      In the case of nRF9160 DK, the reset button must be pressed to restart the device and to start the application.
