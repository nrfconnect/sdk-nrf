.. _caf_leds:

CAF: LEDs module
################

.. contents::
   :local:
   :depth: 2

The LEDs module of the :ref:`lib_caf` (CAF) is responsible for controlling LEDs in response to LED effect set by a ``led_event``.
The source of such events could be any other module in |NCS|.

The module uses its own variant of Zephyr's :ref:`zephyr:pwm_api` driver for setting the LED color, either RGB or monochromatic.
For this reason, it requires a manual configuration of the PWM ports and LEDs.

Configuration
*************

To use the module, you must enable the following Kconfig options:

* :option:`CONFIG_CAF_LEDS` - This option enables the LEDs module.
* :option:`CONFIG_PWM` - This option enables Zephyr's :ref:`zephyr:pwm_api` driver, which is used for setting the LED color (that is, the brightness of the diodes).
* :option:`CONFIG_LED` - This option enables the LED driver API.
* :option:`CONFIG_LED_PWM` - This option enabled the LED PWM driver.

Additionally, you need to complete the steps described in `Configuring PWM ports and LEDs`_.

The following Kconfig options are also available for this module:

* :option:`CONFIG_CAF_LEDS_PM_EVENTS` - This option enables the reaction to `Power management events`_.

Configuring PWM ports and LEDs
==============================

To properly use the LEDs module and have LEDs driven by PWM, you must configure the PWM driver and the LED PWM driver.

Configuring the PWM driver specifies which PWM channel is related to which GPIO pin.
Configuring the LED PWM driver defines which PWM port is to be used for each LED and selects the GPIO pin for usage.
In case of the LED PWM driver, the GPIO pin must match the one passed to the PWM driver.

The configuration process requires enabling the PWM ports and enabling or creating the LED PWM nodes.
Both these steps are to be done in the devicetree file, either in the board-specific :file:`dts` file or in the :file:`dts.overlay` file.
Using the option with the :file:`dts.overlay` file will merge the settings to configuration.

Make sure to configure all PWM ports and channels that are used by the application.
For more help, see :ref:`zephyr:dt-guide`.

Enabling the PWM ports
----------------------

To enable the PWM ports, you must set the PWM port status to ``"okay"`` in the devicetree file and specify the PWM channel in relation to the GPIO pin number.
This can be done using one of the following options:

* Enabling relevant PWM ports in the board-specific :file:`dts` file.
* Enabling relevant PWM ports in the :file:`dts.overlay` file, which are to be merged to configuration.

The following code snippets show examples of how the file contents could look in either :file:`dts` or :file:`dts.overlay` files:

* Example 1 (enabling an existing port node):

  .. code-block:: none

	&pwm0 {
		status = "okay";
		ch0-pin = <8>;
	};

  In this example, the ``pwm0`` has its ``ch0`` channel bound to the GPIO pin number ``8``.
* Example 2 (enabling an existing port node):

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
		ch0-pin = <4>;
	};

Enabling the LED PWM nodes
--------------------------

To enable the LED PWM nodes in the devicetree file, you must set their status to ``"okay"`` in the devicetree file and specify to which PWM node they are related to.
You can also decide to create these nodes from scratch.
There is no limit to the number of node instances you can create.

The LEDs module assumes that a single LED PWM node is a separate and complete logical LED.
The LEDs module expects that a single LED PWM node will hold configuration of HW LEDs responsible for reproducing all required color channels.
The number of HW LEDs configured to reproduce color channels can be either one or three (either monochromatic or following the RGB order, with the red channel defined first, then the green one, then the blue one).
If only one HW LED is used for a monochromatic setting, the module will convert the tri-channel color to a single value of brightness and pass it to this single HW LED.

The configuration can be done using one of the following options:

* Enabling or creating relevant nodes in a board-specific :file:`dts` file.
* Enabling or creating relevant nodes in :file:`dts.overlay` file, which are to be merged to configuration.

For the LEDs to be configured correctly, make sure that LED PWM node pin numbers in the :file:`dts` file are matching the PWM nodes set when `Enabling the PWM ports`_.

The following code snippets show examples of how the file contents could look in either :file:`dts` or :file:`dts.overlay` files:

* Example 1 (enabling existing LED node):

  .. code-block:: none

	&pwm_led0 {
		status = "okay";
		pwms = <&pwm0 8>;
		label = "LED0";
	};

  In this example, the ``pwms`` property is pointing to the ``pwm0`` PWM node set in Example 1 in `Enabling the PWM ports`_, with the respective channel GPIO pin number (``8``).
* Example 2 (creating new LED nodes):

  .. code-block:: none

	pwmleds0 {
		compatible = "pwm-leds";
		status = "okay";

		pwm_led0: led_pwm_0 {
			status = "okay";
			pwms = <&pwm0 11>;
			label = "LED0 red";
		};

		pwm_led1: led_pwm_1 {
			status = "okay";
			pwms = <&pwm0 26>;
			label = "LED0 green";
		};

		pwm_led2: led_pwm_2 {
			status = "okay";
			pwms = <&pwm0 27>;
			label = "LED0 blue";
		};
	};

	pwmleds1 {
		compatible = "pwm-leds";
		status = "okay";

		pwm_led3: led_pwm_3 {
			status = "okay";
			pwms = <&pwm1 4>;
			label = "LED1";
		};
	};

     In this example, ``pwmleds0`` is a tri-channel color LED node, while ``pwmleds1`` is a monochromatic LED node.
     Both ``pwmleds`` nodes are pointing to the ``pwms`` properties corresponding to PWM nodes set in Example 2 in `Enabling the PWM ports`_, with the respective channel GPIO pin numbers.

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

To achieve the desired LED effect, the LED color is updated periodically based on the LED steps defined for the given LED effect, which in turn are divided in multiple smaller updates called *substeps*.

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
