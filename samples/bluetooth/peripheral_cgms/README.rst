.. _peripheral_cgms:

Bluetooth: Continuous Glucose Monitoring Service (CGMS)
#######################################################

.. contents::
   :local:
   :depth: 2

The Peripheral Continuous Glucose Monitoring Service (CGMS) sample demonstrates how to use the :ref:`cgms_readme` to implement a glucose monitoring device.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample demonstrates a basic Bluetooth® Low Energy Peripheral role functionality that exposes the Continuous Glucose Monitoring GATT Service.
Once it connects to a Central device, it generates dummy glucose measurement values.
You can use it together with the Continuous Glucose module in the nRF Toolbox app.
You can also use the `nRF Connect for Mobile`_ to interact with the CGMS module.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/peripheral_cgms`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

After programming the sample to your development kit, connect to a smartphone
with the nRF Toolbox installed to test it as follows:

1. |connect_terminal_specific|
#. Reset the development kit.
#. Open the nRF Toolbox app and select the **Continuous Glucose** module.
#. Click the name of the development kit to connect to it.

   Authentication is required.
   The device prints the pairing key in the terminal.
#. Enter the pairing key on the client side.
#. If pairing is successful, in the terminal window, observe that notifications are enabled::

      <inf> cgms: CGMS Measurement: notification enabled
      <inf> cgms: CGMS RACP: indication enabled
      <inf> cgms: CGMS SOCP: indication enabled

#. The glucose measurement is sent to the client using a notification.
   The client can also retrieve the record using the record access control point.

Dependencies
************

This sample uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :file:`include/errno.h`
* :file:`include/zephyr.h`
* :file:`include/sys/printk.h`
* :file:`include/sys/byteorder.h`

* :ref:`zephyr:bluetooth_api`:

* :file:`include/bluetooth/bluetooth.h`
* :file:`include/bluetooth/conn.h`
* :file:`include/bluetooth/uuid.h`
* :file:`include/bluetooth/gatt.h`
* :file:`include/bluetooth/services/cgms.h`
