.. _peripheral_rscs:

Bluetooth: Peripheral Running Speed and Cadence Service (RSCS)
###############################################################

.. contents::
   :local:
   :depth: 2

The peripheral RSCS sample demonstrates how to use the :ref:`rscs_readme`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample also requires a phone or tablet running a compatible application, for example `nRF Connect for Mobile`_ or `nRF Toolbox`_.

Overview
********

This sample demonstrates the use of Running Speed and Cadence Service.
It simulates a sensor and sends measurements to the connected device, such as a phone or a tablet.

The mobile application on the device can configure sensor parameters using the SC Control Point characteristic.

User interface
**************

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         Blinks, toggling on/off every second, when the main loop is running and the device is advertising.

      LED 2:
         Lit when connected.

   .. group-tab:: nRF54 DKs

      LED 0:
         Blinks, toggling on/off every second, when the main loop is running and the device is advertising.

      LED 1:
         Lit when connected.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/peripheral_rscs`

.. include:: /includes/build_and_run_ns.txt

.. include:: /includes/nRF54H20_erase_UICR.txt

.. _peripheral_rscs_testing:

Testing
=======

This testing procedure assumes that you are using `nRF Connect for Mobile`_.
After programming the sample to your development kit, test it by performing the following steps:

1. Power on your development kit.
#. Connect to the device through nRF Connect for Mobile (the device is advertising as "Nordic_RSCS").
#. Observe that the services of the connected device are shown.
#. In :guilabel:`Running Speed and Cadence Service`, tap the :guilabel:`Notify` button for the "RSC Measurement" characteristic.
#. Observe that notifications with the measurement values are received.
#. In :guilabel:`RSC Feature`, tap the :guilabel:`Read` button to get the supported features.
#. In :guilabel:`Sensor Location`, tap the :guilabel:`Read` button to read the location of the sensor.
#. In :guilabel:`SC Control Point`, tap the :guilabel:`Indicate` button to control the sensor.
#. The following Op Codes (with data if required) can be written into the **SC Control Point**:

   * ``01 xx xx xx xx`` to set the Total Distance Value to the entered value in meters. (if the server supports the Total Distance Measurement feature).
   * ``02`` to start the sensor calibration process (if the server supports the Sensor Calibration feature).
   * ``03 xx`` to update the sensor location (if the server supports the Multiple Sensor Locations feature).
   * ``04`` to get a list of supported localizations (if the server supports the Multiple Sensor Locations feature).
#. The answer consists of the following fields:

   * ``10`` Response Code.
   * ``xx`` Required Op Code.
   * ``xx`` Status: ``01`` Success, ``02`` Op Code not supported, ``03`` Invalid Operand, ``04`` Operation Failed.
   * ``data`` Optional, response data.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`rscs_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :file:`lib/libc/minimal/include/errno.h`
* :file:`include/sys/printk.h`
* :file:`include/random/random.h`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/uuid.h`
  * :file:`include/bluetooth/gatt.h`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
