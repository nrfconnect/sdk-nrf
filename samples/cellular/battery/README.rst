.. _battery:

Cellular: Battery
#################

.. contents::
   :local:
   :depth: 2

The Battery sample demonstrates how to obtain the following battery related information from the modem of an nRF91 Series DK:

* Modem battery voltage
* Modem battery voltage low level notifications
* Power-off warnings (modem firmware v1.3.1 and higher)

The sample uses the :ref:`modem_battery_readme` library.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample first calls the :c:func:`modem_battery_low_level_handler_set` and :c:func:`modem_battery_pofwarn_handler_set` functions to set the respective handlers, and then sets up the workqueues used to handle the low level battery voltage notifications and the power-off warning notifications.
The sample then initializes the :ref:`nrfxlib:nrf_modem`.
Next, it enters a state machine that measures the modem battery voltage at every iteration and then executes an activity based on that.
The first activity (:c:func:`init_activity`) sets the modem to *receive-only* mode if the battery voltage drops below the low level threshold or to *normal* mode if it rises above the threshold and updates the state accordingly.

Then, depending on the modem's battery voltage, the modem switches mode according to the following conditions:

* If the modem is in normal mode and the battery voltage drops below the low level threshold, the modem switches to receive-only mode.
* If the modem is in receive-only mode and the battery voltage is below the low level threshold, the application periodically checks the battery voltage and switches state if it rises above the low level threshold.
* If the modem is in receive-only mode and the battery voltage is above the low level threshold, the application executes a connectivity evaluation and if the conditions are either normal (``7``), good (``8``), or excellent (``9``), the modem switches to normal mode.
  When connectivity conditions are poor, the number of packets re-transmitted is higher thus causing an increase in power consumption, which must be avoided when battery voltage is not at an adequately safe level.
  Full connectivity is recommended when battery voltage is at a sufficient level.
* If the modem is in normal mode and the battery voltage is above the low level threshold, the application executes some IP traffic (DNS lookup) to maintain an active LTE connection.
  The modem performs background monitoring of the battery voltage and sends a notification of low level in case it drops below the set threshold.
* If the modem battery voltage drops below the power-off warning level, the modem is automatically set offline by the hardware.
* If the modem battery voltage rises above the power-off warning level, the modem is set to receive-only mode and initialized.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration options for the sample:

.. _CONFIG_HIGH_BAT_REFRESH_INTERVAL_SEC:

CONFIG_HIGH_BAT_REFRESH_INTERVAL_SEC
   Sets the refresh interval (seconds) when the battery voltage is high.

.. _CONFIG_LOW_BAT_REFRESH_INTERVAL_SEC:

CONFIG_LOW_BAT_REFRESH_INTERVAL_SEC
   Sets the refresh interval (seconds) when the battery voltage is low.

.. _CONFIG_POFWARN_REFRESH_INTERVAL_SEC:

CONFIG_POFWARN_REFRESH_INTERVAL_SEC
   Sets the refresh interval (seconds) after receiving power-off warning.

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/battery`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Connect the nRF91 Series DK to the `Power Profiler Kit II (PPK2)`_ and set up for external power output.
#. `Install the Power Profiler app`_ in the `nRF Connect for Desktop`_.
#. Connect the Power Profiler Kit II (PPK2) to the PC using a micro-USB cable and `connect to it using the App <Using the Power Profiler app_>`_.
#. Power on or reset your nRF91 Series DK.
#. Observe that the sample starts, initializes the modem, and changes functional mode depending on the external voltage.

Sample output
=============

The sample shows the following output when battery voltage is high:

.. code-block:: console

   Battery sample started
   Initializing modem library
   Battery voltage: 5019
   Setting modem to normal mode...
   Normal mode set.
   Initializing modem and connecting...
   Connected.
   Battery voltage: 4977
   Executing DNS lookup for 'example.com'...
   Battery voltage: 5015
   Executing DNS lookup for 'google.com'...
   Battery voltage: 4989
   Executing DNS lookup for 'apple.com'...
   Battery voltage: 4977
   Executing DNS lookup for 'amazon.com'...
   Battery voltage: 5019
   Executing DNS lookup for 'microsoft.com'...

The sample shows the following output when battery voltage drops from a high value to a low value:

.. code-block:: console

   Battery voltage: 3191
   Executing DNS lookup for 'google.com'...
   Battery voltage: 3195
   Executing DNS lookup for 'apple.com'...
   Battery voltage: 3191
   Executing DNS lookup for 'amazon.com'...
   Battery voltage: 3195
   Executing DNS lookup for 'microsoft.com'...
   Battery voltage: 3195
   Executing DNS lookup for 'example.com'...
   Battery low level: 3191
   Battery low level: 3195
   Battery low level: 3253
   Battery voltage: 3250
   Setting modem to RX only mode...
   RX only mode set.
   Initializing modem and connecting...
   Battery low level: 3191
   Connected.
   Battery low level: 3195

The sample shows the following output when battery voltage rises from a low value to a high value:

.. code-block:: console

   Battery low level: 3214
   Battery voltage: 3421
   Battery voltage: 3421
   Energy estimate: 8
   Setting modem to normal mode...
   Normal mode set.
   Battery voltage: 3457
   Executing DNS lookup for 'google.com'...
   Battery voltage: 3421
   Executing DNS lookup for 'apple.com'...
   Battery voltage: 3421
   Executing DNS lookup for 'amazon.com'...

The sample shows the following output when battery voltage drops to a very low value:

.. code-block:: console

   Battery voltage: 3175
   Battery voltage: 3175
   Modem Event Batter LOW:
   ******************************************************************
   * Attention! Do not attempt to write to NVM while in this state. *
   * The NVM operation will sometimes appear to finish successfully *
   * without actually being executed at all.                        *
   * The modem has been set to Offline.                             *
   ******************************************************************

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lte_lc_readme`
* :ref:`nrf_modem_lib_readme`
* :ref:`modem_battery_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
