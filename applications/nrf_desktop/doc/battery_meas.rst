.. _nrf_desktop_battery_meas:

Battery measurement
###################

The ``battery_meas`` module periodically measures the battery voltage and sends the ``battery_level_event`` event that informs about the current battery level.

Module Events
*************

+----------------+-------------+------------------+-------------------------+------------------+
| Source Module  | Input Event | This Module      | Output Event            | Sink Module      |
+================+=============+==================+=========================+==================+
|                |             | ``battery_meas`` | ``battery_level_event`` | ``bas``          |
+----------------+-------------+------------------+-------------------------+------------------+

Configuration
*************

The module uses the Zephyr :ref:`zephyr:adc_api` driver to measure voltage.
For this reason, set the following options:

* :option:`CONFIG_ADC` - the module implementation uses Zephyr ADC driver.
* :option:`CONFIG_ADC_ASYNC` - the module implementation uses asynchronous calls.
* :option:`CONFIG_ADC_0` - the module implementation uses ``ADC_0``.
* :option:`CONFIG_ADC_NRFX_SAADC` - the module implementation uses nrfx SAADC driver for nRF52 MCU series.

Set ``CONFIG_DESKTOP_BATTERY_MEAS`` to enable the module.
Also, make sure to define ``CONFIG_DESKTOP_BATTERY_MEAS_POLL_INTERVAL_MS``, that is the amount of time between the subsequent battery measurements (in ms).

If the measurement circuit contains a voltage divider made of two resistors, set the ``CONFIG_DESKTOP_BATTERY_MEAS_HAS_VOLTAGE_DIVIDER`` option.
The following defines specify the values of these two resistors (in kOhm):

* ``CONFIG_DESKTOP_BATTERY_MEAS_VOLTAGE_DIVIDER_UPPER`` - upper resistor.
* ``CONFIG_DESKTOP_BATTERY_MEAS_VOLTAGE_DIVIDER_LOWER`` - lower resistor.

The module measures the voltage over the lower resistor.

The battery voltage is converted to the state of charge (in percentage level units) according to the lookup table defined in the ``battery_def.h`` file.
You can find this file the in board-specific directory in the application configuration folder.

Remember to also define the following battery parameters (otherwise, the default values will be used):

* ``CONFIG_DESKTOP_BATTERY_MEAS_MIN_LEVEL`` - battery voltage in mV that corresponds to the 0% battery level.
* ``CONFIG_DESKTOP_BATTERY_MEAS_MAX_LEVEL`` - battery voltage in mV that corresponds to the 100% battery level.
* ``CONFIG_DESKTOP_VOLTAGE_TO_SOC_DELTA`` - difference in mV between the adjacent elements in the conversion lookup table.

If a pin is used to enable the battery measurement, enable the ``CONFIG_DESKTOP_BATTERY_MEAS_HAS_ENABLE_PIN`` option.
The number of the pin used for this purpose must be defined as ``CONFIG_DESKTOP_BATTERY_MEAS_ENABLE_PIN``.
The implementation uses the ``GPIO0`` port.
Because the Zephyr :ref:`zephyr:gpio_api` driver is used to control this pin, also set the :option:`CONFIG_GPIO` option.

Implementation details
**********************

The work (:c:type:`struct k_delayed_work`) starts the asynchronous voltage measurement and resubmits itself.
When it is processed the next time, it reads the measured voltage.
This read sequence is repeated periodically.
When the system goes to the low-power state, the work is cancelled.

During the first voltage readout, the Analog to Digital Converter is automatically calibrated to increase the measurement accuracy.
