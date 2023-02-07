.. _mds_ble_gateway_script:

MDS BLE gateway script
######################

.. contents::
   :local:
   :depth: 2

The MDS BLE gateway is an application used as a client for the :ref:`mds_readme`.
It forwards data received on the Memfault Diagnostic Service to the Memfault`s cloud for further analysis.
The destination data URI and the Memfault project key are read from the service characteristics.

Overview
********

The script scans for the :ref:`mds_readme` UUID in the advertising packets.
Once it finds the MDS service UUID, it connects to the remote device and reads the MDS data.
The scripts waits for the MDS notifications and forwards them to the URI read from the URI characteristics.

For more details about the Memfault, see `Memfault SDK`_ and :ref:`ug_memfault` for details on integration with the |NCS|.

Requirements
************

The script source files are located in the :file:`scripts/memfault` directory.

To install the script's requirements, run the following command in its directory:

.. code-block:: console

   python3 -m pip install -r requirements.txt

This script also requires an the nRF52 Series development kit to run the connectivity firmware that is programmed automatically when running the script.

Using the script
****************

1. Connect your nRF52 Series development kit to your PC.
#. Run the script commands in the :file:`scripts/memfault` directory.

Your DK will be programmed with the connectivity firmware.
To list the script arguments, run the following command:

.. code-block:: console

   python3 mds_ble_gateway.py -h

The following help information describes the script arguments available:

.. code-block:: console

   Memfault BLE gateway

   optional arguments:
     -h, --help            show this help message and exit
     --snr SNR             Segger chip ID
     --com COM             COM port name. For example COM0 or /dev/ttyACM0
     --erase, -e           Erase target device before flashing the firmware
     --bond-disable        Disable bonding simulation
     --reconnections RECONNECTIONS
                           Number of reconnection attempts
     --timeout TIMEOUT, -t TIMEOUT
                           Connection establish timeout in seconds

To use the script without any additional configuration, run the following command:

.. parsed-literal::

   python3 mds_ble_gateway.py --snr *your DK Segger chip ID* --com *Serial port name*

For example:

.. code-block:: console

   python3 mds_ble_gateway.py --snr 68290047 --com /dev/ttyACM0

Dependencies
************

The script uses the following Python libraries:

   * `pc-ble-driver-py`_
   * `Requests`_
