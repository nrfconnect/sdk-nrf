.. _power_profiling:

Bluetooth: Peripheral power profiling
#####################################

.. contents::
   :local:
   :depth: 2

The Peripheral power profiling sample can be used to measure power consumption when Bluetooth® Low Energy stack is used for communication.
You can measure power consumption during advertising and data transmission.
For this purpose, the sample uses the vendor-specific service and demonstrates advertising in a connectable and non-connectable mode.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Optionally, you can use the `Power Profiler Kit II (PPK2)`_ for power profiling and optimizing your configuration.
You can use also your proprietary solution for measuring the power consumption.

Overview
********

This sample uses Zephyr's power management mechanism to implement the low power mode and the system off mode.

**Low power mode**

The application enters CPU sleep mode when the kernel is in idle state, which means it has nothing to schedule.
Wake-up events are interrupts triggered by one of the SoC peripheral modules, such as RTC or SystemTick.

**System off mode**

This is the deepest power saving mode the system can enter.
In this mode, all core functionalities are powered down, and most peripherals are non-functional or non-responsive.
The only mechanisms that are functional in this mode are reset and wake-up.

.. note::
   Currently, the **System off** mode is not supported by this sample for the nRF54H20 platform.

To wake up your development kit from the system off state, you have the following options:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      * Press the **RESET** button on your development kit.
      * Press **Button 1** to start connectable advertising.
      * Press **Button 2** to start non-connectable advertising.
      * Approach the NFC field of the development kit antenna.

   .. group-tab:: nRF54 DKs

      * Press the **RESET** button on your development kit.
      * Press **Button 0** to start connectable advertising.
      * Press **Button 1** to start non-connectable advertising.
      * Approach the NFC field of the development kit antenna.

  The development kit starts advertising automatically.
  If you use a mobile phone, it connects and bonds with your device.

When you establish a connection with the central device, you can test different connection parameters to optimize the power consumption.
When the central device enables the notification characteristic, your development kit starts sending notifications until the timeout expires.
The device disconnects from the central and enters the system off mode.
You can press a button or bring your phone close to the NFC antenna to wake up the device and continue testing.

For more details about power saving, refer to the :ref:`app_power_opt` section.

Power profiling
===============

You can use the `Power Profiler Kit II (PPK2)`_ for power profiling with this sample.
See the device documentation for details about preparing your development kit for measurements.

The following parameters have an impact on power consumption:

   * Connection interval
   * Advertising duration
   * Latency
   * Notifications data size and interval

You can configure these parameters with the Kconfig options.
If your central device is the `Bluetooth Low Energy app`_ from `nRF Connect for Desktop`_, you can use it to change the current connection parameters.

Example measurements:

.. figure:: /images/power_profiling_meas.png
   :alt: Power consumption measurement

   Connection interval 150 ms, the signal peaks on the chart indicate connection event every 150 milliseconds interval.

.. figure:: /images/power_profiling_meas_adv.png
   :alt: Non-connectable advertising measurement

   Non-connectable advertising setup: Minimal interval - 1000 ms, maximum interval - 1200 ms.

Service UUID
============

This sample implements the vendor-specific service.

The 128-bit service UUID is ``00001630-1212-EFDE-1523-785FEABCD123``.

Characteristics
===============

The 128-bit characteristic UUID is ``00001630-1212-EFDE-1524-785FEABCD123``.
This characteristic value can be read or sent by the notification mechanism.
The value is an array filled with zeroes.
You can configure the length of data using the :ref:`CONFIG_BT_POWER_PROFILING_DATA_LENGTH <CONFIG_BT_POWER_PROFILING_DATA_LENGTH>` Kconfig option.

This characteristic has a CCC descriptor associated with it.

User interface
**************

The sample uses buttons and LEDs to provide a simple user interface.

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         Blinks, toggling on/off every second, when the main loop is running and the device is advertising.
         Off when the device is in system off state.

      LED 2:
         Lit when an NFC field is detected.

      Button 1:
         Starts connectable advertising and wakes up the SoC from the system off state.

      Button 2:
         Starts non-connectable advertising and wakes up the SoC from the system off state.

   .. group-tab:: nRF54 DKs

      LED 0:
         Blinks, toggling on/off every second, when the main loop is running and the device is advertising.
         Off when the device is in system off state.

      LED 1:
         Lit when an NFC field is detected.

      Button 0:
         Starts connectable advertising and wakes up the SoC from the system off state.

      Button 1:
         Starts non-connectable advertising and wakes up the SoC from the system off state.

.. note::
   When you use buttons to wake up the SoC from the system off state, the button state is read in the main thread after booting the Zephyr kernel.
   This causes a delay between the SoC wake up and button press processing.
   If you want to start advertising on system start, you must keep pressing the button until you see a log message confirming the advertising start on the terminal.

Configuration
*************

|config|

The Peripheral Power Profiling sample allows configuring some of its settings using Kconfig.
You can set different options to monitor the power consumption of your development kit.
You can modify the following options (available in the Kconfig file at :file:`samples/bluetooth/peripheral_power_profiling`):

.. _CONFIG_BT_POWER_PROFILING_DATA_LENGTH:

CONFIG_BT_POWER_PROFILING_DATA_LENGTH - Notification data length
   Sets the length of the data sent through notification mechanism.

.. _CONFIG_BT_POWER_PROFILING_NOTIFICATION_INTERVAL:

CONFIG_BT_POWER_PROFILING_NOTIFICATION_INTERVAL - Notification interval
   Sets the notification send interval in milliseconds. Notification data is sent on every interval expiration.
   Notifications are started when the central device enables it by writing to the characteristic CCC descriptor.

.. _CONFIG_BT_POWER_PROFILING_NOTIFICATION_TIMEOUT:

CONFIG_BT_POWER_PROFILING_NOTIFICATION_TIMEOUT - Notification timeout
   Sets the notification timeout in milliseconds. When this timeout fires the device will stop sending notifications.
   After that, the sample disconnects and enters the system off mode.

.. _CONFIG_BT_POWER_PROFILING_CONNECTABLE_ADV_DURATION:

CONFIG_BT_POWER_PROFILING_CONNECTABLE_ADV_DURATION - Connectable advertising duration
   Sets the connectable advertising duration in N*10 milliseconds unit.
   If the connection is not established during advertising, the device enters the system off state.

.. _CONFIG_BT_POWER_PROFILING_NON_CONNECTABLE_ADV_DURATION:

CONFIG_BT_POWER_PROFILING_NON_CONNECTABLE_ADV_DURATION - Non-connectable advertising duration
   Sets the non-connectable advertising duration in N*10 milliseconds unit.
   When the advertising ends, the device enters the system off state if there is no outgoing connection.

.. _CONFIG_BT_POWER_PROFILING_NFC_ADV_DURATION:

CONFIG_BT_POWER_PROFILING_NFC_ADV_DURATION - Advertising duration when the NFC field is detected
   Sets the advertising duration when the NFC field is detected in N*10 milliseconds unit.
   If the connection was not established during advertising, the device enters the system off state.

.. _CONFIG_BT_POWER_PROFILING_CONNECTABLE_ADV_INTERVAL_MIN:

CONFIG_BT_POWER_PROFILING_CONNECTABLE_ADV_INTERVAL_MIN - Connectable advertising minimum interval
   Sets the connectable advertising minimum interval in 0.625 milliseconds unit.

.. _CONFIG_BT_POWER_PROFILING_CONNECTABLE_ADV_INTERVAL_MAX:

CONFIG_BT_POWER_PROFILING_CONNECTABLE_ADV_INTERVAL_MAX - Connectable advertising maximum interval
   Sets the connectable advertising maximum interval in 0.625 milliseconds unit.

.. _CONFIG_BT_POWER_PROFILING_NON_CONNECTABLE_ADV_INTERVAL_MIN:

CONFIG_BT_POWER_PROFILING_NON_CONNECTABLE_ADV_INTERVAL_MIN - Non-connectable advertising minimum interval
   Sets the non-connectable advertising minimum interval in 0.625 milliseconds unit.

.. _CONFIG_BT_POWER_PROFILING_NON_CONNECTABLE_ADV_INTERVAL_MAX:

CONFIG_BT_POWER_PROFILING_NON_CONNECTABLE_ADV_INTERVAL_MAX - Non-connectable advertising maximum interval
   Sets the non-connectable advertising maximum interval in 0.625 milliseconds unit.

.. _CONFIG_BT_POWER_PROFILING_LED_DISABLED:

CONFIG_BT_POWER_PROFILING_LED_DISABLED - Disable LEDs
   Disables the LEDs to reduce power consumption.

.. _CONFIG_BT_POWER_PROFILING_NFC_DISABLED:

CONFIG_BT_POWER_PROFILING_NFC_DISABLED - Disable NFC
   Disables the NFC to reduce power consumption.

The console is disabled by default to reduce power consumption.
To enable the console, set the following Kconfig options to ``y``:

* :kconfig:option:`CONFIG_SERIAL`
* :kconfig:option:`CONFIG_CONSOLE`
* :kconfig:option:`CONFIG_UART_CONSOLE`

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/peripheral_power_profiling`

.. include:: /includes/build_and_run.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

Testing
=======

This testing procedure assumes that you are using `nRF Connect for Mobile`_ or `nRF Connect for Desktop`_.
After programming the sample to your development kit, you need another device for measuring the power consumption.
`Power Profiler Kit II (PPK2)`_ is the recommended device for the measurement.

Testing with Bluetooth Low Energy app and Power Profiler Kit II (PPK2)
----------------------------------------------------------------------

1. Set up `Power Profiler Kit II (PPK2)`_ and prepare your development kit for current measurement.
#. Run the `Power Profiler app`_ from nRF Connect for Desktop.
#. To see terminal messages (at the cost of a very small increase in power consumption), enable the following Kconfig  options:

   * :kconfig:option:`CONFIG_SERIAL`
   * :kconfig:option:`CONFIG_CONSOLE`
   * :kconfig:option:`CONFIG_UART_CONSOLE`

#. |connect_terminal_ANSI|
#. Reset your development kit.
#. Observe that the sample starts.

   The device enters the system off state after five seconds.
   Observe a significant power consumption drop when the device enters the system off state.
#. Use the device buttons to wake up your device and start advertising or approach the NFC field of the kit antenna.

   You can measure power consumption during advertising with different intervals.
   Use the Kconfig options to change the interval or other parameters.
#. Connect to the device through the `Bluetooth Low Energy app`_.

   Observe the power consumption on the current connection interval.
#. Set different connection parameters:

   a. Click :guilabel:`Settings` for the connected device in the Bluetooth Low Energy app.
   #. Select :guilabel:`Update connection`.
   #. Set new connection parameters.
   #. Click :guilabel:`Update` to negotiate new parameters with your device.

#. Observe the power consumption with the new connection parameters.
#. In :guilabel:`000016301212EFDE1523785FEABCD123`, click :guilabel:`Notify` for the characteristic.
#. Observe that notifications are received.

   Monitor the power consumption during notification sending.
#. After the timeout set by the :ref:`CONFIG_BT_POWER_PROFILING_NOTIFICATION_TIMEOUT <CONFIG_BT_POWER_PROFILING_NOTIFICATION_TIMEOUT>` option, your development kit disconnects and enters the system off mode.
#. Repeat this test using different wake-up methods and different parameters, and monitor the power consumption for your new setup.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`dk_buttons_and_leds_readme`
* :ref:`nfc_ndef_msg`
* :ref:`nfc_ch`
* :ref:`nfc_ndef_le_oob`


It uses the Type 2 Tag library from `sdk-nrfxlib`_:

* :ref:`nrfxlib:nfc_api_type2`

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr/sys/kernel.h`
* :file:`include/zephyr/sys/atomic.h`
* :ref:`zephyr:settings_api`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/zephyr/bluetooth/bluetooth.h`
  * :file:`include/zephyr/bluetooth/conn.h`
  * :file:`include/zephyr/bluetooth/uuid.h`
  * :file:`include/zephyr/bluetooth/gatt.h`

* :ref:`zephyr:pm-guide`:

  * :file:`include/zephyr/pm/pm.h`
  * :file:`include/zephyr/pm/policy.h`
