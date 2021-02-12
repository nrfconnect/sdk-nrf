.. _shell_bt_nus:

Bluetooth: NUS shell transport
##############################

.. contents::
   :local:
   :depth: 2

The Nordic UART Service (NUS) shell transport sample demonstrates how to use the :ref:`shell_bt_nus_readme` to receive shell commands from a remote device.

Overview
********

When the connection is established, you can connect to the sample through the :ref:`nus_service_readme` by using a host application.
You can then send shell commands, that are executed on the device that runs the sample, and see the logs.
See :ref:`shell_bt_nus_host_tools` for more information about the host tools available, in |NCS|, for communicating with the sample.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuappns, nrf52840dk_nrf52840, nrf52dk_nrf52832, nrf52833dk_nrf52833, nrf52833dk_nrf52820

You also need an additional nRF52 development kit, like the PCA10040 for connecting using the :file:`bt_nus_shell.py` script.
Alternatively, you can use :ref:`ble_console_readme` for connecting, using Linux only.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/shell_bt_nus`

.. include:: /includes/build_and_run.txt

Testing
*******

The |NCS| provides two alternatives for testing the sample:

* `Testing using shell_bt_nus`_. It is a Python 3 script that requires a console application, like PuTTY, and a second development kit.
* `Testing using the Bluetooth LE Console`_. It is a stand-alone application for Linux.

Testing using shell_bt_nus
==========================

.. include:: ../../../include/shell/shell_bt_nus.rst
   :start-after: testing_bt_nus_shell_intro_start
   :end-before: testing_bt_nus_shell_intro_end

After programming the sample to your development kits, test it by performing the following steps:

1. Start a console application, like PuTTY, and connect through UART to the ``shell_bt_nus`` application running on the development kit to check the log.
   See :ref:`gs_testing` for more information on how to connect with PuTTY through UART.
#. Run the following command in the |NCS| root directory to install the script dependencies:

   .. code-block:: console

      pip install --user -r scripts/shell/requirements.txt

#. Connect to your PC the nRF52 development kit meant to use the :file:`bt_nus_shell.py` script.
#. Start the :file:`bt_nus_shell.py` script with the correct parameters, for example:

   .. code-block:: console

      bt_nus_shell.py --name BT_NUS_shell --com COM237 --family NRF52 --snr 682560213

#. Open a console application, like PuTTY, and open a new session, setting the :guilabel:`Connection Type` to :guilabel:`Raw` and the :guilabel:`Destination Address` to ``127.0.0.1:8889``.
#. Press Enter in the terminal window.
   A console prompt is displayed showing a log message that indicates the active connection.
#. Enter the commands that you want to execute.

Testing using the Bluetooth LE Console
======================================

See :ref:`ble_console_readme` for more information on how to test the sample using the Bluetooth LE Console.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`shell_bt_nus_readme`
* :ref:`nus_service_readme`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``
  * ``include/bluetooth/gatt.h``
  * ``samples/bluetooth/gatt/bas.h``
* :ref:`zephyr:logging_api`
