.. _shell_bt_nus:

Bluetooth: NUS shell transport
##############################

.. contents::
   :local:
   :depth: 2

The Nordic UART Service (NUS) shell transport sample demonstrates how to use the :ref:`shell_bt_nus_readme` to receive shell commands from a remote device over Bluetooth®.

Overview
********

When the connection is established, you can connect to the sample through the :ref:`nus_service_readme` by using a host application.
You can then send shell commands that are executed on the device running the sample, and see the logs.
See :ref:`shell_bt_nus_host_tools` for more information about the host tools available in |NCS| for communicating with the sample.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

You also need an additional nRF52 development kit, like the PCA10040 for connecting using the :file:`bt_nus_shell.py` script.
Alternatively, you can use :ref:`ble_console_readme` for connecting, using Linux only.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/shell_bt_nus`

.. include:: /includes/build_and_run_ns.txt

Testing
*******

The |NCS| provides two alternatives for testing the sample:

* `Testing using the shell_bt_nus script`_. It is a Python 3 script that requires a console application, like PuTTY, and a second development kit.
* `Testing using the Bluetooth LE Console`_. It is a stand-alone application for Linux.

Testing using the shell_bt_nus script
=====================================

.. include:: /libraries/shell/shell_bt_nus.rst
   :start-after: testing_bt_nus_shell_intro_start
   :end-before: testing_bt_nus_shell_intro_end

After programming the sample to your development kits, complete the following steps to test it:

1. Start a console application, like PuTTY, and connect through UART to the ``shell_bt_nus`` application running on the development kit to check the log.
   See :ref:`testing` for more information on how to connect with PuTTY through UART.
#. Run the following command in the |NCS| root directory to install the script dependencies:

   .. code-block:: console

      pip install --user -r scripts/shell/requirements.txt

#. Connect to your PC the nRF52 development kit meant to use the :file:`bt_nus_shell.py` script.
#. Start the :file:`bt_nus_shell.py` script with the correct parameters, for example:

   .. code-block:: console

      bt_nus_shell.py --name BT_NUS_shell --com COM237 --family NRF52 --snr 682560213

#. Open a console application, like PuTTY, and open a new session, setting the **Connection Type** to **Raw** and the **Destination Address** to ``127.0.0.1:8889``.
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

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/uuid.h`
  * :file:`include/bluetooth/gatt.h`
  * :file:`samples/bluetooth/gatt/bas.h`
* :ref:`zephyr:logging_api`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
