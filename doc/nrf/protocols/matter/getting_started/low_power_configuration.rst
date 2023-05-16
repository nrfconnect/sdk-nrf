.. _ug_matter_device_low_power_configuration:

Reducing power consumption in Matter applications
#################################################

.. contents::
   :local:
   :depth: 2

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
When optimizing the power consumption, you must be aware of the trade-off between the power consumption and the device performance, and choose the optimal configuration for your use case.

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

You can also configure the SED active threshold to make the device stay active for some time after the network activity, instead of going to idle state immediately.
The reason to use this functionality is to make the device wait for potentially delayed incoming traffic and avoid energy-wasting retransmissions.
The higher the threshold value, the greater the communication reliability, but it also leads to higher power consumption.
In practice, it is reasonable to set a non-zero threshold time only for devices using :kconfig:option:`CONFIG_CHIP_SED_IDLE_INTERVAL` value bigger than a few seconds.
To configure the SED active threshold, set the following Kconfig option:

* :kconfig:option:`CONFIG_CHIP_SED_ACTIVE_THRESHOLD` to a value in milliseconds that determines how long the device stays in the active mode after network activity.

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

You can also configure the SED active threshold to make the device stay active for some time after the network activity, instead of going to idle state immediately.
The reason to use this functionality is to make the device wait for potentially delayed incoming traffic and avoid energy-wasting retransmissions.
The higher the threshold value, the greater the communication reliability, but it also leads to higher power consumption.
In practice, it is reasonable to set a non-zero threshold time only for devices using :kconfig:option:`CONFIG_CHIP_SED_IDLE_INTERVAL` value bigger than a few seconds.
To configure the SED active threshold, set the following Kconfig option:

* :kconfig:option:`CONFIG_CHIP_SED_ACTIVE_THRESHOLD` to a value in milliseconds that determines how long the device stays in the active mode after network activity.

Matter over Wi-Fi
=================

The Wi-Fi protocol introduces the power save mechanism that allows the Station device (STA) to spend the majority of time in a sleep state and wake-up periodically to check for pending traffic.
This is coordinated by the Access Point device (AP) using a mechanism called Delivery Traffic Indication Message (DTIM).
The message is sent in a predefined subset of the beacons, so the STA device needs to wake up only to receive this message and not every beacon (as it would happen for the not-optimized case).
For more information about the Wi-Fi power save mechanism, see the :ref:`Wi-Fi MAC layer <wifi_mac_layer>` documentation.

To enable the Wi-Fi power save mode, set the :kconfig:option:`CONFIG_NRF_WIFI_LOW_POWER` Kconfig option to ``y``.

Optimize subscription report intervals
**************************************

The majority of Matter controllers establishes :ref:`subscriptions <ug_matter_overview_int_model>` to some attributes of the Matter accessory in order to receive periodic data reports.
The node that initiates subscription (subscriber) recommends using data report interval within the requested min-max range.
The node that receives the subscription request (publisher) may accept or modify the maximum interval value.

The default implementation assumes that the publisher node accepts the requested intervals, which may result in sending data reports very often and consuming significant amounts of power.
You can use one of the following ways to modify this behavior and select the optimal timings for your specific use case:

* Enable the nRF platform's implementation of the subscription request handling and specify the preferred data report interval value.
  The implementation looks at the value requested by the initiator and the value preferred by the publisher and selects the higher of the two.
  To enable it, complete the following steps:

  1. Set the :kconfig:option:`CONFIG_CHIP_ICD_SUBSCRIPTION_HANDLING` Kconfig option to ``y``.
  2. Set the :kconfig:option:`CONFIG_CHIP_MAX_PREFERRED_SUBSCRIPTION_REPORT_INTERVAL` Kconfig option to the preferred value of the maximum data report interval in seconds.

* Provide your own policy and implementation of the subscription request handling.
  To do this, implement the ``OnSubscriptionRequested`` method in your application to set values of subscription report intervals that are appropriate for your use case.
  See the following code snippet for an example of how this implementation could look:

  .. code-block::

     #include <app/ReadHandler.h>

     class SubscriptionApplicationCallback : public chip::app::ReadHandler::ApplicationCallback
     {
        CHIP_ERROR OnSubscriptionRequested(chip::app::ReadHandler & aReadHandler,
                                           chip::Transport::SecureSession & aSecureSession) override;
     };

     CHIP_ERROR SubscriptionApplicationCallback::OnSubscriptionRequested(chip::app::ReadHandler & aReadHandler,
                                                          chip::Transport::SecureSession & aSecureSession)
     {
        /* Set the interval in seconds appropriate for your application use case, e.g. 15 seconds. */
        uint32_t exampleMaxInterval = 15;
        return aReadHandler.SetReportingIntervals(exampleMaxInterval);
     }

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

See :ref:`ug_matter_gs_transmission_power` for more information.
