.. _add_new_led_example:

Adding a dimmable LED node
##########################

.. contents::
   :local:
   :depth: 2

This example demonstrates how to add support for a dimmable LED node to your board in an overlay file.

To implement this example, you can either edit the devicetree files manually or use |nRFVSC| with its `Devicetree language support`_ and the `Devicetree Visual Editor <How to work with Devicetree Visual Editor_>`_ (recommended).

For more advanced LED control, you can also use the :ref:`LEDs module <caf_leds>` of the :ref:`Common Application Framework (CAF) <lib_caf>`, which provides additional features, such as LED effects and power management integration.

Implementation overview
***********************

This example uses Zephyr's generic binding for PWM LED controllers named ``pwm-leds``.

|devicetree_bindings|

See also the following related documentation:

* :ref:`zephyr:pwm_api` - Zephyr's PWM driver API documentation
* :ref:`zephyr:dt-guide` - Zephyr's devicetree user guide

.. rst-class:: numbered-step

Open or create the overlay file to edit
***************************************

Overlay files are a category of devicetree's :ref:`zephyr:devicetree-in-out-files`.
These files can override node property values in multiple ways.

You can add them to your configuration :ref:`manually <zephyr:set-devicetree-overlays>` or by using |nRFVSC| (see `How to create devicetree files`_).

.. rst-class:: numbered-step

Add the PWM LED controller node
*******************************

Complete the following steps:

1. Add the controller node ``pwmleds`` under the root node in devicetree.
#. Add the ``pwm-leds`` binding for the driver to pick up.
#. Add LEDs as child nodes on the ``pwmleds`` controller node, with ``pwms`` and ``label`` properties.
#. Make sure the ``pwms`` property of the ``phandle-array`` type points to a PWM instance.
   The PWM instance takes the pin number as its only parameter.

The following code example uses pins 6 and 7 on the GPIO port 0 (``&pwm0``):

.. code-block:: C

    / { // Devicetree root
        // ...
        pwmleds { // Controller node
                compatible = "pwm-leds"; // Compatible binding
                led_pwm_1 {
                        pwms = <&pwm0 6>; // Pin assigned to GPIO port 0 for LED 1
                        label = "PWM LED 1";
                };

                led_pwm_2 {
                        pwms = <&pwm0 7>; // Pin assigned to GPIO port 0 for LED 2
                        label = "PWM LED 2";
                };
        };
    };

.. rst-class:: numbered-step

Configure the PWM node
**********************

Enable the referenced PWM0 node by setting ``status`` to ``"okay"`` and configuring the node's channels.
Use the ``&pwm0`` node label reference on the root of the file:

.. code-block:: C

    &pwm0 {
        status = "okay"; // Status
        ch0-pin = <6>; // Pin assignment for channel 0
        ch1-pin = <7>; // Pin assignment for channel 1
    };

.. rst-class:: numbered-step

Enable the LED PWM driver
*************************

Add the following line to your :file:`prj.conf` file:

.. code-block:: none

    CONFIG_LED_PWM=y

Once you have added the LED PWM driver, :ref:`build your application <building>` and :ref:`program it to your board <programming>`.
