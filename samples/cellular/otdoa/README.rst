.. _otdoa_sample:

Cellular: ``hellaPHY`` OTDOA Positioning
########################################

.. contents::
   :local:
   :depth: 2

hellaPHY OTDOA is a new positioning technology that uses signals broadcast by the
terrestrial cellular network to estimate the position of the UE. This sample
demonstrates how to use the hellaPHY OTDOA library and adaptation layer to
estimate the position of your device.

Requirements
************

This sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The hellaPHY OTDOA sample initializes the hellaPHY OTDOA AL library and provides shell commands
to run tests and display metadata to demonstrate the basic use of the OTDOA API.

Configuration
*************

|config|


Configuration files
===================

The sample provides predefined configuration files for typical use cases.

The following files are available:

* :file:`prj.conf` - Standard default configuration file.

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/otdoa`

.. include:: /includes/build_and_run_ns.txt

Running the sample application
==============================

|test_sample|

#. |connect_kit|
#. |connect_terminal|  The terminal application should be set for 921600 baud for this sample.
#. Power on or reset your device
#. Observe that the sample starts and connects to the LTE network
#. Enter 'phywi' on the terminal and verify that the phywi shell command help is displayed.

.. note::
   The device must be provisioned with a private key before it can be used
   for OTDOA positioning. The corresponding public key must be sent to PHY Wireless
   to be provisioned on the network server.

To provision the device, follow these steps:

#. connect a terminal application as described aove.
#. Verify that the device boots and 'phywi' displays the shell command help.
#. Disconnect the terminal application.
#. Program the private key into the device using the ``pkey`` script (provided with the OTDOA binary library).

.. code-block:: bash

   # provision the DK with new key
   # Assumes device is connected with CLI on /dev/ttyACM0
   ./pkey <file name>.pem /dev/ttyACM0

#. Once completed, the device should be reset (e.g. using its reset button).
#. Reconnect the terminal application and verify that the device boots and responds to 'phywi'.
#. Run ``phywi test`` on the device.
