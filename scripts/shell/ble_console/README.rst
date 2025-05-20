.. _ble_console_readme:

Bluetooth LE Console
####################

.. contents::
   :local:
   :depth: 2

Bluetooth® LE Console (located in :file:`scripts/shell/ble_console`) is a desktop application that can be used to communicate with an nRF device over Bluetooth® Low Energy using the :ref:`shell_bt_nus_readme`.

The application supports Linux only and cannot be run on Windows.
You should run it on a natively installed Linux.
If you use a virtual machine instead, you might see problems with the Bluetooth LE connection.


Requirements
************

* Bluetooth LE compliant receiver or dongle
* Linux host with BlueZ driver
* :ref:`shell_bt_nus` for communication on the target device

Dependencies
************

Make sure to install the following Python dependencies::

   sudo apt install python-gtk2
   sudo apt install python-vte
   sudo apt install python-dbus


BlueZ configuration
*******************

Bluetooth LE Console requires the Bluetooth daemon to be run in experimental mode.

If you want the Bluetooth daemon to start in experimental mode by default, complete the following steps:

1. Open the file :file:`/etc/systemd/system/bluetooth.service` for editing::

	nano /etc/systemd/system/bluetooth.service

#. Add ``-E`` after ``ExecStart=/usr/lib/bluetooth/bluetoothd``.
   The resulting line should look like this::

	ExecStart=/usr/lib/bluetooth/bluetoothd -E

#. Save the file and exit the editor.

#. Restart the service::

	sudo service bluetooth restart

If you are using a using virtual machine, the Bluetooth service must probably be restarted after every machine start.
To do so, run the following command::

	sudo service bluetooth restart

Pairing with the device
***********************

After configuring the Bluetooth daemon, you can pair with the device using the Bluetooth tools provided by your Linux vendor.
For example, in Ubuntu go to :guilabel:`Settings` > :guilabel:`Bluetooth`.

After pairing successfully, you can start using Bluetooth LE Console.
