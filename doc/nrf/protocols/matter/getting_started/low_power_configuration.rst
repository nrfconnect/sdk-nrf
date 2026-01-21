.. _ug_matter_device_low_power_configuration:

Reducing power consumption in Matter
####################################

.. contents::
   :local:
   :depth: 2

The Matter protocol can be used in various device types that are designed to be battery supplied, where low power consumption is of critical importance.

There are many ways to reduce the power consumption in your application, including methods related to the adopted network technology, disabling specific modules, or configuring features meant for optimizing power consumption.
See the following sections for more information.

The following Matter samples and applications use the low power configuration by default and you can use them as reference designs:

* :ref:`Matter door lock sample <matter_lock_sample>`
* :ref:`Matter light switch sample <matter_light_switch_sample>`
* :ref:`Matter smoke CO alarm <matter_smoke_co_alarm_sample>`
* :ref:`Matter window covering sample <matter_window_covering_sample>`
* :ref:`Matter weather station application <matter_weather_station_app>`
* :ref:`Matter temperature sensor sample <matter_temperature_sensor_sample>`
* :ref:`Matter contact sensor sample <matter_contact_sensor_sample>`

The following additional materials and tools might help you to optimize, estimate, and measure the power consumption of your device are:

* `nWP049 - Matter over Thread: Power consumption and battery life`_
* :ref:`ug_matter_gs_tools_opp`

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

.. _ug_matter_device_low_power_icd_sit_lit:

Short idle time and long idle time devices
==========================================

The Matter specification divides ICD into short idle time (SIT) and long idle time (LIT) devices.
The division is based on the Slow Polling Interval parameter: equal to or shorter than 15 seconds for SIT, and longer than 15 seconds for LIT.

The typical use case for a SIT device are actuators, meaning devices such as door locks or window coverings, where the controller usually initiates communication and the accessory device is expected to answer with a small latency.
Conversely, LIT devices are designed to be used for sensors or light switches, devices that only report data and are not controllable.
In such scenarios, the LIT device initiates communication and it is not able to answer with a small latency, but it can sleep for extended periods of time and achieve much lower power consumption than an SIT.

The LIT device starts operation in the SIT mode and remains in this state until the first ICD client registers to it.
This is necessary because the device is not responsive in the LIT mode, so client registration would be difficult.
Once the ICD client is registered, the ICD device switches to LIT mode in order to save energy.

The LIT device implementation requires multiple new features, such as Check-In protocol (CIP) support, ICD client registration, and User Active Mode Trigger (UAT).
These features are not required for SIT device implementation, but can be optionally enabled.

You can enable optional Dynamic SIT LIT switching (DSLS) support for the LIT device.
When enabled, the device can dynamically switch between SIT and LIT modes, even if it has an ICD client registered.
The primary use case for this feature is device types like Smoke/CO Alarm, allowing the device to work as SIT when using a wired power source and switch to LIT and using a battery power source in case of a power outage.
This feature is not available for the SIT device.

To configure the LIT, CIP, UAT or DSLS, use the following Kconfig options:

* :kconfig:option:`CONFIG_CHIP_ICD_LIT_SUPPORT` to enable the Long Idle Time device support.
* :kconfig:option:`CONFIG_CHIP_ICD_CHECK_IN_SUPPORT` to enable the Check-In protocol support.
  The Check-In protocol provides a way for an accessory device (ICD server) to notify the registered controller (ICD client) that it is available for communication.
  This option is by default enabled for the LIT device.
* :kconfig:option:`CONFIG_CHIP_ICD_CLIENTS_PER_FABRIC` to set the number of ICD clients that can be registered to an ICD server and benefit from CIP functionality.
* :kconfig:option:`CONFIG_CHIP_ICD_UAT_SUPPORT` to enable the User Active Mode Trigger support.
  The User Active Mode Trigger allows triggering the ICD device to move from the idle to active state and make it immediately responsive, for example to change its configuration.
  This option is by default enabled for the LIT device.
* :kconfig:option:`CONFIG_CHIP_ICD_SIT_SLOW_POLL_LIMIT` to limit the slow polling interval value while the device is in the SIT mode.
  This option can be used to limit the slow poll interval of an LIT device while temporarily working in the SIT mode.
* :kconfig:option:`CONFIG_CHIP_ICD_DSLS_SUPPORT` to enable Dynamic SIT LIT switching (DSLS) support.
  The DSLS support allows the application to dynamically switch between SIT and LIT modes, as long as the requirements for these modes are met.
  This option is by default disabled for the LIT device.

You can enable optional reporting on entering the active mode.
When enabled, the device sends a data report to the subscribed devices.
This could be useful especially in the combination with the User Active Mode Trigger (UAT) feature, to inform the subscribed Matter controller that the user triggered an ICD to enter the active mode.
To enable this functionality, set the :kconfig:option:`CONFIG_CHIP_ICD_REPORT_ON_ACTIVE_MODE` Kconfig option to ``y``.

Enable low power mode for the selected networking technology
************************************************************

The Matter supports using Thread and Wi-Fi® as the IPv6-based networking technologies.
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

Child timeouts configuration
----------------------------

The device working in a Thread child role uses additional mechanisms for periodically ensuring that the communication with the parent is still possible.
These mechanisms lead to waking up the device and exchanging the messages with the parent, if the related timeout expires.
In case of using the SED poll period value greater than any of these timeouts, the device wakes up more often than what is defined by the poll period.
To ensure that the SED device wakes up exactly at every poll period, set the following Kconfig options to the value greater than the poll period value (for Matter ICD :kconfig:option:`CONFIG_CHIP_ICD_SLOW_POLL_INTERVAL`):

* :kconfig:option:`CONFIG_OPENTHREAD_MLE_CHILD_TIMEOUT`
* :kconfig:option:`CONFIG_OPENTHREAD_CHILD_SUPERVISION_CHECK_TIMEOUT`
* :kconfig:option:`CONFIG_OPENTHREAD_CHILD_SUPERVISION_INTERVAL`

Matter over Wi-Fi
=================

The Wi-Fi protocol introduces the power save mechanism that allows the Station device (STA) to spend the majority of time in a sleep state and wake-up periodically to check for pending traffic.
This is coordinated by the Access Point device (AP) using a mechanism called Delivery Traffic Indication Message (DTIM).
The message is sent in a predefined subset of the beacons, so the STA device needs to wake up only to receive this message and not every beacon (as it would happen for the not-optimized case).
For more information about the Wi-Fi power save mechanism, see the :ref:`Wi-Fi MAC layer <wifi_mac_layer>` documentation.

To enable the Wi-Fi power save mode, set the :kconfig:option:`CONFIG_NRF_WIFI_LOW_POWER` Kconfig option to ``y``.

Configure Bluetooth LE advertising duration
*******************************************

A Matter device uses Bluetooth® Low Energy (LE) to advertise its service for device commissioning purposes.
The duration of this advertising is configurable and can last up to 15 minutes in the standard mode and up to 48 hours in the Extended Announcement mode.
An extended advertising duration may improve the user experience, as it gives more time for the user to set up the device, but it also increases the energy consumption.

Selecting the optimal advertising duration is a compromise and depends on the specific application use case.
Use the following Kconfig options to configure the advertising and reduce the consumed energy:

* :kconfig:option:`CONFIG_CHIP_BLE_EXT_ADVERTISING` -  Set to ``n`` to disable Extended Announcement (also called Extended Beaconing).
  When disabled, the device is not allowed to advertise for a duration of more than 15 minutes.
* :kconfig:option:`CONFIG_CHIP_BLE_ADVERTISING_DURATION` - Set how long the device advertises (in minutes).

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

Disable LEDs module
===================

When performing the power measurements on various development kits, the LEDs can either be included in the measurement circuit or not:

* For the nRF52840 DK and nRF5340 DK, the LEDs are excluded from the measurement circuit, so they can be enabled for the low power configuration and it is not going to impact the measurement results.
* For the nRF54L15 DK and nRF54LM20 DK, the MOSFET transistors controlling the LEDs are included in the measurement circuit.
  This results in measurement results being increased by an additional, small leakage current that appears if an LED is turned on.
  To measure the current consumption of the nRF54L15 or nRF54LM20 SoC without including development kit components, such as LEDs, it is recommended to disable them.

To disable LEDs in the Matter samples and applications, set the :option:`CONFIG_NCS_SAMPLE_MATTER_LEDS` Kconfig option to ``n``.

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

.. _ug_matter_device_low_power_calibration_period:

Configure radio driver temperature calibration period
*****************************************************

The radio driver requires periodic calibration events to compensate the potential temperature changes that may affect the clock configuration.
The frequency of the calibration depends on the device's environment and the temperature changes the device is exposed to.
For example, a device that is mobile or installed outdoors likely requires shorter calibration period than a device that is installed indoors.

Additionally, the calibration period has an impact on the device's power consumption, because it results in device wake-ups.
It is recommended not to use a calibration period value smaller than required to avoid the unnecessary increase in power consumption.

You can configure the calibration period using the following Kconfig options:

* :kconfig:option:`CONFIG_NRF_802154_TEMPERATURE_UPDATE_PERIOD` -  Configures the 802.15.4 radio driver calibration period in milliseconds.
  The default value is 60000 ms (60 s), which is optimal for most use cases.
* :kconfig:option:`CONFIG_MPSL_CALIBRATION_PERIOD` - Configures Multiprotocol Service Layer (MPSL) driver calibration period in milliseconds.
  This is used only, if the application uses LFRC or it is run on an nRF54L Series SoC.
  The default value is 4000 ms (4 s), which is likely too aggressive for most use cases.

Disable unused RAM sections
***************************

The :ref:`lib_ram_pwrdn` library allows you to disable unused sections of RAM and save power in low-power applications.
Unused sections of RAM depend on the SoC architecture and the total amount of used static RAM.
In Matter, you can use this feature by setting the :kconfig:option:`CONFIG_RAM_POWER_DOWN_LIBRARY` Kconfig option to ``y``.

Once the feature is enabled, the :c:func:`power_down_unused_ram` function is called automatically in the :file:`matter_init.cpp` file during the initialization process.
