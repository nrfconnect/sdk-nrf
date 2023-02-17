.. _ug_matter_device_low_power_configuration:

Reducing power consumption in Matter applications
#################################################

The Matter protocol can be used in various device types that are designed to be battery supplied, where a low power consumption is of critical importance.

There are many ways to reduce the power consumption in your application, including methods related to the adopted network technology, disabling specific modules, or configuring features meant for optimizing power consumption.
See the following sections for more information.

The following Matter samples and applications use the low power configuration by default and you can use them as reference designs:

* :ref:`Matter door lock sample <matter_lock_sample>`
* :ref:`Matter light switch sample <matter_light_switch_sample>`
* :ref:`Matter window covering sample <matter_window_covering_sample>`
* :ref:`Matter weather station application <matter_weather_station_app>`

Enable low power mode for the selected networking technology
************************************************************

The Matter supports using Thread and Wi-Fi as the IPv6-based networking technologies.
Both of the technologies come with their own solutions for optimizing the protocol behavior in terms of power consumption.
However, the general goal of the optimization for both is to reduce the time spent in the active state and put the device in the inactive (sleep) state whenever possible.
Reducing the device activity time usually comes with a higher response time and a lower performance.
When optimizing the power consumption, you must be aware of the trade-off between the power consumption and the device performance, and choose the optimal configuration for his use case.

Matter over Thread
==================

The Thread protocol defines two types of devices working in a low power mode: Sleepy End Devices (SEDs) and Synchronized Sleepy End Devices (SSEDs).
Both types are variants of Minimal Thread Device (MTD) type.
For more information about MTD and SEDs, see :ref:`Thread device types <thread_ot_device_types>`.

Sleepy End Device (SED) configuration in Matter
-----------------------------------------------

To enable Thread SED support in Matter, set the following configuration options:

* :kconfig:option:`CONFIG_CHIP_ENABLE_SLEEPY_END_DEVICE_SUPPORT` to ``y``
* :kconfig:option:`CONFIG_OPENTHREAD_MTD` to ``y``
* :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MTD` to ``y``

Additionally, you can configure the SED wake-up intervals according to your needs, but keep in mind that the bigger sleep periods, the smaller power consumption and bigger device response time.
To configure the SED wake-up intervals, set both of the following Kconfig options:

* :kconfig:option:`CONFIG_CHIP_SED_IDLE_INTERVAL` to a value in milliseconds that determines how often the device wakes up to poll for data in the idle state (for example, ``1000``).
* :kconfig:option:`CONFIG_CHIP_SED_ACTIVE_INTERVAL` to a value in milliseconds that determines how often the device wakes up to poll for data in the active state (for example, ``200``).

Synchronized Sleepy End Device (SSED) configuration in Matter
-------------------------------------------------------------

To enable the Thread SSED support in Matter, set the following Kconfig options:

* :kconfig:option:`CONFIG_CHIP_ENABLE_SLEEPY_END_DEVICE_SUPPORT` to ``y``
* :kconfig:option:`CONFIG_OPENTHREAD_MTD` to ``y``
* :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MTD` to ``y``
* :kconfig:option:`CONFIG_CHIP_THREAD_SSED` to ``y``

Additionally, you can configure the SSED wake-up intervals according to your needs, but keep in mind that the bigger sleep periods, the smaller power consumption and bigger device response time.
To configure the SSED wake-up intervals, set both of the following Kconfig options:

* :kconfig:option:`CONFIG_CHIP_SED_IDLE_INTERVAL` to a value in milliseconds that determines how often the device wakes up to listen for data in the idle state (for example, ``500``).
* :kconfig:option:`CONFIG_CHIP_SED_ACTIVE_INTERVAL` to a value in milliseconds that determines how often the device wakes up to listen for data in the active state (for example, ``500``).

Matter over Wi-Fi
=================

The Wi-Fi protocol introduces the power save mechanism that allows the Station device (STA) to spend the majority of time in a sleep state and wake-up periodically to check for pending traffic.
This is coordinated by the Access Point device (AP) using a mechanism called Delivery Traffic Indication Message (DTIM).
The message is sent in a predefined subset of the beacons, so the STA device needs to wake up only to receive this message and not every beacon (as it would happen for the not-optimized case).
For more information about the Wi-Fi power save mechanism, see the :ref:`Wi-Fi MAC layer <wifi_mac_layer>` documentation.

To enable the Wi-Fi power save mode, set the :kconfig:option:`CONFIG_NRF_WIFI_LOW_POWER` Kconfig option to ``y``.

Disable serial logging
**********************

The majority of samples and applications that run in the debug mode are configured to log the information over serial port (usually UART).
The peripherals for serial communication use HFCLK, which significantly increases the device power consumption.

To disable the serial logging and the UART peripheral, complete the following steps:

1. Set the :kconfig:option:`CONFIG_LOG` to ``n``.
#. Set the UART peripheral state in the board's :file:`dts` overlay to ``disabled``.
   For example, for **UART1**:

   .. code-block:: devicetree

      &uart1 {
          status = "disabled";
      };

Disable unused pins and peripherals
***********************************

Some of the pins and peripherals are enabled by default for some boards.
Depending on the peripheral or the pin type, they can increase the device power consumption to a different extent.
If the application does not use them, make sure they are disabled.

To disable a particular peripheral, set its state in the board's :file:`dts` overlay to ``disabled``.
For example, for **ADC**:

.. code-block:: devicetree

    &adc {
        status = "disabled";
    };

.. _ug_matter_enable_pm_module:

Enable Device Power Management module
*************************************

The Device Power Management module provides an interface that the device drivers use to be informed about entering the suspend state or resuming from the suspend state.
This allows the device drivers to do any necessary power management operations, such as turning off device clocks and peripherals, which lowers the power consumption.

To enable suspending peripherals when CPU goes to sleep, set the :kconfig:option:`CONFIG_PM_DEVICE` Kconfig option to ``y``.

Put the external flash into sleep mode in inactivity periods
************************************************************

When the CPU goes to sleep, some of the peripherals are suspended by their drivers, as described in the :ref:`Enable Device Power Management module <ug_matter_enable_pm_module>`.
However, the driver is not always able to know the application behavior and optimally handle the peripheral state.

One of such cases is the external flash usage by the Matter applications.
It is typically used very rarely and only for the Device Firmware Upgrade purposes.
For this reason, you might want to suspend the external flash for majority of time and have it resumed to the active state only if needed.
The Device Firmware Upgrade case is properly handled in the nRF Connect platform, but for other proprietary use cases, you should handle state changes in your own implementation.

For example, to control the QSPI NOR external flash, you can use the following implementation:

.. code-block:: C++

    #include <zephyr/pm/device.h>

    const auto * qspi_dev = DEVICE_DT_GET(DT_INST(0, nordic_qspi_nor));
    if (device_is_ready(qspi_dev))
    {
        // Put the peripheral into suspended state.
        pm_device_action_run(qspi_dev, PM_DEVICE_ACTION_SUSPEND);

        // Resume the peripheral from the suspended state.
        pm_device_action_run(qspi_dev, PM_DEVICE_ACTION_RESUME);
    }

Configure radio transmitter power
*********************************

The radio transmitter power (radio TX power) has a significant impact on the device power consumption.
The higher the transmitting power, the greater the wireless communication range, which leads to higher power consumption.
Make sure to choose the optimal configuration for your specific use case.

The radio transmitter power configuration must be modified separately for every wireless technology used in the Matter applications.

Bluetooth LE
============

To change the radio TX power used by the Bluetooth LE protocol, set the :kconfig:option:`CONFIG_BT_CTLR_TX_PWR` Kconfig option to the desired value.
However, you cannot set this config value directly, as it obtains the value from the selected ``CONFIG_BT_CTLR_TX_PWR_MINUS_<X>`` or ``CONFIG_BT_CTLR_TX_PWR_PLUS_<X>``, where *<X>* is replaced by the desired power value.
For example, to set Bluetooth LE TX power to +5 dBM, set the :kconfig:option:`CONFIG_BT_CTLR_TX_PWR_PLUS_5` Kconfig option to ``y``.
The configuration must be applied to both the application and the network core images in a consistent manner.

Thread
======

To change the radio TX power used by the Thread protocol, set the :kconfig:option:`CONFIG_OPENTHREAD_DEFAULT_TX_POWER` to the desired value.

Wi-Fi
=====

Changing TX power for the Wi-Fi protocol is currently not supported.
