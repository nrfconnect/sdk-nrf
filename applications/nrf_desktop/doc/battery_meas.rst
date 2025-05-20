.. _nrf_desktop_battery_meas:

Battery measurement module
##########################

.. contents::
   :local:
   :depth: 2

Use the |battery_meas| to periodically measure the battery voltage and send the ``battery_level_event`` event that informs about the current battery level.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_battery_meas_start
    :end-before: table_battery_meas_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The |battery_meas| uses Zephyr's :ref:`zephyr:adc_api` driver to measure voltage.
For this reason, set the following options:

* :kconfig:option:`CONFIG_ADC` - The module's implementation uses Zephyr ADC driver.
* :kconfig:option:`CONFIG_ADC_ASYNC` - The module's implementation uses asynchronous calls.
* :kconfig:option:`CONFIG_ADC_NRFX_SAADC` - The module's implementation uses nrfx SAADC driver for nRF52 MCU series.

Set :ref:`CONFIG_DESKTOP_BATTERY_MEAS <config_desktop_app_options>` to enable the module.
Also, make sure to define :ref:`CONFIG_DESKTOP_BATTERY_MEAS_POLL_INTERVAL_MS <config_desktop_app_options>`, that is the amount of time between the subsequent battery measurements (in ms).

If the measurement circuit contains a voltage divider made of two resistors, set the :ref:`CONFIG_DESKTOP_BATTERY_MEAS_HAS_VOLTAGE_DIVIDER <config_desktop_app_options>` option.
The following defines specify the values of these two resistors (in kOhm):

* :ref:`CONFIG_DESKTOP_BATTERY_MEAS_VOLTAGE_DIVIDER_UPPER <config_desktop_app_options>` - Upper resistor.
* :ref:`CONFIG_DESKTOP_BATTERY_MEAS_VOLTAGE_DIVIDER_LOWER <config_desktop_app_options>` - Lower resistor.

The module measures the voltage over the lower resistor.

The battery voltage is converted to the state of charge (in percentage level units) according to the lookup table defined in the :file:`battery_def.h` file.
You can find this file the in board-specific directory in the application configuration directory.

Remember to also define the following battery parameters (otherwise, the default values will be used):

* :ref:`CONFIG_DESKTOP_BATTERY_MEAS_MIN_LEVEL <config_desktop_app_options>` - Battery voltage in mV that corresponds to the 0% battery level.
* :ref:`CONFIG_DESKTOP_BATTERY_MEAS_MAX_LEVEL <config_desktop_app_options>` - Battery voltage in mV that corresponds to the 100% battery level.
* :ref:`CONFIG_DESKTOP_VOLTAGE_TO_SOC_DELTA <config_desktop_app_options>` - Difference in mV between the adjacent elements in the conversion lookup table.

If a pin is used to enable the battery measurement, enable the :ref:`CONFIG_DESKTOP_BATTERY_MEAS_HAS_ENABLE_PIN <config_desktop_app_options>` option.
The number of the pin used for this purpose must be defined as :ref:`CONFIG_DESKTOP_BATTERY_MEAS_ENABLE_PIN <config_desktop_app_options>` option.
The implementation uses the ``GPIO0`` port.
Because Zephyr's :ref:`zephyr:gpio_api` driver is used to control this pin, also set the :kconfig:option:`CONFIG_GPIO` option.

Implementation details
**********************

:c:struct:`k_work_delayable` starts the asynchronous voltage measurement and resubmits itself.
When it is processed the next time, it reads the measured voltage.
This read sequence is repeated periodically.
When the system goes to the low-power state, the work is canceled.

During the first voltage readout, the Analog to Digital Converter is automatically calibrated to increase the measurement accuracy.
