.. _hw_id_sample:

Hardware ID
###########

.. contents::
   :local:
   :depth: 2

The hardware ID sample prints a unique hardware ID by using the :ref:`lib_hw_id` library.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

You can use this sample to try out the different hardware ID sources supported by the :ref:`lib_hw_id` library.

Configuration
*************

|config|

By default, the hardware ID sample uses the hardware ID provided by Zephyr's HW Info API.
You can change the hardware ID source by using one of the following :file:`conf` files:

* :file:`overlay-ble-mac` file for Bluetooth® Low Energy MAC address.
* :file:`overlay-imei` file for the :term:`International Mobile (Station) Equipment Identity (IMEI)` of the modem.
* :file:`overlay-uuid` file for the UUID of the modem.


Building and running
********************

.. |sample path| replace:: :file:`samples/hw_id`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

      1. |connect_terminal_specific|
      #. Reset the kit.
      #. Observe the following output:

         .. code-block:: console

             hw_id: DEADBEEF00112233

         If an error occurs, the sample prints the following message with the error code:

         .. code-block:: console

             hw_id_get failed (err -5)
             hw_id: unsupported

Dependencies
************

This sample uses the following |NCS| library:

* :ref:`lib_hw_id`
