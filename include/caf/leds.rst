.. _caf_leds:

CAF: LEDs module
################

.. contents::
   :local:
   :depth: 2

The LEDs module of the :ref:`lib_caf` (CAF) is responsible for controlling LEDs in response to LED effect set by a ``led_event``.
The source of such events could be any other module in |NCS|.

Configuration
*************

To use the module, you must enable the following Kconfig options:

* :option:`CONFIG_CAF_LEDS` - This option enables the LEDs module.
* :option:`CONFIG_PWM` - This option enables Zephyr's :ref:`zephyr:pwm_api` driver, which is used for setting the LED color (that is, the brightness of the diodes).
  Setting this option requires you to define PWM ports, as described in the following section.

The following Kconfig options are available for this module:

* :option:`CONFIG_CAF_LEDS_DEF_PATH`
* :option:`CONFIG_CAF_LEDS_PM_EVENTS`
* :option:`CONFIG_CAF_LEDS_COUNT`
* :option:`CONFIG_CAF_LEDS_COLOR_COUNT`
* :option:`CONFIG_CAF_LEDS_BRIGHTNESS_MAX`

Defining PWM ports
==================

To define PWM ports used by the application, complete the following steps:

1. Enable the ports in the devicetree file by setting the status to ``"okay"``. This can be done by using one of the following options:

	* Enabling relevant PWM ports in a board-specific :file:`dts` file.
	* Enabling relevant PWM ports in :file:`dts.overlay` file, which are to be merged to configuration.

	The following code snippets show several examples of how the file contents could look:

	* Example 1:

	.. code-block:: none

		&pwm0 {
			status = "okay";
			ch0-pin = <8>;
		};

	* Example 2:

	.. code-block:: none

		&pwm0 {
			status = "okay";
			ch0-pin = <11>;
			ch0-inverted;
			ch1-pin = <26>;
			ch1-inverted;
			ch2-pin = <27>;
			ch2-inverted;
		};

		&pwm1 {
			status = "okay";
			ch0-pin = <23>;
			ch1-pin = <25>;
			ch2-pin = <7>;
		};

	Make sure to configure all PWM ports that are used by the application.
	In particular, the number of PWM ports used must match the value set in :option:`CONFIG_CAF_LEDS_COUNT`.
	For more help, see :ref:`zephyr:dt-guide`.

2. Create a configuration file with the following array of arrays:

	* ``led_pins`` - contains PWM port and pin mapping for each LED used in the application.

	The size of the array is determined by the :option:`CONFIG_CAF_LEDS_COUNT` and :option:`CONFIG_CAF_LEDS_COLOR_COUNT` Kconfig options.
	The following code snippets show several examples of how the file contents could look:

	* Example 1:

	.. code-block:: c

		static const size_t led_pins[CONFIG_CAF_LEDS_COUNT]
					    [CONFIG_CAF_LEDS_COLOR_COUNT] = {
			{
				DT_PROP(DT_NODELABEL(pwm0), ch0_pin)
			}
		};

	* Example 2:

	.. code-block:: c

		static const size_t led_pins[CONFIG_CAF_LEDS_COUNT]
					    [CONFIG_CAF_LEDS_COLOR_COUNT] = {
			{
				DT_PROP(DT_NODELABEL(pwm0), ch0_pin),
				DT_PROP(DT_NODELABEL(pwm0), ch1_pin),
				DT_PROP(DT_NODELABEL(pwm0), ch2_pin)
			},
			{
				DT_PROP(DT_NODELABEL(pwm1), ch0_pin),
				DT_PROP(DT_NODELABEL(pwm1), ch1_pin),
				DT_PROP(DT_NODELABEL(pwm1), ch2_pin)
			}
		};

3. Specify location of the configuration file with the :option:`CONFIG_CAF_LEDS_DEF_PATH` Kconfig option.

	.. note::
		The configuration file should be included only by the configured module.
		Do not include the configuration file in other source files.

.. warning::
	   For the PWM ports to be configured correctly, both the configuration file and the :file:`dts` file must match.

Implementation details
**********************

The LED effect defines the LED behavior over time for the LED diodes by setting their brightness level periodically.
This allows for different RGB or monochromatic colors.
An example may be an LED that is blinking or breathing with a given color.
Such LED behavior is referred to as *LED effect*.

The LED color is achieved by setting the proper pulse widths for the PWM signals.
To achieve the desired LED effect, colors for the given LED are periodically updated using work (:c:struct:`k_delayed_work`).
One work automatically updates the color of a single LED.

If the application goes to the error state, the LEDs are used to indicate error.

LED effect
==========

The LED effect (:c:struct:`led_effect`) is described by the following characteristics:

* Pointer to an array of LED steps (:c:member:`led_effect.steps`).
* Size of the array (:c:member:`led_effect.step_count`).
* Flag indicating if the sequence should start over after it finishes (:c:member:`led_effect.loop_forever`).

To achieve the desired LED effect, the LED color is updated periodically based on LED steps defined for the given LED effect, which in turn are divided in multiple smaller updates called *substeps*.

.. figure:: /images/caf_led_effect_structure.png
   :alt: Characteristics of a led_effect

   Characteristics of a led_effect

During every substep, the next LED color is calculated using a linear approximation between the current LED color and the :c:member:`led_effect_step.color` described in the next LED step.
A single LED step also defines the number of substeps for color change between the given LED step and the previous one (:c:member:`led_effect_step.substep_count`), as well as the period of time between color updates (:c:member:`led_effect_step.substep_time`).
After achieving the color described in the next step, the index of the next step is updated.

After the last step, the sequence restarts if the :c:member:`led_effect.loop_forever` flag is set for the given LED effect.
If the flag is not set, the sequence stops and the given LED effect ends.

Power management events
=======================

If the :option:`CONFIG_CAF_LEDS_PM_EVENTS` Kconfig option is enabled, the module can react to following power management events:

* ``power_down_event``
* ``wake_up_event``

If a ``power_down_event`` comes, the module turns LEDs off.
The PWM drivers are set to the suspended state to reduce power consumption.

If a ``wake_up_event`` comes, PWM drivers are set to state active and LED effects are updated.
