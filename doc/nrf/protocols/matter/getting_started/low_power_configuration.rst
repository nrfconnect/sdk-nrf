.. _ug_matter_device_low_power_configuration:

Reducing power consumption in Matter
####################################

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

.. _ug_matter_device_low_power_icd:

Enable Matter Intermittently Connected Devices support
******************************************************

In Matter, power-optimized devices are part of the wider group of devices which includes devices that are intermittently connected to the network or otherwise unreachable for periods of time.
These devices are known as Intermittently Connected Devices (ICD) and can be unreachable in the network for various reasons, the details of which are out of scope for the Matter specification to define.
The ICD management layer can be used regardless of the underlying network layer and focuses on managing the device reachability on a more abstract level.

To enable the ICD support in Matter, set the :kconfig:option:`CONFIG_CHIP_ENABLE_ICD_SUPPORT` Kconfig option to ``y``.

.. _ug_matter_device_low_power_icd_modes:

Operating modes
===============

An ICD can operate in one of two states: idle mode or active mode.

* Idle mode is a state where the device is unreachable for a certain period of time.
  The maximum time it is unreachable is specified by the Idle Mode Duration parameter (previously called the Idle Mode Interval parameter).

  In idle mode, the device configures the networking interface to use slow polling.
  This means that the fastest it is able to receive messages is the Slow Polling Interval.

* Active mode is a state where the device is reachable and will answer in a timely manner.
  The minimum time it remains in this state is specified by the Active Mode Duration parameter (previously called the Active Mode Interval parameter).

  In active mode, the device configures the networking interface to use fast polling.
  This means that the fastest it is able to receive messages is the Fast Polling Interval.

The client of the ICD has to respect the slow polling and fast polling intervals and not attempt to send messages with a higher frequency.

To configure the ICD communication, use the following Kconfig options:

* :kconfig:option:`CONFIG_CHIP_ICD_IDLE_MODE_DURATION` to set the Idle Mode Duration value in seconds (for example, ``120``).
* :kconfig:option:`CONFIG_CHIP_ICD_ACTIVE_MODE_DURATION` to set the Active Mode Duration value in milliseconds (for example, ``300``).
* :kconfig:option:`CONFIG_CHIP_ICD_SLOW_POLL_INTERVAL` to set the Slow Polling Interval value in milliseconds (for example, ``5000``).
* :kconfig:option:`CONFIG_CHIP_ICD_FAST_POLLING_INTERVAL` to set the Fast Polling Interval value in milliseconds (for example, ``200``).

An ICD switches modes from idle to active for various reasons, such as:

* Idle Mode Duration elapsed.
* Maximum reporting interval for an active subscription elapsed.
* Processing an attribute change that has to be reported to a subscriber.
* Processing an application event.

Active Mode Threshold
=====================

An ICD is allowed to switch to the idle mode to save power once it has spent time in the active mode equal to the active mode duration and there are no further pending exchanges.
In some scenarios it can be beneficial for the device to spend more time in the active mode instead of going to the idle mode immediately.

The period of time that an ICD additionally stays in the active mode after finishing network activity is called the Active Mode Threshold and it can be configured using the :kconfig:option:`CONFIG_CHIP_ICD_ACTIVE_MODE_THRESHOLD` Kconfig option.
Use this functionality to make the device wait for potentially delayed incoming traffic and to avoid energy-wasting retransmissions.

The higher the threshold value, the higher the communication reliability, but the power consumption is also higher.
A high threshold is useful mainly for an ICD with a long Slow Polling Interval, typically bigger than a few seconds, where getting another chance to receive a message will happen after a long time.
For devices with a short Slow Polling Interval, using this functionality can lead to unnecessary power consumption.

Short idle time and long idle time devices
==========================================

The Matter v1.2 specification divides ICD into short idle time (SIT) and long idle time (LIT) devices.
The division is based on the idle mode duration parameter: equal to or shorter than 15 seconds for SIT, and longer than 15 seconds for LIT.

The SIT device behavior is aligned with what was called SED in Matter v.1.1 specification.
The LIT device implementation requires multiple new features,  such as Check-In protocol support and ICD client registration.
The division of ICD types and related features were not finalized for Matter v.1.2 and they are marked as provisional, so it is not recommended to use them, though you can find some of the LIT implementation in the Matter SDK and Matter specification.

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

In a Thread network, :ref:`Matter ICD <ug_matter_device_low_power_icd>` behave like Thread SED, but the two terms are not interchangeable.

Sleepy End Device (SED) configuration in Matter
-----------------------------------------------

To enable Thread SED support in Matter, set the following configuration options:

* :kconfig:option:`CONFIG_OPENTHREAD_MTD` to ``y``
* :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MTD` to ``y``

Additionally, you can configure the SED wake-up intervals according to your needs, but keep in mind that the bigger sleep periods, the smaller power consumption and bigger device response time.
To configure the SED wake-up intervals, set both of the :ref:`ICD slow polling and ICD fast polling intervals <ug_matter_device_low_power_icd_modes>` to determine how often the device wakes up to poll for data.

Synchronized Sleepy End Device (SSED) configuration in Matter
-------------------------------------------------------------

To enable the Thread SSED support in Matter, set the following Kconfig options:

* :kconfig:option:`CONFIG_OPENTHREAD_MTD` to ``y``
* :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MTD` to ``y``
* :kconfig:option:`CONFIG_CHIP_THREAD_SSED` to ``y``

Additionally, you can configure the SSED wake-up intervals according to your needs, but keep in mind that the bigger sleep periods, the smaller power consumption and bigger device response time.
To configure the SSED wake-up intervals, set both of the :ref:`ICD slow polling and ICD fast polling intervals <ug_matter_device_low_power_icd_modes>` to determine how often the device wakes up to poll for data.

The SSED uses the Coordinated Sampled Listening (CSL) protocol, which requires exchanging messages with the Thread parent on every interval value change.
Switching the Matter :ref:`ug_matter_device_low_power_icd_modes` and frequently updating polling intervals may result in increasing the device power consumption due to additional exchanges on the Thread protocol layer.
To avoid this issue, set the :kconfig:option:`CONFIG_CHIP_ICD_SLOW_POLL_INTERVAL` and :kconfig:option:`CONFIG_CHIP_ICD_FAST_POLLING_INTERVAL` Kconfig options to the same value (for example, ``500``).
The typical use case that the SSED is best suited for is battery-powered devices that require short response time, such as door locks or window blinds.

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

.. include:: /test_and_optimize/optimizing/power_general.rst
   :start-after: disable_unused_pins_start
   :end-before: disable_unused_pins_end

.. _ug_matter_enable_pm_module:

Enable Device Power Management module
*************************************

.. include:: /test_and_optimize/optimizing/power_general.rst
   :start-after: enable_device_pm_start
   :end-before: enable_device_pm_end

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

.. include:: /test_and_optimize/optimizing/power_general.rst
   :start-after: radio_power_start
   :end-before: radio_power_end

See :ref:`ug_matter_gs_transmission_power` for more information.
