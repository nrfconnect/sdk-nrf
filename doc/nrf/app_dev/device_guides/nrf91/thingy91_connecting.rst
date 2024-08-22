.. _thingy91_serialports:

Connecting to Thingy:91
#######################

.. contents::
   :local:
   :depth: 2

You can connect to Thingy:91 wirelessly (using the `nRF Toolbox`_ app) or over a serial connection (using `nRF Connect Serial Terminal`_, `Cellular Monitor`_, or a serial terminal).

Using nRF Toolbox
*****************

To connect to your Thingy:91 wirelessly, you need to meet the following prerequisites:

* The :ref:`connectivity_bridge` installed on your Thingy:91.
* The :ref:`nus_service_readme` enabled.

  .. note::
     By default, the Bluetooth LE interface is off, as the connection is not encrypted or authenticated.
     To turn it on at runtime, set the appropriate option in the :file:`Config.txt` file located on the USB Mass storage Device.

Using a serial terminal
***********************

If you prefer to use a standard serial terminal, you need to specify the baud rate manually.

Thingy:91 uses the following UART baud rate configuration:

.. list-table::
   :header-rows: 1
   :align: center

   * - UART Interface
     - Baud Rate
   * - UART_0
     - 115200
   * - UART_1
     - 1000000

Using nRF Connect Serial Terminal
*********************************

You can use the `nRF Connect Serial Terminal`_ application to get debug output and send AT commands to the Thingy:91.
In the case of nRF Connect Serial Terminal or Cellular Monitor, the baud rate for the communication is set automatically.
