.. _bt_nus_shell_script:

Bluetooth LE NUS shell script
#############################

.. contents::
   :local:
   :depth: 2

The Bluetooth® LE NUS shell script forwards data between TCP clients and a Bluetooth LE device using the Nordic UART Service.

Overview
********

The script scans for a Bluetooth LE device advertising with a specified name (for example, ``BT_NUS_shell``).
Once connected, it creates a TCP socket server on the local machine (default port: 8889) that forwards data between TCP clients and the Bluetooth LE device's NUS characteristics.

Requirements
************

.. note::
   This script depends on the ``pc_ble_driver_py`` Python package, which is no longer maintained.
   As a result, only Python versions above or equal to 3.7 and below 3.11 are supported.

The script source files are located in the :file:`scripts/shell` directory.

To install the script's requirements, run the following command in its directory:

.. code-block:: console

   python3 -m pip install -r requirements.txt

This script also requires an nRF52 Series development kit to run the connectivity firmware that is programmed automatically when running the script with the ``--snr`` argument.

Using the script
****************

1. Connect your nRF52 Series development kit to your PC.
#. Run the script commands in the :file:`scripts/shell` directory.

If you provide the ``--snr`` argument, your DK will be programmed with the connectivity firmware.
To list the script arguments, run the following command:

.. code-block:: console

   python3 bt_nus_shell.py -h

The following help information describes the script arguments available:

.. code-block:: console

   Bluetooth LE serial daemon.

   required arguments:
     --com COM [COM ...]   COM port name (e.g. COM110).

   optional arguments:
     -h, --help            show this help message and exit
     --snr SNR             segger id.
     --family FAMILY       chip family.
     --name NAME [NAME ...]
                           Device name (advertising name).
     --port PORT           Local port to use.

To use the script without any additional configuration, run the following command:

.. parsed-literal::

   python3 bt_nus_shell.py --com *Serial port name*

For example:

.. code-block:: console

   python3 bt_nus_shell.py --com /dev/ttyACM0

To connect to a specific device name and use a custom port, run the following command:

.. code-block:: console

   python3 bt_nus_shell.py --com /dev/ttyACM0 --name MyDevice --port 9999

Dependencies
************

The script uses the following Python libraries:

   * `pc-ble-driver-py`_
   * `nRF Util`_
